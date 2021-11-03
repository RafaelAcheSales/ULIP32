#include <stdio.h>
#include "tty.h"
#include "esp_log.h"
#include "hw_timer.h"
#include "esp_timer.h"
#include "gpio_drv.h"
#include "driver/timer.h"
#include "uart_drv.h"
#include "soc/soc.h"
#include "soc/uart_periph.h"
#define TTY_BSIZE           512
#define TTY_BITBANG_BITS    10
#define TTY_TIMEOUT         100000

#define UART0               0
#define UART0_RX_PIN        8
#define UART0_TX_PIN        9

#define UART1               1
#define UART1_RX_PIN        34
#define UART1_TX_PIN        2

#define UART2               2
#define UART2_RX_PIN        35
#define UART2_TX_PIN        15

#define UART3               3
#define UART3_RX_PIN        14
#define UART3_TX_PIN        12
#define UART3_RX_INTR       3

#define GPIO_INPUT_PIN_SEL (1ULL<<UART3_RX_PIN) | (1ULL<<UART2_RX_PIN) | (1ULL<<UART1_RX_PIN)
#define GPIO_OUTPUT_PIN_SEL (1ULL<<UART3_TX_PIN) | (1ULL<<UART2_TX_PIN) | (1ULL<<UART1_TX_PIN) 


#define TTY_FIFO_CNT(head,tail,size) \
    ((tail >= head) ? (tail - head) : (size - (head - tail)))
#define TTY_FIFO_SPACE(head,tail,size) \
    ((tail >= head) ? (size - (tail - head) - 1) : (head - tail - 1))

typedef struct _tty_dev {
    int tty;
    tty_func_t func;
    void *user_data;
    uint8_t recv_buf[TTY_BSIZE];
    uint16_t recv_head;
    uint16_t recv_tail;
    uint32_t recv_xsr;
    uint8_t recv_bits;
    uint8_t xmit_buf[TTY_BSIZE];
    uint16_t xmit_head;
    uint16_t xmit_tail;
    uint32_t xmit_xsr;
    uint8_t xmit_bits;
} tty_dev_t;

static portMUX_TYPE my_mutex = portMUX_INITIALIZER_UNLOCKED;
static int cnt = 0;
static tty_dev_t tty_dev[TTY_NUM_DEV] = { 
    {
        .tty = 0,
        .func = NULL,
        .user_data = NULL,
    },
    {   
        .tty = 1,
        .func = NULL,
        .user_data = NULL,
    },
    {   
        .tty = 2,
        .func = NULL,
        .user_data = NULL,
    },
    {   
        .tty = 3,
        .func = NULL,
        .user_data = NULL,
    }
};

static bool hw_timer_enabled = false;
static uint32_t hw_timer_counter = 0;
static esp_timer_handle_t tty_task_timer;
//static xQueueHandle sw_timer_queue;

void tty_hw_timer_enable(void)
{
    if (!hw_timer_enabled) {
        hw_timer_enabled = true;
        hw_timer_counter = 0;
        hw_timer_arm();
    }
}

void tty_hw_timer_disable(void)
{
    if (hw_timer_enabled) {
        hw_timer_enabled = false;
        hw_timer_counter = 0;
        hw_timer_disarm();
    }
}

static void tty_gpio_interrupt(int intr, void *user_data)
{
    tty_dev_t *p = user_data;
    //ESP_LOGI("TTY", "iterrupto via intr: %d", intr);
    taskENTER_CRITICAL(&my_mutex);

    if (p->tty == UART3) {
        p->recv_bits = TTY_BITBANG_BITS;
        tty_hw_timer_enable();
    }

    taskEXIT_CRITICAL(&my_mutex);
}

