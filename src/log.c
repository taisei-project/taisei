/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include <SDL_bits.h>
#include <SDL_mutex.h>

#include "log.h"
#include "util.h"
#include "list.h"

typedef struct Logger {
	LIST_INTERFACE(struct Logger);

	FormatterObj formatter;
	SDL_RWops *out;
	uint levels;
} Logger;

static struct {
	Logger *outputs;
	size_t num_outputs;
	char *fmt_buf1;
	char *fmt_buf2;
	size_t fmt_buf_size;
	SDL_mutex *mutex;
	uint enabled_log_levels;

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

noreturn static void log_abort(const char *msg) {
#ifdef LOG_FATAL_MSGBOX
	static const char *const title = "Taisei: fatal error";

	if(msg) {
		if(logging.err_appendix) {
			char *m = strfmt("%s\n\n%s", msg, logging.err_appendix);
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, m, NULL);
			free(m);
		} else {
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, msg, NULL);
		}
	} else if(logging.err_appendix) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, logging.err_appendix, NULL);
	}
#endif

	log_shutdown();

	// abort() doesn't clean up, but it lets us get a backtrace, which is more useful
	abort();
}

void log_set_gui_error_appendix(const char *message) {
#ifdef LOG_FATAL_MSGBOX
	stralloc(&logging.err_appendix, message);
#endif
}

static void realloc_buffers(size_t min_size) {
	min_size = topow2_u64(min_size);

	if(min_size > MAX_BUF_SIZE) {
		min_size = MAX_BUF_SIZE;
	}

	if(logging.fmt_buf_size >= min_size) {
		return;
	}

	logging.fmt_buf1 = realloc(logging.fmt_buf1, min_size);
	logging.fmt_buf2 = realloc(logging.fmt_buf2, min_size);
	logging.fmt_buf_size = min_size;
}

static void add_debug_info(char **buf) {
	IF_DEBUG({
		DebugInfo *debug_info = get_debug_info();
		DebugInfo *debug_meta = get_debug_meta();

		char *dbg = strfmt(
			"%s\n\n\n"
			"Debug info: %s:%i:%s\n"
			"Debug info set at: %s:%i:%s\n"
			"Note: debug info may not be relevant to this issue\n",
			*buf,
			debug_info->file, debug_info->line, debug_info->func,
			debug_meta->file, debug_meta->line, debug_meta->func
		);

		free(*buf);
		*buf = dbg;
	});
}

