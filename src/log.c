/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include <SDL_bits.h>
#include <SDL_mutex.h>

#include "log.h"
#include "util.h"
#include "list.h"
#include "util/strbuf.h"
#include "thread.h"
#include "random.h"
#include "coroutine/cotask.h"

typedef struct Logger {
	LIST_INTERFACE(struct Logger);

	FormatterObj formatter;
	SDL_RWops *out;
	uint levels;
} Logger;

typedef struct QueuedLogEntry {
	LIST_INTERFACE(struct QueuedLogEntry);
	LogEntry e;
	char message[];
} QueuedLogEntry;

typedef struct LogFilterEntry {
	struct {
		char *module;
		char *func;
	} patterns;
	LogLevelDiff diff;
} LogFilterEntry;

static struct {
	Logger *outputs;
	size_t num_outputs;
	SDL_mutex *mutex;
	uint enabled_log_levels;

	struct {
		StringBuffer pre_format;
		StringBuffer format;
	} buffers;

	struct {
		Thread *thread;
		SDL_mutex *mutex;
		SDL_cond *cond;
		LIST_ANCHOR(QueuedLogEntry) queue;
		int shutdown;
	} queue;

	DYNAMIC_ARRAY(LogFilterEntry) filters;

	bool initialized;

#ifdef LOG_FATAL_MSGBOX
	char *err_appendix;
#endif
} logging;

#define INIT_BUF_SIZE 0x100
#define MAX_BUF_SIZE 0x10000

static const char *level_prefix_map[] = {
	[_LOG_DEBUG_ID] = "D",
	[_LOG_INFO_ID]  = "I",
	[_LOG_WARN_ID]  = "W",
	[_LOG_ERROR_ID] = "E",
	[_LOG_FATAL_ID] = "F",
};

static const char *level_name_map[] = {
	[_LOG_DEBUG_ID] = "DEBUG",
	[_LOG_INFO_ID]  = "INFO",
	[_LOG_WARN_ID]  = "WARNING",
	[_LOG_ERROR_ID] = "ERROR",
	[_LOG_FATAL_ID] = "FATAL",
};

attr_unused
static const char *level_ansi_style_map[] = {
	[_LOG_DEBUG_ID] = "\x1b[1;35m",
	[_LOG_INFO_ID]  = "\x1b[1;32m",
	[_LOG_WARN_ID]  = "\x1b[1;33m",
	[_LOG_ERROR_ID] = "\x1b[1;31m",
	[_LOG_FATAL_ID] = "\x1b[1;31m",
};

#define INDEX_MAP(map, lvl) \
	uint idx = SDL_MostSignificantBitIndex32(lvl); \
	assert_nolog(idx < sizeof(map) / sizeof(*map)); \
	return map[idx];

static const char* level_prefix(LogLevel lvl) { INDEX_MAP(level_prefix_map, lvl) }
static const char* level_name(LogLevel lvl) { INDEX_MAP(level_name_map, lvl) }
attr_unused static const char* level_ansi_style_code(LogLevel lvl) { INDEX_MAP(level_ansi_style_map, lvl) }

static void log_queue_shutdown_internal(bool force_sync);

noreturn static void log_abort(const char *msg) {
#ifdef LOG_FATAL_MSGBOX
	static const char *const title = "Taisei: fatal error";

	if(msg) {
		if(logging.err_appendix) {
			char *m = strfmt("%s\n\n%s", msg, logging.err_appendix);
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, m, NULL);
			mem_free(m);
		} else {
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, msg, NULL);
		}
	} else if(logging.err_appendix) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, logging.err_appendix, NULL);
	}
#endif

	log_queue_shutdown_internal(true);
	log_shutdown();

	// abort() doesn't clean up, but it lets us get a backtrace, which is more useful
	abort();
}

void log_set_gui_error_appendix(const char *message) {
#ifdef LOG_FATAL_MSGBOX
	stralloc(&logging.err_appendix, message);
#endif
}

