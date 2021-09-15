#include <stdio.h>
#include "ctl.h"
#include "driver/gpio.h"
#include "gpio_drv.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ap.h"
#define QRCODE_PIN      32
#define BUZZER_PIN      13
#define LED_PIN         35
#define RELAY_EXT_PIN   14
#define OUTPUT_MASK     ((1ULL<<QRCODE_PIN) | (1ULL<<RELAY_EXT_PIN))
#define FPM_PIN         12
#define SENSOR_PIN      35
#define SENSOR_INTR     4
#define INPUT_MASK      (1ULL<<FPM_PIN | 1ULL<<SENSOR_PIN | 1ULL<<BUZZER_PIN ) 
#define CTL_BEEP_TIME   100000
#define CTL_BEEP_COUNT  3
#define CTL_TIMEOUT     100000
#define CTL_RELAY_PULSE 200
static esp_timer_handle_t ctl_timer;
static uint32_t ctl_beep_time;
static uint8_t ctl_beep_count;
static uint32_t ctl_buzzer_time;
static uint8_t ctl_mode = CTL_MODE_NORMAL;
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
static uint32_t ctl_relay_ext_time;
static uint32_t ctl_led_timeout;
static uint32_t ctl_led_time;

static ctl_event_func_t ctl_event_func = NULL;

static void ctl_interrupt_handler(int intr, void *user_data)
{
    // ESP_LOGI("ctl","interruot ctl\n");
    
    if (ctl_event_func)
        ctl_event_func(CTL_EVT_SENSOR, 0);

    gpio_interrupt_enable(SENSOR_INTR, GPIO_INTR_POSEDGE);
}

static void ctl_timeout(void *arg)
{
    //ESP_LOGI("CTL", "ctl timedout\n");
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

    if (ctl_mode == CTL_MODE_NORMAL) {
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
    } else {
//         /* RELAY */
        if (ctl_relay_time > 0) {
            ctl_relay_time -= CTL_TIMEOUT;
            if (ctl_relay_time <= 0) {

            }
        }

        // /* RELAY EXT */
        // if (ctl_relay_ext_time > 0) {
        //     ctl_relay_ext_time -= CTL_TIMEOUT;
        //     if (ctl_relay_ext_time <= 0) {
        //         ctl_relay_ext_off();
        //     }
        // }

        /* LED */
        // if (ctl_led_timeout) {
        //     ctl_led_time -= CTL_TIMEOUT;
        //     if (ctl_led_time <= 0) {
        //         ctl_led_toggle();
        //         ctl_led_time = ctl_led_timeout;
        //     }
        // }
    }
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

bool ctl_check_ap_mode(void)
{
    bool ap;
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_INPUT);
    vTaskDelay(10);
    ap = !gpio_get_level(BUZZER_PIN);
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_PIN, 0);

    return ap;
}

int ctl_init(int mode, ctl_event_func_t func) {

    //init qrcode
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = OUTPUT_MASK;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

     //init qrcode
    gpio_config_t io_conf2;
    //disable interrupt
    io_conf2.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf2.mode = GPIO_MODE_INPUT;
    io_conf2.pin_bit_mask = INPUT_MASK;
    //disable pull-down mode
    io_conf2.pull_down_en = 0;
    //disable pull-up mode
    io_conf2.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf2);
    gpio_set_level(RELAY_EXT_PIN, 1);
    if (!mode) {

        gpio_set_level(QRCODE_PIN, 1);

        /* RELAY */
        gpio_set_direction(RELAY_EXT_PIN, GPIO_MODE_OUTPUT);
        
        // gpio16_output_conf();
        // gpio16_output_set(1);

        /* BUZZER */
        gpio_set_level(BUZZER_PIN, 0);

        /* SENSOR */
        ctl_sensor_pin = gpio_get_level(SENSOR_PIN);

        /* ALARM */
        ctl_alarm_state = CTL_ALARM_OFF;

        /* PANIC */
        ctl_panic_state = CTL_PANIC_OFF;
    } else {

        /* SENSOR */
        gpio_set_direction(SENSOR_PIN, GPIO_MODE_INPUT);
        ctl_sensor_pin = gpio_get_level(SENSOR_PIN);

        /* LED */
        gpio_set_level(LED_PIN, 0);
    }

    ctl_mode = mode;
    ctl_event_func = func;
    const esp_timer_create_args_t ctl_timer_args = {
            .callback = &ctl_timeout,
            .name = "ctl_timeout"
    };
    gpio_set_level(BUZZER_PIN, 0);
    ESP_ERROR_CHECK(esp_timer_create(&ctl_timer_args, &ctl_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(ctl_timer, CTL_TIMEOUT));
    if (ctl_check_ap_mode()) {
        start_ap();
    }
    return 1;
}
void ctl_release(void)
{
    //gpio_interrupt_close(SENSOR_INTR);
    ESP_ERROR_CHECK(esp_timer_stop(ctl_timer));
    ESP_ERROR_CHECK(esp_timer_delete(ctl_timer));
    gpio_set_level(QRCODE_PIN, 1);
    gpio_set_level(RELAY_EXT_PIN, 1);
    gpio_set_level(BUZZER_PIN, 0);
    ctl_alarm_state = CTL_ALARM_OFF;
    ctl_panic_state = CTL_PANIC_OFF;
    ctl_mode = CTL_MODE_NORMAL;
    ctl_event_func = NULL;
}



void ctl_qrcode_on(void) {
    //ESP_LOGI("CTL", "ctl_qrcode_on");
    gpio_set_level(QRCODE_PIN, 1);
}
void ctl_qrcode_off(void) {
    //ESP_LOGI("CTL", "ctl_qrcode_off");
    gpio_set_level(QRCODE_PIN, 0);
}
int ctl_fpm_get(void) {
    //ESP_LOGI("CTL", "ctl_qrcode_on");
    return gpio_get_level(FPM_PIN);
}
void ctl_beep(uint8_t beep)
{
    gpio_set_level(BUZZER_PIN, 1);
    ctl_beep_time = CTL_BEEP_TIME;
    ctl_beep_count = beep ? beep : CTL_BEEP_COUNT;
    ctl_beep_count <<= 1;
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
void ctl_relay_on(uint32_t time)
{
    ESP_LOGI("ctl", "relay on\n");

    gpio_set_level(RELAY_EXT_PIN, 0);
    ctl_relay_time = time;

    
    if (ctl_event_func)
        ctl_event_func(CTL_EVT_RELAY, CTL_RELAY_ON);
}

void ctl_relay_off(void)
{
    ESP_LOGI("ctl", "relay off\n");

    gpio_set_level(RELAY_EXT_PIN, 1);
    ctl_relay_time = 0;
   
    if (ctl_event_func)
        ctl_event_func(CTL_EVT_RELAY, CTL_RELAY_OFF);
}
void ctl_set_sensor_mode(uint8_t mode)
{
    ctl_sensor_mode = mode;
    if (mode == CTL_SENSOR_LEVEL) {
        gpio_interrupt_close(SENSOR_INTR);
    } else {
        gpio_interrupt_open(SENSOR_INTR, SENSOR_PIN,
                            GPIO_INTR_POSEDGE, GPIO_INTR_DISABLED,
                            ctl_interrupt_handler, NULL);
    }
}

uint8_t ctl_relay_ext_status(void)
{
    return !gpio_get_level(RELAY_EXT_PIN);
}

