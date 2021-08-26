#include <stdio.h>
#include "ctl.h"
#include "driver/gpio.h"
#include "esp_log.h"
#define QRCODE_PIN      32
#define OUTPUT_MASK     (1ULL<<QRCODE_PIN)
#define FPM_PIN         12
#define INPUT_MASK      (1ULL<<FPM_PIN)

int ctl_init(){//int mode, ctl_event_func_t func) {

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
int ctl_fpm_get(void) {
    //ESP_LOGI("CTL", "ctl_qrcode_on");
    return gpio_get_level(FPM_PIN);
}

