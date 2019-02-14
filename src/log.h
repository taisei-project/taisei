/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_log_h
#define IGUARD_log_h

#include "taisei.h"

#include <SDL.h>

enum {
	_LOG_DEBUG_ID,
	_LOG_INFO_ID,
	_LOG_WARN_ID,
	_LOG_ERROR_ID,
	_LOG_FATAL_ID,
};

typedef enum LogLevel {
	LOG_NONE  = 0,
	LOG_DEBUG = (1 << _LOG_DEBUG_ID),
	LOG_INFO  = (1 << _LOG_INFO_ID),
	LOG_WARN  = (1 << _LOG_WARN_ID),
	LOG_ERROR = (1 << _LOG_ERROR_ID),
	LOG_FATAL = (1 << _LOG_FATAL_ID),

	LOG_SPAM = LOG_DEBUG | LOG_INFO,
	LOG_ALERT = LOG_WARN | LOG_ERROR | LOG_FATAL,

	LOG_ALL = LOG_SPAM | LOG_ALERT,
} LogLevel;

#ifndef LOG_DEFAULT_LEVELS
	#define LOG_DEFAULT_LEVELS LOG_ALL
#endif

#ifndef LOG_DEFAULT_LEVELS_FILE
	#define LOG_DEFAULT_LEVELS_FILE LOG_ALL
#endif

#ifndef LOG_DEFAULT_LEVELS_CONSOLE
	#ifdef DEBUG
		#define LOG_DEFAULT_LEVELS_CONSOLE LOG_ALL
	#else
		#define LOG_DEFAULT_LEVELS_CONSOLE LOG_ALERT
	#endif
#endif

#ifndef LOG_DEFAULT_LEVELS_STDOUT
	#define LOG_DEFAULT_LEVELS_STDOUT LOG_SPAM
#endif

#ifndef LOG_DEFAULT_LEVELS_STDERR
	#define LOG_DEFAULT_LEVELS_STDERR LOG_ALERT
#endif

typedef struct LogEntry {
	const char *message;
	const char *file;
	const char *func;
	uint time;
	uint line;
	LogLevel level;
} LogEntry;

typedef struct FormatterObj FormatterObj;

struct FormatterObj {
	int (*format)(FormatterObj *self, char *buf, size_t buf_size, LogEntry *entry);
	void (*free)(FormatterObj *self);
	void *data;
};

// "constructor". initializer, actually.
typedef void Formatter(FormatterObj *obj, const SDL_RWops *output);

extern Formatter log_formatter_file;
extern Formatter log_formatter_console;

void log_init(LogLevel lvls);
void log_shutdown(void);
void log_add_output(LogLevel levels, SDL_RWops *output, Formatter *formatter) attr_nonnull(3);
void log_backtrace(LogLevel lvl);
LogLevel log_parse_levels(LogLevel lvls, const char *lvlmod) attr_nodiscard;
bool log_initialized(void) attr_nodiscard;
void log_set_gui_error_appendix(const char *message);

#ifdef DEBUG
	#define log_debug(...) log_custom(LOG_DEBUG, __VA_ARGS__)
	#undef UNREACHABLE
	#define UNREACHABLE log_fatal("This code should never be reached")
#else
	#define log_debug(...)
	#define LOG_NO_FILENAMES
#endif

#ifdef LOG_NO_FILENAMES
	#define _do_log(func, lvl, ...) (func)(lvl, __func__, "<unknown>", 0, __VA_ARGS__)
#else
	#define _do_log(func, lvl, ...) (func)(lvl, __func__, __FILE__, __LINE__, __VA_ARGS__)
#endif

#define log_custom(lvl, ...) _do_log(_taisei_log, lvl, __VA_ARGS__)
#define log_info(...) log_custom(LOG_INFO, __VA_ARGS__)
#define log_warn(...) log_custom(LOG_WARN, __VA_ARGS__)
#define log_error(...) log_custom(LOG_ERROR, __VA_ARGS__)
#define log_fatal(...) _do_log(_taisei_log_fatal, LOG_FATAL, __VA_ARGS__)

#define log_sdl_error(lvl, funcname) log_custom(lvl, "%s() failed: %s", funcname, SDL_GetError())

//
// don't call these directly, use the macros
//

void _taisei_log(LogLevel lvl, const char *funcname, const char *filename, uint line, const char *fmt, ...)
	attr_printf(5, 6);

noreturn void _taisei_log_fatal(LogLevel lvl, const char *funcname, const char *filename, uint line, const char *fmt, ...)
	attr_printf(5, 6);

#endif // IGUARD_log_h
