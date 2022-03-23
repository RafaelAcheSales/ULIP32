#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "osapi.h"
#include "gpio_drv.h"
#include "ctl.h"

#define CTL_TIMEOUT     100

/* QRCODE - GPIO 32 */
#define QRCODE_PIN      32

/* BUZZER - GPIO 13 */
#define BUZZER_PIN      13

/* SENSOR - GPIO 36 */
#define SENSOR_PIN      36
#define SENSOR_INTR     4

/* RELAY - GPIO 16 */
#define RELAY_PIN       16

#define CTL_BEEP_TIME   100
#define CTL_BEEP_COUNT  3

static uint8_t ctl_sensor_mode = CTL_SENSOR_LEVEL;

static uint8_t ctl_sensor_pin;
static uint8_t ctl_alarm_state;
static uint8_t ctl_panic_state;
static uint8_t ctl_breakin_state;
static uint8_t ctl_relay_pin;

static uint32_t ctl_relay_time;
static uint32_t ctl_buzzer_time;
static uint32_t ctl_beep_time;
static uint8_t ctl_beep_count;
static os_timer_t ctl_timer;

static ctl_event_func_t ctl_event_func = NULL;


static void ctl_interrupt_handler(int intr, void *user_data)
{
    if (ctl_event_func)
        ctl_event_func(CTL_EVT_SENSOR, 0);

    gpio_interrupt_enable(SENSOR_INTR);
}

static void ctl_timeout(void *arg)
{
    uint8_t sensor_pin;

    /* SENSOR */
    if (ctl_sensor_mode == CTL_SENSOR_LEVEL) {
        sensor_pin = gpio_get_level(SENSOR_PIN);
        if (ctl_sensor_pin != sensor_pin) {
            ctl_sensor_pin = sensor_pin;
            if (ctl_event_func)
                ctl_event_func(CTL_EVT_SENSOR, ctl_sensor_pin);
        }
    }

    /* RELAY */
    if (ctl_relay_time > 0) {
        ctl_relay_time -= CTL_TIMEOUT;
        if (ctl_relay_time <= 0) {
            ctl_relay_off();
        }
    }

    /* BUZZER */
    if (ctl_buzzer_time > 0) {
        ctl_buzzer_time -= CTL_TIMEOUT;
        if (ctl_buzzer_time <= 0) {
            ctl_buzzer_off();
        }
    }
    if (ctl_beep_time > 0) {
        ctl_beep_time -= CTL_TIMEOUT;
        if (ctl_beep_time <= 0) {
            if (--ctl_beep_count & 1) {
                gpio_set_level(BUZZER_PIN, 1);
            } else {
                gpio_set_level(BUZZER_PIN, 0);
            }
            if (ctl_beep_count > 0)
                ctl_beep_time = CTL_BEEP_TIME;
        }
    }
}

