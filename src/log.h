
#ifndef TAISEI_LOG_H
#define TAISEI_LOG_H

#include <stdnoreturn.h>
#include <SDL.h>

typedef enum LogLevel {
    LOG_NONE = 0,
    LOG_DEBUG = 1,
    LOG_INFO = 2,
    LOG_WARN = 4,
    LOG_ERR = 8,

    LOG_SPAM = LOG_DEBUG | LOG_INFO,
    LOG_ALERT = LOG_WARN | LOG_ERR,
    LOG_ALL = LOG_SPAM | LOG_ALERT,
} LogLevel;

#ifndef LOG_DEFAULT_LEVELS
    #define LOG_DEFAULT_LEVELS LOG_ALL
#endif

void log_init(LogLevel lvls);
void log_shutdown(void);
void log_add_output(LogLevel levels, SDL_RWops *output);

#ifdef DEBUG
    #define log_debug(...) _taisei_log(LOG_DEBUG, __func__, __VA_ARGS__)
#else
    #define log_debug(...)
#endif

#define log_info(...) _taisei_log(LOG_INFO, __func__, __VA_ARGS__)
#define log_warn(...) _taisei_log(LOG_WARN, __func__, __VA_ARGS__)
#define log_err(...) _taisei_log_fatal(LOG_ERR, __func__, __VA_ARGS__)
#define log_custom(lvl, ...) _taisei_log(lvl, __func__, __VA_ARGS__)

//
// don't call these directly, use the macros
//

void _taisei_log(LogLevel lvl, const char *funcname, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

noreturn void _taisei_log_fatal(LogLevel lvl, const char *funcname, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

#endif