static void tty_hw_timeout(void)
{
    // ets_printf("hwtiem\n");
    
    
    tty_dev_t *p;
    uint32_t bit;
    int rc = 0;
    // ets_printf("hwtime\n");
    taskENTER_CRITICAL(&my_mutex);
    
    /* Only receive */
    // p = &tty_dev[UART1];
    // if (p->func) {
        
    // }
    //     if (!(hw_timer_counter & 1)) {
    //         if (p->recv_bits > 0) {
    //             p->recv_bits--;
    //             /* Read GPIO */
    //             bit = GPIO_INPUT_GET(UART1_RX_PIN);
    //             p->recv_xsr >>= 1;
    //             p->recv_xsr |= (bit << (TTY_BITBANG_BITS - 1));
    //             if (!p->recv_bits) {
    //                 /* Check stop bit */
    //                 if (bit) {
    //                     p->recv_xsr >>= 1;
    //                     p->recv_buf[p->recv_tail] = p->recv_xsr & 0xFF;
    //                     p->recv_tail = ++p->recv_tail & (TTY_BSIZE - 1);
    //                 }
    //                 p->recv_xsr = 0;
    //                 /* Enable interrupt */
    //                 gpio_pin_intr_state_set(UART1_RX_PIN, GPIO_PIN_INTR_NEGEDGE);
    //             }
    //         }
    //     }
    //     /* Work pending */
    //     if (p->recv_bits) {
    //         rc++;
    //     }
    // }

    /* Transmit and Receive */
    p = &tty_dev[UART3];
    if (p->func) {
        if (!(hw_timer_counter & 1)) {
            if (p->recv_bits > 0) {
                p->recv_bits--;
                /* Read GPIO */
                bit = gpio_get_level(UART3_RX_PIN);
                //ESP_LOGI("tty","got bit %d", bit);
                p->recv_xsr >>= 1;
                p->recv_xsr |= (bit << (TTY_BITBANG_BITS - 1));
                if (!p->recv_bits) {
                    /* Check stop bit */
                    if (bit) {
                        p->recv_xsr >>= 1;
                        p->recv_buf[p->recv_tail] = p->recv_xsr & 0xFF;
                        p->recv_tail = (p->recv_tail+1) & (TTY_BSIZE - 1);
                    }
                    p->recv_xsr = 0;
                    /* Enable interrupt */
                    gpio_set_intr_type(UART3_RX_PIN, GPIO_PIN_INTR_NEGEDGE);
                }
            }
            if (p->xmit_head != p->xmit_tail) {
                if (!p->xmit_bits) {
                    p->xmit_xsr = p->xmit_buf[p->xmit_head];
                    /* Put start and stop bit */
                    p->xmit_xsr <<= 1;
                    p->xmit_xsr |= (1 << (TTY_BITBANG_BITS - 1));
                    p->xmit_bits = TTY_BITBANG_BITS;
                }
                /* Write GPIO */
                bit = (p->xmit_xsr >> (TTY_BITBANG_BITS - p->xmit_bits)) & 1;
                // gpio_set_level(12, cnt & 1);
                // cnt++;
                // ets_printf("a\n");
                gpio_set_level(UART3_TX_PIN, bit);
                p->xmit_bits--;
                if (!p->xmit_bits)
                    p->xmit_head = (p->xmit_head + 1) & (TTY_BSIZE - 1);
            }
        }
        /* Work pending */
        if (p->recv_bits || p->xmit_bits || p->xmit_head != p->xmit_tail) {
            rc++;
        }
    }

    hw_timer_counter++;

    if (!rc)
        tty_hw_timer_disable();

    taskEXIT_CRITICAL(&my_mutex);
    
}


static int 
tty_read_fifo(int tty, char *buf, int len)
{
    tty_dev_t *p;
    int size;
    int i;

    p = &tty_dev[tty];

    size = TTY_FIFO_CNT(p->recv_head, p->recv_tail, TTY_BSIZE);
    // printf("fifo size: %d\n", size);
    if (size) {
        if (size > len) size = len;
        for (i = 0; i < size; i++) {
            buf[i] = p->recv_buf[p->recv_head];
            p->recv_head = (p->recv_head+1) & (TTY_BSIZE - 1);
        }
    }

    return size;
}


static int tty_write_fifo(int tty, unsigned char *buf, int len)
{
    tty_dev_t *p;
    int rc = -1;
    int size;
    int i;

    p = &tty_dev[tty];

    size = TTY_FIFO_SPACE(p->xmit_head, p->xmit_tail, TTY_BSIZE);
    if (size >= len) {
        for (i = 0; i < len; i++) {
            p->xmit_buf[p->xmit_tail] = buf[i];
            p->xmit_tail = (p->xmit_tail + 1) & (TTY_BSIZE - 1);
        }
        rc = OK;
    }

    /* Enable UART3 bitbang */
    if (tty == UART3)
        tty_hw_timer_enable();

    return rc;
}

