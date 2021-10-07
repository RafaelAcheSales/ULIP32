#include "esp_log.h"
#include "gpio_drv.h"
#include "time.h"
#include "keeloq.h"
#include "rf433.h"
#include "string.h"
#include "esp_timer.h"

#undef DEBUG
// #define TEST_INTR               1
#define RF433_RX_PIN            12
#define INPUT_MASK              (1ULL<<RF433_RX_PIN)
#define RF433_RX_INTR           2
#define GPIO_OUTPUT             4
#define GPIO_OUTPUT_PIN_SEL (1ULL<<GPIO_OUTPUT)
#define RF433_CODESIZE          16
#define RF433_TIMEOUT           1000

#define RCSWITCH_MAX_PROTO      8

/*
 * Number of maximum high/Low changes per packet.
 * We can handle up to => 66 bit * 2 H/L changes per bit + 
 *                        24 for preamble +
 *                        1 for header +
 *                        2 for sync
 */
// #define RCSWITCH_MAX_CHANGES    800
#define RCSWITCH_MAX_CHANGES    159

/**
 * Description of a single pulse, which consists of a high signal
 * whose duration is "high" times the base pulse length, followed
 * by a low signal lasting "low" times the base pulse length.
 * Thus, the pulse overall lasts (high+low)*pulseLength
 */
typedef struct {
    uint8_t high;
    uint8_t low;
} HighLow;

/**
 * A "protocol" describes how zero and one bits are encoded into high/low
 * pulses.
 */
typedef struct {
    /** base pulse length in microseconds, e.g. 350 */
    uint16_t pulseLength;

    HighLow syncFactor;
    HighLow zero;
    HighLow one;

    /**
     * If true, interchange high and low logic levels in all transmissions.
     *
     * By default, RCSwitch assumes that any signals it sends or receives
     * can be broken down into pulses which start with a high signal level,
     * followed by a a low signal level. This is e.g. the case for the
     * popular PT 2260 encoder chip, and thus many switches out there.
     *
     * But some devices do it the other way around, and start with a low
     * signal level, followed by a high signal level, e.g. the HT6P20B. To
     * accommodate this, one can set invertedSignal to true, which causes
     * RCSwitch to change how it interprets any HighLow struct FOO: It will
     * then assume transmissions start with a low signal lasting
     * FOO.high*pulseLength microseconds, followed by a high signal lasting
     * FOO.low*pulseLength microseconds.
     */
    bool invertedSignal;

    bool rollingCode;
} rc_protocol_t;

static rc_protocol_t rc_proto[RCSWITCH_MAX_PROTO] = {
    { 350, {  1, 31 }, {  1,  3 }, {  3,  1 }, false, false },    // protocol 1
    { 650, {  1, 10 }, {  1,  2 }, {  2,  1 }, false, false },    // protocol 2
    { 100, { 30, 71 }, {  4, 11 }, {  9,  6 }, false, false },    // protocol 3
    { 380, {  1,  6 }, {  1,  3 }, {  3,  1 }, false, false },    // protocol 4
    { 500, {  6, 14 }, {  1,  2 }, {  2,  1 }, false, false },    // protocol 5
    { 450, { 23,  1 }, {  1,  2 }, {  2,  1 }, true,  false },    // protocol 6 (HT6P20B)
    { 150, {  2, 62 }, {  1,  6 }, {  6,  1 }, false, false },    // protocol 7 (HS2303-PT, i. e. used in AUKEY Remote)
    { 400, {  1, 40 }, {  2,  1 }, {  1,  2 }, false, true  },    // HCS301
};
#if defined(TEST_INTR)
static int cnt = 0;
#endif
static uint32_t nRollingCodeValue = 0;
static unsigned long int nReceivedValue = 0;
static uint8_t nButtonValue = 0;
static uint8_t nStatusValue = 0;
static unsigned int nReceivedBitlength = 0;
static unsigned int nReceivedDelay = 0;
static unsigned nReceivedProtocol = 0;
static unsigned nReceiveTolerance = 50;
static unsigned int nSeparationLimit = 4500;


// separationLimit: minimum microseconds between received codes, closer codes are ignored.
// according to discussion on issue #14 it might be more suitable to set the separation
// limit to the same time as the 'low' part of the sync signal for the current protocol.
static unsigned long long int timings[RCSWITCH_MAX_CHANGES];

static bool rf433_rolling_code = false;
static bool rf433_button_code = true;
static rf433_handler_t rf433_func = NULL;
static void *rf433_user_data = NULL;
static char rf433_code[RF433_CODESIZE];
static uint8_t rf433_button = 0;
static uint32_t rf433_timestamp;
static int rf433_panic_timeout = 0;
static int rf433_panic_timestamp = 0;


static long long diff(long long int a, long long int b)
{
    return llabs(a - b);
}

static bool rf433_receive(int p, unsigned int changeCount)
{
    // ESP_ERROR_CHECK(esp_task_wdt_reset());
    // Assuming the longer pulse length is the pulse captured in timings[0]

    rc_protocol_t *proto = &rc_proto[p];
    unsigned int syncLengthInPulses = proto->syncFactor.low > proto->syncFactor.high ?
                                      proto->syncFactor.low : proto->syncFactor.high;
    unsigned int delay = timings[0] / syncLengthInPulses;
    unsigned int delayTolerance = (delay * nReceiveTolerance) / 100;
    unsigned int firstDataTiming = (proto->invertedSignal) ? (2) : (1);
    uint32_t code = 0;
    int bits = 0;
    int i;
    
    /* For protocols that start low, the sync period looks like
     *               _________
     * _____________|         |XXXXXXXXXXXX|
     *
     * |--1st dur--|-2nd dur-|-Start data-|
     *
     * The 3rd saved duration starts the data.
     *
     * For protocols that start high, the sync period looks like
     *
     *  ______________
     * |              |____________|XXXXXXXXXXXXX|
     *
     * |-filtered out-|--1st dur--|--Start data--|
     *
     * The 2nd saved duration starts the data
     */

    /* Check for rolling code */
    
    if (!proto->rollingCode) {
        if (changeCount > 67)
            return false;
        i = firstDataTiming;
    } else {
        /* Find header */
        if (changeCount <= 67)
            return false;
        for (i = 1; i < changeCount - 1; i++) {
            if (diff(timings[i], 10 * delay) < 10 * delayTolerance) {
                break;
            }
        }
        if (i == changeCount - 1)
            return false;
        i += firstDataTiming;
    }
    
    for (; i < changeCount - 1; i += 2) {
        if (!proto->rollingCode) {
            /* MSB first */
            code <<= 1;
        } else {
            /* LSB first */
            code >>= 1;
        }
        // ets_printf(" 1st %llu 2nd %llu 3rd %llu 4th %llu\n", diff(timings[i], delay * proto->zero.high), (proto->zero.high * delayTolerance), diff(timings[i + 1], delay * proto->zero.low) , (proto->zero.low * delayTolerance) );
        if (diff(timings[i], delay * proto->zero.high) < (proto->zero.high * delayTolerance) &&
            diff(timings[i + 1], delay * proto->zero.low) < (proto->zero.low * delayTolerance)) {
            // Zero
        } else if (diff(timings[i], delay * proto->one.high) < (proto->one.high * delayTolerance) &&
                   diff(timings[i + 1], delay * proto->one.low) < (proto->one.low * delayTolerance)) {
            // One
            if (!proto->rollingCode) {
                code |= 1;
            } else {
                code |= (1 << 31);
            }
        } else {
            // Failed
            
            return false;
        }
        bits++;
        if (proto->rollingCode) {
            /* 32 encrypted bits */
            if (bits == 32) {
                nRollingCodeValue = code;
                code = 0;
            }
            /* 28 serial bits */
            if (bits == 60) {
                nReceivedValue = (code >> 4);
                code = 0;
            }
            /* 4 button bits */
            if (bits == 64) {
                nButtonValue = (code >> 28);
                code = 0;
            }
            /* 2 status bits (Repeat + VLow) */
            if (bits == 65) {
                /* Ignore repeat bit */
                nStatusValue = (code >> 31);
            }
        }
    }
    // ESP_ERROR_CHECK(esp_task_wdt_reset());
    if (bits >= 12) {    // ignore very short transmissions: no device sends them, so this must be noise
        /* HT6P20B */
        if (p == 5) {
            nButtonValue = (code >> 4) & 0xf;
            code &= 0xfffff0f;
        }
        if (!proto->rollingCode)
            nReceivedValue = code;
        nReceivedBitlength = (changeCount - 1) / 2;
        nReceivedDelay = delay;
        nReceivedProtocol = p;
        return true;
    }

    // ets_printf("failed2\n");
    return false;
}
#if defined(TEST_INTR)
//test callback
static void rf433_interrupt_handler(int intr, void *user_data) {

    cnt += 1;
    gpio_set_level(GPIO_OUTPUT, cnt & 1);
}
#else
static void rf433_interrupt_handler(int intr, void *user_data)
{
    // ESP_LOGI("RF433", "interrupt handle 433");
    static unsigned int changeCount = 0;
    static unsigned long lastTime = 0;
    static unsigned int repeatCount = 0;
    uint64_t now = esp_timer_get_time();
    uint64_t duration = now - lastTime;
    char code[RF433_CODESIZE];
    uint16_t sync = 0;
    uint8_t button = 0;
    uint8_t status = 0;
    rc_protocol_t *p;
    uint32_t rcdata;
    int rc;
    int i;
    
    // ets_printf("%lld\n", now);
    if (duration > nSeparationLimit) {
        // A long stretch without signal level change occurred. This could
        // be the gap between two transmission.
        if (diff(duration, timings[0]) < 200) {
            // ESP_ERROR_CHECK(esp_task_wdt_reset());
            // This long signal is close in length to the long signal which
            // started the previously recorded timings; this suggests that
            // it may indeed by a a gap between two transmissions (we assume
            // here that a sender will send the signal multiple times,
            // with roughly the same gap between them).
            if (++repeatCount == 2) {
                // vTaskDelay(1);
                // ets_printf("cg %d dura %lu now %lu lt %lu\n", changeCount, duration, now, lastTime);
                for (i = 0; i < RCSWITCH_MAX_PROTO; i++) {
                    p = &rc_proto[i];
                    // ets_printf("porraloka\n");
                    if (rf433_receive(i, changeCount)) {
                        // ets_printf("porra\n\n\n\n\n\n\n");
                        if (rf433_rolling_code) {
                            if (!p->rollingCode)
                                continue;
                            rcdata = keeloq_decrypt(nRollingCodeValue);
                            /* Check discrimination value */
                            if (((rcdata >> 16) & 0x3ff) != (nReceivedValue & 0x3ff))
                                continue;
                            sync = rcdata & 0xffff;
                            button = (rcdata >> 28) & 0xf;
                            status = nStatusValue;
                        } else {
                            button = nButtonValue;
                            status = nStatusValue;
                        }
                        if (rf433_button_code) {
                            /* HT6P20B */
                            if (i == 5)
                                sprintf(code, "%lu", nReceivedValue | (button << 4));
                            else
                                sprintf(code, "%lu", (button << 28 | nReceivedValue));
                        } else {
                            sprintf(code, "%lu", nReceivedValue);
                        }
                        /* Debounce */
                        if (rf433_timestamp) {
                            if (!strcmp(rf433_code, code) && rf433_button == button) {
                                duration = (now - rf433_timestamp) / 1000;
                                if (duration <= RF433_TIMEOUT) {
                                    ESP_LOGD("RF433", "Debounce code [%s]", code);
                                    // ESP_ERROR_CHECK(esp_task_wdt_reset());
                                    /* Panic */
                                    if (rf433_panic_timestamp > 0) {
                                        rf433_panic_timestamp -= duration;
                                        if (rf433_panic_timestamp <= 0) {
                                            if (rf433_func) {
                                                rc = rf433_func(RF433_EVT_CHECK, code,
                                                                strlen(rf433_code), sync,
                                                                button, status, rf433_user_data);
                                                if (!rc) {
                                                    ESP_LOGD("RF433", "Panic code [%s]", code);
                                                    rc = rf433_func(RF433_EVT_PANIC, code,
                                                                    strlen(rf433_code), sync,
                                                                    button, status, rf433_user_data);
                                                }
                                            }
                                            rf433_panic_timestamp = 0;
                                        }
                                    }
                                    rf433_timestamp = now;
                                    break;
                                }
                            }
                        }
                        strncpy(rf433_code, code, sizeof(rf433_code) - 1);
                        rf433_button = button;
                        rf433_timestamp = now;
                        rf433_panic_timestamp = rf433_panic_timeout;
                        if (rf433_func)
                            rc = rf433_func(RF433_EVT_CODE, rf433_code,
                                            strlen(rf433_code), sync,
                                            button, status, rf433_user_data);
                        break;
                    }
                }
                repeatCount = 0;
            }
        }
        changeCount = 0;
    }
 
    // Detect overflow
    if (changeCount >= RCSWITCH_MAX_CHANGES) {
        ESP_LOGD("RF433", "Overflow");
        changeCount = 0;
        repeatCount = 0;
    }

    timings[changeCount++] = duration;
    lastTime = now;
}
#endif
static void periodic_timer_callback(void* arg)
{
    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI("timer", "Periodic timer called, time since boot: %lld us", time_since_boot);
}

