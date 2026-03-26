#ifndef DEBUG_PRINTF_H
#define DEBUG_PRINTF_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
#ifndef DEBUG_PRINTF_ENABLE
#define DEBUG_PRINTF_ENABLE 1
#endif

#ifndef DEBUG_PRINTF_DEFAULT_BAUDRATE
#define DEBUG_PRINTF_DEFAULT_BAUDRATE 921600U
#endif

/* 非阻塞串口配置 */
#ifndef DEBUG_TX_BUFFER_SIZE
#define DEBUG_TX_BUFFER_SIZE      512U  /* 发送缓冲区大小 */
#endif

#ifndef DEBUG_TX_DMA_ENABLE
#define DEBUG_TX_DMA_ENABLE       1    /* 使用DMA发送 */
#endif

#ifndef DEBUG_TX_TIMEOUT_MS
#define DEBUG_TX_TIMEOUT_MS       10U  /* 发送超时时间 */
#endif

#if DEBUG_PRINTF_ENABLE
/*********************************************************************************************************
*                                              函数声明
*********************************************************************************************************/
void debug_printf_init(uint32_t baudrate);
void debug_printf_deinit(void);
void debug_putchar(char ch);
void debug_write(const uint8_t *data, size_t len);
void debug_write_raw(const uint8_t *data, size_t len);
int debug_vprintf(const char *fmt, va_list args);

#if defined(__GNUC__)
int debug_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#else
int debug_printf(const char *fmt, ...);
#endif

void debug_hexdump(const void *data, size_t len);

/* 非阻塞模式接口 */
void debug_tx_task(void);                    /* 需要在主循环中调用的发送任务 */
int debug_tx_busy(void);                     /* 检查发送是否繁忙 */
void debug_tx_flush(void);                   /* 强制刷新发送缓冲区 */
int debug_tx_available(void);                /* 获取发送缓冲区剩余空间 */

#else /* DEBUG_PRINTF_ENABLE */
/*********************************************************************************************************
*                                              空实现
*********************************************************************************************************/
static inline void debug_printf_init(uint32_t baudrate) {(void)baudrate;}
static inline void debug_printf_deinit(void) {}
static inline void debug_putchar(char ch) {(void)ch;}
static inline void debug_write(const uint8_t *data, size_t len) {(void)data; (void)len;}
static inline void debug_write_raw(const uint8_t *data, size_t len) {(void)data; (void)len;}
static inline int debug_vprintf(const char *fmt, va_list args) {(void)fmt; (void)args; return 0;}
static inline int debug_printf(const char *fmt, ...) {(void)fmt; return 0;}
static inline void debug_hexdump(const void *data, size_t len) {(void)data; (void)len;}

/* 非阻塞模式接口 - 空实现 */
static inline void debug_tx_task(void) {}
static inline int debug_tx_busy(void) {return 0;}
static inline void debug_tx_flush(void) {}
static inline int debug_tx_available(void) {return 0;}

#endif

#ifdef __cplusplus
}
#endif

#endif