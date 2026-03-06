#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>

// colors
#define LOG_COLOR_RESET "\x1b[0m"
#define LOG_COLOR_RED "\x1b[31m"
#define LOG_COLOR_YELLOW "\x1b[33m"
#define LOG_COLOR_GREEN "\x1b[32m"
#define LOG_COLOR_CYAN "\x1b[36m"

static inline void _log_timestamp(char *buf, size_t sz)
{
    time_t t = time(NULL);
    struct tm *tmv = localtime(&t);

    if (tmv != NULL)
    {
        strftime(buf, sz, "%Y-%m-%d %H:%M:%S", tmv);
    }
    else
    {
        snprintf(buf, sz, "0000-00-00 00:00:00");
    }
}

static inline void _log_print(
    const char *color,
    const char *level,
    const char *fmt,
    ...)
{
    char _ts[20];
    va_list args;

    _log_timestamp(_ts, sizeof(_ts));
    fprintf(stderr, "%s[%s] [%s] ", color, _ts, level);

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, LOG_COLOR_RESET "\n");
}

#define LOG(...) _log_print(LOG_COLOR_GREEN, "LOG", __VA_ARGS__)
#define DEBUG(...) _log_print(LOG_COLOR_CYAN, "DEBUG", __VA_ARGS__)
#define WARN(...) _log_print(LOG_COLOR_YELLOW, "WARN", __VA_ARGS__)
#define ERROR(...) _log_print(LOG_COLOR_RED, "ERROR", __VA_ARGS__)

#endif
