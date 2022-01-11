/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "account.h"
#include "ap.h"
#include "bluetooth.h"
#include "config2.h"
#include "ctl.h"
#include "esp32_perfmon.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "utils.h"
#include "eth.h"
#include "fpm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio_drv.h"
#include "http.h"
#include "httpd2.h"
#include "qrcode2.h"
#include "rf433.h"
#include "rfid.h"
#include "rs485.h"
#include "sdkconfig.h"
#include "string.h"
#include "time.h"
#include "tty.h"
#include "udp_logging.h"
#include <stdio.h>
#include <sys/time.h>

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define GPIO_INPUT 16
#define GPIO_INPUT_PIN_SEL (1ULL << GPIO_INPUT)
#define GPIO_OUTPUT 4
#define GPIO_OUTPUT_PIN_SEL (1ULL << GPIO_OUTPUT)
#define BITBANG 3
#define UART_TTY 2
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define ULIP_MODEL "MLI-1WB"
#define MAGIC_CODE "uTech"

static int clicks = 0;
static int64_t cnt = 0;
static int priority = 10;
static double average = 0;
// static unsigned char cmd[] = {0x7e, 0x00, 0x08, 0x01, 0x00, 0x00, 0x88, 0x64, 0x19};
// static bool initialized = false;
static esp_timer_handle_t reboot_timer;
static esp_netif_ip_info_t wifi_ip_info;
static bool system_restored = false;
esp_netif_ip_info_t eth_ip_info;
const char *weekday[7];
const char *monthday[12];


typedef union
{
    unsigned char *b;
    unsigned short *w;
    unsigned long *dw;
} pgen_t;

void ulip_core_restore_config(bool restart)
{
    // uint32 rf_cal_sec = 0;

    /* System settings */
    CFG_Default();
    /* Account settings */
    account_db_remove_all();
#if !defined(__MLI_1WRS_TYPE__) && !defined(__MLI_1WLS_TYPE__) && \
    !defined(__MLI_1WRG_TYPE__) && !defined(__MLI_1WLG_TYPE__) && \
    !defined(__MLI_1WRC_TYPE__) && !defined(__MLI_1WLC_TYPE__)
    account_db_log_remove_all();
    // telemetry_db_remove_all();
#endif
#if defined(__MLI_1WB_TYPE__) || defined(__MLI_1WQB_TYPE__)
    fpm_delete_all();
#endif
    if (!CFG_get_wifi_disable())
        wifi_reset();

    /* Restart system */
    if (restart)
        esp_restart();

    system_restored = true;
}

