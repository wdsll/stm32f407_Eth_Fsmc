#ifndef LOG_H
#define LOG_H

#include "debug_printf.h"

#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern volatile uint32_t g_ms;

typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN  = 1,
    LOG_LEVEL_INFO  = 2,
    LOG_LEVEL_DEBUG = 3
} log_level_t;

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

static inline const char *log_level_str(log_level_t level)
{
    switch (level) {
    case LOG_LEVEL_ERROR:
        return "ERROR";
    case LOG_LEVEL_WARN:
        return "WARN";
    case LOG_LEVEL_INFO:
        return "INFO";
    case LOG_LEVEL_DEBUG:
        return "DEBUG";
    default:
        return "UNKNOWN";
    }
}

static inline void log_printf(log_level_t level, const char *tag, const char *fmt, ...)
{
    if (level > LOG_LEVEL) {
        return;
    }

    debug_printf("[%s][%s] ", log_level_str(level), tag);

    va_list args;
    va_start(args, fmt);
    (void)debug_vprintf(fmt, args);
    va_end(args);
}

#define LOG_ERROR(tag, fmt, ...) log_printf(LOG_LEVEL_ERROR, tag, fmt, ##__VA_ARGS__)
#define LOG_WARN(tag, fmt, ...)  log_printf(LOG_LEVEL_WARN, tag, fmt, ##__VA_ARGS__)
#define LOG_INFO(tag, fmt, ...)  log_printf(LOG_LEVEL_INFO, tag, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(tag, fmt, ...) log_printf(LOG_LEVEL_DEBUG, tag, fmt, ##__VA_ARGS__)

#define LOG_EVERY_MS(level, tag, interval_ms, fmt, ...)                 \
    do {                                                                \
        static uint32_t log_last_ms = 0U;                                \
        uint32_t log_now_ms = g_ms;                                      \
        if ((uint32_t)(log_now_ms - log_last_ms) >= (interval_ms)) {     \
            log_last_ms = log_now_ms;                                    \
            log_printf(level, tag, fmt, ##__VA_ARGS__);                  \
        }                                                               \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif