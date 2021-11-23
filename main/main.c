/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <sys/time.h>
#include "string.h"
#include "time.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "tty.h"
#include "gpio_drv.h"
#include "eth.h"
#include "ctl.h"
#include "qrcode2.h"
#include "config2.h"
#include "fpm.h"
#include "ap.h"
#include "http.h"
#include "httpd.h"
#include "esp_log.h"
#include "account.h"
#include "bluetooth.h"
#include "rf433.h"
#include "rfid.h"
#include "rs485.h"
#include "udp_logging.h"
#include "esp32_perfmon.h"
#include "sdkconfig.h"

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
#define GPIO_INPUT 16
#define GPIO_INPUT_PIN_SEL (1ULL<<GPIO_INPUT)
#define GPIO_OUTPUT 4
#define GPIO_OUTPUT_PIN_SEL (1ULL<<GPIO_OUTPUT)
#define BITBANG 3
#define UART_TTY 2
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define ULIP_MODEL "MLI-1WB"
#define MAGIC_CODE "uTech"
    
static int clicks = 0;
static int64_t cnt = 0;
static double average = 0;
static unsigned char cmd[] = {0x7e, 0x00, 0x08, 0x01, 0x00, 0x00, 0x88, 0x64, 0x19};
static bool initialized = false;
static esp_timer_handle_t handle;
static esp_timer_handle_t handle2;
typedef union {
    unsigned char *b;
    unsigned short *w;
    unsigned long *dw;
} pgen_t;

