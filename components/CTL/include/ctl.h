#ifndef __CTL_H__
#define __CTL_H__

#define CTL_EVT_RELAY       0
#define CTL_EVT_BUZZER      1
#define CTL_EVT_SENSOR      2

#define CTL_RELAY_OFF       0
#define CTL_RELAY_ON        1
#define CTL_BUZZER_OFF      0
#define CTL_BUZZER_ON       1
#define CTL_SENSOR_OFF      0
#define CTL_SENSOR_ON       1
#define CTL_ALARM_OFF       0
#define CTL_ALARM_ON        1
#define CTL_PANIC_OFF       0
#define CTL_PANIC_ON        1
#define CTL_BREAKIN_OFF     0
#define CTL_BREAKIN_ON      1

#define CTL_BUZZER_ERROR    1000
#define CTL_BUZZER_ALARM    100

#define CTL_SENSOR_LEVEL    0
#define CTL_SENSOR_EDGE     1

typedef void (*ctl_event_func_t)(int event, int status);

int ctl_init(ctl_event_func_t func);
void ctl_release(void);

void ctl_qrcode_on(void);
void ctl_qrcode_off(void);

void ctl_relay_on(uint32_t time);
void ctl_relay_off(void);

void ctl_buzzer_on(uint32_t time);
void ctl_buzzer_off(void);

void ctl_alarm_on(void);
void ctl_alarm_off(void);
void ctl_panic_on(void);
void ctl_panic_off(void);
void ctl_breakin_on(void);
void ctl_breakin_off(void);

bool ctl_check_ap_mode(void);

uint8_t ctl_relay_status(void);
uint8_t ctl_sensor_status(void);
uint8_t ctl_alarm_status(void);
uint8_t ctl_panic_status(void);
uint8_t ctl_breakin_status(void);

void ctl_beep(uint8_t beep);

void ctl_set_sensor_mode(uint8_t mode);
uint8_t ctl_get_sensor_mode(void);

#endif  /* __CTL_H__ */