int ctl_init(ctl_event_func_t func)
{
    gpio_config_t io_config;

    memset(&io_config, 0, sizeof(io_config));

    io_config.mode = GPIO_MODE_OUTPUT;
    io_config.pin_bit_mask = BIT64(QRCODE_PIN) | BIT64(RELAY_PIN) | BIT64(BUZZER_PIN);
    gpio_config(&io_config);

    /* QRCODE */
    gpio_set_direction(QRCODE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(QRCODE_PIN, 1);

    /* RELAY */
    gpio_set_direction(RELAY_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(RELAY_PIN, 1);

    /* BUZZER */
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_PIN, 0);

    io_config.mode = GPIO_MODE_INPUT;
    io_config.pin_bit_mask = BIT64(SENSOR_PIN);
    gpio_config(&io_config);

    /* SENSOR */
    gpio_set_direction(SENSOR_PIN, GPIO_MODE_INPUT);
    ctl_sensor_pin = GPIO_INPUT_GET(SENSOR_PIN);

    /* ALARM */
    ctl_alarm_state = CTL_ALARM_OFF;

    /* PANIC */
    ctl_panic_state = CTL_PANIC_OFF;

    ctl_event_func = func;

    os_timer_setfn(&ctl_timer, (os_timer_func_t *)ctl_timeout, NULL);
    os_timer_arm(&ctl_timer, CTL_TIMEOUT, TRUE);

    return 0;
}

void ctl_release(void)
{
    gpio_interrupt_close(SENSOR_INTR);
    os_timer_disarm(&ctl_timer);
    gpio_set_level(QRCODE_PIN, 1);
    gpio_set_level(RELAY_PIN, 1);
    gpio_set_level(BUZZER_PIN, 0);
    ctl_alarm_state = CTL_ALARM_OFF;
    ctl_panic_state = CTL_PANIC_OFF;
    ctl_event_func = NULL;
}

void ctl_qrcode_on(void)
{
    gpio_set_level(QRCODE_PIN, 0);
}

void ctl_qrcode_off(void)
{
    gpio_set_level(QRCODE_PIN, 1);
}

void ctl_relay_on(uint32_t time)
{
    gpio_set_level(RELAY_PIN, 0);
    ctl_relay_time = time;
    if (ctl_event_func)
        ctl_event_func(CTL_EVT_RELAY, CTL_RELAY_ON);
}

void ctl_relay_off(void)
{
    gpio_set_level(RELAY_PIN, 1);
    ctl_relay_time = 0;
    if (ctl_event_func)
        ctl_event_func(CTL_EVT_RELAY, CTL_RELAY_OFF);
}

void ctl_buzzer_on(uint32_t time)
{
    gpio_set_level(BUZZER_PIN, 1);
    ctl_buzzer_time = time;
    if (ctl_event_func)
        ctl_event_func(CTL_EVT_BUZZER, CTL_BUZZER_ON);
}

void ctl_buzzer_off(void)
{
    gpio_set_level(BUZZER_PIN, 0);
    ctl_buzzer_time = 0;
    if (ctl_event_func)
        ctl_event_func(CTL_EVT_BUZZER, CTL_BUZZER_OFF);
}

void ctl_alarm_on(void)
{
    if (ctl_alarm_state == CTL_ALARM_OFF) {
        ctl_alarm_state = CTL_ALARM_ON;
        ctl_buzzer_on(0);
    }
}

void ctl_alarm_off(void)
{
    if (ctl_alarm_state == CTL_ALARM_ON) {
        ctl_alarm_state = CTL_ALARM_OFF;
        ctl_breakin_state = CTL_BREAKIN_OFF;
        ctl_buzzer_off();
    }
}

void ctl_panic_on(void)
{
    if (ctl_panic_state == CTL_PANIC_OFF) {
        ctl_panic_state = CTL_PANIC_ON;
        ctl_relay_on(0);
    }
}

void ctl_panic_off(void)
{
    if (ctl_panic_state == CTL_PANIC_ON) {
        ctl_panic_state = CTL_PANIC_OFF;
        ctl_relay_off();
    }
}

void ctl_breakin_on(void)
{
    if (ctl_breakin_state == CTL_BREAKIN_OFF) {
        ctl_breakin_state = CTL_BREAKIN_ON;
        /* Enable alarm */
        ctl_alarm_state = CTL_ALARM_ON;
        ctl_buzzer_on(0);
    }
}

void ctl_breakin_off(void)
{
    if (ctl_breakin_state == CTL_BREAKIN_ON) {
        ctl_breakin_state = CTL_BREAKIN_OFF;
        /* Disable alarm */
        ctl_alarm_state = CTL_ALARM_OFF;
        ctl_buzzer_off();
    }
}

bool ctl_check_ap_mode(void)
{
    bool ap;

    gpio_set_direction(BUZZER_PIN, GPIO_MODE_INPUT);
    mdelay(1);
    ap = !gpio_get_level(BUZZER_PIN);
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_PIN, 0);

    return ap;
}

uint8_t ctl_relay_status(void)
{
    return !gpio_get_level(RELAY_PIN);
}

uint8_t ctl_sensor_status(void)
{
    ctl_sensor_pin = gpio_get_level(SENSOR_PIN);

    return ctl_sensor_pin;
}

uint8_t ctl_alarm_status(void)
{
    return ctl_alarm_state;
}

uint8_t ctl_panic_status(void)
{
    return ctl_panic_state;
}

uint8_t ctl_breakin_status(void)
{
    return ctl_breakin_state;
}

void ctl_beep(uint8_t beep)
{
    gpio_set_level(BUZZER_PIN, 1);
    ctl_beep_time = CTL_BEEP_TIME;
    ctl_beep_count = beep ? beep : CTL_BEEP_COUNT;
    ctl_beep_count <<= 1;
}

void ctl_set_sensor_mode(uint8_t mode)
{
    ctl_sensor_mode = mode;
    if (mode == CTL_SENSOR_LEVEL) {
        gpio_interrupt_close(SENSOR_INTR);
    } else {
        gpio_interrupt_open(SENSOR_INTR, SENSOR_PIN,
                            GPIO_PIN_INTR_POSEDGE, GPIO_INTR_DISABLED,
                            ctl_interrupt_handler, NULL);
    }
}

uint8_t ctl_get_sensor_mode(void)
{
    return ctl_sensor_mode;
}