static void add_debug_info(StringBuffer *buf) {
	IF_DEBUG({
		DebugInfo *debug_info = get_debug_info();
		DebugInfo *debug_meta = get_debug_meta();

		strbuf_printf(buf,
			"\n\n\n"
			"Debug info: %s:%i:%s\n"
			"Debug info set at: %s:%i:%s\n"
			"Note: debug info may not be relevant to this issue\n",
			debug_info->file, debug_info->line, debug_info->func,
			debug_meta->file, debug_meta->line, debug_meta->func
		);
	});
}

#define MODULE_BUFFER_SIZE 128

static size_t modname(const char *filename, size_t filename_len, char *mod) {
	size_t mlen = filename_len;

	char *dot = memchr(filename, '.', mlen);
	if(dot) {
		mlen = dot - filename;
	}

	if(mlen >= MODULE_BUFFER_SIZE) {
		mlen = MODULE_BUFFER_SIZE - 1;
	}

	memcpy(mod, filename, mlen);
	mod[mlen] = 0;

#ifdef _WIN32
	for(char *c = mod; c < mod + mlen; ++c) {
		if(*c == '\\') {
			*c = '/';
		}
	}
#endif

	char *sep = memrchr(mod, '/', mlen);

	if(sep) {
		// simplify "foobar/foobar" into foobar and "foo/bar/foo_bar" into "foo/bar"

		char pfx[sep - mod + 1];
		int i;
		for(i = 0; i < ARRAY_SIZE(pfx) - 1; ++i) {
			char c = mod[i];
			if(c == '/') {
				c = '_';
			}
			pfx[i] = c;
		}
		pfx[i] = 0;

		size_t tlen = strlen(sep + 1);

		if(i == tlen && !memcmp(pfx, sep + 1, tlen)) {
			mlen = tlen;
			mod[mlen] = 0;
		}
	}

	return mlen;
}

static void filter_entry_apply(const LogEntry *entry, const LogFilterEntry *f, LogLevelDiff *diff) {
	if(f->patterns.module && !strmatch(f->patterns.module, entry->module)) {
		return;
	}

	if(f->patterns.func && !strmatch(f->patterns.func, entry->func)) {
		return;
	}

	*diff = log_merge_level_diff(*diff, f->diff);
}

static LogLevel filter_entry(LogEntry *entry) {
	LogLevelDiff d = {};

	dynarray_foreach_elem(&logging.filters, LogFilterEntry *f, {
		filter_entry_apply(entry, f, &d);
	});

	return log_apply_level_diff(LOG_ALL, d);
}

static void log_dispatch(LogEntry *entry) {
	StringBuffer *fmt_buf = &logging.buffers.format;

	bool init = false;
	LogLevel filter_lvlmask = LOG_ALL;

	char mbuf[MODULE_BUFFER_SIZE];

	for(Logger *l = logging.outputs; l; l = l->next) {
		if(l->levels & entry->level) {
			if(!init) {
				modname(entry->file, strlen(entry->file), mbuf);

				DIAGNOSTIC_GCC(push)
				DIAGNOSTIC_GCC(ignored "-Wpragmas")
				DIAGNOSTIC_GCC(ignored "-Wdangling-pointer")
				// The formatter is not expected to leak this out of the the stack frame.
				entry->module = mbuf;
				DIAGNOSTIC_GCC(pop)

				filter_lvlmask = filter_entry(entry);
				init = true;
			}

			if(!((entry->level & filter_lvlmask) | (l->levels & LOG_UNFILTERED_BIT))) {
				continue;
			}

			strbuf_clear(fmt_buf);
			size_t slen = l->formatter.format(&l->formatter, fmt_buf, entry);
			assert_nolog(fmt_buf->buf_size >= slen);
			SDL_RWwrite(l->out, fmt_buf->start, 1, slen);
		}
	}

	if(entry->thread) {
		thread_decref(entry->thread);
	}
}

