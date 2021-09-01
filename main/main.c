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
#define GPIO_INPUT 16
#define GPIO_INPUT_PIN_SEL (1ULL<<GPIO_INPUT)
#define GPIO_OUTPUT -1
#define GPIO_OUTPUT_PIN_SEL (1ULL<<GPIO_OUTPUT)
#define BITBANG 3
#define UART_TTY 2
#define RED "\e[0;31m"
#define GRN "\e[0;32m"




void ulip_core_capture_finger(bool status, int index)
{
    if (status)
        fpm_set_enroll(index);
    else
        fpm_cancel_enroll();
}
static void ctl_event(int event, int status) {
    printf("event rolou ctl");
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
int cnt = 0;
static int qrcode_event(int event, const char *data,
                        int len, void *user_data)
{
    printf(RED "%s\n", data);
    return 1;
}

static tty_func_t bitbang_event(int tty, char *data,
                      int len, void *user_data)
{
    printf("event bitbang rolou: ");
    for (int i = 0; i < len; i++)
    {
        printf("%c",data[i]);
    }
    printf("\n");
    tty_func_t t = NULL;
    return t;
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
    

    if (intr == 4) {
        ulip_core_capture_finger(true, 0);
    }
}
void app_main(void)
{
    // gpio_config_t io_conf;
    // //disable interrupt
    // io_conf.intr_type = GPIO_INTR_DISABLE;
    // //set as output mode
    // io_conf.mode = GPIO_MODE_OUTPUT;
    // //bit mask of the pins that you want to set,e.g.GPIO18/19
    // io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    // //disable pull-down mode
    // io_conf.pull_down_en = 0;
    // //disable pull-up mode
    // io_conf.pull_up_en = 0;
    // //configure GPIO with the given settings
    // gpio_config(&io_conf);


    // gpio_set_level(GPIO_OUTPUT, 1);

    // ctl_init(CTL_MODE_NORMAL, ctl_event);
    // start_eth();
    // // CFG_Load();
    tty_init();
    // qrcode_init(CFG_get_qrcode_led(), true,
    //                 CFG_get_qrcode_timeout(),
    //                 CFG_get_qrcode_panic_timeout(),
    //                 CFG_get_qrcode_dynamic(),
    //                 CFG_get_qrcode_validity(),
    //                 qrcode_event, NULL);
    
    //tty_open(UART_TTY,test_event, NULL);

    fpm_init(0,
                 2,
                 2,
                 fingerprint_event, NULL);
    gpio_interrupt_open(4, GPIO_INPUT, GPIO_INTR_NEGEDGE, 0, buttonPressed, NULL);
    printf("Hello world!\n");
    
}

