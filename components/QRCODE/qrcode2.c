
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "stdbool.h"
#include "esp_log.h"
#include "esp_system.h" 
#include "tty.h"
#include "ctl.h"
#include "esp_timer.h"
#include "qrcode2.h"
#include "sdkconfig.h"
static unsigned char cmd[] = {0x7e, 0x00, 0x08, 0x01, 0x00, 0x00, 0x88, 0x64, 0x19};
#undef DEBUG

#define QRCODE_TTY              2
#define QRCODE_BFSIZE           512
#define QRCODE_TIMEOUT          1000  /* msec */
#define QRCODE_CARDSIZE         256
#define QRCODE_LED_TIMEOUT      500   /* msec */
#define QRCODE_ALARM_TIMEOUT    2000  /* msec */
#define QRCODE_VALIDITY         30

#define WDI2000_XMIT_HEAD       0x007E
#define WDI2000_RECV_HEAD       0x0002
#define WDI2000_READ_CMD        0x07
#define WDI2000_WRITE_CMD       0x08

typedef union {
    unsigned char *b;
    unsigned short *w;
    unsigned long *dw;
} pgen_t;


static void qrcode_led_timeout(void* arg);
static esp_timer_handle_t led_timer;
static void qrcode_polling_timeout(void* arg);
static esp_timer_handle_t polling_timer;


static bool qrcode_configured = false;
static int qrcode_init_stage = 0;
static qrcode_handler_t qrcode_func = NULL;
static void *qrcode_user_data = NULL;
static uint8_t qrcode_buf[QRCODE_BFSIZE];
static int qrcode_buflen = 0;
static bool qrcode_led = false;
static bool qrcode_led_alarm = true;
static int qrcode_led_status = 0;
static int qrcode_led_counter = 0;
static int qrcode_timeout = QRCODE_TIMEOUT;
static int qrcode_panic_timeout = 0;
static bool qrcode_dynamic = false;
static int qrcode_validity = QRCODE_VALIDITY;
static char qrcode_card[QRCODE_CARDSIZE];
static uint32_t qrcode_timestamp = 0;
static int qrcode_alarm_timeout = 0;
static int qrcode_panic_timestamp = 0;


static uint16_t crc16(uint8_t *buf, int len)
{
    unsigned int crc = 0;
    unsigned char b;
    unsigned char i;

    while (len-- > 0) {
        b = *buf;
        for (i = 0x80; i != 0; i >>= 1) {
            crc <<= 1;
            if ((crc & 0x10000) != 0) // last bit CRC x 2, if the first bit is 1, divided by 0x11021
                crc ^= 0x11021;
            if ((b & i) != 0)         // if the base bit is 1, CRC = last bit CRC + base bit/CRC_CCITT
                crc ^= 0x1021;
        }
        buf++;
    }
    return crc;
}

static int qrcode_send_command(uint8_t cmd, uint16_t addr,
                               const uint8_t *data, int len)
{
    unsigned char buf[256];
    uint16_t crc;
    pgen_t p;
    int i;

// #ifdef DEBUG
    // ESP_LOGI("QRCODE", "Send command [0x%02x] address [0x%04x] [%d] bytes",
    //          cmd, addr, len);
// #endif

    p.b = buf;
    *p.w++ = WDI2000_XMIT_HEAD;
    *p.b++ = cmd;
    *p.b++ = len;
    *p.b++ = (addr >> 8) & 0xff;
    *p.b++ = addr & 0xff;
    for (i = 0; i < len; i++)
        *p.b++ = data[i];
    crc = crc16(buf + 2, len + 4);
    *p.b++ = (crc >> 8) & 0xff;
    *p.b++ = crc & 0xff;
    ESP_LOGD("QRCODE","sending command: ");
    for (int i = 0; i < p.b-buf; i++)
    {
        //ESP_LOGI("","%02X", buf[i]);
    }
    
    return tty_write(QRCODE_TTY, buf, p.b - buf);
}