void ulip_core_system_reboot() {
    ESP_LOGI("main", "Rebooting...");
    esp_restart();
}

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
    acc_permission_t perm[ACCOUNT_PERMISSIONS] = {0};
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

    if (!ok)
    {
        ESP_LOGD("ULIP", "RS485 CRC error!");
        return;
    }

    if (control & PSH_DATA)
    {
        // ESP_LOGD("ULIP", "RS485 frame address [%d] len [%d]",
        //          from_addr, len);
        if (len < 7)
            return;
        p.b = frame;
        size = *p.b++;
        size |= (*p.b++ << 8);
        if (size > len)
            return;
        id = *p.b++;
        id |= (*p.b++ << 8);
        id |= (*p.b++ << 16);
        id |= (*p.b++ << 24);
        cmd = *p.b++;
        size -= 7;
        ++cnt;
        int64_t time_elapsed = esp_timer_get_time() / 1000000;
        average = (double)cnt / (double)time_elapsed;
        // ESP_LOGE("main", "cmd: %02x media: %f", cmd, average);
        ctl_beep(1);
        switch (cmd)
        {
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
            if (size <= 0)
                return;
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
            if (state == RS485_CONTROL_OPEN)
            {
                if (CFG_get_control_mode() == CFG_CONTROL_MODE_AUTO)
                    ctl_relay_on(CFG_get_control_timeout());
                else
                    ctl_relay_on(0);
            }
            else if (state == RS485_CONTROL_HOLD)
            {
                ctl_relay_on(0);
            }
            else
            {
                ctl_relay_off();
            }
            break;
        case RS485_CMD_ALARM:
            if (size <= 0)
                return;
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
            if (state)
            {
                ESP_LOGI("main", "alarm onnnnn");
                ctl_alarm_on();
            }
            else
            {
                ESP_LOGI("main", "alarm offfffffff");
                ctl_alarm_off();
            }
            break;
        case RS485_CMD_PANIC:
            if (size <= 0)
                return;
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
            if (state)
            {
                ctl_panic_on();
            }
            else
            {
                ctl_panic_off();
            }
            break;
        case RS485_CMD_ADDUSER:
            if (size < 11)
                return;
            /* Name */
            if (*p.b)
            {
                name = p.b;
                p.b += strlen((char *)p.b) + 1;
            }
            else
            {
                p.b++;
            }
            /* User */
            if (*p.b)
            {
                user = p.b;
                p.b += strlen((char *)p.b) + 1;
            }
            else
            {
                p.b++;
            }
            /* Password */
            if (*p.b)
            {
                password = p.b;
                p.b += strlen((char *)p.b) + 1;
            }
            else
            {
                p.b++;
            }
            /* Card */
            if (*p.b)
            {
                card = p.b;
                p.b += strlen((char *)p.b) + 1;
            }
            else
            {
                p.b++;
            }
            /* QRCode */
            if (*p.b)
            {
                qrcode = p.b;
                p.b += strlen((char *)p.b) + 1;
            }
            else
            {
                p.b++;
            }
            /* RFCode */
            if (*p.b)
            {
                rfcode = p.b;
                p.b += strlen((char *)p.b) + 1;
            }
            else
            {
                p.b++;
            }
            /* Key */
            if (*p.b)
            {
                key = p.b;
                p.b += strlen((char *)p.b) + 1;
            }
            else
            {
                p.b++;
            }
            accessibility = *p.b++;
            panic = *p.b++;
            administrator = *p.b++;
            visitor = *p.b++;
            /* Permissions */
            k = 0;
            for (i = 0; i < ACCOUNT_PERMISSIONS; i++)
            {
                if (*p.b == 0)
                    break;
                len = strlen((char *)p.b) + 1;
                memcpy(perm[size], p.b, len);
                p.b += len;
                k++;
            }
            acc = account_new();
            if (acc)
            {
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
            if (size < 4)
                return;
            /* User */
            if (*p.b)
            {
                user = p.b;
                p.b += strlen((char *)p.b) + 1;
            }
            else
            {
                p.b++;
            }
            /* Card */
            if (*p.b)
            {
                card = p.b;
                p.b += strlen((char *)p.b) + 1;
            }
            else
            {
                p.b++;
            }
            /* QRCode */
            if (*p.b)
            {
                qrcode = p.b;
                p.b += strlen((char *)p.b) + 1;
            }
            else
            {
                p.b++;
            }
            /* RFCode */
            if (*p.b)
            {
                rfcode = p.b;
                p.b += strlen((char *)p.b) + 1;
            }
            else
            {
                p.b++;
            }
            /* Key */
            if (*p.b)
            {
                key = p.b;
                p.b += strlen((char *)p.b) + 1;
            }
            else
            {
                p.b++;
            }
            if (!key)
            {
                index = account_db_find(NULL, (char *)user, (char *)card, (char *)qrcode, (char *)rfcode,
                                        NULL, NULL);
                if (index != -1)
                    account_db_delete(index);
            }
            else
            {
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
            if (size < 1)
                return;
            state = *p.b++;
            size -= 1;
            /* Find account */
            if (size > 0)
            {
                /* User */
                len = strlen((char *)p.b) + 1;
                if (*p.b)
                {
                    user = p.b;
                    p.b += len;
                }
                else
                {
                    p.b++;
                }
                size -= len;
            }
            if (size > 0)
            {
                /* Card */
                len = strlen((char *)p.b) + 1;
                if (*p.b)
                {
                    card = p.b;
                    p.b += len;
                }
                else
                {
                    p.b++;
                }
                size -= len;
            }
            if (size > 0)
            {
                /* QRCode */
                len = strlen((char *)p.b) + 1;
                if (*p.b)
                {
                    qrcode = p.b;
                    p.b += len;
                }
                else
                {
                    p.b++;
                }
                size -= len;
            }
            if (size > 0)
            {
                /* RFCode */
                len = strlen((char *)p.b) + 1;
                if (*p.b)
                {
                    rfcode = p.b;
                    p.b += len;
                }
                else
                {
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
            if (state)
            {
                // probe_user = true;
                // probe_index = index;
            }
            else
            {
                // probe_user = false;
                // probe_index = -1;
            }
            break;
        case RS485_CMD_ERASEUSER:
            if (size <= 0)
                return;
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
            ESP_LOG_BUFFER_CHAR("main", CFG_get_release(), size);
            p.b += size;
            *p.b++ = CFG_get_standalone();
            *p.b++ = CFG_get_rs485_hwaddr();
            *p.b++ = CFG_get_rs485_server_hwaddr();
            rs485_tx_frame(from_addr, buf, len + 11);
            break;
        case RS485_CMD_SETCONFIG:
            if (size < 3)
                return;
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
            if (size < 1)
                return;
            if (*p.b)
            {
                ctl_beep(0);
            }
            else
            {
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
            if (size < 1)
                return;
            d = (char *)p.b;
            d = strtok_r(d, " ", &t);
            d = strtok(d, "-");
            if (!d)
                return;
            /* Date */
            tm.tm_year = strtol(d, NULL, 10);
            d = strtok(NULL, "-");
            if (!d)
                return;
            tm.tm_mon = strtol(d, NULL, 10) - 1;
            d = strtok(NULL, "-");
            if (!d)
                return;
            tm.tm_mday = strtol(d, NULL, 10);
            /* Time */
            t = strtok(t, ":");
            if (!t)
                return;
            tm.tm_hour = strtol(t, NULL, 10);
            t = strtok(NULL, ":");
            if (!t)
                return;
            tm.tm_min = strtol(t, NULL, 10);
            t = strtok(NULL, ":");
            if (!t)
                return;
            tm.tm_sec = strtol(t, NULL, 10);

            struct timeval tv;
            tv.tv_sec = mktime(&tm);
            tv.tv_usec = 0;

            settimeofday(&tv, NULL);
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
            if (size < 2)
                return;
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
                *p.b++ = ctl_sensor_status() == CTL_SENSOR_ON ? RS485_SENSOR_ALARM : RS485_SENSOR_NORMAL;
            else
                *p.b++ = RS485_SENSOR_DISABLED;
            if (CFG_get_sensor_type() == CFG_SENSOR_VOLUME)
            {
                size = strlen((char *)CFG_get_sensor_str_volume()) + 1;
                memcpy(p.b, CFG_get_sensor_str_volume(), size);
                p.b += size;
                if (CFG_get_sensor_limit())
                {
                    if (CFG_get_sensor_volume() > CFG_get_sensor_limit())
                        *p.b++ = RS485_SENSOR_ALARM;
                    else
                        *p.b++ = RS485_SENSOR_NORMAL;
                }
                else
                {
                    *p.b++ = RS485_SENSOR_NORMAL;
                }
            }
            else
            {
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
            if (CFG_get_sensor_cycles())
            {
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
    ESP_LOGE("main", "event rfid %s", data);
    return 1;
}
char tasks_info[1024];
static void got_ip_event()
{
}
static void ulip_core_http_callback(char *uri, char *response_body, int http_status,
                                    char *response_headers, int body_size)
{
    ESP_LOGI("ULIP", "HTTP response [%s] status [%d]",
            uri, http_status);
}
static void got_ip_event2(char * ip_address)
{
    uint8_t mode;
    uint8_t level;
    char *host;
    uint16_t port;
    CFG_get_debug(&mode, &level, &host, &port);
    // udp_logging_init(host, port, udp_logging_vprintf);
    CFG_set_server_ip(ip_address);

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
    ESP_LOGE("main", "%s", data);
    if (1)
    {
        ctl_beep(3);
    }

    return 1;
}
static void http_event(char *url, char *response_body, int http_status, char *response_header_key, char *response_header_value, int body_size)
{
    printf("event %s", response_body);
}
static void http_event2(char *url, char *response_body, int http_status, char *response_header_key, char *response_header_value, int body_size)
{
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
        printf("%c", data[i]);
    }
    printf("\n");
    tty_func_t t = NULL;
    return t;
}
void release_task()
{
    // for (int i = 0; i < 50; i++) {
    //     char user[32];
    //     sprintf(user, "user%d", i);
    //     account_t *account = account_new();
    //     account_set_name(account, user);
    //     account_set_card(account,user);
    //     account_set_code(account, user);
    //     account_set_user(account, user);
    //     account_set_key(account, "password");
    //     account_db_insert(account);
    // }
    ESP_LOGI("main", "accounts added");
    for (int i = 0; i < 50; i++) {
        char user[32];
        char date[32];
        sprintf(user, "user%d", i);
        sprintf(date, "2022-0%d-10 15:46:57", i%10);
        account_log_t *log = account_log_new();
        account_log_set_date(log, date);
        account_log_set_name(log, user);
        account_log_set_code(log, user);
        account_log_set_granted(log, i%2);
        account_log_set_type(log, i%3);
        account_db_log_insert(log);
        account_log_t *log2 = account_db_log_get_index(i);
        ESP_LOGI("main", "date added %s", account_log_get_date(log2));
    }
    vTaskDelete(NULL);
}
void update_ip_info()
{
    get_wifi_ip(CFG_get_ap_mode(), &wifi_ip_info);
    // ESP_LOGI("main", "wifiIP:" IPSTR, IP2STR(&wifi_ip_info.ip));
    // ESP_LOGI("main" , "wifinet:" IPSTR, IP2STR(&wifi_ip_info.netmask));
    // ESP_LOGI("main", "wifigw:" IPSTR, IP2STR(&wifi_ip_info.gw));
    get_eth_ip(&eth_ip_info);
    ESP_LOGI("main", "ponter %p", &eth_ip_info);
    ESP_LOGI("main", "ethIP:" IPSTR, IP2STR(&eth_ip_info.ip));
    ESP_LOGI("main" , "ETHMASK:" IPSTR, IP2STR(&eth_ip_info.netmask));
    ESP_LOGI("main", "ETHGW:" IPSTR, IP2STR(&eth_ip_info.gw));
    // vTaskDelete(NULL);
}
static void ctl_event(int event, int status)
{
    // printf("event rolou ctl: %d status %d\n", event, status);
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

unsigned char *data = (unsigned char *)"\x88";

static void rs485_rx_data(int tty, const char *data,
                          int len, void *user_data)

{
    printf("data %s", data);
}
static int ulip_core_http_request(const char *url)
{
    const char *server = NULL;
    const char *user = NULL;
    const char *pass = NULL;
    const char *param = NULL;
    int port = 0;
    char auth[128] = "";
    char stmp[1024];
    char *p;
    char *l;
    char *t;

    if (!url) return -1;

    /* Parse URL */
    strncpy(stmp, url, sizeof(stmp));
    p = strtok_r(stmp, "/", &l);
    if (strcmp(p, "http:")) return -1;
    p = strtok_r(NULL, "/", &l);
    if (!p) return -1;
    /* User:Password */
    if ((t = strchr(p, '@'))) {
        if (t < l) {
            *t = '\0';
            user = strtok_r(p, ":", &p);
            pass = strtok_r(NULL, ":", &p);
            p = ++t;
        }
    }

    /* Server:Port */
    if ((t = strchr(p, ':'))) {
        if (t < l) {
            server = strtok_r(p, ":", &p);
            if (!p) return -1;
            port = strtol(p, NULL, 10);
        }
    } else {
        server = p;
    }
    param = l;
    if (user && pass)
        sprintf(auth, "%s:%s", user, pass);

    http_raw_request(server, port, false, user,pass, param, NULL,
                     "", "", 0, ulip_core_http_callback);

    return 0;
}
static esp_err_t ulip_core_httpd_request(httpd_req_t *req)
{
    char request[128] = "";
    char state[32] = "";
    char url[256] = "";
    char file[32] = "";
    char *body;
    char slen[32];
    char *config;
    char *uid = NULL;
    char *name = NULL;
    char *user = NULL;
    char *password = NULL;
    char *card = NULL;
    char *qrcode = NULL;
    char *rfcode = NULL;
    uint8_t *fingerprint = NULL;
    uint8_t template[512] = {0};
    char *finger = NULL;
    uint8_t lifecount = 0;
    uint8_t accessibility = 0;
    uint8_t panic = 0;
    char *key = NULL;
    uint8_t level = 0;
    uint8_t visitor = 0;
    acc_permission_t perm[5] = {0};
    char *start = NULL;
    char *end = NULL;
    char filter[128];
    // esp_netif_ip_info_t wifi_ip_info;
    char ip_address[16] = "";
    char netmask[16] = "";
    char gateway[16] = "";
    char eth_ip_address[16] = "";
    char eth_netmask[16] = "";
    char eth_gateway[16] = "";
    const char *server;
    int port;
    int retries;
    char username[64] = "";
    char passwd[64] = "";
    char auth[128] = "";
    int size = 0;
    account_t *acc;
// #if !defined(__MLI_1WRS_TYPE__) && !defined(__MLI_1WLS_TYPE__) && 
//     !defined(__MLI_1WRG_TYPE__) && !defined(__MLI_1WLG_TYPE__) && 
//     !defined(__MLI_1WRP_TYPE__) && !defined(__MLI_1WRC_TYPE__) && 
//     !defined(__MLI_1WLC_TYPE__)
//     account_log_t *log;
// #else
//     telemetry_t *log;
// #endif
    bool update = false;
    const char *levelstatus;
    const char *volume;
    const char *volumestatus;
    struct tm *tm;
    time_t rawtime;
    char date[64];
    int index;
    char *p;
    int len;
    esp_err_t rc;
    int i;
    httpdConnData *connData = (httpdConnData *)req->user_ctx;
    char *buf;
    size_t buffer_len = httpd_req_get_url_query_len(req) + 1;
    // esp_err_t has_query = httpd_req_get_url_query_str(req, buf, buffer_len);
    ESP_LOGI("main", "HTTPD uri [%s] memory [%u] size [%u]",
             req->uri,
             esp_get_free_heap_size(), buffer_len);
    /* Authenticate */
    rc = basic_auth_get_handler(req);
    ESP_LOGE("httpd", "uri %s", req->uri);
    if (rc != ESP_OK)
        return rc;

    buf = malloc(buffer_len);
    if (buffer_len > 1 && httpd_req_get_url_query_str(req, buf, buffer_len) == ESP_OK)
    {
        ESP_LOGI("main", "Found URL query => %s", buf);
        // char param[32];
        if (httpd_query_key_value(buf, "request", request, sizeof(request)) == ESP_OK)
        {
            
            if (!strcmp(request, "version"))
            {
                ESP_LOGI("ULIP", "version %s", request);
                /* JSON */
                body = (char *)malloc(128);
                if (!body)
                {
                    ESP_LOGE("ULIP", "malloc failed");
                    httpd_resp_send_500(req);
                    free(buf);
                    return ESP_FAIL;
                }
                len = sprintf(body, "{\"version\":\"%s\",\"serial\":\"%s\",\"release\":\"%s\"}",
                                ULIP_MODEL, CFG_get_serialnum(), CFG_get_release());
                // httpd_resp_set_hdr(req, "Content-Type", "application/json");
                httpd_resp_set_type(req, "application/json");
                httpd_resp_send(req, (const char *)body, len);
                sprintf(slen, "%d", len);
                free(body);
                free(buf);
                return ESP_OK;
            }
            else if (!strcmp(request, "status"))
            {
                body = (char *)malloc(256);
                if (!body)
                {
                    ESP_LOGE("ULIP", "malloc failed");
                    httpd_resp_send_500(req);
                    free(buf);
                    return ESP_FAIL;
                }
                len = sprintf(body, "{\"description\":\"%s\",\"relay\":\"%s\",\"sensor\":\"%s\"}",
                                CFG_get_control_description() ? CFG_get_control_description() : "",
                                ctl_relay_status() == CTL_RELAY_OFF ? "off" : "on",
                                ctl_sensor_status() == CTL_SENSOR_OFF ? "off" : "on");
                rawtime = time(NULL);
                tm = gmtime (&rawtime);
                
                // tm = localtime(&rawtime);
                // printf("%d, %lu, %s\n", tm->tm_year, rawtime, weekday[tm->tm_wday]);
                sprintf(date, "%s, %02d %s %d %02d:%02d:%02d GMT",
                        weekday[tm->tm_wday], tm->tm_mday, monthday[tm->tm_mon],
                        tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
                httpd_resp_set_hdr(req, "Date", date);
                httpd_resp_set_type(req, "application/json");
                sprintf(slen, "%d", len);
                httpd_resp_send(req, (const char *)body, len);
                free(body);
                free(buf);
                return ESP_OK;
                
            }
            else if (!strcmp(request, "relay"))
            {
                httpd_query_key_value(buf, "state", state, sizeof(state));
                // httpdFindArg(connData->getArgs, "state",
                //             state, sizeof(state));
                server = CFG_get_server_ip();
                port = CFG_get_server_port();
                if (!port)
                    port = 80;
                retries = CFG_get_server_retries();
                if (CFG_get_server_user() && CFG_get_server_passwd())
                    sprintf(auth, "%s:%s", CFG_get_server_user(),
                            CFG_get_server_passwd());
                if (!strcmp(state, "on") || !strcmp(state, "hold"))
                {
                    if (!CFG_get_control_external())
                    {
                        /* Local control */
                        if (!strcmp(state, "on"))
                        {
                            if (CFG_get_control_mode() == CFG_CONTROL_MODE_AUTO)
                                ctl_relay_on(CFG_get_control_timeout());
                            else
                                ctl_relay_on(0);
                        }
                        else
                        {
                            ctl_relay_on(0);
                        }
                    }
                    else
                    {
                        /* External control */
                        ulip_core_http_request(CFG_get_control_url());
                    }

                    if (!authBasicGetUsername(req, username, sizeof(username)))
                    {
                        index = account_db_find(NULL, username, NULL, NULL,
                                                NULL, NULL, NULL);
                        ESP_LOGI("main", "index %d", index);                       
                        if (index != -1)
                        {
    #if 0
    // #if !defined(__MLI_1WRS_TYPE__) && !defined(__MLI_1WLS_TYPE__) && 
    //     !defined(__MLI_1WRG_TYPE__) && !defined(__MLI_1WLG_TYTE__) && 
    //     !defined(__MLI_1WRP_TYPE__) && !defined(__MLI_1WRC_TYPE__) && 
    //     !defined(__MLI_1WLC_TYPE__)
                            /* LOG */
                            log = account_log_new();
                            if (log)
                            {
                                acc = account_db_get_index(index);
                                account_log_set_name(log, account_get_name(acc));
                                tm = rtc_localtime();
                                sprintf(date, "%02d/%02d/%02d %02d:%02d:%02d",
                                        tm->tm_mday, tm->tm_mon + 1, tm->tm_year % 100,
                                        tm->tm_hour, tm->tm_min, tm->tm_sec);
                                account_log_set_date(log, date);
                                account_log_set_code(log, username);
                                account_log_set_granted(log, TRUE);
                                if (acc_log[acc_log_count])
                                    account_log_destroy(acc_log[acc_log_count]);
                                acc_log[acc_log_count] = log;
                                acc_log_count = ++acc_log_count & (MAX_ACC_LOG - 1);
                                account_db_log_insert(log);
                                account_destroy(acc);
                            }
    #endif
                            if (CFG_get_server_url())
                                sprintf(url, "/%s?request=user&user=%s&state=granted",
                                        CFG_get_server_url(), username);
                            else
                                sprintf(url, "/?request=user&user=%s&state=granted",
                                        username);
                            printf("HTTP request to host %s path %s\n", server, url);
                            http_raw_request(server, port, false, CFG_get_server_user(), CFG_get_server_passwd(), url, NULL,
                                            "", "", retries, ulip_core_http_callback);
                            ctl_beep(0);
                        }
                    }
                }

                else if (!strcmp(state, "off"))
                {
                    if (!CFG_get_control_external())
                    {
                        /* Local control */
                        ctl_relay_off();
                    }
                }
                // httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                // httpdStartResponse(connData, 200);
                // httpdHeader(connData, "Content-Length", "0");
                // httpdEndHeaders(connData);
                httpd_resp_send(req, NULL, 0);
                free(buf);
                return ESP_OK;
            }
            else if (!strcmp(request, "alarm"))
            {
                httpd_query_key_value(buf, "state", state, sizeof(state));
                if (!strcmp(state, "on"))
                {
                    ctl_alarm_on();
                }
                else
                {
                    ctl_alarm_off();
                }
                httpd_resp_send(req, NULL, 0);
                free(buf);
                return ESP_OK;
            }
            else if (!strcmp(request, "panic"))
            {
                httpd_query_key_value(buf, "state", state, sizeof(state));
                if (!strcmp(state, "on"))
                {
                    ctl_panic_on();
                }
                else
                {
                    ctl_panic_off();
                }
                httpd_resp_send(req, NULL, 0);
                free(buf);
                return ESP_OK;
            }
            else if (!strcmp(request, "update"))
            {
                httpd_query_key_value(buf, "url", url, sizeof(url));
                if (*url != '\0')
                {
                    //TODO #2
                    // ulip_core_system_update(url);
                    // httpdStartResponse(connData, 200);
                    httpd_resp_send(req, NULL, 0);
                }
                else
                {
                    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request");
                }

                free(buf);
                return ESP_OK;
            }
            else if (!strcmp(request, "reboot"))
            {
                httpd_resp_send(req, NULL, 0);
                esp_timer_create_args_t timer_args = {
                    .callback = ulip_core_system_reboot,
                    .name = "ulip_core_system_reboot"
                };
                free(buf);
                esp_timer_create(&timer_args, &reboot_timer);
                esp_timer_start_once(reboot_timer, 1000*1000);
                return ESP_OK;
            }
            else if (!strcmp(request, "setconfig"))
            {
                int content_length = req->content_len;
                char buffer[HTTPD_MAX_POST_LEN];
                httpd_req_recv(req, buffer,  sizeof(buffer));
                if (req->method != HTTP_POST || content_length < 1)
                {
                    httpd_resp_send_err(req, 400, "Bad Request");
                    return ESP_OK;
                }
                /* JSON */
                strchr(buffer, '}')[1] = '\0';
                config = buffer;
                ESP_LOGI("main", "config %s", config);
                while ((p = strtok(config, ",")))
                {
                    ESP_LOGI("main", "p %s", p);
                    config = NULL;
                    strdelimit(p, "{}\r\n", ' ');
                    strstrip(p);
                    if (!strncmp("\"hotspot\":", p, 10))
                    {
                        p += 10;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_hotspot(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"ap_mode\":", p, 10))
                    {
                        p += 10;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_ap_mode(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"standalone\":", p, 13))
                    {
                        p += 13;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_standalone(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"ssid\":", p, 7))
                    {
                        p += 7;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_wifi_ssid(p);
                    }
                    else if (!strncmp("\"password\":", p, 11))
                    {
                        p += 11;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_wifi_passwd(p);
                    }
                    else if (!strncmp("\"channel\":", p, 10))
                    {
                        p += 10;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_wifi_channel(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"beacon_interval\":", p, 18))
                    {
                        p += 18;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_wifi_beacon_interval(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"ssid_hidden\":", p, 14))
                    {
                        p += 14;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_ssid_hidden(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"dhcp\":", p, 7))
                    {
                        p += 7;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_dhcp(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"ip\":", p, 5))
                    {
                        p += 5;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_ip_address(p);
                    }
                    else if (!strncmp("\"netmask\":", p, 10))
                    {
                        p += 10;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_netmask(p);
                    }
                    else if (!strncmp("\"gateway\":", p, 10))
                    {
                        p += 10;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_gateway(p);
                    }
                    else if (!strncmp("\"eth_dhcp\":", p, 11))
                    {
                        p += 11;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_eth_dhcp(!strcmp(p, "on"));
                        ESP_LOGI("main", "eth_dhcp %s", p);
                    }
                    else if (!strncmp("\"eth_ip\":", p, 9))
                    {
                        p += 9;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_eth_ip_address(p);
                        ESP_LOGI("main", "eth_ip %s", CFG_get_eth_ip_address());
                    }
                    else if (!strncmp("\"eth_netmask\":", p, 14))
                    {
                        p += 14;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_eth_netmask(p);
                        ESP_LOGI("main", "eth_netmask %s", CFG_get_eth_netmask());
                    }
                    else if (!strncmp("\"eth_gateway\":", p, 14))
                    {
                        p += 14;
                        ESP_LOGI("main", "eth_gateway %s", p);
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_eth_gateway(p);
                        ESP_LOGI("main", "eth_gateway %s", p);
                    }
                    else if (!strncmp("\"dns\":", p, 6))
                    {
                        p += 6;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_dns(p);
                    }
                    else if (!strncmp("\"ntp\":", p, 6))
                    {
                        p += 6;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_ntp(p);
                    }
                    else if (!strncmp("\"hostname\":", p, 11))
                    {
                        p += 11;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_hostname(p);
                    }
                    else if (!strncmp("\"timezone\":", p, 11))
                    {
                        p += 11;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_timezone(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"dst\":", p, 6))
                    {
                        p += 6;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_dst(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"dst_date\":", p, 11))
                    {
                        p += 11;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_dst_date(p);
                    }
                    else if (!strncmp("\"server\":", p, 9))
                    {
                        p += 9;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_server_ip(p);
                    }
                    else if (!strncmp("\"server_port\":", p, 14))
                    {
                        p += 14;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_server_port(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"server_user\":", p, 14))
                    {
                        p += 14;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_server_user(p);
                    }
                    else if (!strncmp("\"server_password\":", p, 18))
                    {
                        p += 18;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_server_passwd(p);
                    }
                    else if (!strncmp("\"server_url\":", p, 13))
                    {
                        p += 13;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_server_url(p);
                    }
                    else if (!strncmp("\"server_retries\":", p, 17))
                    {
                        p += 17;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_server_retries(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"user_auth\":", p, 12))
                    {
                        p += 12;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_user_auth(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"ota_url\":", p, 10))
                    {
                        p += 10;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_ota_url(p);
                    }
                    else if (!strncmp("\"rfid\":", p, 7))
                    {
                        p += 7;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_rfid_enable(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"rfid_timeout\":", p, 15))
                    {
                        p += 15;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_rfid_timeout(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"rfid_panic_timeout\":", p, 21))
                    {
                        p += 21;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_rfid_panic_timeout(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"rfid_nfc\":", p, 11))
                    {
                        p += 11;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_rfid_nfc(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"rfid_panic_timeout\":", p, 21))
                    {
                        p += 21;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_rfid_panic_timeout(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"rfid_retries\":", p, 15))
                    {
                        p += 15;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_rfid_retries(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"qrcode\":", p, 9))
                    {
                        p += 9;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_qrcode_enable(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"qrcode_timeout\":", p, 17))
                    {
                        p += 17;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_qrcode_timeout(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"qrcode_dynamic\":", p, 17))
                    {
                        p += 17;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_qrcode_dynamic(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"qrcode_validity\":", p, 18))
                    {
                        p += 18;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_qrcode_validity(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"qrcode_panic_timeout\":", p, 23))
                    {
                        p += 23;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_qrcode_panic_timeout(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"qrcode_config\":", p, 16))
                    {
                        p += 16;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_qrcode_config(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"control_external\":", p, 19))
                    {
                        p += 19;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_control_external(!strcmp(p, "true"));
                    }
                    else if (!strncmp("\"control_url\":", p, 14))
                    {
                        p += 14;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_control_url(p);
                    }
                    else if (!strncmp("\"control_mode\":", p, 15))
                    {
                        p += 15;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_control_mode(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"control_timeout\":", p, 18))
                    {
                        p += 18;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_control_timeout(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"control_description\":", p, 22))
                    {
                        p += 22;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_control_description(p);
                    }
                    else if (!strncmp("\"control_acc_timeout\":", p, 22))
                    {
                        p += 22;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_control_acc_timeout(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"control_doublepass_timeout\":", p, 29))
                    {
                        p += 29;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_control_doublepass_timeout(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"rs485\":", p, 8))
                    {
                        p += 8;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_rs485_enable(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"rs485_address\":", p, 16))
                    {
                        p += 16;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_rs485_hwaddr(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"rs485_server_address\":", p, 23))
                    {
                        p += 23;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_rs485_server_hwaddr(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"debug\":", p, 8))
                    {
                        /* Format: <mode>:<level>:<server>:<port> */
                        uint8_t mode = 0;
                        uint8_t level = 0;
                        char *server = NULL;
                        int port = 0;
                        p += 8;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        mode = strtol(strtok_r(p, ":", &p), NULL, 10);
                        if (p)
                            level = strtol(strtok_r(NULL, ":", &p), NULL, 10);
                        if (p)
                            server = strtok_r(NULL, ":", &p);
                        if (p)
                            port = strtol(strtok_r(NULL, ":", &p), NULL, 10);
                        switch (mode)
                        {
                        case DEBUG_MODE_NONE:
                            esp_log_level_set("*", ESP_LOG_NONE);
                            esp_log_set_vprintf(vprintf);
                            break;
                        case DEBUG_MODE_SERIAL:
                            esp_log_level_set("*", level);
                            esp_log_set_vprintf(vprintf);
                            break;
                        case DEBUG_MODE_NETWORK:
                            esp_log_level_set("*", level);
                            udp_logging_init(server, port, udp_logging_vprintf);
                            break;
                        }
                        CFG_set_debug(mode, level, server, port);
                    }
                    else if (!strncmp("\"rf433\":", p, 8))
                    {
                        p += 8;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_rf433_enable(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"fingerprint\":", p, 14))
                    {
                        p += 14;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_fingerprint_enable(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"fingerprint_timeout\":", p, 22))
                    {
                        p += 22;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_fingerprint_timeout(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"fingerprint_security\":", p, 23))
                    {
                        p += 23;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_fingerprint_security(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"fingerprint_retries\":", p, 22))
                    {
                        p += 22;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_fingerprint_identify_retries(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"latitude\":", p, 11))
                    {
                        p += 11;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_latitude(p);
                    }
                    else if (!strncmp("\"longitude\":", p, 12))
                    {
                        p += 12;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_longitude(p);
                    }
                    else if (!strncmp("\"watchdog_shutdown\":", p, 20))
                    {
                        p += 20;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_rtc_shutdown(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"ddns\":", p, 7))
                    {
                        p += 7;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_ddns(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"ddns_domain\":", p, 14))
                    {
                        p += 14;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_ddns_domain(p);
                    }
                    else if (!strncmp("\"ddns_user\":", p, 12))
                    {
                        p += 12;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_ddns_user(p);
                    }
                    else if (!strncmp("\"ddns_password\":", p, 16))
                    {
                        p += 16;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_ddns_passwd(p);
                    }
                    else if (!strncmp("\"rf433_rc\":", p, 11))
                    {
                        p += 11;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_rf433_rc(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"rf433_hc\":", p, 11))
                    {
                        p += 11;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_rf433_hc(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"rf433_bc\":", p, 11))
                    {
                        p += 11;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_rf433_bc(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"rf433_bp\":", p, 11))
                    {
                        p += 11;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_rf433_bp(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"rf433_panic_timeout\":", p, 22))
                    {
                        p += 22;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_rf433_panic_timeout(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"rf433_ba\":", p, 11))
                    {
                        p += 11;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_rf433_ba(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"lora\":", p, 7))
                    {
                        p += 7;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_lora_enable(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"lora_channel\":", p, 15))
                    {
                        p += 15;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_lora_channel(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"lora_baudrate\":", p, 16))
                    {
                        p += 16;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_lora_baudrate(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"lora_address\":", p, 15))
                    {
                        p += 15;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_lora_address(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"lora_server_address\":", p, 22))
                    {
                        p += 22;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_lora_server_address(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"dht\":", p, 6))
                    {
                        p += 6;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_dht_enable(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"dht_timeout\":", p, 14))
                    {
                        p += 14;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_dht_timeout(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"dht_temp_upper\":", p, 17))
                    {
                        p += 17;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_dht_temp_upper(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"dht_temp_lower\":", p, 17))
                    {
                        p += 17;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_dht_temp_lower(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"dht_rh_upper\":", p, 15))
                    {
                        p += 15;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_dht_rh_upper(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"dht_rh_lower\":", p, 15))
                    {
                        p += 16;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_dht_rh_lower(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"dht_relay\":", p, 12))
                    {
                        p += 12;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_dht_relay(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"dht_alarm\":", p, 12))
                    {
                        p += 12;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_dht_alarm(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"mq2\":", p, 22))
                    {
                        p += 6;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_mq2_enable(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"mq2_timeout\":", p, 14))
                    {
                        p += 14;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_mq2_timeout(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"mq2_limit\":", p, 12))
                    {
                        p += 12;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_mq2_limit(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"mq2_relay\":", p, 12))
                    {
                        p += 12;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_mq2_relay(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"mq2_alarm\":", p, 12))
                    {
                        p += 12;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_mq2_alarm(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"pir\":", p, 6))
                    {
                        p += 6;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pir_enable(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"pir_chime\":", p, 12))
                    {
                        p += 12;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pir_chime(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"pir_relay\":", p, 12))
                    {
                        p += 12;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pir_relay(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"pir_alarm\":", p, 12))
                    {
                        p += 12;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pir_alarm(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"pir_timeout\":", p, 14))
                    {
                        p += 14;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pir_timeout(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"sensor_type\":", p, 14))
                    {
                        p += 14;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_sensor_type(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"sensor_flow\":", p, 14))
                    {
                        p += 14;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_sensor_flow(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"sensor_limit\":", p, 15))
                    {
                        p += 15;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_sensor_limit(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"temt\":", p, 7))
                    {
                        p += 7;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_temt_enable(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"temt_timeout\":", p, 15))
                    {
                        p += 15;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_temt_timeout(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"temt_upper\":", p, 13))
                    {
                        p += 13;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_temt_upper(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"temt_lower\":", p, 13))
                    {
                        p += 13;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_temt_lower(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"temt_relay\":", p, 13))
                    {
                        p += 13;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_temt_relay(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"temt_alarm\":", p, 13))
                    {
                        p += 13;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_temt_alarm(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"relay_status\":", p, 15))
                    {
                        p += 15;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_relay_status(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"pow_voltage_cal\":", p, 18))
                    {
                        p += 18;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pow_voltage_cal(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"pow_voltage_upper\":", p, 20))
                    {
                        p += 20;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pow_voltage_upper(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"pow_voltage_lower\":", p, 20))
                    {
                        p += 20;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pow_voltage_lower(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"pow_current_cal\":", p, 18))
                    {
                        p += 18;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pow_current_cal(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"pow_current_upper\":", p, 20))
                    {
                        p += 20;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pow_current_upper(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"pow_current_lower\":", p, 20))
                    {
                        p += 20;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pow_current_lower(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"pow_power_upper\":", p, 18))
                    {
                        p += 18;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pow_power_upper(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"pow_power_lower\":", p, 18))
                    {
                        p += 18;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pow_power_lower(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"pow_relay\":", p, 12))
                    {
                        p += 12;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pow_relay(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"pow_alarm_time\":", p, 17))
                    {
                        p += 17;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pow_alarm_time(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"pow_relay_timeout\":", p, 20))
                    {
                        p += 20;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pow_relay_timeout(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"pow_relay_ext\":", p, 16))
                    {
                        p += 16;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pow_relay_ext(!strcmp(p, "on"));
                    }
                    else if (!strncmp("\"pow_interval\":", p, 15))
                    {
                        p += 15;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pow_interval(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"pow_day\":", p, 10))
                    {
                        p += 10;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pow_day(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"pow_energy_cal\":", p, 17))
                    {
                        p += 17;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_pow_energy_cal(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"energy_daily_limit\":", p, 21))
                    {
                        p += 21;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_energy_daily_limit(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"energy_monthly_limit\":", p, 23))
                    {
                        p += 23;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_energy_monthly_limit(strtol(p, NULL, 10));
                    }
                    else if (!strncmp("\"energy_total_limit\":", p, 21))
                    {
                        p += 21;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        CFG_set_energy_total_limit(strtol(p, NULL, 10));
                    }
                }
                CFG_Save();

                httpd_resp_send(req, NULL, 0);
                free(buf);
                return ESP_OK;
            }
            #if 1
            else if (!strcmp(request, "getconfig"))
            {
                uint8_t mode = 0;
                uint8_t level = 0;
                const char *server = NULL;
                uint16_t port = 0;
                /* JSON */
                body = (char *)malloc(2048);
                if (!body)
                {
                    httpd_resp_send_500(req);
                    free(buf);
                    return ESP_OK;
                }

                update_ip_info();

                sprintf(ip_address, IPSTR, IP2STR(&wifi_ip_info.ip));
                ESP_LOGI("main_httpd", "wifiIP:" IPSTR, IP2STR(&wifi_ip_info.ip));
                sprintf(netmask, IPSTR, IP2STR(&wifi_ip_info.netmask));
                sprintf(gateway, IPSTR, IP2STR(&wifi_ip_info.gw));
                sprintf(eth_ip_address, IPSTR, IP2STR(&eth_ip_info.ip));
                ESP_LOGI("main_httpd", "ETHIP:" IPSTR, IP2STR(&eth_ip_info.ip));
                sprintf(eth_netmask, IPSTR, IP2STR(&eth_ip_info.netmask));
                sprintf(eth_gateway, IPSTR, IP2STR(&eth_ip_info.gw));
                CFG_get_debug(&mode, &level, &server, &port);
                len = sprintf(body, "{\"model\":\"%s\",\"serial\":\"%s\",\"mac\":\"%s\",\"release\":\"%s\",\"hotspot\":\"%s\",\"ap_mode\":\"%s\","
                                    "\"standalone\":\"%s\",\"ssid\":\"%s\",\"password\":\"%s\",\"channel\":\"%d\",\"beacon_interval\":\"%d\","
                                    "\"ssid_hidden\":\"%s\",\"dhcp\":\"%s\",\"ip\":\"%s\",\"netmask\":\"%s\",\"gateway\":\"%s\",\"eth_dhcp\":\"%s\",\"eth_ip\":\"%s\",\"eth_netmask\":\"%s\",\"eth_gateway\":\"%s\" ,\"dns\":\"%s\","
                                    "\"ntp\":\"%s\",\"hostname\":\"%s\",\"ddns\":\"%s\",\"ddns_domain\":\"%s\",\"ddns_user\":\"%s\",\"ddns_password\":\"%s\","
                                    "\"timezone\":\"%d\",\"dst\":\"%s\",\"dst_date\":\"%s\",\"server\":\"%s\",\"server_port\":\"%d\",\"server_user\":\"%s\","
                                    "\"server_password\":\"%s\",\"server_url\":\"%s\",\"server_retries\":\"%d\",\"ota_url\":\"%s\""
    #if !defined(__MLI_1WF_TYPE__) && !defined(__MLI_1WQF_TYPE__) && !defined(__MLI_1WRF_TYPE__) &&  \
        !defined(__MLI_1WRS_TYPE__) && !defined(__MLI_1WLS_TYPE__) && !defined(__MLI_1WRP_TYPE__) && \
        !defined(__MLI_1WRC_TYPE__) && !defined(__MLI_1WLC_TYPE__)
                                    ",\"rfid\":\"%s\",\"rfid_timeout\":\"%d\",\"rfid_nfc\":\"%s\",\"rfid_panic_timeout\":\"%d\",\"rfid_retries\":\"%d\""
    #endif
    #if defined(__MLI_1WQ_TYPE__) || defined(__MLI_1WQB_TYPE__) || \
        defined(__MLI_1WQF_TYPE__) || defined(__MLI_1WRQ_TYPE__)
                                    ",\"qrcode\":\"%s\",\"qrcode_timeout\":\"%d\",\"qrcode_dynamic\":\"%s\",\"qrcode_validity\":\"%d\","
                                    "\"qrcode_panic_timeout\":\"%d\",\"qrcode_config\":\"%s\""
    #endif
    #if defined(__MLI_1WB_TYPE__) || defined(__MLI_1WQB_TYPE__)
                                    ",\"fingerprint\":\"%s\",\"fingerprint_timeout\":\"%d\",\"fingerprint_retries\":\"%d\","
                                    "\"fingerprint_security\":\"%d\""
    #endif
    #if defined(__MLI_1WF_TYPE__) || defined(__MLI_1WQF_TYPE__) || \
        defined(__MLI_1WRF_TYPE__)
                                    ",\"rf433\":\"%s\",\"rf433_rc\":\"%s\",\"rf433_hc\":\"%d\",\"rf433_alarm\":\"%s\","
                                    "\"rf433_bc\":\"%s\",\"rf433_bp\":\"%d\",\"rf433_panic_timeout\":\"%d\",\"rf433_ba\":\"%d\""
    #endif
    #if !defined(__MLI_1WRP_TYPE__)
                                    ",\"control_description\":\"%s\",\"control_mode\":\"%d\",\"control_timeout\":\"%d\""
    #if !defined(__MLI_1WRS_TYPE__) && !defined(__MLI_1WLS_TYPE__) && \
        !defined(__MLI_1WRG_TYPE__) && !defined(__MLI_1WLG_TYPE__) && \
        !defined(__MLI_1WRC_TYPE__) && !defined(__MLI_1WLC_TYPE__)
                                    ",\"control_external\":\"%s\",\"control_url\":\"%s\",\"control_acc_timeout\":\"%d\","
                                    "\"control_doublepass_timeout\":\"%d\""
    #endif
    #endif
    #if defined(__MLI_1WRQ_TYPE__) || defined(__MLI_1WR_TYPE__) ||  \
        defined(__MLI_1WRF_TYPE__) || defined(__MLI_1WRS_TYPE__) || \
        defined(__MLI_1WRG_TYPE__) || defined(__MLI_1WRP_TYPE__) || \
        defined(__MLI_1WRC_TYPE__)
                                    ",\"rs485\":\"%s\",\"rs485_address\":\"%d\",\"rs485_server_address\":\"%d\""
    #endif
                                    ",\"latitude\":\"%s\",\"longitude\":\"%s\",\"user_auth\":\"%s\",\"watchdog_shutdown\":\"%d\",\"debug\":\"%d:%d:%s:%d\""
    #if defined(__MLI_1WLS_TYPE__) || defined(__MLI_1WLG_TYTE__)
                                    ",\"lora\":\"%s\",\"lora_channel\":\"%d\",\"lora_baudrate\":\"%d\",\"lora_address\":\"%d\",\"lora_server_address\":\"%d\""
    #endif
    #if defined(__MLI_1WRS_TYPE__) || defined(__MLI_1WLS_TYPE__) || \
        defined(__MLI_1WRG_TYPE__) || defined(__MLI_1WLG_TYPE__) || \
        defined(__MLI_1WRC_TYPE__) || defined(__MLI_1WLC_TYPE__)
                                    ",\"dht\":\"%s\",\"dht_timeout\":\"%d\",\"dht_temp_upper\":\"%d\",\"dht_temp_lower\":\"%d\","
                                    "\"dht_rh_upper\":\"%d\",\"dht_rh_lower\":\"%d\",\"dht_relay\":\"%s\",\"dht_alarm\":\"%s\","
                                    "\"mq2\":\"%s\",\"mq2_timeout\":\"%d\",\"mq2_limit\":\"%d\",\"mq2_relay\":\"%s\",\"mq2_alarm\":\"%s\","
                                    "\"pir\":\"%s\",\"pir_chime\":\"%s\",\"pir_relay\":\"%s\",\"pir_alarm\":\"%s\",\"pir_timeout\":\"%d\","
                                    "\"sensor_type\":\"%d\",\"sensor_flow\":\"%d\",\"sensor_limit\":\"%d\","
                                    "\"temt\":\"%s\",\"temt_timeout\":\"%d\",\"temt_upper\":\"%d\",\"temt_lower\":\"%d\",\"temt_relay\":\"%s\","
                                    "\"temt_alarm\":\"%s\""
    #endif
    #if defined(__MLI_1WRP_TYPE__)
                                    ",\"relay_status\":\"%s\",\"pow_voltage_cal\":\"%d\",\"pow_voltage_upper\":\"%d\","
                                    "\"pow_voltage_lower\":\"%d\",\"pow_current_cal\":\"%d\",\"pow_current_upper\":\"%d\",\"pow_current_lower\":\"%d\","
                                    "\"pow_power_upper\":\"%d\",\"pow_power_lower\":\"%d\",\"pow_relay\":\"%s\",\"pow_alarm_time\":\"%d\","
                                    "\"pow_relay_timeout\":\"%d\",\"pow_relay_ext\":\"%s\",\"pow_interval\":\"%d\",\"pow_day\":\"%d\","
                                    "\"pow_energy_cal\":\"%d\",\"energy_daily_limit\":\"%d\",\"energy_monthly_limit\":\"%d\",\"energy_total_limit\":\"%d\""
    #endif
                                    "}",
                                ULIP_MODEL, CFG_get_serialnum(),
                                CFG_get_ethaddr(), CFG_get_release(),
                                CFG_get_hotspot() ? "on" : "off",
                                CFG_get_ap_mode() ? "on" : "off",
                                CFG_get_standalone() ? "on" : "off",
                                CFG_get_wifi_ssid() ? CFG_get_wifi_ssid() : "",
                                CFG_get_wifi_passwd() ? CFG_get_wifi_passwd() : "",
                                CFG_get_wifi_channel(),
                                CFG_get_wifi_beacon_interval(),
                                CFG_get_ssid_hidden() ? "on" : "off",
                                CFG_get_dhcp() ? "on" : "off",
                                ip_address, netmask, gateway,
                                CFG_get_eth_dhcp() ? "on" : "off",
                                eth_ip_address, eth_netmask, eth_gateway,
                                CFG_get_dns() ? CFG_get_dns() : "",
                                CFG_get_ntp() ? CFG_get_ntp() : "",
                                CFG_get_hostname() ? CFG_get_hostname() : "",
                                CFG_get_ddns() ? "on" : "off",
                                CFG_get_ddns_domain() ? CFG_get_ddns_domain() : "",
                                CFG_get_ddns_user() ? CFG_get_ddns_user() : "",
                                CFG_get_ddns_passwd() ? CFG_get_ddns_passwd() : "",
                                CFG_get_timezone(),
                                CFG_get_dst() ? "on" : "off",
                                CFG_get_dst_date() ? CFG_get_dst_date() : "",
                                CFG_get_server_ip() ? CFG_get_server_ip() : "",
                                CFG_get_server_port(),
                                CFG_get_server_user() ? CFG_get_server_user() : "",
                                CFG_get_server_passwd() ? CFG_get_server_passwd() : "",
                                CFG_get_server_url() ? CFG_get_server_url() : "",
                                CFG_get_server_retries(),
                                CFG_get_ota_url() ? CFG_get_ota_url() : "",
    #if !defined(__MLI_1WF_TYPE__) && !defined(__MLI_1WQF_TYPE__) && !defined(__MLI_1WRF_TYPE__) &&  \
        !defined(__MLI_1WRS_TYPE__) && !defined(__MLI_1WLS_TYPE__) && !defined(__MLI_1WRP_TYPE__) && \
        !defined(__MLI_1WRC_TYPE__) && !defined(__MLI_1WLC_TYPE__)
                                CFG_get_rfid_enable() ? "on" : "off",
                                CFG_get_rfid_timeout(),
                                CFG_get_rfid_nfc() ? "on" : "off",
                                CFG_get_rfid_panic_timeout(),
                                CFG_get_rfid_retries(),
    #endif
    #if defined(__MLI_1WQ_TYPE__) || defined(__MLI_1WQB_TYPE__) || \
        defined(__MLI_1WQF_TYPE__) || defined(__MLI_1WRQ_TYPE__)
                                CFG_get_qrcode_enable() ? "on" : "off",
                                CFG_get_qrcode_timeout(),
                                CFG_get_qrcode_dynamic() ? "on" : "off",
                                CFG_get_qrcode_validity(),
                                CFG_get_qrcode_panic_timeout(),
                                CFG_get_qrcode_config() ? "on" : "off",
    #endif
    #if defined(__MLI_1WB_TYPE__) || defined(__MLI_1WQB_TYPE__)
                                CFG_get_fingerprint_enable() ? "on" : "off",
                                CFG_get_fingerprint_timeout(),
                                CFG_get_fingerprint_security(),
                                CFG_get_fingerprint_identify_retries(),
    #endif
    #if defined(__MLI_1WF_TYPE__) || defined(__MLI_1WQF_TYPE__) || \
        defined(__MLI_1WRF_TYPE__)
                                CFG_get_rf433_enable() ? "on" : "off",
                                CFG_get_rf433_rc() ? "on" : "off",
                                CFG_get_rf433_hc(),
                                CFG_get_rf433_alarm() ? "on" : "off",
                                CFG_get_rf433_bc() ? "on" : "off",
                                CFG_get_rf433_bp(),
                                CFG_get_rf433_panic_timeout(),
                                CFG_get_rf433_ba(),
    #endif
    #if !defined(__MLI_1WRP_TYPE__)
                                CFG_get_control_description() ? CFG_get_control_description() : "",
                                CFG_get_control_mode(),
                                CFG_get_control_timeout(),
    #if !defined(__MLI_1WRS_TYPE__) && !defined(__MLI_1WLS_TYPE__) && \
        !defined(__MLI_1WRG_TYPE__) && !defined(__MLI_1WLG_TYPE__) && \
        !defined(__MLI_1WRC_TYPE__) && !defined(__MLI_1WLC_TYPE__)
                                CFG_get_control_external() ? "true" : "false",
                                CFG_get_control_url() ? CFG_get_control_url() : "",
                                CFG_get_control_acc_timeout(),
                                CFG_get_control_doublepass_timeout(),
    #endif
    #endif
    #if defined(__MLI_1WRQ_TYPE__) || defined(__MLI_1WR_TYPE__) ||  \
        defined(__MLI_1WRF_TYPE__) || defined(__MLI_1WRS_TYPE__) || \
        defined(__MLI_1WRG_TYPE__) || defined(__MLI_1WRP_TYPE__) || \
        defined(__MLI_1WRC_TYPE__)
                                CFG_get_rs485_enable() ? "on" : "off",
                                CFG_get_rs485_hwaddr(),
                                CFG_get_rs485_server_hwaddr(),
    #endif
                                CFG_get_latitude() ? CFG_get_latitude() : "",
                                CFG_get_longitude() ? CFG_get_longitude() : "",
                                CFG_get_user_auth() ? "on" : "off",
                                CFG_get_rtc_shutdown(),
                                mode, level, server ? server : "", port
    #if defined(__MLI_1WLS_TYPE__) || defined(__MLI_1WLG_TYTE__)
                                ,
                                CFG_get_lora_enable() ? "on" : "off",
                                CFG_get_lora_channel(),
                                CFG_get_lora_baudrate(),
                                CFG_get_lora_address(),
                                CFG_get_lora_server_address()
    #endif
    #if defined(__MLI_1WRS_TYPE__) || defined(__MLI_1WLS_TYPE__) || \
        defined(__MLI_1WRG_TYPE__) || defined(__MLI_1WLG_TYPE__) || \
        defined(__MLI_1WRC_TYPE__) || defined(__MLI_1WLC_TYPE__)
                                    ,
                                CFG_get_dht_enable() ? "on" : "off",
                                CFG_get_dht_timeout(),
                                CFG_get_dht_temp_upper(),
                                CFG_get_dht_temp_lower(),
                                CFG_get_dht_rh_upper(),
                                CFG_get_dht_rh_lower(),
                                CFG_get_dht_relay() ? "on" : "off",
                                CFG_get_dht_alarm() ? "on" : "off",
                                CFG_get_mq2_enable() ? "on" : "off",
                                CFG_get_mq2_timeout(),
                                CFG_get_mq2_limit(),
                                CFG_get_mq2_relay() ? "on" : "off",
                                CFG_get_mq2_alarm() ? "on" : "off",
                                CFG_get_pir_enable() ? "on" : "off",
                                CFG_get_pir_chime() ? "on" : "off",
                                CFG_get_pir_relay() ? "on" : "off",
                                CFG_get_pir_alarm() ? "on" : "off",
                                CFG_get_pir_timeout(),
                                CFG_get_sensor_type(),
                                CFG_get_sensor_flow(),
                                CFG_get_sensor_limit(),
                                CFG_get_temt_enable() ? "on" : "off",
                                CFG_get_temt_timeout(),
                                CFG_get_temt_upper(),
                                CFG_get_temt_lower(),
                                CFG_get_temt_relay() ? "on" : "off",
                                CFG_get_temt_alarm() ? "on" : "off"
    #endif
    #if defined(__MLI_1WRP_TYPE__)
                                ,
                                CFG_get_relay_status() ? "on" : "off",
                                CFG_get_pow_voltage_cal(),
                                CFG_get_pow_voltage_upper(),
                                CFG_get_pow_voltage_lower(),
                                CFG_get_pow_current_cal(),
                                CFG_get_pow_current_upper(),
                                CFG_get_pow_current_lower(),
                                CFG_get_pow_power_upper(),
                                CFG_get_pow_power_lower(),
                                CFG_get_pow_relay() ? "on" : "off",
                                CFG_get_pow_alarm_time(),
                                CFG_get_pow_relay_timeout(),
                                CFG_get_pow_relay_ext() ? "on" : "off",
                                CFG_get_pow_interval(),
                                CFG_get_pow_day(),
                                CFG_get_pow_energy_cal(),
                                CFG_get_energy_daily_limit(),
                                CFG_get_energy_monthly_limit(),
                                CFG_get_energy_total_limit()
    #endif
                );
                // httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                // httpdStartResponse(connData, 200);
                // httpdHeader(connData, "Content-Type", "application/json");
                httpd_resp_set_type(req, "application/json");
                // sprintf(slen, "%d", len);
                // httpdHeader(connData, "Content-Length", slen);
                // httpdEndHeaders(connData);
                // httpdSendData(connData, body, len);
                httpd_resp_send(req, body, len);
                free(body);
                free(buf);
                return ESP_OK;
            }
            #endif
            else if (!strcmp(request, "restoreconfig"))
            {
                ulip_core_restore_config(true);
            }
            else if (!strcmp(request, "users"))
            {
                if (!account_db_get_size()) {
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No users found");
                    free(buf);
                    return ESP_OK;
                }
                httpd_query_key_value(buf, "file", file, sizeof(file));
                body = (char *)malloc(2048);
                index = 0;
                if (!body) {
                    free(buf);
                    return ESP_OK;
                }
                if (*file != '\0')
                {
                    /* CSV */
                    httpd_resp_set_type(req, "text/csv");
                    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=accounts.csv");
                    ESP_LOGI("main", "setting headers csv");
                    for (int i = 0; i < account_db_get_size(); i++) {
                        len = account_db_string(i, body, 2048);
                        ESP_LOGI("main", "%d", i);
                        httpd_resp_send_chunk(req, body, len);
                    }
                

                }
                else
                {
                    /* JSON */
                    // httpd_resp_set_type(req, "application/json; charset=iso-8859-1");
                    httpd_resp_set_type(req, "application/json");
                    ESP_LOGI("main", "setting headers json");
                    httpd_resp_sendstr_chunk(req, "[");
                    for (int i = 0; i < account_db_get_size(); i++) {
                        len = account_db_json(i, body, 2048);
                        ESP_LOGI("main", "%d", i);
                        httpd_resp_send_chunk(req, body, len);
                        if (i < account_db_get_size() - 1) {
                            httpd_resp_sendstr_chunk(req, ",");
                        }
                    }
                    httpd_resp_sendstr_chunk(req, "]");
                }
                httpd_resp_send_chunk(req, body, 0);
                free(body);                
                free(buf);
                return ESP_OK;
            }
            else if (!strcmp(request, "usersummary"))
            {
                body = (char *)malloc(512);
                if (!body)
                {
                    httpd_resp_send_500(req);
                    free(buf);
                    return ESP_FAIL;
                }
                len = account_db_json_summary(body, 512);
                httpd_resp_set_type(req, "application/json");
                httpd_resp_send(req, body, len);
                free(body);
                free(buf);
                return ESP_OK;
            }
            else if (!strcmp(request, "accesslog"))
            {
                httpd_query_key_value(buf, "file", file, sizeof(file));

                size_t recv_size = MIN(req->content_len, sizeof(filter));

                int ret = httpd_req_recv(req, filter, recv_size);
                filter[recv_size] = '\0';
                // httpdFindArg(connData->getArgs, "file", file, sizeof(file));
                /* Check filter */
                ESP_LOGI("main", "filter: %s", filter);
                if (ret > 0)
                {
                    // strcpy(filter, connData->post->buff);
                    config = filter;
                    while ((p = strtok(config, ",")))
                    {
                        config = NULL;
                        strdelimit(p, "{}\r\n", ' ');
                        strstrip(p);
                        if (!strncmp("\"start\":", p, 8))
                        {
                            p += 8;
                            strdelimit(p, "\"", ' ');
                            strstrip(p);
                            start = p;
                        }
                        else if (!strncmp("\"end\":", p, 6))
                        {
                            p += 6;
                            strdelimit(p, "\"", ' ');
                            strstrip(p);
                            end = p;
                        }
                    }
                } else {
                    httpd_resp_send_408(req);
                    free(buf);
                    return ESP_FAIL;
                }
                if (*file != '\0')
                {
                    // /* CSV */
                    // httpdHeader(connData, "Content-type", "text/csv");
                    // httpdHeader(connData, "Content-Disposition", "attachment; filename=accesses.csv");
                    httpd_resp_set_type(req, "text/csv");
                    // httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=accesses.csv");
                    body = (char *)malloc(1024);
                    if (!body)
                    {
                        httpd_resp_send_500(req);
                        free(buf);
                        return ESP_FAIL;
                    }
                    if (account_db_log_get_first() == 0) {
                        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No accesses found");
                        free(buf);
                        free(body);
                        return ESP_OK;
                    }
                    for (int i = account_db_log_get_first(); i >= 0; i--)
                    {
                        ESP_LOGI("main", "i: %d", i);
                        account_log_t *log = account_db_log_get_index(i);
                        if (account_log_get_date(log) < start || account_log_get_date(log) > end)
                        {
                            ESP_LOGI("main", "%s", account_log_get_date(log));
                            ESP_LOGI("main", "skip %d", account_log_get_date(log) < start);
                            continue;
                        }
                        len = account_db_log_string(i, body, 512);
                        ESP_LOGI("main", "log %s", body);
                        httpd_resp_send_chunk(req, body, len);
                    }
                    free(body);
                    // ESP_LOGI("main", "first: %d", i);
                }
                else
                {
                    /* JSON */
                    // httpdHeader(connData, "Content-type", "application/json; charset=iso-8859-1");
                    httpd_resp_set_type(req, "application/json");
                }
                httpd_resp_send_chunk(req, " ", 0);
                #if 0
                if (!connData->cgiData)
                {
                    httpdStartResponse(connData, 200);
                    httpdEndHeaders(connData);
                    if (*file == '\0')
                        httpdSendData(connData, "[", 1);
                    /* Get first log */
                    index = account_db_log_get_first();
                }
                else
                {
                    /* Get previous log */
                    index = (int)connData->cgiData & ~(3 << 30);
                    index = account_db_log_get_previous(index);
                    if (index == account_db_log_get_first())
                    {
                        if (*file == '\0')
                            httpdSendData(connData, "]", 1);
                        return HTTPD_CGI_DONE;
                    }
                }
                /* Check filter */
                if (start && end)
                {
                    log = account_db_log_get_index(index);
                    if (log)
                    {
                        if (strcmp(end, account_log_get_date(log)) < 0)
                        {
                            if (connData->cgiData)
                            {
                                httpdSuspend(connData, 10);
                                if ((int)connData->cgiData & (1 << 30))
                                    index |= (1 << 30);
                            }
                            connData->cgiData = (void *)(index | (1 << 31));
                            free(log);
                            return HTTPD_CGI_MORE;
                        }
                        if (strcmp(start, account_log_get_date(log)) > 0)
                        {
                            if (*file == '\0')
                                httpdSendData(connData, "]", 1);
                            free(log);
                            return HTTPD_CGI_DONE;
                        }
                        httpdSuspend(connData, 0);
                        free(log);
                    }
                }
                body = (char *)malloc(1024);
                if (!body)
                    return HTTPD_CGI_DONE;
                if (*file == '\0')
                    len = account_db_log_json(index, body, 1024);
                else
                    len = account_db_log_string(index, body, 1024);
                if (len > 0)
                {
                    if (*file == '\0')
                    {
                        if ((int)connData->cgiData & (1 << 30))
                            httpdSendData(connData, ",", 1);
                    }
                    httpdSendData(connData, body, len);
                    connData->cgiData = (void *)(index | (3 << 30));
                    free(body);
                    return HTTPD_CGI_MORE;
                }
                else
                {
                    if (*file == '\0')
                        httpdSendData(connData, "]", 1);
                    free(body);
                    return HTTPD_CGI_DONE;
                }
                #endif
            }
        }
    }
    free(buf);
    return ESP_OK;
#if 0
            else if (!strcmp(request, "delaccesslog"))
            {
                ulip_core_log_remove();
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                httpdStartResponse(connData, 200);
                httpdHeader(connData, "Content-Length", "0");
                httpdEndHeaders(connData);
                return HTTPD_CGI_DONE;
    // #endif
            }
            else if (!strcmp(request, "adduser"))
            {
    #if defined(__MLI_1WB_TYPE__) || defined(__MLI_1WQB_TYPE__)
                if (connData->cgiData)
                {
                    index = (int)connData->cgiData & ~(1 << 31);
                    /* Check FPM status */
                    if (fpm_is_busy())
                    {
                        httpdSuspend(connData, 100);
                        return HTTPD_CGI_MORE;
                    }
                    /* JSON */
                    body = (char *)malloc(128);
                    if (!body)
                    {
                        httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                        httpdStartResponse(connData, 500);
                        httpdHeader(connData, "Content-Length", "0");
                        httpdEndHeaders(connData);
                        return HTTPD_CGI_DONE;
                    }
                    acc = account_db_get_index(index);
                    key = (char *)account_get_key(acc);
                    len = sprintf(body, "{\"id\":\"%d\",\"key\":\"%s\"}",
                                    index, key ? key : "");
                    account_destroy(acc);
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 200);
                    httpdHeader(connData, "Content-Type", "application/json");
                    sprintf(slen, "%d", len);
                    httpdHeader(connData, "Content-Length", slen);
                    httpdEndHeaders(connData);
                    httpdSendData(connData, body, len);
                    free(body);
                    return HTTPD_CGI_DONE;
                }
    #endif
                if (!connData->post || !connData->post->buff)
                {
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 400);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    return HTTPD_CGI_DONE;
                }
                /* JSON */
                config = connData->post->buff;
                while ((p = strtok(config, ",")))
                {
                    config = NULL;
                    strdelimit(p, "{}\r\n", ' ');
                    strstrip(p);
                    if (!strncmp("\"name\":", p, 7))
                    {
                        name = p + 7;
                        strdelimit(name, "\"", ' ');
                        strstrip(name);
                    }
                    else if (!strncmp("\"user\":", p, 7))
                    {
                        user = p + 7;
                        strdelimit(user, "\"", ' ');
                        strstrip(user);
                    }
                    else if (!strncmp("\"password\":", p, 11))
                    {
                        password = p + 11;
                        strdelimit(password, "\"", ' ');
                        strstrip(password);
                    }
                    else if (!strncmp("\"card\":", p, 7))
                    {
                        card = p + 7;
                        strdelimit((char *)card, "\"", ' ');
                        strstrip((char *)card);
                    }
                    else if (!strncmp("\"qrcode\":", p, 9))
                    {
                        qrcode = p + 9;
                        strdelimit((char *)qrcode, "\"", ' ');
                        strstrip((char *)qrcode);
                    }
                    else if (!strncmp("\"rfcode\":", p, 9))
                    {
                        rfcode = p + 9;
                        strdelimit((char *)rfcode, "\"", ' ');
                        strstrip((char *)rfcode);
                    }
                    else if (!strncmp("\"fingerprint\":", p, 14))
                    {
                        p += 14;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        base64Decode(strlen(p), p, sizeof(template),
                                    template);
                        fingerprint = template;
                    }
                    else if (!strncmp("\"lifecount\":", p, 12))
                    {
                        p += 12;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        lifecount = strtol(p, NULL, 10);
                    }
                    else if (!strncmp("\"accessibility\":", p, 16))
                    {
                        p += 16;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        accessibility = !strncasecmp(p, "true", 4);
                    }
                    else if (!strncmp("\"panic\":", p, 8))
                    {
                        p += 8;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        panic = !strncasecmp(p, "true", 4);
                    }
                    else if (!strncmp("\"key\":", p, 6))
                    {
                        p += 6;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        key = p;
                    }
                    else if (!strncmp("\"administrator\":", p, 16))
                    {
                        p += 16;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        level = !strncasecmp(p, "true", 4);
                    }
                    else if (!strncmp("\"visitor\":", p, 10))
                    {
                        p += 10;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        visitor = !strncasecmp(p, "true", 4);
                    }
                    else if (!strncmp("\"finger\":", p, 9))
                    {
                        p += 9;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        finger = p;
                    }
                    else if (!strncmp("\"perm1\":", p, 8))
                    {
                        p += 8;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        os_strcpy(perm[size++], p);
                    }
                    else if (!strncmp("\"perm2\":", p, 8))
                    {
                        p += 8;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        os_strcpy(perm[size++], p);
                    }
                    else if (!strncmp("\"perm3\":", p, 8))
                    {
                        p += 8;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        os_strcpy(perm[size++], p);
                    }
                    else if (!strncmp("\"perm4\":", p, 8))
                    {
                        p += 8;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        os_strcpy(perm[size++], p);
                    }
                    else if (!strncmp("\"perm5\":", p, 8))
                    {
                        p += 8;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        os_strcpy(perm[size++], p);
                    }
                }
                /* Check account */
                index = account_db_find(NULL, user, card, qrcode, rfcode,
                                        fingerprint, NULL);
                if (index != -1)
                {
                    acc = account_db_get_index(index);
                    if (!acc)
                    {
                        httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                        httpdStartResponse(connData, 500);
                        httpdHeader(connData, "Content-Length", "0");
                        httpdEndHeaders(connData);
                        return HTTPD_CGI_DONE;
                    }
                    if (name && strcmp(name, account_get_name(acc) ? account_get_name(acc) : ""))
                    {
                        update = TRUE;
                    }
                    else if (user && strcmp(user, account_get_user(acc) ? account_get_user(acc) : ""))
                    {
                        update = TRUE;
                    }
                    else if (password && strcmp(password, account_get_password(acc) ? account_get_password(acc) : ""))
                    {
                        update = TRUE;
                    }
                    else if (card && strcmp(card, account_get_card(acc) ? account_get_card(acc) : ""))
                    {
                        update = TRUE;
                    }
                    else if (qrcode && strcmp(qrcode, account_get_code(acc) ? account_get_code(acc) : ""))
                    {
                        update = TRUE;
                    }
                    else if (rfcode && strcmp(rfcode, account_get_rfcode(acc) ? account_get_rfcode(acc) : ""))
                    {
                        update = TRUE;
                    }
                    else if (fingerprint && ((!!*fingerprint != !!account_get_fingerprint(acc)) ||
                                            os_memcmp(fingerprint, account_get_fingerprint(acc) ? account_get_fingerprint(acc) : fingerprint,
                                                    ACCOUNT_FINGERPRINT_SIZE)))
                    {
                        update = TRUE;
                    }
                    else if (lifecount != account_get_lifecount(acc))
                    {
                        update = TRUE;
                    }
                    else if (accessibility != account_get_accessibility(acc))
                    {
                        update = TRUE;
                    }
                    else if (panic != account_get_panic(acc))
                    {
                        update = TRUE;
                    }
                    else if (key && strcmp(key, account_get_key(acc) ? account_get_key(acc) : ""))
                    {
                        update = TRUE;
                    }
                    else if (level != account_get_level(acc))
                    {
                        update = TRUE;
                    }
                    else if (visitor != account_get_visitor(acc))
                    {
                        update = TRUE;
                    }
                    else if (finger && strcmp(finger, account_get_finger(acc) ? account_get_finger(acc) : ""))
                    {
                        update = TRUE;
                    }
                    else
                    {
                        acc_permission_t *k = account_get_permission(acc);
                        if (size && k)
                        {
                            for (i = 0; i < ACCOUNT_PERMISSIONS; i++)
                            {
                                if (strcmp(perm[i], k[i]))
                                {
                                    update = TRUE;
                                    break;
                                }
                            }
                        }
                        else if (size || k)
                        {
                            update = TRUE;
                        }
                    }
                    free(acc);
                    if (!update)
                    {
                        /* JSON */
                        body = (char *)malloc(128);
                        if (!body)
                        {
                            httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                            httpdStartResponse(connData, 500);
                            httpdHeader(connData, "Content-Length", "0");
                            httpdEndHeaders(connData);
                            return HTTPD_CGI_DONE;
                        }
                        len = sprintf(body, "{\"id\":\"%d\",\"key\":\"%s\"}",
                                        index, key ? key : "");
                        httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                        httpdStartResponse(connData, 200);
                        httpdHeader(connData, "Content-Type", "application/json");
                        sprintf(slen, "%d", len);
                        httpdHeader(connData, "Content-Length", slen);
                        httpdEndHeaders(connData);
                        httpdSendData(connData, body, len);
                        free(body);
                        return HTTPD_CGI_DONE;
                    }
                }
                // Hack to allocate memory on stack
    #if 0
                acc = account_new();
                if (!acc) {
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 500);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    return HTTPD_CGI_DONE;
                }
    #else
                uint8_t __acc[ACCOUNT_DATA_SIZE];
                acc = (account_t *)__acc;
                memset(acc, 0, ACCOUNT_DATA_SIZE);
    #endif
                if (name)
                    account_set_name(acc, name);
                if (user)
                    account_set_user(acc, user);
                if (password)
                    account_set_password(acc, password);
                if (card)
                    account_set_card(acc, card);
                if (qrcode)
                    account_set_code(acc, qrcode);
                if (rfcode)
                    account_set_rfcode(acc, rfcode);
                if (*template != 0)
                    account_set_fingerprint(acc, template);
                account_set_lifecount(acc, lifecount);
                account_set_accessibility(acc, accessibility);
                account_set_panic(acc, panic);
                if (key)
                    account_set_key(acc, key);
                account_set_level(acc, level);
                account_set_visitor(acc, visitor);
                if (finger)
                    account_set_finger(acc, finger);
                if (size)
                    account_set_permission(acc, perm, size);
                index = account_db_insert(acc);
                if (index == -1)
                {
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 500);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
    #if 0
                    account_destroy(acc);
    #endif
                    return HTTPD_CGI_DONE;
                }
    #if 0
                account_destroy(acc);
    #endif
    #if defined(__MLI_1WB_TYPE__) || defined(__MLI_1WQB_TYPE__)
                if (*template == 0)
                    fpm_delete_template(index);
                else
                    fpm_set_template(index, template);
                connData->cgiData = (void *)(index | (1 << 31));
                httpdSuspend(connData, 100);
                return HTTPD_CGI_MORE;
    #else
                /* JSON */
                body = (char *)malloc(128);
                if (!body)
                {
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 500);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    return HTTPD_CGI_DONE;
                }
                len = sprintf(body, "{\"id\":\"%d\",\"key\":\"%s\"}",
                                index, key ? key : "");
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                httpdStartResponse(connData, 200);
                httpdHeader(connData, "Content-Type", "application/json");
                sprintf(slen, "%d", len);
                httpdHeader(connData, "Content-Length", slen);
                httpdEndHeaders(connData);
                httpdSendData(connData, body, len);
                free(body);
                return HTTPD_CGI_DONE;
    #endif
            }
            else if (!strcmp(request, "getuser"))
            {
                if (!connData->post || !connData->post->buff)
                {
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 400);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    return HTTPD_CGI_DONE;
                }
                /* JSON */
                config = connData->post->buff;
                while ((p = strtok(config, ",")))
                {
                    config = NULL;
                    strdelimit(p, "{}\r\n", ' ');
                    strstrip(p);
                    if (!strncmp("\"id\":", p, 5))
                    {
                        uid = p + 5;
                        strdelimit(uid, "\"", ' ');
                        strstrip(uid);
                    }
                    else if (!strncmp("\"user\":", p, 7))
                    {
                        user = p + 7;
                        strdelimit(user, "\"", ' ');
                        strstrip(user);
                    }
                    else if (!strncmp("\"card\":", p, 7))
                    {
                        card = p + 7;
                        strdelimit((char *)card, "\"", ' ');
                        strstrip((char *)card);
                    }
                    else if (!strncmp("\"qrcode\":", p, 9))
                    {
                        qrcode = p + 9;
                        strdelimit((char *)qrcode, "\"", ' ');
                        strstrip((char *)qrcode);
                    }
                    else if (!strncmp("\"rfcode\":", p, 9))
                    {
                        rfcode = p + 9;
                        strdelimit((char *)rfcode, "\"", ' ');
                        strstrip((char *)rfcode);
                    }
                    else if (!strncmp("\"fingerprint\":", p, 14))
                    {
                        p += 14;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        base64Decode(strlen(p), p, sizeof(template),
                                    template);
                        fingerprint = template;
                    }
                }
                if (!uid)
                    index = account_db_find(NULL, user, card, qrcode, rfcode,
                                            fingerprint, NULL);
                else
                    index = strtol(uid, NULL, 10);
                if (index == -1)
                {
                    os_info("ULIP", "User not found");
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 404);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    return HTTPD_CGI_DONE;
                }
                /* JSON */
                body = (char *)malloc(2048);
                if (!body)
                {
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 500);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    return HTTPD_CGI_DONE;
                }
                len = account_db_json(index, body, 2048);
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                httpdStartResponse(connData, 200);
                httpdHeader(connData, "Content-Type", "application/json");
                sprintf(slen, "%d", len);
                httpdHeader(connData, "Content-Length", slen);
                httpdEndHeaders(connData);
                httpdSendData(connData, body, len);
                free(body);
                return HTTPD_CGI_DONE;
            }
            else if (!strcmp(request, "deluser"))
            {
                if (!connData->post || !connData->post->buff)
                {
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 400);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    return HTTPD_CGI_DONE;
                }
                /* JSON */
                config = connData->post->buff;
                while ((p = strtok(config, ",")))
                {
                    config = NULL;
                    strdelimit(p, "{}\r\n", ' ');
                    strstrip(p);
                    if (!strncmp("\"id\":", p, 5))
                    {
                        uid = p + 5;
                        strdelimit(uid, "\"", ' ');
                        strstrip(uid);
                    }
                    else if (!strncmp("\"key\":", p, 6))
                    {
                        key = p + 6;
                        strdelimit(key, "\"", ' ');
                        strstrip(key);
                        if (*key == '\0')
                            key = NULL;
                    }
                    else if (!strncmp("\"user\":", p, 7))
                    {
                        user = p + 7;
                        strdelimit(user, "\"", ' ');
                        strstrip(user);
                    }
                    else if (!strncmp("\"card\":", p, 7))
                    {
                        card = p + 7;
                        strdelimit((char *)card, "\"", ' ');
                        strstrip((char *)card);
                    }
                    else if (!strncmp("\"qrcode\":", p, 9))
                    {
                        qrcode = p + 9;
                        strdelimit((char *)qrcode, "\"", ' ');
                        strstrip((char *)qrcode);
                    }
                    else if (!strncmp("\"rfcode\":", p, 9))
                    {
                        rfcode = p + 9;
                        strdelimit((char *)rfcode, "\"", ' ');
                        strstrip((char *)rfcode);
                    }
                    else if (!strncmp("\"fingerprint\":", p, 14))
                    {
                        p += 14;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        base64Decode(strlen(p), p, sizeof(template),
                                    template);
                        fingerprint = template;
                    }
                }
                if (!key)
                {
                    if (!uid)
                        index = account_db_find(NULL, user, card, qrcode, rfcode,
                                                fingerprint, NULL);
                    else
                        index = strtol(uid, NULL, 10);
                    if (index == -1)
                    {
                        os_info("ULIP", "User not found");
                        httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                        httpdStartResponse(connData, 400);
                        httpdHeader(connData, "Content-Length", "0");
                        httpdEndHeaders(connData);
                        return HTTPD_CGI_DONE;
                    }
                    if (account_db_delete(index))
                    {
                        httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                        httpdStartResponse(connData, 500);
                        httpdHeader(connData, "Content-Length", "0");
                        httpdEndHeaders(connData);
                        return HTTPD_CGI_DONE;
                    }
    #if defined(__MLI_1WB_TYPE__) || defined(__MLI_1WQB_TYPE__)
                    fpm_delete_template(index);
    #endif
                }
                else
                {
                    /* Remove by key */
                    while ((index = account_db_find(NULL, NULL, NULL, NULL,
                                                    NULL, NULL, key)) != -1)
                    {
                        account_db_delete(index);
    #if defined(__MLI_1WB_TYPE__) || defined(__MLI_1WQB_TYPE__)
                        fpm_delete_template(index);
    #endif
                    }
                }
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                httpdStartResponse(connData, 200);
                httpdHeader(connData, "Content-Length", "0");
                httpdEndHeaders(connData);
                return HTTPD_CGI_DONE;
            }
            else if (!strcmp(request, "probeuser"))
            {
                httpdFindArg(connData->getArgs, "state",
                            state, sizeof(state));
                /* JSON */
                index = -1;
                if (connData->post && connData->post->buff)
                {
                    config = connData->post->buff;
                    while ((p = strtok(config, ",")))
                    {
                        config = NULL;
                        strdelimit(p, "{}\r\n", ' ');
                        strstrip(p);
                        if (!strncmp("\"id\":", p, 5))
                        {
                            uid = p + 5;
                            strdelimit(uid, "\"", ' ');
                            strstrip(uid);
                            if (*uid == '\0')
                                uid = NULL;
                        }
                        else if (!strncmp("\"user\":", p, 7))
                        {
                            user = p + 7;
                            strdelimit(user, "\"", ' ');
                            strstrip(user);
                        }
                        else if (!strncmp("\"card\":", p, 7))
                        {
                            card = p + 7;
                            strdelimit((char *)card, "\"", ' ');
                            strstrip((char *)card);
                        }
                        else if (!strncmp("\"qrcode\":", p, 9))
                        {
                            qrcode = p + 9;
                            strdelimit((char *)qrcode, "\"", ' ');
                            strstrip((char *)qrcode);
                        }
                        else if (!strncmp("\"rfcode\":", p, 9))
                        {
                            rfcode = p + 9;
                            strdelimit((char *)rfcode, "\"", ' ');
                            strstrip((char *)rfcode);
                        }
                        else if (!strncmp("\"fingerprint\":", p, 14))
                        {
                            fingerprint = p + 14;
                            strdelimit((char *)fingerprint, "\"", ' ');
                            strstrip((char *)fingerprint);
                        }
                    }
                    if (!uid)
                        index = account_db_find(NULL, user, card, qrcode, rfcode,
                                                fingerprint, NULL);
                    else
                        index = strtol(uid, NULL, 10);
                }
                if (!strcmp(state, "off"))
                {
                    probe_user = false;
                    probe_index = -1;
                }
                else
                {
                    probe_user = TRUE;
                    probe_index = index;
                }
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                httpdStartResponse(connData, 200);
                httpdHeader(connData, "Content-Length", "0");
                httpdEndHeaders(connData);
                return HTTPD_CGI_DONE;
            }
            else if (!strcmp(request, "eraseuser"))
            {
                httpdFindArg(connData->getArgs, "state",
                            state, sizeof(state));
                if (!strcmp(state, "off"))
                    erase_user = false;
                else
                    erase_user = TRUE;
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                httpdStartResponse(connData, 200);
                httpdHeader(connData, "Content-Length", "0");
                httpdEndHeaders(connData);
                return HTTPD_CGI_DONE;
            }
            else if (!strcmp(request, "checkuser"))
            {
                if (!connData->post || !connData->post->buff)
                {
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 400);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    return HTTPD_CGI_DONE;
                }
                /* JSON */
                config = connData->post->buff;
                while ((p = strtok(config, ",")))
                {
                    config = NULL;
                    strdelimit(p, "{}\r\n", ' ');
                    strstrip(p);
                    if (!strncmp("\"id\":", p, 5))
                    {
                        uid = p + 5;
                        strdelimit(uid, "\"", ' ');
                        strstrip(uid);
                    }
                    else if (!strncmp("\"user\":", p, 7))
                    {
                        user = p + 7;
                        strdelimit(user, "\"", ' ');
                        strstrip(user);
                    }
                    else if (!strncmp("\"card\":", p, 7))
                    {
                        card = p + 7;
                        strdelimit((char *)card, "\"", ' ');
                        strstrip((char *)card);
                    }
                    else if (!strncmp("\"qrcode\":", p, 9))
                    {
                        qrcode = p + 9;
                        strdelimit((char *)qrcode, "\"", ' ');
                        strstrip((char *)qrcode);
                    }
                    else if (!strncmp("\"rfcode\":", p, 9))
                    {
                        rfcode = p + 9;
                        strdelimit((char *)rfcode, "\"", ' ');
                        strstrip((char *)rfcode);
                    }
                    else if (!strncmp("\"fingerprint\":", p, 14))
                    {
                        p += 14;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        base64Decode(strlen(p), p, sizeof(template),
                                    template);
                        fingerprint = template;
                    }
                }
                if (!uid)
                    index = account_db_find(NULL, user, card, qrcode, rfcode,
                                            fingerprint, NULL);
                else
                    index = strtol(uid, NULL, 10);
                if (index == -1)
                {
                    os_info("ULIP", "User not found");
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 404);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    return HTTPD_CGI_DONE;
                }
                acc = account_db_get_index(index);
                rc = account_check_permission(acc);
                account_destroy(acc);
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                if (!rc)
                {
                    os_info("ULIP", "User blocked");
                    httpdStartResponse(connData, 403);
                }
                else
                {
                    httpdStartResponse(connData, 200);
                }
                httpdHeader(connData, "Content-Length", "0");
                httpdEndHeaders(connData);
                return HTTPD_CGI_DONE;
            }
            else if (!strcmp(request, "eraseall"))
            {
                account_db_remove_all();
    #if defined(__MLI_1WB_TYPE__) || defined(__MLI_1WQB_TYPE__)
                fpm_delete_all();
    #endif
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                httpdStartResponse(connData, 200);
                httpdHeader(connData, "Content-Length", "0");
                httpdEndHeaders(connData);
                return HTTPD_CGI_DONE;
    #if defined(__MLI_1WB_TYPE__) || defined(__MLI_1WQB_TYPE__)
            }
            else if (!strcmp(request, "finger"))
            {
                if (!connData->post || !connData->post->buff)
                {
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 400);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    return HTTPD_CGI_DONE;
                }
                /* JSON */
                config = connData->post->buff;
                while ((p = strtok(config, ",")))
                {
                    config = NULL;
                    strdelimit(p, "{}\r\n", ' ');
                    strstrip(p);
                    if (!strncmp("\"id\":", p, 5))
                    {
                        uid = p + 5;
                        strdelimit(uid, "\"", ' ');
                        strstrip(uid);
                    }
                    else if (!strncmp("\"user\":", p, 7))
                    {
                        user = p + 7;
                        strdelimit(user, "\"", ' ');
                        strstrip(user);
                    }
                    else if (!strncmp("\"card\":", p, 7))
                    {
                        card = p + 7;
                        strdelimit((char *)card, "\"", ' ');
                        strstrip((char *)card);
                    }
                    else if (!strncmp("\"qrcode\":", p, 9))
                    {
                        qrcode = p + 9;
                        strdelimit((char *)qrcode, "\"", ' ');
                        strstrip((char *)qrcode);
                    }
                    else if (!strncmp("\"rfcode\":", p, 9))
                    {
                        rfcode = p + 9;
                        strdelimit((char *)rfcode, "\"", ' ');
                        strstrip((char *)rfcode);
                    }
                    else if (!strncmp("\"finger\":", p, 9))
                    {
                        p += 9;
                        strdelimit((char *)p, "\"", ' ');
                        strstrip((char *)p);
                        finger = p;
                    }
                    else if (!strncmp("\"panic\":", p, 8))
                    {
                        p += 8;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        panic = !strncasecmp(p, "true", 4);
                    }
                }
                if (!uid)
                    index = account_db_find(NULL, user, card, qrcode, rfcode,
                                            NULL, NULL);
                else
                    index = strtol(uid, NULL, 10);
                if (index == -1)
                {
                    os_info("ULIP", "User not found");
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 400);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    return HTTPD_CGI_DONE;
                }
                httpdFindArg(connData->getArgs, "state",
                            state, sizeof(state));
                if (!strcmp(state, "off"))
                {
                    ulip_core_capture_finger(false, index);
                }
                else
                {
                    acc = account_db_get_index(index);
                    if (acc)
                    {
                        account_set_fingerprint(acc, NULL);
                        account_set_finger(acc, finger);
                        account_set_panic(acc, panic);
                        account_db_insert(acc);
                        account_destroy(acc);
                    }
                    ulip_core_capture_finger(TRUE, index);
                }
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                httpdStartResponse(connData, 200);
                httpdHeader(connData, "Content-Length", "0");
                httpdEndHeaders(connData);
                return HTTPD_CGI_DONE;
    #endif
            }
            else if (!strcmp(request, "location"))
            {
                /* JSON */
                body = (char *)malloc(256);
                if (!body)
                {
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 500);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    return HTTPD_CGI_DONE;
                }
                len = sprintf(body, "{\"latitude\":\"%s\",\"longitude\":\"%s\"}",
                                CFG_get_latitude() ? CFG_get_latitude() : "",
                                CFG_get_longitude() ? CFG_get_longitude() : "");
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                httpdStartResponse(connData, 200);
                httpdHeader(connData, "Content-Type", "application/json");
                sprintf(slen, "%d", len);
                httpdHeader(connData, "Content-Length", slen);
                httpdEndHeaders(connData);
                httpdSendData(connData, body, len);
                free(body);
                return HTTPD_CGI_DONE;
            }
            else if (!strcmp(request, "alarms"))
            {
                /* JSON */
                body = (char *)malloc(256);
                if (!body)
                {
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 500);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    return HTTPD_CGI_DONE;
                }
                len = sprintf(body, "{\"alarm\":\"%s\",\"panic\":\"%s\",\"breakin\":\"%s\"}",
                                ctl_alarm_status() ? "on" : "off",
                                ctl_panic_status() ? "on" : "off",
                                ctl_breakin_status() ? "on" : "off");
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                httpdStartResponse(connData, 200);
                httpdHeader(connData, "Content-Type", "application/json");
                sprintf(slen, "%d", len);
                httpdHeader(connData, "Content-Length", slen);
                httpdEndHeaders(connData);
                httpdSendData(connData, body, len);
                free(body);
                return HTTPD_CGI_DONE;
            }
            else if (!strcmp(request, "getdatetime"))
            {
                /* JSON */
                body = (char *)malloc(256);
                if (!body)
                {
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 500);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    return HTTPD_CGI_DONE;
                }
                tm = rtc_localtime();
                sprintf(date, "%04d-%02d-%02d %02d:%02d:%02d",
                        tm->tm_year, tm->tm_mon + 1,
                        tm->tm_mday, tm->tm_hour,
                        tm->tm_min, tm->tm_sec);
                len = sprintf(body, "{\"datetime\":\"%s\",\"timestamp\":\"%lu\"}",
                                date, rtc_time());
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                httpdStartResponse(connData, 200);
                httpdHeader(connData, "Content-Type", "application/json");
                sprintf(slen, "%d", len);
                httpdHeader(connData, "Content-Length", slen);
                httpdEndHeaders(connData);
                httpdSendData(connData, body, len);
                free(body);
                return HTTPD_CGI_DONE;
            }
            else if (!strcmp(request, "setdatetime"))
            {
                if (!connData->post || !connData->post->buff)
                {
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 400);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    return HTTPD_CGI_DONE;
                }
                /* JSON */
                config = connData->post->buff;
                while ((p = strtok(config, ",")))
                {
                    config = NULL;
                    strdelimit(p, "{}\r\n", ' ');
                    strstrip(p);
                    if (!strncmp("\"datetime\":", p, 11))
                    {
                        struct tm t;
                        char *l;
                        p += 11;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        p = strtok_r(p, " ", &l);
                        p = strtok(p, "-");
                        if (!p)
                            continue;
                        /* Date */
                        t.tm_year = strtol(p, NULL, 10);
                        p = strtok(NULL, "-");
                        if (!p)
                            continue;
                        t.tm_mon = strtol(p, NULL, 10) - 1;
                        p = strtok(NULL, "-");
                        if (!p)
                            continue;
                        t.tm_mday = strtol(p, NULL, 10);
                        /* Time */
                        l = strtok(l, ":");
                        if (!l)
                            continue;
                        t.tm_hour = strtol(l, NULL, 10);
                        l = strtok(NULL, ":");
                        if (!l)
                            continue;
                        t.tm_min = strtol(l, NULL, 10);
                        l = strtok(NULL, ":");
                        if (!l)
                            continue;
                        t.tm_sec = strtol(l, NULL, 10);
                        rtc_set_time(rtc_mktime(&t));
                        /* Save date and time in flash */
                        if (abs(rtc_time() - CFG_get_rtc_time()) > 60)
                        {
                            CFG_set_rtc_time(rtc_time());
                            CFG_Save();
                        }
                    }
                    else if (!strncmp("\"timestamp\":", p, 12))
                    {
                        p += 12;
                        strdelimit(p, "\"", ' ');
                        strstrip(p);
                        rtc_set_time(strtol(p, NULL, 10));
                        /* Save date and time in flash */
                        if (abs(rtc_time() - CFG_get_rtc_time()) > 60)
                        {
                            CFG_set_rtc_time(rtc_time());
                            CFG_Save();
                        }
                    }
                }
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                httpdStartResponse(connData, 200);
                httpdHeader(connData, "Content-Length", "0");
                httpdEndHeaders(connData);
                return HTTPD_CGI_DONE;
    #if defined(__MLI_1WQ_TYPE__) || defined(__MLI_1WQB_TYPE__) || \
        defined(__MLI_1WQF_TYPE__) || defined(__MLI_1WRQ_TYPE__)
            }
            else if (!strcmp(request, "getqrcode"))
            {
                /* Check account */
                if (!qrcode_get_dynamic() ||
                    authBasicGetUsername(connData, username, sizeof(username)))
                {
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 400);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    return HTTPD_CGI_DONE;
                }
                index = account_db_find(NULL, username, NULL, NULL, NULL,
                                        NULL, NULL);
                if (index == -1)
                {
                    os_info("ULIP", "User not found");
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 404);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    return HTTPD_CGI_DONE;
                }
                acc = account_db_get_index(index);
                if (!account_get_code(acc) || !account_get_key(acc))
                {
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 500);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    account_destroy(acc);
                    return HTTPD_CGI_DONE;
                }
                /* JSON */
                body = (char *)malloc(256);
                if (!body)
                {
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 500);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    account_destroy(acc);
                    return HTTPD_CGI_DONE;
                }
                tm = rtc_localtime();
                sprintf(date, "%04d-%02d-%02d %02d:%02d:%02d",
                        tm->tm_year, tm->tm_mon + 1,
                        tm->tm_mday, tm->tm_hour,
                        tm->tm_min, tm->tm_sec);
                len = sprintf(body, "{\"qrcode\":\"%s\",\"key\":\"%s\",\"validity\":\"%d\"}",
                                account_get_code(acc), account_get_key(acc),
                                qrcode_get_validity());
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                httpdStartResponse(connData, 200);
                httpdHeader(connData, "Content-Type", "application/json");
                sprintf(slen, "%d", len);
                httpdHeader(connData, "Content-Length", slen);
                httpdEndHeaders(connData);
                httpdSendData(connData, body, len);
                account_destroy(acc);
                free(body);
                return HTTPD_CGI_DONE;
    #endif
            }
            else if (!strcmp(request, "beep"))
            {
                httpdFindArg(connData->getArgs, "state",
                            state, sizeof(state));
                if (!strcmp(state, "granted") || !strcmp(state, "panic"))
                    ctl_beep(0);
                else if (!strcmp(state, "blocked"))
                    ctl_buzzer_on(CTL_BUZZER_ERROR);
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                httpdStartResponse(connData, 200);
                httpdHeader(connData, "Content-Length", "0");
                httpdEndHeaders(connData);
                return HTTPD_CGI_DONE;
            }
    #if defined(__MLI_1WRS_TYPE__) || defined(__MLI_1WLS_TYPE__) || \
        defined(__MLI_1WRG_TYPE__) || defined(__MLI_1WLG_TYPE__) || \
        defined(__MLI_1WRP_TYPE__) || defined(__MLI_1WRC_TYPE__) || \
        defined(__MLI_1WLC_TYPE__)
            else if (!strcmp(request, "telemetry"))
            {
                /* JSON */
                body = (char *)malloc(512);
                if (!body)
                {
                    httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                    httpdStartResponse(connData, 500);
                    httpdHeader(connData, "Content-Length", "0");
                    httpdEndHeaders(connData);
                    return HTTPD_CGI_DONE;
                }
                /* Check sensor function */
                switch (CFG_get_sensor_type())
                {
                /* Level */
                case CFG_SENSOR_LEVEL:
                    levelstatus = ctl_sensor_status() == CTL_SENSOR_ON ? "alarm" : "normal";
                    volume = NULL;
                    volumestatus = NULL;
                    break;
                /* Volume */
                case CFG_SENSOR_VOLUME:
                    levelstatus = NULL;
                    volume = CFG_get_sensor_str_volume();
                    volumestatus = "normal";
                    if (CFG_get_sensor_limit())
                    {
                        if (CFG_get_sensor_volume() >= CFG_get_sensor_limit())
                            volumestatus = "alarm";
                    }
                    break;
                /* Normal */
                default:
                    levelstatus = NULL;
                    volume = NULL;
                    volumestatus = NULL;
                    break;
                }
    #if defined(__MLI_1WRS_TYPE__) || defined(__MLI_1WLS_TYPE__)
                len = sprintf(body, "{\"temperature\":\"%s\",\"temperaturestatus\":\"%s\","
                                    "\"humidity\":\"%s\",\"humiditystatus\":\"%s\",\"luminosity\":\"%d\","
                                    "\"luminositystatus\":\"%s\",\"pirstatus\":\"%s\",\"levelstatus\":\"%s\","
                                    "\"volume\":\"%s\",\"volumestatus\":\"%s\"}",
                                dht_get_str_temperature(),
                                dht_get_temperature_alarm() ? "alarm" : "normal",
                                dht_get_str_humidity(),
                                dht_get_humidity_alarm() ? "alarm" : "normal",
                                temt_get_lux(), temt_get_alarm() ? "alarm" : "normal",
                                pir_get_status() ? "alarm" : "normal",
                                levelstatus ? levelstatus : "",
                                volume ? volume : "",
                                volumestatus ? volumestatus : "");
    #elif defined(__MLI_1WRG_TYPE__) || defined(__MLI_1WLG_TYPE__)
                len = sprintf(body, "{\"gas\":\"%d\",\"gasstatus\":\"%s\","
                                    "\"pirstatus\":\"%s\",\"levelstatus\":\"%s\","
                                    "\"volume\":\"%s\",\"volumestatus\":\"%s\"}",
                                mq2_get_gas(), mq2_get_alarm() ? "alarm" : "normal",
                                pir_get_status() ? "alarm" : "normal",
                                levelstatus ? levelstatus : "",
                                volume ? volume : "",
                                volumestatus ? volumestatus : "");
    #elif defined(__MLI_1WRC_TYPE__) || defined(__MLI_1WLC_TYPE__)
                len = sprintf(body, "{\"temperature\":\"%s\",\"temperaturestatus\":\"%s\","
                                    "\"humidity\":\"%s\",\"humiditystatus\":\"%s\","
                                    "\"loop\":\"%d\",\"loopstatus\":\"%s\","
                                    "\"pirstatus\":\"%s\",\"levelstatus\":\"%s\","
                                    "\"volume\":\"%s\",\"volumestatus\":\"%s\"}",
                                dht_get_str_temperature(),
                                dht_get_temperature_alarm() ? "alarm" : "normal",
                                dht_get_str_humidity(),
                                dht_get_humidity_alarm() ? "alarm" : "normal",
                                cli_get_value(), cli_get_alarm() ? "alarm" : "normal",
                                pir_get_status() ? "alarm" : "normal",
                                levelstatus ? levelstatus : "",
                                volume ? volume : "",
                                volumestatus ? volumestatus : "");
    #else
                len = sprintf(body, "{\"voltage\":\"%s\",\"voltagestatus\":\"%s\","
                                    "\"current\":\"%s\",\"currentstatus\":\"%s\",\"power\":\"%s\","
                                    "\"powerstatus\":\"%s\",\"powerfactor\":\"%s\",\"energydaily\":\"%s\","
                                    "\"energydailylast\":\"%s\",\"energymonthly\":\"%s\","
                                    "\"energymonthlylast\":\"%s\",\"energytotal\":\"%s\","
                                    "\"energystatus\":\"%s\",",
                                pow_get_voltage_str(),
                                pow_get_voltage_alarm() ? "alarm" : "normal",
                                pow_get_current_str(),
                                pow_get_current_alarm() ? "alarm" : "normal",
                                pow_get_active_power_str(),
                                pow_get_power_alarm() ? "alarm" : "normal",
                                pow_get_power_factor_str(),
                                CFG_get_energy_daily_str(),
                                CFG_get_energy_daily_last_str(),
                                CFG_get_energy_monthly_str(),
                                CFG_get_energy_monthly_last_str(),
                                CFG_get_energy_total_str(),
                                pow_get_energy_alarm() ? "alarm" : "normal");
                len += sprintf(body + len, "\"energymonth\":{");
                for (i = 0; i < 12; i++)
                    len += sprintf(body + len, "\"%d\":\"%s\"%s",
                                    i, CFG_get_energy_month_str(i),
                                    i < 11 ? "," : "");
                len += sprintf(body + len, "}}");
    #endif
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                httpdStartResponse(connData, 200);
                httpdHeader(connData, "Content-Type", "application/json");
                sprintf(slen, "%d", len);
                httpdHeader(connData, "Content-Length", slen);
                httpdEndHeaders(connData);
                httpdSendData(connData, body, len);
                free(body);
                return HTTPD_CGI_DONE;
            }
            else if (!strcmp(request, "deltelemetry"))
            {
                sensor_cycles = 0;
                if (CFG_get_sensor_cycles())
                {
                    CFG_set_sensor_cycles(0);
                    CFG_Save();
                }
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                httpdStartResponse(connData, 200);
                httpdHeader(connData, "Content-Length", "0");
                httpdEndHeaders(connData);
                return HTTPD_CGI_DONE;
            }
            else if (!strcmp(request, "telemetrylog"))
            {
                httpdFindArg(connData->getArgs, "file", file, sizeof(file));
                /* Check filter */
                if (connData->post && connData->post->buff)
                {
                    os_strcpy(filter, connData->post->buff);
                    config = filter;
                    while ((p = strtok(config, ",")))
                    {
                        config = NULL;
                        strdelimit(p, "{}\r\n", ' ');
                        strstrip(p);
                        if (!strncmp("\"start\":", p, 8))
                        {
                            p += 8;
                            strdelimit(p, "\"", ' ');
                            strstrip(p);
                            start = p;
                        }
                        else if (!strncmp("\"end\":", p, 6))
                        {
                            p += 6;
                            strdelimit(p, "\"", ' ');
                            strstrip(p);
                            end = p;
                        }
                    }
                }
                if (!connData->cgiData)
                {
                    httpdStartResponse(connData, 200);
                    if (*file != '\0')
                    {
                        /* CSV */
                        httpdHeader(connData, "Content-type", "text/csv");
                        httpdHeader(connData, "Content-Disposition", "attachment; filename=telemetry.csv");
                    }
                    else
                    {
                        /* JSON */
                        httpdHeader(connData, "Content-type", "application/json; charset=iso-8859-1");
                    }
                    httpdEndHeaders(connData);
                    if (*file == '\0')
                        httpdSendData(connData, "[", 1);
                    /* Get first log */
                    index = telemetry_db_get_first();
                }
                else
                {
                    /* Get previous log */
                    index = (int)connData->cgiData & ~(3 << 30);
                    index = telemetry_db_get_previous(index);
                    if (index == telemetry_db_get_first())
                    {
                        if (*file == '\0')
                            httpdSendData(connData, "]", 1);
                        return HTTPD_CGI_DONE;
                    }
                }
                /* Check filter */
                if (start && end)
                {
                    log = telemetry_db_get_index(index);
                    if (log)
                    {
                        if (strcmp(end, telemetry_get_date(log)) < 0)
                        {
                            if (connData->cgiData)
                            {
                                httpdSuspend(connData, 10);
                                if ((int)connData->cgiData & (1 << 30))
                                    index |= (1 << 30);
                            }
                            connData->cgiData = (void *)(index | (1 << 31));
                            free(log);
                            return HTTPD_CGI_MORE;
                        }
                        if (strcmp(start, telemetry_get_date(log)) > 0)
                        {
                            if (*file == '\0')
                                httpdSendData(connData, "]", 1);
                            free(log);
                            return HTTPD_CGI_DONE;
                        }
                        httpdSuspend(connData, 0);
                        free(log);
                    }
                }
                body = (char *)malloc(1024);
                if (!body)
                    return HTTPD_CGI_DONE;
                if (*file == '\0')
                    len = telemetry_db_json(index, body, 1024);
                else
                    len = telemetry_db_string(index, body, 1024);
                if (len > 0)
                {
                    if (*file == '\0')
                    {
                        if ((int)connData->cgiData & (1 << 30))
                            httpdSendData(connData, ",", 1);
                    }
                    httpdSendData(connData, body, len);
                    connData->cgiData = (void *)(index | (3 << 30));
                    free(body);
                    return HTTPD_CGI_MORE;
                }
                else
                {
                    if (*file == '\0')
                        httpdSendData(connData, "]", 1);
                    free(body);
                    return HTTPD_CGI_DONE;
                }
            }
            else if (!strcmp(request, "deltelemetrylog"))
            {
                ulip_core_telemetry_remove();
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                httpdStartResponse(connData, 200);
                httpdHeader(connData, "Content-Length", "0");
                httpdEndHeaders(connData);
                return HTTPD_CGI_DONE;
            }
            else if (!strcmp(request, "relayaux"))
            {
                httpdFindArg(connData->getArgs, "state",
                            state, sizeof(state));
                if (!strcmp(state, "on") || !strcmp(state, "hold"))
                {
                    if (!strcmp(state, "on"))
                    {
                        ctl_relay_ext_on(CFG_get_control_timeout());
                    }
                    else
                    {
                        ctl_relay_ext_on(0);
                    }
                }
                else if (!strcmp(state, "off"))
                {
                    ctl_relay_ext_off();
                }
                httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
                httpdStartResponse(connData, 200);
                httpdHeader(connData, "Content-Length", "0");
                httpdEndHeaders(connData);
                return HTTPD_CGI_DONE;
            }
    #endif
            httdSetTransferMode(connData, HTTPD_TRANSFER_CLOSE);
            httpdStartResponse(connData, 404);
            httpdHeader(connData, "Content-Length", "0");
            httpdEndHeaders(connData);
            return HTTPD_CGI_DONE;
        }

        return ulip_cgi_process(connData);
