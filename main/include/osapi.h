#ifndef __OSAPI_H__
#define __OSAPI_H__

#include <esp_private/system_internal.h>
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

#endif