void qrcode_module_initialize(int stage)
{
    uint8_t data;

    ESP_LOGE("QRCODE", "QRCODE init stage [%d]", stage);

    switch (stage) {
        case 0:
            /* LED decode indication, read mode */
            data = 0x80;
            if (qrcode_led)
                data |= 0x08;
            qrcode_send_command(WDI2000_WRITE_CMD, 0x0000, &data, 1);
            break;
        case 1:
            /* Disable setup code */
            data = 0x02;
            qrcode_send_command(WDI2000_WRITE_CMD, 0x0003, &data, 1);
            break;
        case 2:
            /* Single read time */
            data = (qrcode_timeout / 100) >> 2;
            qrcode_send_command(WDI2000_WRITE_CMD, 0x0006, &data, 1);
            break;
        case 3:
            /* Disable deep sleep */
            data = 0x00;
            qrcode_send_command(WDI2000_WRITE_CMD, 0x0007, &data, 1);
            break;
        case 4:
            /* Serial output */
            data = 0x3C;
            qrcode_send_command(WDI2000_WRITE_CMD, 0x000D, &data, 1);
            break;
        case 5:
            /* Configure protocol */
            data = 0xE0;
            qrcode_send_command(WDI2000_WRITE_CMD, 0x0060, &data, 1);
            break;
        default:
            qrcode_configured = 1;
            break;
    }
}

static void qrcode_led_enable(void)
{
    uint8_t data;
    // ESP_LOGE("QRCODE", "led enable");

    /* Enable LED */
    data = 0x88;
    qrcode_send_command(WDI2000_WRITE_CMD, 0x0000, &data, 1);
    qrcode_led_status = 1;
}

static void qrcode_led_disable(void)
{
    uint8_t data;
    // ESP_LOGE("QRCODE", "led disable");

    /* Disable LED */
    data = 0x80;
    qrcode_send_command(WDI2000_WRITE_CMD, 0x0000, &data, 1);
    qrcode_led_status = 0;
}

static void qrcode_led_alarm_enable(void)
{
    uint8_t data;

    /* Enable LED */
    data = 0xA0;
    qrcode_send_command(WDI2000_WRITE_CMD, 0x0000, &data, 1);
}

static void qrcode_led_alarm_disable(void)
{
    uint8_t data;

    /* Disable LED */
    data = 0x80;
    qrcode_send_command(WDI2000_WRITE_CMD, 0x0000, &data, 1);
}

static void qrcode_led_timeout(void *arg)
{
    /* Restore LED state */
    if ((++qrcode_led_counter * QRCODE_LED_TIMEOUT) >= (qrcode_timeout << 1)) {
        ESP_ERROR_CHECK(esp_timer_stop(led_timer));
        qrcode_led_counter = 0;
        if (qrcode_led)
            qrcode_led_enable();
        else
            qrcode_led_disable();
        return;
    }
    if (qrcode_led_status)
        qrcode_led_disable();
    else
        qrcode_led_enable();
}

static void qrcode_led_blink(void)
{
    qrcode_led_counter = 0;
    if (qrcode_led)
        qrcode_led_disable();
    else
        qrcode_led_enable();
    if (!esp_timer_is_active(led_timer))
    {
        ESP_ERROR_CHECK(esp_timer_start_periodic(led_timer, QRCODE_LED_TIMEOUT*1000));
    }

    // os_timer_setfn(&qrcode_led_timer, (os_timer_func_t *)qrcode_led_timeout, NULL);
    // os_timer_arm(&qrcode_led_timer, QRCODE_LED_TIMEOUT, true);
}


