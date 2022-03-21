#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sntp.h>

#include "osapi.h"
#include "debug.h"
#include "rtc2.h"

#define RTC_TIMEOUT     1000
#define SNTP_TIMEOUT    300000

static int8_t timezone = 0;
static uint32_t timestamp = 0;
static uint32_t uptime = 0;
static os_timer_t rtc_timer;
static bool dst_enable = FALSE;
static char dst_interval[32] = { 0 };
static uint32_t shutdown = 0;


static void rtc_update(void *arg)
{
    os_debug("RTC","rtc_update");
    static bool sync = FALSE;
    uint32_t time;
    uint32_t d;

    /* Update uptime by system elapsed time */
    time = esp_system_get_time();
    d = ((time - timestamp) + 500000) / 1000000;
    timestamp = time;
    if (d < (RTC_TIMEOUT / 1000))
        d = RTC_TIMEOUT / 1000;

    /* System uptime */
    uptime += d;
    /* System shutdown */
    if (shutdown && uptime > shutdown) {
        os_info("RTC", "System shutdown");
        rtc_wdt_disable();
        esp_restart();
    }
}

static void rtc_sntp_update(struct timeval *tv)
{
    os_info("RTC", "SNTP update");
}

void rtc_init2(const char *server, int8_t tz,
              bool dst, const char *dst_date)
{
    char stmp[32];

    timezone = tz;
    timestamp = esp_system_get_time();
    uptime = 0;
    shutdown = 0;
    /* DST */
    if (dst && dst_date) {
        dst_enable = TRUE;
        strcpy(dst_interval, dst_date);
    }
    if (server) {
        if (timezone) {
            snprintf(stmp, sizeof(stmp), "GMT%c%d",
                     tz > 0 ? '-' : '+', abs(tz));
            setenv("TZ", stmp, 1);
        } else {
            setenv("TZ", "UTC", 1);
        }
        tzset();
        sntp_setservername(0, (char *)server);
        sntp_set_time_sync_notification_cb(rtc_sntp_update);
        sntp_set_sync_interval(SNTP_TIMEOUT);
        sntp_init();
        printf("RTC: SNTP server %s\n", server);
    }

    os_timer_setfn(&rtc_timer, (os_timer_func_t *)rtc_update, NULL);
    os_timer_arm(&rtc_timer, RTC_TIMEOUT, TRUE);
}

void rtc_release(void)
{
    os_timer_disarm(&rtc_timer);
    sntp_stop();
    timezone = 0;
    timestamp = 0;
    uptime = 0;
    shutdown = 0;
}

void rtc_set_time(uint32_t time)
{
    struct timeval tv;

    tv.tv_sec = time;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
}

uint32_t rtc_mktime(struct tm *tm)
{
    if (!tm) return -1;

    return mktime(tm);
}

uint32_t rtc_mklocaltime(struct tm *tm)
{
    if (!tm) return -1;

    return mktime(tm);
}

uint32_t rtc_time(void)
{
    return time(NULL);
}

struct tm *rtc_localtime(void)
{
    static struct tm tm;
    time_t now;

    now = time(NULL);
    localtime_r(&now, &tm);

    return &tm;
}

struct tm *rtc_gmtime(uint32_t time)
{
    static struct tm tm;

    gmtime_r((time_t *)&time, &tm);

    return &tm;
}

uint32_t rtc_uptime(void)
{
    return uptime;
}

const char *rtc_month(struct tm *tm)
{
    static char *month[] = { "Jan", "Feb", "Mar", "Apr",
                             "May", "Jun", "Jul", "Ago",
                             "Sep", "Oct", "Nov", "Dec" };

    if (!tm) return NULL;

    return month[tm->tm_mon];
}

const char *rtc_weekday(struct tm *tm)
{
    static char *day[] = { "Sun", "Mon", "Tue", "Wed",
                           "Thu", "Fri", "Sat" };

    if (!tm) return NULL;

    return day[tm->tm_wday];
}

void rtc_set_shutdown(uint32_t time)
{
    shutdown = time ? uptime + time : 0;
}

uint32_t rtc_get_shutdown(void)
{
    if (!shutdown) return 0;

    return (shutdown - uptime);
}
