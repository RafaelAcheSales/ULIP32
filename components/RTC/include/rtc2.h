#ifndef __RTC_H__
#define __RTC_H__

#define NTP_SERVER      "pool.ntp.br"

void rtc_init2(const char *server, int8_t tz,
              bool dst, const char *dst_date);
void rtc_release(void);

void rtc_set_time(uint32_t time);

/* UTC time */
uint32_t rtc_mktime(struct tm *tm);

/* Local time */
uint32_t rtc_mklocaltime(struct tm *tm);

/* UTC time */
uint32_t rtc_time(void);

struct tm *rtc_localtime(void);
struct tm *rtc_gmtime(uint32_t time);

/* System uptime */
uint32_t rtc_uptime(void);

const char *rtc_month(struct tm *tm);
const char *rtc_weekday(struct tm *tm);

void rtc_set_shutdown(uint32_t time);
uint32_t rtc_get_shutdown(void);

#endif  /* __RTC_H__ */