static void log_internal(LogLevel lvl, const char *funcname, const char *filename, uint line, const char *fmt, va_list args) {
	assert(fmt[strlen(fmt)-1] != '\n');

	lvl &= logging.enabled_log_levels;

	if(lvl == LOG_NONE) {
		return;
	}

	SDL_LockMutex(logging.mutex);

	va_list args_copy;
	va_copy(args_copy, args);
	int slen = vsnprintf(logging.fmt_buf1, logging.fmt_buf_size, fmt, args_copy);
	va_end(args_copy);

	if(slen >= logging.fmt_buf_size) {
		realloc_buffers(slen + 1);
		va_copy(args_copy, args);
		slen = vsnprintf(logging.fmt_buf1, logging.fmt_buf_size, fmt, args_copy);
		va_end(args_copy);
	}

	assert_nolog(slen >= 0);

	if(lvl == LOG_FATAL) {
		add_debug_info(&logging.fmt_buf1);
	}

	LogEntry entry = {
		.message = logging.fmt_buf1,
		.file = filename,
		.func = funcname,
		.line = line,
		.level = lvl,
		.time = SDL_GetTicks(),
	};

	for(Logger *l = logging.outputs; l; l = l->next) {
		if(l->levels & lvl) {
			slen = l->formatter.format(&l->formatter, logging.fmt_buf2, logging.fmt_buf_size, &entry);

			if(slen >= logging.fmt_buf_size) {
				char *tmp = strdup(logging.fmt_buf1);
				realloc_buffers(slen + 1);
				strcpy(logging.fmt_buf1, tmp);
				free(tmp);
				entry.message = logging.fmt_buf1;

				attr_unused int old_slen = slen;
				slen = l->formatter.format(&l->formatter, logging.fmt_buf2, logging.fmt_buf_size, &entry);
				assert_nolog(old_slen == slen);
			}

			assert_nolog(slen >= 0);

			if(logging.fmt_buf_size < slen) {
				slen = logging.fmt_buf_size;
			}

			SDL_RWwrite(l->out, logging.fmt_buf2, 1, slen);
		}
	}

	if(lvl & LOG_FATAL) {
		log_abort(entry.message);
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

static void* delete_logger(List **loggers, List *logger, void *arg) {
	Logger *l = (Logger*)logger;

	if(l->formatter.free != NULL) {
		l->formatter.free(&l->formatter);
	}

#if HAVE_STDIO_H
	if(l->out->type == SDL_RWOPS_STDFILE) {
		fflush(l->out->hidden.stdio.fp);
	}
#endif

	SDL_RWclose(l->out);
	free(list_unlink(loggers, logger));

	return NULL;
}

void log_init(LogLevel lvls) {
	logging.enabled_log_levels = lvls;
	logging.mutex = SDL_CreateMutex();
	realloc_buffers(INIT_BUF_SIZE);
}

void log_shutdown(void) {
	list_foreach(&logging.outputs, delete_logger, NULL);
	SDL_DestroyMutex(logging.mutex);
	free(logging.fmt_buf1);
	free(logging.fmt_buf2);

#ifdef LOG_FATAL_MSGBOX
	free(logging.err_appendix);
#endif

	memset(&logging, 0, sizeof(logging));
}

bool log_initialized(void) {
	return logging.fmt_buf_size > 0;
}

void log_add_output(LogLevel levels, SDL_RWops *output, Formatter *formatter) {
	if(!output) {
		return;
	}

	if(!(levels & logging.enabled_log_levels)) {
		SDL_RWclose(output);
		return;
	}

	Logger *l = calloc(1, sizeof(Logger));
	l->levels = levels;
	l->out = output;

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

LogLevel log_parse_levels(LogLevel lvls, const char *lvlmod) {
	if(!lvlmod) {
		return lvls;
	}

	bool enable = true;

	for(const char *c = lvlmod; *c; ++c) {
		if(*c == '+') {
			enable = true;
		} else if(*c == '-') {
			enable = false;
		} else if(enable) {
			lvls |= chr2lvl(*c);
		} else {
			lvls &= ~chr2lvl(*c);
		}
	}

	return lvls;
}

static void ensure_newline(char *buf, size_t buf_size, size_t output_size) {
	if(output_size >= buf_size) {
		*(strrchr(buf, '\0') - 1) = '\n';
	}
}

static int log_fmtconsole_format_ansi(FormatterObj *obj, char *buf, size_t buf_size, LogEntry *entry) {
	int r = snprintf(
		buf,
		buf_size,
		"\x1b[1;30m%-9d %s%s\x1b[1;30m: \x1b[1;34m%s\x1b[1;30m: \x1b[0m%s\n",
		entry->time,
		level_ansi_style_code(entry->level),
		level_prefix(entry->level),
		entry->func,
		entry->message
	);
	ensure_newline(buf, buf_size, r);
	return r;
}

static int log_fmtconsole_format_plain(FormatterObj *obj, char *buf, size_t buf_size, LogEntry *entry) {
	int r = snprintf(
		buf,
		buf_size,
		"%-9d %s: %s: %s\n",
		entry->time,
		level_prefix(entry->level),
		entry->func,
		entry->message
	);
	ensure_newline(buf, buf_size, r);
	return r;
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
	if(output_supports_ansi_sequences(output)) {
		obj->format = log_fmtconsole_format_ansi;
	} else {
		obj->format = log_fmtconsole_format_plain;
	}
}

static int log_fmtfile_format(FormatterObj *obj, char *buf, size_t buf_size, LogEntry *entry) {
	int r = snprintf(
		buf,
		buf_size,
		"%d  %s  %s: %s\n",
		entry->time,
		level_name(entry->level),
		entry->func,
		entry->message
	);
	ensure_newline(buf, buf_size, r);
	return r;
}

void log_formatter_file(FormatterObj *obj, const SDL_RWops *output) {
	obj->format = log_fmtfile_format;
}