static void tty_task(void)
{
    char buf[TTY_BSIZE];
    tty_dev_t *p;
    int size;
    int len;
    //int i;
    // ESP_LOGD("TTY", "tty_task");
    p = &tty_dev[UART0];
    if (p->func) {
        ESP_ERROR_CHECK(uart_get_buffered_data_len(UART0, (size_t*)&len));
        size = uart_read_bytes(UART0, buf, len, 50);
        if (size) {
            // ESP_LOGI("TTY", "read %d bytes from UART %d ", size, UART1);
            // ESP_LOG_BUFFER_HEX("TTY", buf, len);
            p->func(UART0, buf, len, p->user_data);
        }
    }
    p = &tty_dev[UART1];
    if (p->func) {
        ESP_ERROR_CHECK(uart_get_buffered_data_len(UART1, (size_t*)&len));
        size = uart_read_bytes(UART1, buf, len, 50);
        if (size) {
            ESP_LOGI("TTY", "read %d bytes from UART %d ", size, UART1);
            // ESP_LOG_BUFFER_HEX("TTY", buf, len);
            p->func(UART1, buf, len, p->user_data);
        }
    }
    p = &tty_dev[UART2];
    if (p->func) {
        ESP_ERROR_CHECK(uart_get_buffered_data_len(UART2, (size_t*)&len));
        size = uart_read_bytes(UART2, buf, len, 50);
        if (size) {
            // ESP_LOGI("TTY", "read %d bytes from UART%d", size, UART2);

            p->func(UART2, buf, len, p->user_data);
        }
    }

    p = &tty_dev[UART3];
    if (p->func) {
        //printf("reading fifo\n");
        size = tty_read_fifo(UART3, buf, sizeof(buf));
        // ESP_LOGI("TTY", "reading from uart3 size %d", size);
        if (size)
            p->func(UART3, buf, size, p->user_data);
    }
    
}




int tty_init(void)
{
    // ESP_LOGI("TTY", "TTY init");

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    gpio_config_t io_conf2;
    io_conf2.intr_type = GPIO_INTR_DISABLE;
    io_conf2.mode = GPIO_MODE_INPUT;
    io_conf2.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf2.pull_down_en = 0;
    io_conf2.pull_up_en = 0;
    gpio_config(&io_conf2);

    ESP_LOGI("tty", "initialized gpios");
    uart_dev uart0_dev = {
        .baud_rate = BIT_RATE_9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bit = UART_STOP_BITS_1,
        .flow_control = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
        .uart_id = UART0,
        .tx_pin = UART0_TX_PIN,
        .rx_pin = UART0_RX_PIN,
        .cts = -1,
        .rts = -1,
    };
    uart_dev uart1_dev = {
        .baud_rate = BIT_RATE_9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bit = UART_STOP_BITS_1,
        .flow_control = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
        .uart_id = UART1,
        .tx_pin = UART1_TX_PIN,
        .rx_pin = UART1_RX_PIN,
        .cts = -1,
        .rts = -1,
    };

    uart_dev uart2_dev = {
        .baud_rate = BIT_RATE_9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bit = UART_STOP_BITS_1,
        .flow_control = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
        .uart_id = UART2,
        .tx_pin = UART2_TX_PIN,
        .rx_pin = UART2_RX_PIN,
        .cts = -1,
        .rts = -1,
    };
    // uart_initialize(&uart0_dev);
    uart_initialize(&uart1_dev);
    uart_initialize(&uart2_dev);

    ESP_LOGI("tty", "initialized uarts");
    // tty_event.sig = 1;
    // tty_event.par = 0;
    // os_timer_setfn(&tty_timer, (os_timer_func_t *)tty_task, &tty_event);
    // os_timer_arm(&tty_timer, TTY_TIMEOUT, TRUE);

    gpio_set_level(UART3_TX_PIN, 1);
    gpio_drv_init();
    // ESP_LOGI("tty", "configed gpio");

    hw_timer_set_func(tty_hw_timeout);

    hw_timer_init(52);
    ESP_LOGI("tty", "hw timer init 0");

    const esp_timer_create_args_t tty_task_timer_args = {
            .callback = &tty_task,
            /* name is optional, but may help identify the timer when debugging */
            .name = "tty task"
    };
    
    ESP_ERROR_CHECK(esp_timer_create(&tty_task_timer_args, &tty_task_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tty_task_timer, TTY_TIMEOUT));
    // sw_timer_set_func(tty_task);
    // ESP_LOGI("tty", "sw timer setfunc");
    // hw_timer_arm(TTY_TIMEOUT,true, TIMER_GROUP_1);
    // ESP_LOGI("tty", "armed sw timer");
    return 1;
}