static void qrcode_event(int tty, const char *event,
                         int len, void *user_data)
{
    ESP_LOGD("QRCODE", "qrcode event: ");
    // ESP_LOG_BUFFER_HEX("qrcode", event, len);
    char card[QRCODE_CARDSIZE];
    uint64_t now;
    struct timeval tv_now;
    uint16_t head;
    uint8_t cmd;
    uint16_t size;
    uint16_t crc;
    uint32_t d;
    pgen_t p;
    int rc;
    int i;
   
// #ifdef DEBUG 
    ESP_LOGD("QRCODE", "QRCODE read [%d] bytes len [%d]",
             len, qrcode_buflen);

    for (i = 0; i < len; i++) {
        //ESP_LOGD("QRCODE", "%c", (uint8_t)event[i]);
    }
// #endif

    /* Ignore data in alarm */
    if (qrcode_alarm_timeout > 0) {
        ESP_LOGW("QRCODE", "QRCODE ignore data! %d", qrcode_alarm_timeout);
        qrcode_alarm_timeout = QRCODE_ALARM_TIMEOUT;
        return;
    }

    if (qrcode_buflen + len > QRCODE_BFSIZE) {
        ESP_LOGW("QRCODE", "QRCODE buffer overflow");
        qrcode_buflen = 0;
        return;
    }
    
    memcpy(qrcode_buf + qrcode_buflen, event, len);
    qrcode_buflen += len;

    /* Check packet size */
    if (qrcode_buflen < 4) return;

    p.b = qrcode_buf;
    while (qrcode_buflen >= 3) {
        /* Check header */
        ESP_LOGD("QRCODE", "header %02X", p.b[0]);
        if (p.b[0] == 0x03) {
            size = (p.b[1] << 8) | p.b[2];
            if (!size) {
                ESP_LOGW("QRCODE", "Invalid size!");
                qrcode_led_alarm_enable();
                qrcode_alarm_timeout = QRCODE_ALARM_TIMEOUT;
                qrcode_buflen = 0;
                break;
            }
            /* Check size */
            if (size + 3 > qrcode_buflen)
                break;
            p.b += 3;
            len = size;
            if (len > QRCODE_CARDSIZE - 1)
                len = QRCODE_CARDSIZE - 1;
            memcpy(card, p.b, len);
            card[len] = '\0';
            p.b += size;
            qrcode_buflen -= size + 3;
            /* Debounce */
            //time(&now);
            
            gettimeofday(&tv_now, NULL);
            now = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;

            // ESP_LOGI("QRCODE", "now is :%lld timestamp is: %d ", (long long int)now, qrcode_timestamp);

            if (qrcode_timestamp) {
                if (!strcmp(qrcode_card, card)) {
                    d = (now - qrcode_timestamp);// * 1000000; /// 1000;
                    // ESP_LOGI("QRCODE", "D value is: %d, qrcodetimeout is: %d", d, qrcode_timeout<<1);
                    if (d <= (qrcode_timeout << 1)) {
                        ESP_LOGD("QRCODE", "Debounce card [%s]", card);
                        /* Panic */
                        if (qrcode_panic_timestamp > 0) {
                            qrcode_panic_timestamp -= d;
                            if (qrcode_panic_timestamp <= 0) {
                                rc = qrcode_func(QRCODE_EVT_CHECK, card, len,
                                                 qrcode_user_data);
                                if (!rc) {
                                    ESP_LOGI("QRCODE", "Panic card [%s]", card);
                                    rc = qrcode_func(QRCODE_EVT_PANIC, card, len,
                                                     qrcode_user_data);
                                }
                                qrcode_panic_timestamp = 0;
                            }
                        }
                        qrcode_timestamp = now;
                        qrcode_led_counter = 0;
                        ESP_LOGI("QRCODE", "to be continued...");
                        continue;
                    }
                    ESP_LOGI("QRCODE", "zone 1");
                }
                ESP_LOGI("QRCODE", "zone 2");
            }
            ESP_LOGI("QRCODE", "should it run led? %d", qrcode_led_alarm);
            if (qrcode_led_alarm)
                qrcode_led_blink();
            strcpy(qrcode_card, card);
            qrcode_timestamp = now;
            qrcode_panic_timestamp = qrcode_panic_timeout;
            if (qrcode_func)
                qrcode_func(QRCODE_EVT_QR, qrcode_card, strlen(qrcode_card),
                            qrcode_user_data);
        } else {
            head = p.b[0] | (p.b[1] << 8);
            if (head != WDI2000_RECV_HEAD) {
                ESP_LOGW("QRCODE", "Invalid header!");
                qrcode_led_alarm_enable();
                qrcode_alarm_timeout = QRCODE_ALARM_TIMEOUT;
                qrcode_buflen = 0;
                break;
            }
            /* Check size */
            if (qrcode_buflen <= 4)
                break;
            cmd = p.b[2];
            size = p.b[3];
            if (qrcode_buflen < size + 6)
                break;
            p.b += 2;
            if (size + 6 > qrcode_buflen) {
                ESP_LOGW("QRCODE", "Invalid size!");
                qrcode_buflen = 0;
                break;
            }
            /* CRC check */
            crc = crc16(p.b, size + 2);
            p.b += size + 2;
            crc -= (p.b[0] << 8 | p.b[1]);
            if (crc) {
                ESP_LOGW("QRCODE", "CRC error!");
                qrcode_buflen = 0;
                break;
            }
            p.b += 2;
            switch (cmd) {
                case 0x00:

                    ESP_LOGI("QRCODE", "Command response received");

                    if (!qrcode_configured)
                        qrcode_module_initialize(++qrcode_init_stage);
                    break;
                default:
                    break;
            }
            qrcode_buflen -= size + 6;
        }
    }
    if (qrcode_buflen > 0) {
        if (p.b != qrcode_buf)
            memmove(qrcode_buf, p.b, qrcode_buflen);
    }
}
// static void qrcode_event_hardware_uart(int tty, const char *event,
//                          int len, void *user_data)
// {
//     // qrcode_event(tty, event, len, user_data);
//     ESP_LOGE("QRCODE", "UART2 event: ");
//     ESP_LOG_BUFFER_HEX("qrcode", event, len);
// }
static void qrcode_polling_timeout(void *data)
{
    ESP_LOGD("QRCODE", "polling timed out");
    static int stage = 0;
    static int val = 0;

    /* Ignore data */
    if (qrcode_alarm_timeout > 0) {
        ESP_LOGD("QRCODE", "polling ignore %d", qrcode_alarm_timeout);
        qrcode_alarm_timeout -= QRCODE_TIMEOUT;
        if (qrcode_alarm_timeout > 0)
            return;
        qrcode_led_alarm_disable();
        qrcode_alarm_timeout = 0;
    }

    if (!qrcode_configured) {
        if (qrcode_init_stage == stage)
            qrcode_module_initialize(stage);
            ESP_LOGD("QRCODE","module initialize");

        stage = qrcode_init_stage;
    } else {
        /* Toggle trigger input */
        if (!val)
            ctl_qrcode_on();
        else
            ctl_qrcode_off();
        val ^= 1;
    }
}
void print_status_debug(void) {
    ESP_LOGD("QRCODE", "timer on %d  ", esp_timer_is_active(polling_timer));
}
static tty_func_t test_event(int tty, char *data,
                      int len, void *user_data)
{
    printf("event rolou teste\n");
    ESP_LOG_BUFFER_HEX("main", data, len);

    tty_func_t t = NULL;
    return t;
}
int qrcode_init(bool led, bool led_alarm, int timeout,
                int panic_timeout, bool dynamic,
                int validity, qrcode_handler_t func,
                void *user_data, int tty)
{
    ESP_LOGI("QRCODE", "Initialize QRCODE");
    
    /* Open tty */
    if (tty_open(QRCODE_TTY, qrcode_event, NULL)) {
        ESP_LOGW("QRCODE", "Failed to initialize TTY3!");
        return -1;
    }

    qrcode_led = led;
    qrcode_led_alarm = led_alarm;
    qrcode_timeout = timeout;
    qrcode_panic_timeout = panic_timeout;
    qrcode_dynamic = dynamic;
    qrcode_validity = validity;
    qrcode_func = func;
    qrcode_user_data = user_data;
    qrcode_timestamp = 0;
    qrcode_alarm_timeout = 0;

    /* Initialize module */
    // tty_write(QRCODE_TTY, cmd, 9);
    qrcode_module_initialize(0);

    /* Start polling */
    if (!qrcode_timeout)
        qrcode_timeout = QRCODE_TIMEOUT;
    const esp_timer_create_args_t polling_timer_args = {
            .callback = &qrcode_polling_timeout,
            /* name is optional, but may help identify the timer when debugging */
            .name = "polling timeout"
    };
    
    // ESP_LOGI("QRCODE", "timeout is %d", qrcode_timeout);
    ESP_ERROR_CHECK(esp_timer_create(&polling_timer_args, &polling_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(polling_timer, (qrcode_timeout>>1)*1000));
    // ESP_LOGI("QRCODE", "poling timer started");
    const esp_timer_create_args_t led_timer_args = {
            .callback = &qrcode_led_timeout,
            /* name is optional, but may help identify the timer when debugging */
            .name = "led timeout"
    };
    
    ESP_ERROR_CHECK(esp_timer_create(&led_timer_args, &led_timer));
    // os_timer_setfn(&qrcode_timer, (os_timer_func_t *)qrcode_polling_timeout, NULL);
    // os_timer_arm(&qrcode_timer, qrcode_timeout >> 1, true);
    return 0;
}

void qrcode_release(void)
{
    tty_close(QRCODE_TTY);
    ESP_ERROR_CHECK(esp_timer_stop(polling_timer));
    ESP_ERROR_CHECK(esp_timer_delete(polling_timer));
    ESP_ERROR_CHECK(esp_timer_stop(led_timer));
    ESP_ERROR_CHECK(esp_timer_delete(led_timer));
    qrcode_led = false;
    qrcode_led_alarm = true;
    qrcode_timeout = QRCODE_TIMEOUT;
    qrcode_panic_timeout = 0;
    qrcode_dynamic = false;
    qrcode_validity = QRCODE_VALIDITY;
    qrcode_func = NULL;
    qrcode_user_data = NULL;
    qrcode_timestamp = 0;
    qrcode_configured = false;
    qrcode_init_stage = 0;
    qrcode_alarm_timeout = 0;
}

bool qrcode_get_dynamic(void)
{
    return qrcode_dynamic;
}

int qrcode_get_validity(void)
{
    return qrcode_validity;
}
