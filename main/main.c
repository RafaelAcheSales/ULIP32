/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "tty.h"
#include "gpio_drv.h"
#include "eth.h"
#include "ctl.h"
#include "qrcode.h"
#include "config.h"
#include "fpm.h"
#include "ap.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
#define GPIO_INPUT 16
#define GPIO_INPUT_PIN_SEL (1ULL<<GPIO_INPUT)
#define GPIO_OUTPUT -1
#define GPIO_OUTPUT_PIN_SEL (1ULL<<GPIO_OUTPUT)
#define BITBANG 3
#define UART_TTY 2
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
int cnt = 0;

void ulip_core_capture_finger(bool status, int index)
{
    if (status)
        fpm_set_enroll(index);
    else
        fpm_cancel_enroll();
}
static void ctl_event(int event, int status) {
    printf("event rolou ctl");
//     tty_release();
//     ctl_release();
//     fpm_release();
}
static void fingerprint_event(int event, int index,
                              uint8_t *data, int len,
                              int error, void *user_data)
{
    printf("event rolou fingerprint of len: %d erro %X\n", len, event);
    for (int i = 0; i < len; i++)
    {
        printf("%02X", data[i]);
    }
}
static int qrcode_event(int event, const char *data,
                        int len, void *user_data)
{
    printf(RED "%s\n", data);
    return 1;
}


static tty_func_t test_event(int tty, char *data,
                      int len, void *user_data)
{
    printf("event rolou: ");
    for (int i = 0; i < len; i++)
    {
        printf("%c",data[i]);
    }
    printf("\n");
    tty_func_t t = NULL;
    return t;
}
unsigned char * data = (unsigned char *)"\x88";
static void buttonPressed(int intr, void *user_data) {
    

    if (intr == 1) {
        printf("event rolou ctl\n");
        // tty_release();
        // ctl_release();
        // printf("ctl released\n");
        ctl_beep(3);
        
        // fpm_release();
        
        // fpm_delete_all();
        // ulip_core_capture_finger(true, 3);
        // if (cnt%2==0) {
        //     printf("buz on\n");

        // } else {
        //     printf("buz off\n");
        //     gpio_set_level(GPIO_NUM_13, 0);

        // }
        cnt++;
    }
}
void app_main(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    ctl_init(CTL_MODE_NORMAL, ctl_event);
    // start_eth();
    // CFG_Load();
    tty_init();
    // ctl_set_sensor_mode(1);
    // qrcode_init(false, true,
    //                 1000000,
    //                 2000000,
    //                 true,
    //                 30,
    //                 qrcode_event, NULL);
    
    // tty_open(BITBANG,test_event, NULL);

    // fpm_init(0,2,2,fingerprint_event, NULL);
    gpio_interrupt_open(1, GPIO_INPUT, GPIO_INTR_NEGEDGE, 0, buttonPressed, NULL);
    
    printf("Hello world!\n");
    // start_ap();
}