static void log_dispatch_async(LogEntry *entry) {
	for(Logger *l = logging.outputs; l; l = l->next) {
		if(l->levels & entry->level) {
			size_t msg_len = strlen(entry->message);
			auto qle = ALLOC_FLEX(QueuedLogEntry, msg_len + 1);
			memcpy(&qle->e, entry, sizeof(*entry));
			memcpy(qle->message, entry->message, msg_len);
			qle->e.message = qle->message;
			SDL_LockMutex(logging.queue.mutex);
			alist_append(&logging.queue.queue, qle);
			SDL_CondBroadcast(logging.queue.cond);
			SDL_UnlockMutex(logging.queue.mutex);
			break;
		}
	}
}

static void *sync_logger(List **loggers, List *logger, void *arg) {
	Logger *l = (Logger*)logger;
	SDL_RWsync(l->out);
	return NULL;
}

static void log_internal(LogLevel lvl, const char *funcname, const char *filename, uint line, const char *fmt, va_list args) {
	assert(fmt[strlen(fmt)-1] != '\n');

	bool noabort = lvl & LOG_NOABORT_BIT;
	lvl &= logging.enabled_log_levels;

	if(lvl == LOG_NONE) {
		return;
	}

	SDL_LockMutex(logging.mutex);

	StringBuffer *buf = &logging.buffers.pre_format;
	strbuf_clear(buf);

	va_list args_copy;
	va_copy(args_copy, args);
	attr_unused int slen = strbuf_vprintf(buf, fmt, args_copy);
	va_end(args_copy);
	assert_nolog(slen >= 0);

	if(lvl & LOG_FATAL) {
		add_debug_info(buf);
	}

	LogEntry entry = {
		.message = buf->start,
		.file = filename,
		.func = funcname,
		.line = line,
		.level = lvl,
		.time = SDL_GetTicks(),
		.thread = thread_get_current(),
		.thread_id = thread_get_current_id(),
	};

	CoTask *task = cotask_active();

	if(task) {
		entry.task = cotask_get_name(task);
		entry.task_id = cotask_box(task).unique_id;
	}

	if(entry.thread) {
		thread_incref(entry.thread);
	}

	if(logging.queue.thread) {
		log_dispatch_async(&entry);
	} else {
		log_dispatch(&entry);
	}

	if(lvl & LOG_FATAL) {
		if(noabort) {
			// Will likely abort externally (e.g. assertion failure), so sync everything now.
			list_foreach(&logging.outputs, sync_logger, NULL);
		} else {
			log_abort(entry.message);
		}
	}

	SDL_UnlockMutex(logging.mutex);
}

void _taisei_log(LogLevel lvl, const char *funcname, const char *filename, uint line, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	log_internal(lvl, funcname, filename, line, fmt, args);
	va_end(args);
}

void _taisei_log_fatal(LogLevel lvl, const char *funcname, const char *filename, uint line, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	log_internal(lvl, funcname, filename, line, fmt, args);
	va_end(args);

	// should usually not get here, log_internal will abort earlier if lvl is LOG_FATAL
	// that is unless LOG_FATAL is disabled for some reason
	log_abort(NULL);
}

static void *delete_logger(List **loggers, List *logger, void *arg) {
	Logger *l = (Logger*)logger;

	if(l->formatter.free != NULL) {
		l->formatter.free(&l->formatter);
	}

	SDL_RWsync(l->out);
	SDL_RWclose(l->out);
	mem_free(list_unlink(loggers, logger));

	return NULL;
}

static void *log_queue_thread(void *a) {
	SDL_mutex *mtx = logging.queue.mutex;
	SDL_cond *cond = logging.queue.cond;

	SDL_LockMutex(mtx);

	for(;;) {
		SDL_CondWait(cond, mtx);
		QueuedLogEntry *qle;

		while((qle = alist_pop(&logging.queue.queue)) && logging.queue.shutdown < 2) {
			SDL_UnlockMutex(mtx);
			log_dispatch(&qle->e);
			mem_free(qle);
			SDL_LockMutex(mtx);
		}

		SDL_CondBroadcast(cond);

		if(logging.queue.shutdown) {
			break;
		}
	}

	SDL_UnlockMutex(mtx);
	return NULL;
}

