#include <stdarg.h>
#include <lwip/sockets.h>

#include "osapi.h"
#include "debug.h"

typedef void (*debug_handler_t)(const char *domain,
                                const char *format,
                                va_list args);

static bool debug_is_enabled = FALSE;
static int debug_level = 0;
static char debug_host[16] = { 0 };
static int debug_port = -1;
static debug_handler_t debug_handler = NULL;
static struct sockaddr_in debug_addr;
static int debug_fdes = -1;


/* UART 0 */
IRAM_ATTR
static void debug_serial_handler(const char *domain,
                                 const char *format,
                                 va_list args)
{
    char buf[512];
    int len;

    len = printf(buf, "%s: ", domain);
    len += vsnprintf(buf + len, sizeof(buf) - len,
                     format, args);
    printf("%s", buf);
}

IRAM_ATTR
static void debug_network_handler(const char *domain,
                                  const char *format,
                                  va_list args)
{
    char buf[512];
    int len;

    len = sprintf(buf, "%s: ", domain);
    len += vsnprintf(buf + len, sizeof(buf) - len,
                     format, args);
    if (len < sizeof(buf)) {
        buf[len] = '\n';
        sendto(debug_fdes, buf, len + 1, 0, (struct sockaddr *)&debug_addr,
               sizeof(debug_addr));
    }
}

void os_debug_enable(void)
{
    debug_is_enabled = TRUE;
}

void os_debug_disable(void)
{
    debug_is_enabled = FALSE;
    debug_handler = NULL;
}

void os_debug_set_level(int level)
{
    debug_level = (1 << (level + 1)) - 1;
}

void os_debug_set_dump_serial(void)
{
    debug_handler = debug_serial_handler;
}

void os_debug_set_dump_network(const char *host, int port)
{
    struct in_addr addr;

    if (!host) return;

    if (debug_fdes != -1)
        close(debug_fdes);
    debug_fdes = socket(AF_INET, SOCK_DGRAM, 0);
    if (debug_fdes == -1) return;

    strcpy(debug_host, host);
    debug_port = port;
    debug_handler = debug_network_handler;

    /* Configure UDP */
    memset(&debug_addr, 0, sizeof(debug_addr));
    inet_aton(host, &addr);
    debug_addr.sin_family = AF_INET;
    debug_addr.sin_port = htons(port);
    debug_addr.sin_addr = addr;
}

IRAM_ATTR
void os_debug(const char *domain, const char *format, ...)
{
    va_list ap;

    if (!domain || !format) return;

    if (debug_level & DEBUG_LEVEL_DEBUG) {
        va_start(ap, format);
        if (debug_handler)
            debug_handler(domain, format, ap);
        va_end(ap);
    }
}

IRAM_ATTR
void os_info(const char *domain, const char *format, ...)
{
    va_list ap;

    if (!domain || !format) return;

    if (debug_level & DEBUG_LEVEL_INFO) {
        va_start(ap, format);
        if (debug_handler)
            debug_handler(domain, format, ap);
        va_end(ap);
    }
}

void os_message(const char *domain, const char *format, ...)
{
    va_list ap;

    if (!domain || !format) return;

    if (debug_level & DEBUG_LEVEL_MESSAGE) {
        va_start(ap, format);
        if (debug_handler)
            debug_handler(domain, format, ap);
        va_end(ap);
    }
}

IRAM_ATTR
void os_warning(const char *domain, const char *format, ...)
{
    va_list ap;

    if (!domain || !format) return;

    if (debug_level & DEBUG_LEVEL_WARNING) {
        va_start(ap, format);
        if (debug_handler)
            debug_handler(domain, format, ap);
        va_end(ap);
    }
}

void os_critical(const char *domain, const char *format, ...)
{
    va_list ap;

    if (!domain || !format) return;

    if (debug_level & DEBUG_LEVEL_CRITICAL) {
        va_start(ap, format);
        if (debug_handler)
            debug_handler(domain, format, ap);
        va_end(ap);
    }
}

void os_error(const char *domain, const char *format, ...)
{
    va_list ap;

    if (!domain || !format) return;

    if (debug_level & DEBUG_LEVEL_ERROR) {
        va_start(ap, format);
        if (debug_handler)
            debug_handler(domain, format, ap);
        va_end(ap);
    }
}
