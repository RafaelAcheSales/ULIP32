#ifndef __OSAPI_H__
#define __OSAPI_H__

#include <esp_private/system_internal.h>
#include <rom/ets_sys.h>
#include <rtc_wdt.h>

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    1
#endif

#define os_timer_t      ETSTimer
#define os_timer_func_t ETSTimerFunc
#define os_timer_setfn  ets_timer_setfn
#define os_timer_arm    ets_timer_arm
#define os_timer_disarm ets_timer_disarm

#define udelay(x)   do { \
                        uint32_t __time = esp_system_get_time() + x; \
                        while (esp_system_get_time() < __time); \
                    } while(0)
#define mdelay(x)   udelay(1000 * (x))

#endif