static void log_queue_init(void) {
	if(!env_get("TAISEI_LOG_ASYNC", true)) {
		return;
	}

	if(!(logging.queue.cond = SDL_CreateCond())) {
		log_sdl_error(LOG_ERROR, "SDL_CreateCond");
		return;
	}

	if(!(logging.queue.mutex = SDL_CreateMutex())) {
		log_sdl_error(LOG_ERROR, "SDL_CreateMutex");
		return;
	}

	logging.queue.thread = thread_create("Log queue", log_queue_thread, NULL, THREAD_PRIO_LOW);

	if(!logging.queue.thread) {
		log_error("Failed to create log queue thread");
		return;
	}
}

static void log_queue_shutdown_internal(bool force_sync) {
	if(!logging.queue.thread) {
		return;
	}

	SDL_LockMutex(logging.queue.mutex);
	logging.queue.shutdown = env_get("TAISEI_LOG_ASYNC_FAST_SHUTDOWN", false) && !force_sync ? 2 : 1;
	SDL_CondBroadcast(logging.queue.cond);
	SDL_UnlockMutex(logging.queue.mutex);
	thread_wait(logging.queue.thread);
	logging.queue.thread = NULL;
	SDL_DestroyMutex(logging.queue.mutex);
	logging.queue.mutex = NULL;
	SDL_DestroyCond(logging.queue.cond);
	logging.queue.cond = NULL;
}

void log_queue_shutdown(void) {
	log_queue_shutdown_internal(false);
}

void log_init(LogLevel lvls) {
	logging.enabled_log_levels = lvls;
	logging.mutex = SDL_CreateMutex();
	log_queue_init();
	logging.initialized = true;
}

void log_shutdown(void) {
	logging.initialized = false;
	log_queue_shutdown_internal(false);
	list_foreach(&logging.outputs, delete_logger, NULL);
	SDL_DestroyMutex(logging.mutex);
	strbuf_free(&logging.buffers.pre_format);
	strbuf_free(&logging.buffers.format);

	log_remove_filters();
	dynarray_free_data(&logging.filters);

#ifdef LOG_FATAL_MSGBOX
	mem_free(logging.err_appendix);
#endif

	memset(&logging, 0, sizeof(logging));
}

void log_sync(bool flush) {
	SDL_LockMutex(logging.queue.mutex);

	while(logging.queue.queue.first) {
		SDL_CondWait(logging.queue.cond, logging.queue.mutex);
	}

	if(flush) {
		list_foreach(&logging.outputs, sync_logger, NULL);
	}

	SDL_UnlockMutex(logging.queue.mutex);
}

bool log_initialized(void) {
	return logging.initialized;
}

void log_add_output(LogLevel levels, SDL_RWops *output, Formatter *formatter) {
	if(!output) {
		return;
	}

	if(!(levels & logging.enabled_log_levels)) {
		SDL_RWclose(output);
		return;
	}

	auto l = ALLOC(Logger, { .levels = levels, .out = output });
	formatter(&l->formatter, l->out);
	assert(l->formatter.format != NULL);

	list_append(&logging.outputs, l);
}

static LogLevel chr2lvl(char c) {
	c = toupper(c);

	for(int i = 0; i < sizeof(level_prefix_map) / sizeof(char*); ++i) {
		if(c == level_prefix_map[i][0]) {
			return (1 << i);
		} else if(c == 'A') {
			return LOG_ALL;
		}
	}

	return 0;
}

LogLevelDiff log_parse_level_diff(const char *lvlmod) {
	LogLevelDiff d = {};

	bool enable = true;

	for(const char *c = lvlmod; *c; ++c) {
		if(*c == '+') {
			enable = true;
		} else if(*c == '-') {
			enable = false;
		} else {
			char l = chr2lvl(*c);
			d.diff[enable] |= l;
			d.diff[!enable] &= ~l;
		}
	}

	assert((d.removed & d.added) == 0);
	return d;
}

LogLevelDiff log_merge_level_diff(LogLevelDiff lower, LogLevelDiff upper) {
	assert((lower.removed & lower.added) == 0);
	assert((upper.removed & upper.added) == 0);
	lower.added = (lower.added & ~upper.removed) | upper.added;
	lower.removed = (lower.removed & ~upper.added) | upper.removed;
	assert((lower.removed & lower.added) == 0);
	return lower;
}

