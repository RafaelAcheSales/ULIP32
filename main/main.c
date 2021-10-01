/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tty.h"
#include "gpio_drv.h"
#include "eth.h"
#include "ctl.h"
#include "qrcode2.h"
#include "config.h"
#include "fpm.h"
#include "ap.h"
#include "http.h"
#include "httpd.h"
#include "esp_log.h"
#include "account.h"
#include "bluetooth.h"
#include "rf433.h"
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
static int rf433_event(int event, const char *data, int len,
                       uint16_t sync, uint8_t button,
                       uint8_t status, void *user_data)
{
    printf("event rolou rf433 of len: %d event %X\n", len, event);
    return 1;
}
static int qrcode_event(int event, const char *data,
                        int len, void *user_data)
{
    printf(RED "%s\n", data);
    return 1;
}
// static void http_event(char *url, char *response_body, int http_status, char *response_header_key,char *response_header_value, int body_size) {
//     printf("event %s", response_body);
// }
// static void http_event2(char *url, char *response_body, int http_status, char *response_header_key,char *response_header_value, int body_size) {
//     printf("event2 %s", response_body);
// }
// static tty_func_t test_event(int tty, char *data,
//                       int len, void *user_data)
// {
//     printf("event rolou: ");
//     for (int i = 0; i < len; i++)
//     {
//         printf("%c",data[i]);
//     }
//     printf("\n");
//     tty_func_t t = NULL;
//     return t;
// }

static void ctl_event(int event, int status) {
    printf("event rolou ctl: %d status %d\n", event, status);
    switch (event)
    {
    case CTL_EVT_RELAY:

        
        break;
    case CTL_EVT_BUZZER:

        
        break;
    case CTL_EVT_SENSOR:
        ESP_LOGI("main","botao");
        // change_value();
        // ulip_core_capture_finger(true, 4);
        // start_httpd();
        // http://www.ibam.org.br
        // http_raw_request("www.ibam.org.br",CFG_get_server_port(), false, "", "", "/media/css/externo.css", NULL, "", "", 5, http_event);
        // http_raw_request("www.ibam.org.br",CFG_get_server_port(), false, "", "", "/media/js/externo.js", NULL, "", "", 5, http_event2);
        
        break;
    default:
        printf("ctl event not supoorted\n");
        break;
    }
}

unsigned char * data = (unsigned char *)"\x88";
static void buttonPressed(int intr, void *user_data) {
    

    if (intr == 2) {

        // ctl_relay_on(500000);
        // printf("reading %d port\n", CFG_Get_blobs());
        // CFG_set_server_port(34);
        // CFG_Save();
        // printf("reading2 %d port\n", CFG_Get_blobs());

        
        
        // // CFG_Load();
        // printf("reading %d port", CFG_get_server_port());

        // tty_release();
        // ctl_release();
        // printf("ctl released\n");
        // ctl_beep(3);
        
        // fpm_release();
        
        // fpm_delete_all();
        // ulip_core_capture_finger(true, 3);
        cnt++;
    }
}
void app_main(void)
{
    // 

    CFG_Load();
    // CFG_set_dhcp(true);
    // CFG_set_qrcode_led(false);
    // gpio_config_t io_conf;
    // //disable interrupt
    // io_conf.intr_type = GPIO_INTR_DISABLE;
    // //set as output mode
    // io_conf.mode = GPIO_MODE_INPUT;
    // //bit mask of the pins that you want to set,e.g.GPIO18/19
    // io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // //disable pull-down mode
    // io_conf.pull_down_en = 0;
    // //disable pull-up mode
    // io_conf.pull_up_en = 0;
    // //configure GPIO with the given settings
    // // gpio_config(&io_conf);

    tty_init();
    // start_eth(CFG_get_dhcp(), CFG_get_ip_address(), CFG_get_gateway(), CFG_get_netmask());
    ctl_init(CTL_MODE_NORMAL, ctl_event);
    ctl_set_sensor_mode(1);
    
    // // qrcode_init(false, true,
    // //                 1000000,
    // //                 2000000,
    // //                 true,
    // //                 30,
    // //                 qrcode_event, NULL);
    // qrcode_init(CFG_get_qrcode_led(), true,
    //                 CFG_get_qrcode_timeout(),
    //                 CFG_get_qrcode_panic_timeout(),
    //                 CFG_get_qrcode_dynamic(),
    //                 CFG_get_qrcode_validity(),
    //                 qrcode_event, NULL);
    // // tty_open(BITBANG,test_event, NULL);
    // // fpm_init(0,2,2,fingerprint_event, NULL);
    // // printf("fpm setup timeout: %d, security: %d, retry: %d\n", CFG_get_fingerprint_timeout(),CFG_get_fingerprint_security(),CFG_get_fingerprint_identify_retries());
    // fpm_init(CFG_get_fingerprint_timeout(),CFG_get_fingerprint_security(),
    //         CFG_get_fingerprint_identify_retries(),fingerprint_event, NULL);
    // // gpio_interrupt_open(2, GPIO_INPUT, GPIO_INTR_NEGEDGE, 0, buttonPressed, NULL);
    // account_init();
    // account_t *acc = account_new();
    // // vTaskDelay(50);
    // printf("created acc\n");
    
    // account_set_user(acc, "usuarioDeCrack");
    // printf("set user\n");
    // account_set_name(acc, "Pirocadura");
    // account_log_t *log = account_log_new();
    // account_log_set_code(log, "codigolog");
    // account_log_set_name(log, "nomedolog");
    // int logindex = account_db_log_insert(log);
    // int index = account_db_insert(acc);
    // printf("index of: %d\n", index);
    // account_destroy(acc);
    // account_log_destroy(log);
    // // vTaskDelay(100);
    // account_t *acc2 = account_db_get_index(index);
    // account_log_t *log2 = account_db_log_get_index(logindex);
    // printf("got account\n");
    // if (!acc2 )
    // {
    //     printf("account not found by index");
        
    // }
    
    // char * name =  account_log_get_code(log2);

    // // printf("got account with name %s\n", name);
    // printf("got log with code %s\n", name);
    // account_db_remove_all();
    // printf("remove all \n");
    // account_t *acc3 = account_db_get_index(index);
    // printf("get acc3 \n");
    // char * name2 = account_get_user(acc3);
    // printf("get user \n");
    // if (name2) {
    //     printf("got acc with name %s\n", name2);
    // }
    rf433_init(CFG_get_rf433_rc(), CFG_get_rf433_bc(),
                   CFG_get_rf433_panic_timeout(),
                   rf433_event, NULL);
    // bluetooth_start();
    printf("Hello world!\n");
    // start_ap();
}