#endif
}

static void timer_callback()
{
    xTaskCreate(release_task, "release task", 4096, NULL, 10, NULL);
}
void set_time_names() {
    weekday[0] = "Sun";
    weekday[1] = "Mon";
    weekday[2] = "Tue";
    weekday[3] = "Wed";
    weekday[4] = "Thu";
    weekday[5] = "Fri";
    weekday[6] = "Sat";
    monthday[0] = "Jan";
    monthday[1] = "Feb";
    monthday[2] = "Mar";
    monthday[3] = "Apr";
    monthday[4] = "May";
    monthday[5] = "Jun";
    monthday[6] = "Jul";
    monthday[7] = "Aug";
    monthday[8] = "Sep";
    monthday[9] = "Oct";
    monthday[10] = "Nov";
    monthday[11] = "Dec";

}
void app_main(void)
{
    set_time_names();
    
    CFG_Load();
    CFG_Default();
    CFG_set_control_mode(0);
    CFG_set_control_timeout(2);
    CFG_set_ip_address("10.0.0.140");
    CFG_set_netmask("255.255.255.0");
    CFG_set_gateway("10.0.0.1");
    CFG_set_ap_mode(false);
    CFG_set_dhcp(true);
    CFG_set_wifi_ssid("uTech-Wifi");
    CFG_set_wifi_passwd("01566062");
    CFG_set_wifi_disable(true);
    CFG_set_eth_dhcp(false);
    CFG_set_eth_enable(true);
    CFG_set_eth_ip_address("10.0.0.243");
    CFG_set_eth_netmask("255.255.255.0");
    CFG_set_eth_gateway("10.0.0.1");
    // CFG_Save();
    CFG_set_fingerprint_timeout(100000);
    CFG_set_debug(1, ESP_LOG_INFO, "10.0.0.140", 64195);
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

    account_init();

    
    // rf433_init(CFG_get_rf433_rc(), CFG_get_rf433_bc(),
    //                CFG_get_rf433_panic_timeout(),
    //                rf433_event, NULL);

    // bluetooth_start();

    // CFG_set_rs485_hwaddr(2);
    // CFG_set_rs485_server_hwaddr(1);
    // rs485_init(0, CFG_get_rs485_hwaddr(), 3, 1000000,
    //                rs485_event, NULL);
    printf("Hello world!\n");
    ESP_LOGI("main", "tasks: %u", uxTaskGetNumberOfTasks());
    // sntp_setoperatingmode(SNTP_OPMODE_POLL);
    // sntp_setservername(0, "pool.ntp.org");
    // sntp_init();
    // vTaskList(tasks_info);
    // ESP_LOGI("main", "\n%s", tasks_info);
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

    // fpm_init(CFG_get_fingerprint_timeout(),CFG_get_fingerprint_security(),
    //         CFG_get_fingerprint_identify_retries(),fingerprint_event, NULL);
    // char *cur_task = pcTaskGetTaskName(xTaskGetCurrentTaskHandle());
    // printf(cur_task);
    if (CFG_get_eth_enable())
        start_eth(CFG_get_eth_dhcp(), CFG_get_eth_ip_address(), CFG_get_eth_gateway(), CFG_get_eth_netmask(), &got_ip_event2);
    ESP_LOGI("main", "eth_enable: %d, eth_dhcp: %d, eth_ip:%s, eth_gateway:%s, eth_netmask:%s",
             CFG_get_eth_enable(), CFG_get_eth_dhcp(), CFG_get_eth_ip_address(), CFG_get_eth_gateway(), CFG_get_eth_netmask());
    start_httpd(&ulip_core_httpd_request);
}
