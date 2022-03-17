#ifndef __DEBUG_H__
#define __DEBUG_H__

#define DEBUG_MODE_NONE         0
#define DEBUG_MODE_SERIAL       1
#define DEBUG_MODE_NETWORK      2

#define DEBUG_LEVEL_ERROR       (1 << 2)
#define DEBUG_LEVEL_CRITICAL    (1 << 3)
#define DEBUG_LEVEL_WARNING     (1 << 4)
#define DEBUG_LEVEL_MESSAGE     (1 << 5)
#define DEBUG_LEVEL_INFO        (1 << 6)
#define DEBUG_LEVEL_DEBUG       (1 << 7)

void os_debug_enable(void);
void os_debug_disable(void);
void os_debug_set_level(int level);
void os_debug_set_dump_serial(void);
void os_debug_set_dump_network(const char *host, int port);

void os_debug(const char *domain, const char *format, ...);
void os_info(const char *domain, const char *format, ...);
void os_message(const char *domain, const char *format, ...);
void os_warning(const char *domain, const char *format, ...);
void os_critical(const char *domain, const char *format, ...);
void os_error(const char *domain, const char *format, ...);

#endif  /* __DEBUG_H__ */