void tty_release(void)
{
    ESP_LOGI("tty", "releasing");
    if (esp_timer_is_active(tty_task_timer))
        esp_timer_delete(tty_task_timer);
    ESP_LOGI("tty", "closed uarts");
    // tty_close(UART3);
    // hw_timer_release();
    ESP_LOGI("tty", "timers disarmed");
    gpio_drv_release();
    ESP_LOGI("tty", "released");
}


int tty_open(int tty, tty_func_t func,
             void *user_data)
{
    tty_dev_t *p;

    if (tty >= TTY_NUM_DEV) return -1;

    ESP_LOGI("TTY", "%d open", tty);

    if (func) {
        p = &tty_dev[tty];
        switch (tty) {
            case UART0:
                uart_open(UART0);
                break;
            case UART1:
                uart_open(UART1);
                break;
            case UART2:
                uart_open(UART2);
                break;
            case UART3:
                gpio_interrupt_open(UART3_RX_INTR, UART3_RX_PIN,
                                    GPIO_PIN_INTR_NEGEDGE, GPIO_INTR_DISABLED,
                                    tty_gpio_interrupt, p); 
                break;
        }
        p->func = func;
        p->user_data = user_data;
        p->recv_head = p->recv_tail = 0;
        p->recv_xsr = 0;
        p->recv_bits = 0;
        p->xmit_head = p->xmit_tail = 0;
        p->xmit_xsr = 0;
        p->xmit_bits = 0;
    }

    return 0;
}


int tty_close(int tty)
{
    tty_dev_t *p;

    if (tty >= TTY_NUM_DEV) return -1;

    ESP_LOGI("TTY", "TTY: %d close", tty);

    p = &tty_dev[tty];
    switch (tty) {
        case UART0:
            uart_close(tty);
            break;
        case UART1:
            uart_close(tty);
            break;
        case UART2:
            uart_close(tty);
            break;
        case UART3:
            gpio_interrupt_close(UART3);
            break;
    }
    p->func = NULL;
    p->user_data = NULL;
    p->recv_head = p->recv_tail = 0;
    p->recv_xsr = 0;
    p->recv_bits = 0;
    p->xmit_head = p->xmit_tail = 0;
    p->xmit_xsr = 0;
    p->xmit_bits = 0;

    return 0;
}


int tty_write(int tty, unsigned char *data, int len)
{
    
    tty_dev_t *p;
    int rc = -1;

    // int i;

    if (tty >= TTY_NUM_DEV) return -1;


    ESP_LOGD("TTY", "TTY: %d write %d bytes", tty, len);


    p = &tty_dev[tty];
    //ESP_LOGI("TTY", "tty = %d and uart is %d", tty, UART3);
    switch (tty) {
        case UART0:
            rc = uart_write_bytes(tty, data, len);
            // for (i = 0; i < len; i++)
            //     rc = uart_tx_one_char(tty, data[i]);
            break;
        case UART1:
            ESP_LOG_BUFFER_HEX("TTY", data, len);
            rc = uart_write_bytes(tty, data, len);
            break;
        case UART2:
            // ESP_LOG_BUFFER_HEX("TTY", data, len);
            rc = uart_write_bytes(tty, data, len);
            break;
        case UART3:
            // ESP_LOGI("TTY", "writing command to UART3: ");
            // ESP_LOG_BUFFER_HEX("TTY", data, len);
            // ESP_LOGE("TTY", "writing command to UART3: ");
            //printf("writing to fifo\n");
            rc = tty_write_fifo(tty, data, len);
            break;
    }

    return rc;
}


int tty_tx_fifo_size(int tty)
{
    tty_dev_t *p;
    int size = 0;

    if (tty >= UART0 && tty <= UART2) {
        // /* UART FIFO */
        size = ((READ_PERI_REG(UART_STATUS_REG(tty)) >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT);
        return size;
    } else if (tty == UART3) {
        p = &tty_dev[tty];
        size = TTY_FIFO_CNT(p->xmit_head, p->xmit_tail, TTY_BSIZE);
        return size;
    }

    return 0;
}


void tty_set_baudrate(int tty, int baudrate)
{
    if (tty >= 0 && tty < 3) {
        uart_set_baudrate(tty, baudrate);
    }
}
uint32_t tty_get_baudrate(int tty)
{
    uint32_t baud;
    if (tty >= 0 && tty < 3) {
        uart_get_baudrate(tty, &baud);
        return baud;
    }
    return 0;
}


void tty_set_parity(int tty, int mode)
{
   if (tty >= 0 && tty < 3) {
        uart_set_parity(tty, mode);
    }
}
