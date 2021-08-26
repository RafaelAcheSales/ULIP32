#include <stdio.h>
#include "ctl.h"
#include "driver/gpio.h"
#include "esp_log.h"
#define QRCODE_PIN      32
#define QRCODE_MASK     (1ULL<<QRCODE_PIN)

int ctl_init(){//int mode, ctl_event_func_t func) {

    //init qrcode
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = QRCODE_MASK;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    return 1;
}
void ctl_release(void) {
    //TODO
}
void ctl_qrcode_on(void) {
    //ESP_LOGI("CTL", "ctl_qrcode_on");
    gpio_set_level(QRCODE_PIN, 1);
}
void ctl_qrcode_off(void) {
    //ESP_LOGI("CTL", "ctl_qrcode_off");
    gpio_set_level(QRCODE_PIN, 0);
}