static void rs485_event(unsigned char from_addr,
                        unsigned char ok,
                        unsigned char control,
                        unsigned char *frame,
                        unsigned short len,
                        void *user_data)
{
    const unsigned char *name = NULL;
    const unsigned char *user = NULL;
    const unsigned char *password = NULL;
    const unsigned char *card = NULL;
    const unsigned char *qrcode = NULL;
    const unsigned char *rfcode = NULL;
    const unsigned char *key = NULL;
    uint8_t accessibility = false;
    uint8_t panic = false;
    uint8_t administrator = false;
    uint8_t visitor = false;
    acc_permission_t perm[ACCOUNT_PERMISSIONS] = { 0 };
    account_t *acc;
    int index = -1;
    const char *url;
    uint16_t size;
    uint32_t id;
    uint8_t cmd;
    uint8_t state;
    char date[32];
    unsigned char buf[64];
    pgen_t p;
    struct tm tm;
    char *d;
    char *t;
    int i;
    int k;

    if (!ok) {
        ESP_LOGD("ULIP", "RS485 CRC error!");
        return;
    }

    if (control & PSH_DATA) {
        // ESP_LOGD("ULIP", "RS485 frame address [%d] len [%d]",
        //          from_addr, len);
        if (len < 7) return;
        p.b = frame;
        size = *p.b++;
        size |= (*p.b++ << 8);
        if (size > len) return;
        id = *p.b++;
        id |= (*p.b++ << 8);
        id |= (*p.b++ << 16);
        id |= (*p.b++ << 24);
        cmd = *p.b++;
        size -= 7;
        ++cnt;
        int64_t time_elapsed = esp_timer_get_time()/1000000;
        average = (double)cnt / (double)time_elapsed;
        ESP_LOGE("main","cmd: %02x media: %f", cmd, average);
        ctl_beep(1);
        switch (cmd) {
            case RS485_CMD_POLLING:
                /* do nothing */
                break;
            case RS485_CMD_STATUS:
                /* Response */
                p.b = buf;
                *p.w++ = 10;
                *p.b++ = id & 0xff;
                *p.b++ = (id >> 8) & 0xff;
                *p.b++ = (id >> 16) & 0xff;
                *p.b++ = (id >> 24) & 0xff;
                *p.b++ = RS485_CMD_STATUS;
                *p.b++ = true;
                *p.b++ = 1 == CTL_RELAY_OFF ? 0 : 1;
                *p.b++ = ctl_sensor_status() == CTL_SENSOR_OFF ? 0 : 1;
                // ESP_LOG_BUFFER_HEX("main", buf, 10);
                rs485_tx_frame(from_addr, buf, 10);
                break;
            case RS485_CMD_RELAY:
                if (size <= 0) return;
                state = *p.b;
                /* Response */
                p.b = buf;
                *p.w++ = 8;
                *p.b++ = id & 0xff;
                *p.b++ = (id >> 8) & 0xff;
                *p.b++ = (id >> 16) & 0xff;
                *p.b++ = (id >> 24) & 0xff;
                *p.b++ = RS485_CMD_RELAY;
                *p.b++ = true;
                rs485_tx_frame(from_addr, buf, 8);
                if (state == RS485_CONTROL_OPEN) {
                    if (CFG_get_control_mode() == CFG_CONTROL_MODE_AUTO)
                        ctl_relay_on(CFG_get_control_timeout());
                    else
                        ctl_relay_on(0);
                } else if (state == RS485_CONTROL_HOLD) {
                    ctl_relay_on(0);
                } else {
                    ctl_relay_off();
                }
                break;
            case RS485_CMD_ALARM:
                if (size <= 0) return;
                state = *p.b;
                /* Response */
                p.b = buf;
                *p.w++ = 9;
                *p.b++ = id & 0xff;
                *p.b++ = (id >> 8) & 0xff;
                *p.b++ = (id >> 16) & 0xff;
                *p.b++ = (id >> 24) & 0xff;
                *p.b++ = RS485_CMD_ALARM;
                *p.b++ = true;
                *p.b++ = state;
                rs485_tx_frame(from_addr, buf, 9);
                if (state) {
                    ESP_LOGI("main", "alarm onnnnn");
                    ctl_alarm_on();
                } else {
                    ESP_LOGI("main", "alarm offfffffff");
                    ctl_alarm_off();
                }
                break;
            case RS485_CMD_PANIC:
                if (size <= 0) return;
                state = *p.b;
                /* Response */
                p.b = buf;
                *p.w++ = 9;
                *p.b++ = id & 0xff;
                *p.b++ = (id >> 8) & 0xff;
                *p.b++ = (id >> 16) & 0xff;
                *p.b++ = (id >> 24) & 0xff;
                *p.b++ = RS485_CMD_PANIC;
                *p.b++ = true;
                *p.b++ = state;
                rs485_tx_frame(from_addr, buf, 9);
                if (state) {
                    ctl_panic_on();
                } else {
                    ctl_panic_off();
                }
                break;
            case RS485_CMD_ADDUSER:
                if (size < 11) return;
                /* Name */
                if (*p.b) {
                    name = p.b;
                    p.b += strlen((char *)p.b) + 1;
                } else {
                    p.b++;
                }
                /* User */
                if (*p.b) {
                    user = p.b;
                    p.b += strlen((char *)p.b) + 1;
                } else {
                    p.b++;
                }
                /* Password */
                if (*p.b) {
                    password = p.b;
                    p.b += strlen((char *)p.b) + 1;
                } else {
                    p.b++;
                }
                /* Card */
                if (*p.b) {
                    card = p.b;
                    p.b += strlen((char *)p.b) + 1;
                } else {
                    p.b++;
                }
                /* QRCode */
                if (*p.b) {
                    qrcode = p.b;
                    p.b += strlen((char *)p.b) + 1;
                } else {
                    p.b++;
                }
                /* RFCode */
                if (*p.b) {
                    rfcode = p.b;
                    p.b += strlen((char *)p.b) + 1;
                } else {
                    p.b++;
                }
                /* Key */
                if (*p.b) {
                    key = p.b;
                    p.b += strlen((char *)p.b) + 1;
                } else {
                    p.b++;
                }
                accessibility = *p.b++;
                panic = *p.b++;
                administrator = *p.b++;
                visitor = *p.b++;
                /* Permissions */
                k = 0;
                for (i = 0; i < ACCOUNT_PERMISSIONS; i++) {
                    if (*p.b == 0)
                        break;
                    len = strlen((char *)p.b) + 1;
                    memcpy(perm[size], p.b, len);
                    p.b += len;
                    k++;
                }
                acc = account_new();
                if (acc) {
                    if (name)
                        account_set_name(acc, (char *)name);
                    if (user)
                        account_set_user(acc, (char *)user);
                    if (password)
                        account_set_password(acc, (char *)password);
                    if (card)
                        account_set_card(acc, (char *)card);
                    if (qrcode)
                        account_set_code(acc, (char *)qrcode);
                    if (rfcode)
                        account_set_rfcode(acc, (char *)rfcode);
                    if (key)
                        account_set_key(acc, (char *)key);
                    account_set_accessibility(acc, accessibility);
                    account_set_panic(acc, panic);
                    account_set_level(acc, administrator);
                    account_set_visitor(acc, visitor);
                    if (k)
                        account_set_permission(acc, perm, k);
                    index = account_db_insert(acc);
                    account_destroy(acc);
                }
                p.b = buf;
                *p.w++ = 9;
                *p.b++ = id & 0xff;
                *p.b++ = (id >> 8) & 0xff;
                *p.b++ = (id >> 16) & 0xff;
                *p.b++ = (id >> 24) & 0xff;
                *p.b++ = RS485_CMD_ADDUSER;
                *p.b++ = true;
                *p.b++ = index != -1 ? true : false;
                rs485_tx_frame(from_addr, buf, 9);
                break;
            case RS485_CMD_DELUSER:
                if (size < 4) return;
                /* User */
                if (*p.b) {
                    user = p.b;
                    p.b += strlen((char *)p.b) + 1;
                } else {
                    p.b++;
                }
                /* Card */
                if (*p.b) {
                    card = p.b;
                    p.b += strlen((char *)p.b) + 1;
                } else {
                    p.b++;
                }
                /* QRCode */
                if (*p.b) {
                    qrcode = p.b;
                    p.b += strlen((char *)p.b) + 1;
                } else {
                    p.b++;
                }
                /* RFCode */
                if (*p.b) {
                    rfcode = p.b;
                    p.b += strlen((char *)p.b) + 1;
                } else {
                    p.b++;
                }
                /* Key */
                if (*p.b) {
                    key = p.b;
                    p.b += strlen((char *)p.b) + 1;
                } else {
                    p.b++;
                }
                if (!key) {
                    index = account_db_find(NULL,(char *) user, (char *)card, (char *)qrcode, (char *)rfcode,
                                            NULL, NULL);
                    if (index != -1)
                        account_db_delete(index);
                } else {
                    while ((index = account_db_find(NULL, NULL, NULL, NULL,
                                                    NULL, NULL, (char *)key)) != -1)
                        account_db_delete(index);
                }
                p.b = buf;
                *p.w++ = 9;
                *p.b++ = id & 0xff;
                *p.b++ = (id >> 8) & 0xff;
                *p.b++ = (id >> 16) & 0xff;
                *p.b++ = (id >> 24) & 0xff;
                *p.b++ = RS485_CMD_DELUSER;
                *p.b++ = true;
                *p.b++ = index != -1 ? true : false;
                rs485_tx_frame(from_addr, buf, 9);
                break;
            case RS485_CMD_PROBEUSER:
                if (size < 1) return;
                state = *p.b++;
                size -= 1;
                /* Find account */
                if (size > 0) {
                    /* User */
                    len = strlen((char *)p.b) + 1;
                    if (*p.b) {
                        user = p.b;
                        p.b += len;
                    } else {
                        p.b++;
                    }
                    size -= len;
                }
                if (size > 0) {
                    /* Card */
                    len = strlen((char *)p.b) + 1;
                    if (*p.b) {
                        card = p.b;
                        p.b += len;
                    } else {
                        p.b++;
                    }
                    size -= len;
                }
                if (size > 0) {
                    /* QRCode */
                    len = strlen((char *)p.b) + 1;
                    if (*p.b) {
                        qrcode = p.b;
                        p.b += len;
                    } else {
                        p.b++;
                    }
                    size -= len;
                }
                if (size > 0) {
                    /* RFCode */
                    len = strlen((char *)p.b) + 1;
                    if (*p.b) {
                        rfcode = p.b;
                        p.b += len;
                    } else {
                        p.b++;
                    }
                    size -= len;
                }
                index = account_db_find(NULL, (char *)user, (char *)card, (char *)qrcode,
                                        (char *)rfcode, NULL, NULL);
                /* Response */
                p.b = buf;
                *p.w++ = 9;
                *p.b++ = id & 0xff;
                *p.b++ = (id >> 8) & 0xff;
                *p.b++ = (id >> 16) & 0xff;
                *p.b++ = (id >> 24) & 0xff;
                *p.b++ = RS485_CMD_PROBEUSER;
                *p.b++ = true;
                *p.b++ = state;
                rs485_tx_frame(from_addr, buf, 9);
                if (state) {
                    // probe_user = true;
                    // probe_index = index;
                } else {
                    // probe_user = false;
                    // probe_index = -1;
                }
                break;
            case RS485_CMD_ERASEUSER:
                if (size <= 0) return;
                state = *p.b;
                /* Response */
                p.b = buf;
                *p.w++ = 9;
                *p.b++ = id & 0xff;
                *p.b++ = (id >> 8) & 0xff;
                *p.b++ = (id >> 16) & 0xff;
                *p.b++ = (id >> 24) & 0xff;
                *p.b++ = RS485_CMD_ERASEUSER;
                *p.b++ = true;
                *p.b++ = state;
                rs485_tx_frame(from_addr, buf, 9);
                // if (state) {
                //     erase_user = true;
                // } else {
                //     erase_user = false;
                // }
                break;
            case RS485_CMD_ERASEALL:
                account_db_remove_all();
                /* Response */
                p.b = buf;
                *p.w++ = 8;
                *p.b++ = id & 0xff;
                *p.b++ = (id >> 8) & 0xff;
                *p.b++ = (id >> 16) & 0xff;
                *p.b++ = (id >> 24) & 0xff;
                *p.b++ = RS485_CMD_ERASEALL;
                *p.b++ = true;
                rs485_tx_frame(from_addr, buf, 8);
                break;
            case RS485_CMD_REBOOT:
                esp_restart();
                break;
            case RS485_CMD_GETCONFIG:
                /* Response */
                len = strlen((char *)ULIP_MODEL) + 1;
                len += strlen((char *)CFG_get_serialnum()) + 1;
                len += strlen((char *)CFG_get_release()) + 1;
                p.b = buf;
                *p.w++ = len + 11;
                *p.b++ = id & 0xff;
                *p.b++ = (id >> 8) & 0xff;
                *p.b++ = (id >> 16) & 0xff;
                *p.b++ = (id >> 24) & 0xff;
                *p.b++ = RS485_CMD_GETCONFIG;
                *p.b++ = true;
                size = strlen((char *)ULIP_MODEL) + 1;
                memcpy(p.b, ULIP_MODEL, size);
                p.b += size;
                size = strlen((char *)CFG_get_serialnum()) + 1;
                memcpy(p.b, CFG_get_serialnum(), size);
                p.b += size;
                size = strlen((char *)CFG_get_release()) + 1;
                memcpy(p.b, CFG_get_release(), size);
                ESP_LOG_BUFFER_CHAR("main",CFG_get_release(), size);
                p.b += size;
                *p.b++ = CFG_get_standalone();
                *p.b++ = CFG_get_rs485_hwaddr();
                *p.b++ = CFG_get_rs485_server_hwaddr();
                rs485_tx_frame(from_addr, buf, len + 11);
                break;
            case RS485_CMD_SETCONFIG:
                if (size < 3) return;
                CFG_set_standalone(*p.b++);
                CFG_set_rs485_hwaddr(*p.b++);
                CFG_set_rs485_server_hwaddr(*p.b++);
                CFG_set_rs485_enable(true);
                /* Save configuration */
                CFG_Save();
                /* Response */
                p.b = buf;
                *p.w++ = 8;
                *p.b++ = id & 0xff;
                *p.b++ = (id >> 8) & 0xff;
                *p.b++ = (id >> 16) & 0xff;
                *p.b++ = (id >> 24) & 0xff;
                *p.b++ = RS485_CMD_SETCONFIG;
                *p.b++ = true;
                rs485_tx_frame(from_addr, buf, 8);
                break;
            case RS485_CMD_BEEP:
                if (size < 1) return;
                if (*p.b) {
                    ctl_beep(0);
                } else {
                    ctl_buzzer_on(CTL_BUZZER_ERROR);
                }
                /* Response */
                p.b = buf;
                *p.w++ = 8;
                *p.b++ = id & 0xff;
                *p.b++ = (id >> 8) & 0xff;
                *p.b++ = (id >> 16) & 0xff;
                *p.b++ = (id >> 24) & 0xff;
                *p.b++ = RS485_CMD_BEEP;
                *p.b++ = true;
                rs485_tx_frame(from_addr, buf, 8);
                break;
            case RS485_CMD_GETDATETIME:
                /* Response */
                p.b = buf;
                time(&tm);
                len = sprintf(date, "%04d-%02d-%02d %02d:%02d:%02d",
                                 tm.tm_year, tm.tm_mon + 1, tm.tm_mday,
                                 tm.tm_hour, tm.tm_min, tm.tm_sec);
                len += 1;
                *p.w++ = len + 8;
                *p.b++ = id & 0xff;
                *p.b++ = (id >> 8) & 0xff;
                *p.b++ = (id >> 16) & 0xff;
                *p.b++ = (id >> 24) & 0xff;
                *p.b++ = RS485_CMD_GETDATETIME;
                *p.b++ = true;
                memcpy(p.b, date, len);
                p.b += len;
                rs485_tx_frame(from_addr, buf, len + 8);
                break;
            case RS485_CMD_SETDATETIME:
                if (size < 1) return;
                d = (char *)p.b;
                d = strtok_r(d, " ", &t);
                d = strtok(d, "-");
                if (!d) return;
                /* Date */
                tm.tm_year = strtol(d, NULL, 10); 
                d = strtok(NULL, "-");
                if (!d) return;
                tm.tm_mon = strtol(d, NULL, 10) - 1;
                d = strtok(NULL, "-");
                if (!d) return;
                tm.tm_mday = strtol(d, NULL, 10);
                /* Time */
                t = strtok(t, ":");
                if (!t) return;
                tm.tm_hour = strtol(t, NULL, 10);
                t = strtok(NULL, ":");
                if (!t) return;
                tm.tm_min = strtol(t, NULL, 10);
                t = strtok(NULL, ":");
                if (!t) return;
                tm.tm_sec = strtol(t, NULL, 10);
                
                struct timeval tv;
                tv.tv_sec = mktime(&tm);
                tv.tv_usec = 0;

                settimeofday(&tv,NULL);
                // rtc_set_time(rtc_mktime(&tm));
                /* Save date and time in flash */
                // if (abs(rtc_time() - CFG_get_rtc_time()) > 60) {
                //     CFG_set_rtc_time(rtc_time());
                //     CFG_Save();
                // }
                /* Response */
                p.b = buf;
                *p.w++ = 8;
                *p.b++ = id & 0xff;
                *p.b++ = (id >> 8) & 0xff;
                *p.b++ = (id >> 16) & 0xff;
                *p.b++ = (id >> 24) & 0xff;
                *p.b++ = RS485_CMD_SETDATETIME;
                *p.b++ = true;
                rs485_tx_frame(from_addr, buf, 8);
                break;
        case RS485_CMD_UPDATE:
                if (size < 2) return;
                url = (const char *)p.b;
                CFG_set_ota_url(url);
                CFG_Save();
                // ulip_core_system_update(url);
                break;
#if defined(__MLI_1WRS_TYPE__) || defined(__MLI_1WRG_TYPE__) || \
    defined(__MLI_1WRC_TYPE__)
            case RS485_CMD_TELEMETRY:
                /* Response */
                p.b = buf;
                len = strlen((char *)dht_get_str_temperature()) + 1;
                len += strlen((char *)dht_get_str_humidity()) + 1;
                if (CFG_get_sensor_type() == CFG_SENSOR_VOLUME)
                    len += strlen((char *)CFG_get_sensor_str_volume());
                len += 1;
                *p.w++ = len + 16;
                *p.b++ = id & 0xff;
                *p.b++ = (id >> 8) & 0xff;
                *p.b++ = (id >> 16) & 0xff;
                *p.b++ = (id >> 24) & 0xff;
                *p.b++ = RS485_CMD_TELEMETRY;
                *p.b++ = true;
                size = strlen((char *)dht_get_str_temperature()) + 1;
                memcpy(p.b, dht_get_str_temperature(), size);
                p.b += size;
                *p.b++ = dht_get_temperature_alarm();
                size = strlen((char *)dht_get_str_humidity()) + 1;
                memcpy(p.b, dht_get_str_humidity(), size);
                p.b += size;
                *p.b++ = dht_get_humidity_alarm();
#if defined(__MLI_1WRS_TYPE__)
                *p.w++ = temt_get_lux();
                *p.b++ = temt_get_alarm();
#elif defined(__MLI_1WRG_TYPE__)
                *p.w++ = mq2_get_gas();
                *p.b++ = mq2_get_alarm();
#else
                *p.w++ = cli_get_value();
                *p.b++ = cli_get_alarm();
#endif
                *p.b++ = pir_get_status();
                /* Check sensor type */
                if (CFG_get_sensor_type() == CFG_SENSOR_LEVEL)
                    *p.b++ = ctl_sensor_status() == CTL_SENSOR_ON ?
                             RS485_SENSOR_ALARM : RS485_SENSOR_NORMAL;
                else
                    *p.b++ = RS485_SENSOR_DISABLED;
                if (CFG_get_sensor_type() == CFG_SENSOR_VOLUME) {
                    size = strlen((char *)CFG_get_sensor_str_volume()) + 1;
                    memcpy(p.b, CFG_get_sensor_str_volume(), size);
                    p.b += size;
                    if (CFG_get_sensor_limit()) {
                        if (CFG_get_sensor_volume() > CFG_get_sensor_limit())
                            *p.b++ = RS485_SENSOR_ALARM;
                        else
                            *p.b++ = RS485_SENSOR_NORMAL;
                    } else {
                        *p.b++ = RS485_SENSOR_NORMAL;
                    }
                } else {
                    *p.b++ = 0;
                    *p.b++ = RS485_SENSOR_DISABLED;
                }
                rs485_tx_frame(from_addr, buf, len + 16);
                break;
            case RS485_CMD_DELTELEMETRY:
                /* Response */
                p.b = buf;
                *p.w++ = 8;
                *p.b++ = id & 0xff;
                *p.b++ = (id >> 8) & 0xff;
                *p.b++ = (id >> 16) & 0xff;
                *p.b++ = (id >> 24) & 0xff;
                *p.b++ = RS485_CMD_DELTELEMETRY;
                *p.b++ = true;
                sensor_cycles = 0;
                if (CFG_get_sensor_cycles()) {
                    CFG_set_sensor_cycles(0);
                    CFG_Save();
                }
                rs485_tx_frame(from_addr, buf, 8);
                break;
#endif
        }
    }
}