int rf433_init(bool rolling_code, bool button_code,
               int panic_timeout, rf433_handler_t func,
               void *user_data)
{
    ESP_LOGI("RF433", "Initialize RF433");
    // const esp_timer_create_args_t periodic_timer_args = {
    //         .callback = &periodic_timer_callback,
    //         /* name is optional, but may help identify the timer when debugging */
    //         .name = "periodic"
    // };
    
    // esp_timer_handle_t periodic_timer;
    // ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    // esp_timer_start_once(periodic_timer, 0);
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = INPUT_MASK;  
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
#if defined(TEST_INTR)
    gpio_config_t io_conf2;
    //disable interrupt
    io_conf2.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf2.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf2.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf2.pull_down_en = 0;
    //disable pull-up mode
    io_conf2.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf2);
#endif
    if (gpio_interrupt_open(RF433_RX_INTR, RF433_RX_PIN,
                            GPIO_INTR_ANYEDGE, 0,
                            rf433_interrupt_handler, NULL)) {
        ESP_LOGW("RF433", "Failed to initialize RF433!");
        return -1;
    }
    rf433_rolling_code = rolling_code;
    rf433_button_code = button_code;
    rf433_panic_timeout = panic_timeout;
    rf433_func = func;
    rf433_user_data = user_data;
    memset(rf433_code, 0, sizeof(rf433_code));
    rf433_timestamp = 0;

    /* Keeloq */
    keeloq_set_key(KEELOQ_HIGH_KEY, KEELOQ_LOW_KEY);

    return 0;
}

void rf433_release(void)
{
    gpio_interrupt_close(RF433_RX_INTR);
    rf433_func = NULL;
    rf433_user_data = NULL;
    rf433_timestamp = 0;
}