LogLevel log_apply_level_diff(LogLevel lvls, LogLevelDiff diff) {
	assert((diff.removed & diff.added) == 0);
	return (lvls & ~diff.removed) | diff.added;
}

LogLevel log_parse_levels(LogLevel lvls, const char *lvlmod) {
	return log_apply_level_diff(lvls, log_parse_level_diff(lvlmod));
}

static inline int thread_color(uint64_t tid) {
	uint64_t hash = splitmix64(&tid);
	return 0x10 + hash % 0xD8;
}

static const char *thread_name(LogEntry *entry, size_t tmpsize, char tmpbuf[tmpsize]) {
	if(entry->thread) {
		return thread_get_name(entry->thread);
	} else {
		snprintf(tmpbuf, tmpsize, "%llx", (unsigned long long)entry->thread_id);
		return tmpbuf;
	}
}

typedef struct ConsoleFormatterData {
	struct {
		bool timestamps;
		bool context;
		bool color;
	} cfg;

	struct {
		int max_context_len;
	} state;
} ConsoleFormatterData;

#define C(x) (fdata->cfg.color ? (x) : 0)

static int log_fmtconsole_format_context(
	ConsoleFormatterData *fdata,
	StringBuffer *buf,
	LogEntry *entry
) {
	char tmp[32];
	int txtlen = 0;
	int clrlen = 0;

	if(entry->thread_id != thread_get_main_id()) {
		clrlen += C(strbuf_printf(buf, "\x1b[38;5;%im", thread_color(entry->thread_id)));
		txtlen +=   strbuf_printf(buf, "%-18s ", thread_name(entry, sizeof(tmp), tmp));
		clrlen += C(strbuf_cat(buf, "\x1b[1;30m"));
	}

	if(entry->task_id != 0) {
		clrlen += C(strbuf_cat(buf, "\x1b[1;35m"));
		txtlen +=   strbuf_cat(buf, entry->task ?: "<unknown task>");
		clrlen += C(strbuf_cat(buf, "\x1b[1;30m"));
		txtlen +=   strbuf_cat(buf, "[");
		clrlen += C(strbuf_cat(buf, "\x1b[1;37m"));
		txtlen +=   strbuf_printf(buf, "%x", entry->task_id);
		clrlen += C(strbuf_cat(buf, "\x1b[1;30m"));
		txtlen +=   strbuf_cat(buf, "] ");
	}

	if(txtlen) {
		if(fdata->state.max_context_len < txtlen) {
			fdata->state.max_context_len = txtlen;
		} else {
			static char spaces[] = "                                      ";
			size_t n = min(sizeof(spaces) - 1, fdata->state.max_context_len - txtlen);
			txtlen += strbuf_ncat(buf, n, spaces);
		}
	}

	return txtlen + clrlen;
}

static int log_fmtconsole_format(FormatterObj *obj, StringBuffer *buf, LogEntry *entry) {
	ConsoleFormatterData *fdata = obj->data;
	int len = 0;

	if(fdata->cfg.timestamps) {
		len += strbuf_printf(buf, "\x1b[1;30m%-9d ", entry->time);
	}

	if(fdata->cfg.context) {
		len += log_fmtconsole_format_context(fdata, buf, entry);
	}

	len += C(strbuf_cat(buf, level_ansi_style_code(entry->level)));
	len +=   strbuf_cat(buf, level_prefix(entry->level));
	len += C(strbuf_cat(buf, "\x1b[1;30m"));
	len +=   strbuf_cat(buf, ": ");

	len += C(strbuf_cat(buf, "\x1b[1;36m"));
	len +=   strbuf_cat(buf, entry->module);
	len += C(strbuf_cat(buf, "\x1b[1;30m"));
	len +=   strbuf_cat(buf, ">");
	len += C(strbuf_cat(buf, "\x1b[1;34m"));
	len +=   strbuf_cat(buf, entry->func);
	len += C(strbuf_cat(buf, "\x1b[1;30m"));
	len +=   strbuf_cat(buf, ": ");
	len += C(strbuf_cat(buf, "\x1b[0m"));
	len +=   strbuf_cat(buf, entry->message);
	len +=   strbuf_cat(buf, "\n");

	return len;
}