static int rfid_event(int event, const char *data, int len,
                      void *user_data)
{
    ESP_LOGE("main","event rfid %s", data);
    return 1;
}
char tasks_info[1024];
static void got_ip_event() {
    uint8_t mode;
    uint8_t level;
    char * host;
    uint16_t port;
    CFG_get_debug(&mode, &level, &host, &port);
    udp_logging_init( host, port, udp_logging_vprintf );
    // rfid_init(CFG_get_rfid_timeout(),
    //             CFG_get_rfid_retries(),
    //             CFG_get_rfid_nfc(),
    //             CFG_get_rfid_panic_timeout(),
    //             CFG_get_rfid_format(),
    //             rfid_event, NULL);
}
static void ctl_event(int event, int status);
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
    ets_printf("chave %s\n", data);
    return 1;
}
static int qrcode_event_main(int event, const char *data, int len, void *user_data)
{
    printf(RED "%s and compare %d\n", data, strcmp(data, MAGIC_CODE));
    if (1) {
        ctl_beep(3);
    }
    
    return 1;
}
static void http_event(char *url, char *response_body, int http_status, char *response_header_key,char *response_header_value, int body_size) {
    printf("event %s", response_body);
}
static void http_event2(char *url, char *response_body, int http_status, char *response_header_key,char *response_header_value, int body_size) {
    printf("event2 %s", response_body);
}
static tty_func_t test_event(int tty, char *data,
                      int len, void *user_data)
{
    printf("event rolou teste\n");
    ESP_LOG_BUFFER_HEX("main", data, len);

    tty_func_t t = NULL;
    return t;
}
static tty_func_t test_event2(int tty, char *data,
                      int len, void *user_data)
{
    printf("event rolou2: ");
    for (int i = 0; i < len; i++)
    {
        printf("%c",data[i]);
    }
    printf("\n");
    tty_func_t t = NULL;
    return t;
}
void release_task(){
    if (clicks == 0) {
         start_eth(CFG_get_dhcp(), CFG_get_ip_address(), CFG_get_gateway(), CFG_get_netmask(), &got_ip_event);
    } else if (clicks > 0 ){
        // vTaskGetRunTimeStats(tasks_info);
        vTaskList(tasks_info);
        ESP_LOGI("main", "\n%s", tasks_info);
    } else if (clicks == 2 ){
        release_eth();
    } else {
        vTaskList(tasks_info);
        ESP_LOGI("main", "\n%s", tasks_info);
    }
    clicks++;
    fpm_init(CFG_get_fingerprint_timeout(),CFG_get_fingerprint_security(),
            CFG_get_fingerprint_identify_retries(),fingerprint_event, NULL);
    // fpm_release();
    // rs485_release();
    // ctl_release();
    // tty_release();
    // vTaskDelay(50);
    // tty_init();
    // ctl_init(CTL_MODE_NORMAL, ctl_event, CFG_get_ap_mode(), CFG_get_ip_address(),
    //          CFG_get_netmask(), CFG_get_gateway(), CFG_get_dhcp(),
    //         CFG_get_wifi_ssid(), CFG_get_wifi_passwd(), CFG_get_wifi_channel(), CFG_get_wifi_disable());
    // ctl_set_sensor_mode(1);
    // fpm_init(CFG_get_fingerprint_timeout(),CFG_get_fingerprint_security(),CFG_get_fingerprint_identify_retries(),fingerprint_event, NULL);
    vTaskDelete(NULL);
}
static void ctl_event(int event, int status) {
    printf("event rolou ctl: %d status %d\n", event, status);
    switch (event)
    {
    case CTL_EVT_RELAY:

        
        break;
    case CTL_EVT_BUZZER:

        
        break;
    case CTL_EVT_SENSOR:
        xTaskCreate(release_task, "release task", 4096, NULL, 10, NULL);
        break;
    default:
        printf("ctl event not supoorted\n");
        break;
    }
}

