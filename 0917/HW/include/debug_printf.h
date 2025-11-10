#ifndef DEBUG_PRINTF_H
#define DEBUG_PRINTF_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef DEBUG_PRINTF_ENABLE
#define DEBUG_PRINTF_ENABLE 1
#endif

#ifndef DEBUG_PRINTF_DEFAULT_BAUDRATE
#define DEBUG_PRINTF_DEFAULT_BAUDRATE 230400U
#endif

#if DEBUG_PRINTF_ENABLE
void debug_printf_init(uint32_t baudrate);
void debug_printf_deinit(void);
void debug_putchar(char ch);
void debug_write(const uint8_t *data, size_t len);
int debug_vprintf(const char *fmt, va_list args);

	#if defined(__GNUC__)
	int debug_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
	#else
	int debug_printf(const char *fmt, ...);
	#endif
void debug_hexdump(const void *data, size_t len);

#else /* DEBUG_PRINTF_ENABLE */
static inline void debug_printf_init(uint32_t baudrate) {(void)baudrate;}
static inline void debug_printf_deinit(void) {}
static inline void debug_putchar(char ch) {(void)ch;}
static inline void debug_write(const uint8_t *data, size_t len) {(void)data; (void)len;}
static inline int debug_vprintf(const char *fmt, va_list args) {(void)fmt; (void)args; return 0;}
static inline int debug_printf(const char *fmt, ...) {(void)fmt; return 0;}
static inline void debug_hexdump(const void *data, size_t len) {(void)data; (void)len;}

#endif

#ifdef __cplusplus
}
#endif

#endif