#undef C

static void log_fmtconsole_free(FormatterObj *fobj) {
	mem_free(fobj->data);
}

#ifdef TAISEI_BUILDCONF_HAVE_POSIX
#include <unistd.h>

static bool output_supports_ansi_sequences(const SDL_RWops *output) {
	if(!strcmp(env_get("TERM", "dumb"), "dumb")) {
		return false;
	}

	if(output->type != SDL_RWOPS_STDFILE) {
		return false;
	}

	return isatty(fileno(output->hidden.stdio.fp));
}
#else
static bool output_supports_ansi_sequences(const SDL_RWops *output) {
	// TODO: Handle it for windows. The windows console supports ANSI escapes, but requires special setup.
	return false;
}
#endif

void log_formatter_console(FormatterObj *obj, const SDL_RWops *output) {
	obj->data = ALLOC(ConsoleFormatterData, {
		.cfg = {
			.color = (
				env_get("TAISEI_LOG_CONSOLE_COLOR", true) &&
				output_supports_ansi_sequences(output)
			),
			.context = env_get("TAISEI_LOG_CONSOLE_CONTEXT", true),
			.timestamps = env_get("TAISEI_LOG_CONSOLE_TIME", false),
		}
	});
	obj->format = log_fmtconsole_format;
	obj->free = log_fmtconsole_free;
}

static int log_fmtfile_format(FormatterObj *obj, StringBuffer *buf, LogEntry *entry) {
	char tmp[32];
	int len = 0;

	len += strbuf_printf(buf, "%d  ", entry->time);

	if(entry->thread_id != thread_get_main_id()) {
		len += strbuf_printf(
			buf,
			"{%s}  ",
			thread_name(entry, sizeof(tmp), tmp)
		);
	}

	if(entry->task_id != 0) {
		len += strbuf_printf(
			buf,
			"%s[%x]  ",
			entry->task ?: "<unknown task>", entry->task_id
		);
	}

	len += strbuf_printf(
		buf,
		"%s  %s %s: %s  [%s:%i]\n",
		level_name(entry->level),
		entry->module,
		entry->func,
		entry->message,
		entry->file,
		entry->line
	);

	return len;
}

void log_formatter_file(FormatterObj *obj, const SDL_RWops *output) {
	obj->format = log_fmtfile_format;
}

static char *copy_pattern(const char *p) {
	const char *orig = p;

	if(!p) {
		return NULL;
	}

	while(*p == '*') {
		++p;
	}

	if(!*p) {
		return NULL;
	}

	return strdup(orig);
}

void log_add_filter(LogLevelDiff diff, const char *pmod, const char *pfunc) {
	dynarray_append(&logging.filters, {
		.patterns.module = copy_pattern(pmod),
		.patterns.func = copy_pattern(pfunc),
		.diff = diff,
	});
}

bool log_add_filter_string(const char *fstr) {
	char buf[strlen(fstr) + 1];
	memcpy(buf, fstr, sizeof(buf));
	char *pbuf = buf;

	char *levelmod = strtok_r(NULL, " \t\r\n", &pbuf);
	if(!levelmod) {
		goto error;
	}

	char *pmodule = strtok_r(NULL, " \t\r\n", &pbuf);
	if(!pmodule) {
		goto error;
	}

	char *pfunc = strtok_r(NULL, " \t\r\n", &pbuf);
	if(!pfunc) {
		goto error;
	}

	log_add_filter(log_parse_level_diff(levelmod), pmodule, pfunc);
	return true;

error:
	log_error("Malformed filter string");
	return false;
}

void log_remove_filters(void) {
	dynarray_foreach_elem(&logging.filters, LogFilterEntry *f, {
		mem_free(f->patterns.module);
		mem_free(f->patterns.func);
	});

	logging.filters.num_elements = 0;
}