unsigned char * data = (unsigned char *)"\x88";

static void rs485_rx_data(int tty, const char *data,
                          int len, void *user_data)
{
    printf("data %s", data);
}

static void timer_callback() {
    xTaskCreate(release_task, "release task", 4096, NULL, 10, NULL);
}
void app_main(void)
{
    
    CFG_Load();
    CFG_set_ip_address("10.0.0.140");
    CFG_set_netmask("255.255.255.0");
    CFG_set_gateway("10.0.0.1");
    CFG_set_ap_mode(false);
    CFG_set_dhcp(true);
    CFG_set_wifi_ssid("uTech-Wifi");
    CFG_set_wifi_passwd("01566062");
    CFG_set_wifi_disable(true);
    CFG_set_debug(1, ESP_LOG_INFO, "10.42.0.79", 64195);
    ESP_LOGI("main", "set config");
    ctl_init(CTL_MODE_NORMAL, ctl_event, CFG_get_ap_mode(), CFG_get_ip_address(),
             CFG_get_netmask(), CFG_get_gateway(), CFG_get_dhcp(),
            CFG_get_wifi_ssid(), CFG_get_wifi_passwd(), CFG_get_wifi_channel(), CFG_get_wifi_disable(), &got_ip_event);
    ESP_LOGI("main", "ctl init");
    tty_init();
    // printf("%d", ++cnt);
    ESP_LOGI("main", "init tty");
    ctl_set_sensor_mode(1);

    // vTaskDelay(pdMS_TO_TICKS(5000));1313
    ESP_LOGI("main", "init eth");
    // perfmon_start();
    
    // qrcode_init(true, true,
    //                 0,
    //                 CFG_get_qrcode_panic_timeout(),
    //                 CFG_get_qrcode_dynamic(),
    //                 CFG_get_qrcode_validity(),
    //                 qrcode_event_main, NULL, 3);

    // account_init();
    // rf433_init(CFG_get_rf433_rc(), CFG_get_rf433_bc(),
    //                CFG_get_rf433_panic_timeout(),
    //                rf433_event, NULL);
    
    // bluetooth_start();

    CFG_set_rs485_hwaddr(2);
    CFG_set_rs485_server_hwaddr(1);
    rs485_init(0, CFG_get_rs485_hwaddr(), 3, 1000000,
                   rs485_event, NULL);
    printf("Hello world!\n");
    ESP_LOGI("main", "tasks: %u", uxTaskGetNumberOfTasks());
    
    vTaskList(tasks_info);
    ESP_LOGI("main", "\n%s", tasks_info);
    // if (!initialized) {
    //     esp_timer_create_args_t timer = {
    //         .callback = &timer_callback
    //     };
        
    //     esp_timer_create(&timer, &handle);
    //     esp_timer_create_args_t timer2 = {
    //         .callback = &app_main
    //     };
    //     esp_timer_create(&timer2, &handle2);
    // }
    // esp_timer_start_once(handle, 5000000);

    // esp_timer_start_once(handle2, 10000000);
    
    // initialized = true;
    
}
