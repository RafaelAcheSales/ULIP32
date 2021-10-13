#include "tty.h"
#include "hdlc.h"
#include "rs485.h"
#include "esp_timer.h"
#include "string.h"
#include "driver/gpio.h"

#define MAX_BUFFER_SIZE_RX      512
#define MAX_BUFFER_SIZE_TX      512
#define MAX_FRAME_SIZE          (MAX_BUFFER_SIZE_TX -  sizeof(rs485_header_t))

#define RS485_TTY               1
#define RS485_EN_PIN            4
#define GPIO_OUTPUT_MASK   1ULL<<RS485_EN_PIN


/* Alternative pinout */
#define RS485_EN_PIN_ALT        1


#define RS485_BROADCAST_ADDR    0xFF
#define MAX_SEQ_NUMBER          0x3

#define ACK_DELAY               10000

#define ARP_PROTO_SIZE          2
#define ARP_PROTO_CONTROL       0xF
#define ARP_REQUEST             0x01
#define ARP_RESPONSE            0x02
#define ARP_DELAY               20000

#define ENABLE_TIMEOUT          5000

#define CIRC_SPACE(tail,head,size) \
    (((tail) > (head)) ? ((tail) - (head) - 1) : (size - ((head) - (tail)) - 1))
#define CIRC_CNT(tail,head,size)  (size - CIRC_SPACE(tail,head,size) - 1)

typedef struct __attribute__ ((packed)) rs485_header_s {
    unsigned char hw_addr_dest;
    unsigned char hw_addr_org;
    union {
        struct {
            unsigned char raw_len;
            unsigned char ack_seq:2;
            unsigned char xmit_seq:2;
            unsigned char control:4;
        } tpl_data;
        unsigned char data[2];
    } tpl_hdr;
} rs485_header_t;
  
typedef struct rs485_s {
    hdlc_tx_state_t hdlc_tx;
    hdlc_rx_state_t hdlc_rx;
    
    int pinout; 
    void *user_data;
    func_rs485_t func;
    unsigned short xmit_tail, xmit_head;
    unsigned short xmit_timeout;

    unsigned char xmit_buf[MAX_BUFFER_SIZE_TX];
    unsigned char xmit_retries;
    unsigned char curr_xmit_retries;
    unsigned char xmit_seq;
    esp_timer_handle_t xmit_timer;
    esp_timer_handle_t ack_delay_timer;

    char curr_recv_addr;
    char curr_recv_seq;
    char hw_addr;
    char curr_to_addr;

    esp_timer_handle_t arp_delay_timer;

    esp_timer_handle_t xmit_enable_timer;

} rs485_t;

static rs485_t rs485_dev;

static void hdlc_rx_frame(void *user_data, unsigned char ok, 
                          unsigned char *frame, 
                          unsigned short len);



static void rs485_xmit_disable(void)
{
    rs485_t *p = &rs485_dev;
    ESP_ERROR_CHECK(gpio_set_level(RS485_EN_PIN, 0));
    // if (!p->pinout)
    //     GPIO_OUTPUT_SET(RS485_EN_PIN, 0);
    // else
    //     GPIO_OUTPUT_SET(RS485_EN_PIN_ALT, 0);
    ESP_ERROR_CHECK(esp_timer_stop(p->xmit_enable_timer));
    // timer_disarm(&p->xmit_enable_timer);
}


static void rs485_xmit_enable(void)
{
    rs485_t *p = &rs485_dev;
    ESP_ERROR_CHECK(gpio_set_level(RS485_EN_PIN, 1));
    // if (!p->pinout)
    //     GPIO_OUTPUT_SET(RS485_EN_PIN, 1);
    // else
    //     GPIO_OUTPUT_SET(RS485_EN_PIN_ALT, 1);
    ESP_ERROR_CHECK(esp_timer_start_periodic(p->xmit_enable_timer, ENABLE_TIMEOUT));
    // timer_arm(&p->xmit_enable_timer, ENABLE_TIMEOUT, true);
}


static void rs485_xmit_enable_timeout(void *data)
{
    // rs485_t *p = &rs485_dev;

    /* Check fifo */
    if (!tty_tx_fifo_size(RS485_TTY))
        rs485_xmit_disable();
}


static void rs485_stop_retransmitions(void)
{
    rs485_t *p = &rs485_dev;

    if (!p->xmit_retries) return;

    ESP_ERROR_CHECK(esp_timer_stop(p->xmit_timer));
    // timer_disarm(&p->xmit_timer);
    p->curr_xmit_retries = 0;
}


static void rs485_start_retransmitions(void)
{
    rs485_t *p = &rs485_dev;

    if (!p->xmit_retries) return;

    ESP_ERROR_CHECK(esp_timer_stop(p->xmit_timer));
    ESP_ERROR_CHECK(esp_timer_start_once(p->xmit_timer, p->xmit_timeout));
    // timer_disarm(&p->xmit_timer);
    // timer_arm(&p->xmit_timer, p->xmit_timeout, false);
    p->curr_xmit_retries = 0;
}


static int rs485_xmit_start(void)
{
    rs485_t *p = &rs485_dev;
    rs485_header_t *hdr;

    while (p->xmit_tail != p->xmit_head)
    {
        hdr = (rs485_header_t *)&p->xmit_buf[p->xmit_tail];
        if (!hdr->tpl_hdr.tpl_data.raw_len) {
            p->xmit_tail = 0;
            continue;
        }

        hdr->tpl_hdr.tpl_data.control |= ACK_DATA;
        hdr->tpl_hdr.tpl_data.ack_seq = p->curr_recv_seq;

        hdlc_tx_frame(&p->hdlc_tx, &p->xmit_buf[p->xmit_tail], 
                      hdr->tpl_hdr.tpl_data.raw_len);
       
        /* Enable transmitter */ 
        rs485_xmit_enable();              
        tty_write(RS485_TTY, p->hdlc_tx.buffer, p->hdlc_tx.len);
            
        p->hdlc_tx.len = 0;
        hdr->tpl_hdr.tpl_data.control &= ~ACK_DATA;
 
        rs485_start_retransmitions();
        
        return true;
    }
    return false;
}


static void rs485_xmit_timeout(void *data)
{
    rs485_t *p = &rs485_dev;
    rs485_header_t *hdr;
    unsigned short len;

    hdr = (rs485_header_t *)&p->xmit_buf[p->xmit_tail];
    if (p->curr_xmit_retries++ >= p->xmit_retries) {
        if (p->func) {
            len = hdr->tpl_hdr.tpl_data.raw_len - \
                            sizeof(rs485_header_t);
            p->func(hdr->hw_addr_org, true, TIMEOUT_DATA,
                    (unsigned char *)(hdr + 1),
                    len, p->user_data);
        }
        p->xmit_tail += hdr->tpl_hdr.tpl_data.raw_len;
        p->curr_xmit_retries = 0;
    }
    else {
#if 0   
        hdlc_tx_frame(&p->hdlc_tx, &p->xmit_buf[p->xmit_tail], 
                      hdr->tpl_hdr.tpl_data.raw_len);
        uart_tx_data((unsigned char)tty, p->hdlc_tx.buffer, p->hdlc_tx.len);
        p->hdlc_tx.len = 0;
#endif
        ESP_ERROR_CHECK(esp_timer_start_once(p->xmit_timer, p->xmit_timeout));
        // timer_arm(&p->xmit_timer, p->xmit_timeout, false);
    }
}


static int rs485_send_ack(void)
{
    unsigned char buf[sizeof(rs485_header_t)];
    rs485_t *p = &rs485_dev;
    rs485_header_t *hdr;
    int retval = 0;
    unsigned short len;

    if (!rs485_xmit_start())
    {
        hdr = (rs485_header_t *)buf;
        hdr->hw_addr_org = p->hw_addr;
        hdr->hw_addr_dest = p->curr_recv_addr;
        len = sizeof(rs485_header_t);
        hdr->tpl_hdr.tpl_data.xmit_seq = p->xmit_seq;
        hdr->tpl_hdr.tpl_data.ack_seq = p->curr_recv_seq;
        hdr->tpl_hdr.tpl_data.control = ACK_DATA;

        /* Acknowledge retransmition */
        if (!(hdr->tpl_hdr.tpl_data.control & RXMIT_DATA)) {
            ++p->xmit_seq;
            p->xmit_seq = p->xmit_seq & MAX_SEQ_NUMBER;
        }

        hdr->tpl_hdr.tpl_data.raw_len = len;
        hdlc_tx_frame(&p->hdlc_tx, buf, len);
        /* Enable transmitter */ 
        rs485_xmit_enable();              
        if (tty_write(RS485_TTY, p->hdlc_tx.buffer, p->hdlc_tx.len))
            retval = -1;
        p->hdlc_tx.len = 0;
    }

    return retval;
}


static void rs485_ack_delay_timeout(void *data)
{
    rs485_send_ack();
}


static int rs485_receive_ack(rs485_header_t *pkt_hdr)
{
    rs485_t *p = &rs485_dev;
    rs485_header_t *hdr;

    while (p->xmit_tail != p->xmit_head)
    {
        hdr = (rs485_header_t *)&p->xmit_buf[p->xmit_tail];
        if (hdr->tpl_hdr.tpl_data.raw_len == 0) {
            p->xmit_tail = 0;
            continue;
        }

        if (hdr->hw_addr_dest != pkt_hdr->hw_addr_org ||
            hdr->tpl_hdr.tpl_data.xmit_seq != pkt_hdr->tpl_hdr.tpl_data.ack_seq)
            return -1;

        p->xmit_tail += hdr->tpl_hdr.tpl_data.raw_len;
        rs485_stop_retransmitions();
        return 0;
    }

    return -1;
}


static void rs485_rx_data(int tty, const char *data,
                          int len, void *user_data)
{
    rs485_t *p = (rs485_t *)&rs485_dev;
    int i;
    
    if (!len) return;

    for (i = 0; i < len; i++)
        hdlc_rx_byte(&p->hdlc_rx, data[i]);
    if (p->curr_to_addr != p->hw_addr) {
        hdlc_rx_init(&p->hdlc_rx, hdlc_rx_frame, (void *)p);
        p->curr_recv_addr = -1;
        p->curr_recv_seq = -1;
        p->curr_to_addr = p->hw_addr;
    }
}


static void rs485_send_arp_response(void *data)
{
    rs485_t *p = &rs485_dev;
    unsigned char buf[sizeof(rs485_header_t) + ARP_PROTO_SIZE];
    rs485_header_t *hdr;
    unsigned char *arp_proto;

    hdr = (rs485_header_t *)buf;
    hdr->hw_addr_dest = p->curr_recv_addr;
    hdr->hw_addr_org = p->hw_addr;
    hdr->tpl_hdr.tpl_data.xmit_seq = 0;
    hdr->tpl_hdr.tpl_data.ack_seq = 0;
    hdr->tpl_hdr.tpl_data.control = ARP_PROTO_CONTROL;
    arp_proto = (unsigned char *)(hdr + 1);
    *arp_proto++ = ARP_RESPONSE;
    *arp_proto++ = p->hw_addr;
    hdlc_tx_frame(&p->hdlc_tx, buf, sizeof(rs485_header_t) + ARP_PROTO_SIZE);
    /* Enable transmitter */ 
    rs485_xmit_enable();              
    tty_write(RS485_TTY, p->hdlc_tx.buffer, p->hdlc_tx.len);
    p->hdlc_tx.len = 0;

}


static void hdlc_rx_frame(void *user_data, unsigned char ok, 
                          unsigned char *frame, 
                          unsigned short len)
{
    rs485_header_t *hdr = (rs485_header_t *)frame;
    rs485_t *p = (rs485_t *)user_data;

    /* Invalid frame size */
    if (len < sizeof(rs485_header_t))
        return;

    if (ok)
    {
        /* endereco destino do pacote recebido da RS485 */
        p->curr_to_addr = hdr->hw_addr_dest;
         
        /* Discard unwanted frames */
        if ((p->hw_addr != hdr->hw_addr_dest) &&
            (hdr->hw_addr_dest != RS485_BROADCAST_ADDR))
            return;

        if (hdr->tpl_hdr.tpl_data.control == ARP_PROTO_CONTROL) {
            unsigned char *arp_proto = (unsigned char *)(hdr + 1);
            /* Send arp response */
            if (arp_proto[0] == ARP_REQUEST)
            {
                ESP_ERROR_CHECK(esp_timer_stop(p->arp_delay_timer));
                ESP_ERROR_CHECK(esp_timer_start_once(p->arp_delay_timer, ARP_DELAY));
                // timer_disarm(&p->arp_delay_timer);
                // timer_arm(&p->arp_delay_timer, ARP_DELAY, false);
                p->curr_recv_addr = hdr->hw_addr_org;
                return;
            }
        }

        if (hdr->tpl_hdr.tpl_data.control) {
            /* Notify retransmitions */
            if (p->curr_recv_addr != -1 && p->curr_recv_seq != -1) {
                if (p->curr_recv_addr == hdr->hw_addr_org &&
                    p->curr_recv_seq == hdr->tpl_hdr.tpl_data.xmit_seq) {
                    hdr->tpl_hdr.tpl_data.control |= RXMIT_DATA;
                }
            }
            p->curr_recv_addr = hdr->hw_addr_org;
            p->curr_recv_seq = hdr->tpl_hdr.tpl_data.xmit_seq;

            /* Receive acknowledge */
            if (hdr->tpl_hdr.tpl_data.control & ACK_DATA) {
                if (rs485_receive_ack(hdr)) {
                    return;
                }
            }
            
            if (hdr->tpl_hdr.tpl_data.control & PSH_DATA) {
                /* Send acknowledge */
                ESP_ERROR_CHECK(esp_timer_stop(p->ack_delay_timer));
                ESP_ERROR_CHECK(esp_timer_start_once(p->ack_delay_timer, ACK_DELAY));
                // timer_disarm(&p->ack_delay_timer);
                // timer_arm(&p->ack_delay_timer, ACK_DELAY, false);
            }
        }
    }

    if (p->func)
        p->func(hdr->hw_addr_org, ok, hdr->tpl_hdr.tpl_data.control, 
                (unsigned char *)(hdr + 1), len - sizeof(rs485_header_t), 
                p->user_data);
}


int rs485_init(int pinout, unsigned char hw_addr,
               unsigned char xmit_retries,
               unsigned short xmit_timeout,
               func_rs485_t func, void *user_data)
{
    rs485_t *p;

    p = &rs485_dev;

    if (tty_open(RS485_TTY, rs485_rx_data, NULL))
        return -1;
    
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_MASK;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    ESP_ERROR_CHECK(gpio_set_level(RS485_EN_PIN, 0));
    // if (!pinout) {
    //     PIN_FUNC_SELECT(RS485_EN_MUX, RS485_EN_FUNC);
    //     GPIO_OUTPUT_SET(RS485_EN_PIN, 0);
    // } else {
    //     PIN_FUNC_SELECT(RS485_EN_MUX_ALT, RS485_EN_FUNC_ALT);
    //     GPIO_OUTPUT_SET(RS485_EN_PIN_ALT, 0);
    // }

    hdlc_rx_init(&p->hdlc_rx, hdlc_rx_frame, (void *)p);
    hdlc_tx_init(&p->hdlc_tx);

    p->pinout = pinout;
    p->hw_addr = hw_addr;
    p->func = func;
    p->user_data = user_data;
    p->curr_xmit_retries = 0;
    p->xmit_tail = p->xmit_head = 0;
    p->xmit_retries = xmit_retries;
    p->xmit_timeout = xmit_timeout;
    p->xmit_seq = 0;

    p->curr_recv_addr = -1;
    p->curr_recv_seq = -1;
    p->curr_to_addr = hw_addr;

    const esp_timer_create_args_t xmit_timer_args = {
            .callback = &rs485_xmit_timeout,
            .name = "rs485_xmit_timeout"
    };
    const esp_timer_create_args_t ack_delay_timer_args = {
            .callback = &rs485_ack_delay_timeout,
            .name = "rs485_ack_delay_timeout"
    };
    const esp_timer_create_args_t arp_delay_timer_args = {
            .callback = &rs485_send_arp_response,
            .name = "rs485_send_arp_response"
    };
    const esp_timer_create_args_t xmit_enable_timer_args = {
            .callback = &rs485_xmit_enable_timeout,
            .name = "rs485_xmit_enable_timeout"
    };
    
    ESP_ERROR_CHECK(esp_timer_create(&xmit_timer_args, &p->xmit_timer));
    ESP_ERROR_CHECK(esp_timer_create(&ack_delay_timer_args, &p->ack_delay_timer));
    ESP_ERROR_CHECK(esp_timer_create(&arp_delay_timer_args, &p->arp_delay_timer));
    ESP_ERROR_CHECK(esp_timer_create(&xmit_enable_timer_args, &p->xmit_enable_timer));
    // timer_setfn(&p->xmit_timer,
    //                (timer_func_t *)rs485_xmit_timeout, NULL);
    // timer_setfn(&p->ack_delay_timer,
    //                (timer_func_t *)rs485_ack_delay_timeout, NULL);
    // timer_setfn(&p->arp_delay_timer,
    //                (timer_func_t *)rs485_send_arp_response, NULL);
    // timer_setfn(&p->xmit_enable_timer,
    //                (timer_func_t *)rs485_xmit_enable_timeout, NULL);

    return 0;
}


void rs485_release(void)
{
    rs485_t *p = &rs485_dev;
    ESP_ERROR_CHECK(esp_timer_stop(p->xmit_timer));
    ESP_ERROR_CHECK(esp_timer_stop(p->ack_delay_timer));
    ESP_ERROR_CHECK(esp_timer_stop(p->ack_delay_timer));
    ESP_ERROR_CHECK(esp_timer_stop(p->xmit_enable_timer));
    ESP_ERROR_CHECK(esp_timer_delete(p->xmit_timer));
    ESP_ERROR_CHECK(esp_timer_delete(p->ack_delay_timer));
    ESP_ERROR_CHECK(esp_timer_delete(p->ack_delay_timer));
    ESP_ERROR_CHECK(esp_timer_delete(p->xmit_enable_timer));
    // timer_disarm(&p->xmit_timer);
    // timer_disarm(&p->ack_delay_timer);
    // timer_disarm(&p->arp_delay_timer);
    // timer_disarm(&p->xmit_enable_timer);
    tty_close(RS485_TTY);
}


int rs485_tx_frame(unsigned char hw_addr_dest,
                   unsigned char *frame,
                   unsigned short len)
{
    rs485_header_t *hdr;
    rs485_t *p;
    unsigned short size;

    if (!frame || !len || len > MAX_FRAME_SIZE) return -1;

    p = &rs485_dev;

    while ((size = CIRC_SPACE(p->xmit_tail, p->xmit_head, MAX_BUFFER_SIZE_TX)))
    {
        if ((size < len + 2 * sizeof(rs485_header_t)) ||
            (HDLC_MAX_SIZE(len + sizeof(rs485_header_t)) > HDLC_MAXFRAME_LEN_TX))  
        {
            return -1;
        }
        if (p->xmit_head + len + 2 * sizeof(rs485_header_t) > MAX_BUFFER_SIZE_TX) {
            hdr = (rs485_header_t *)&p->xmit_buf[p->xmit_head];
            hdr->tpl_hdr.tpl_data.raw_len = 0;
            p->xmit_head = 0;
            continue;
        }
        break;
    }

    hdr = (rs485_header_t *)&p->xmit_buf[p->xmit_head];
    hdr->hw_addr_org = p->hw_addr;
    hdr->hw_addr_dest = hw_addr_dest;
    memcpy(&p->xmit_buf[p->xmit_head] + sizeof(rs485_header_t), frame, len);
    len += sizeof(rs485_header_t);
    hdr->tpl_hdr.tpl_data.control = PSH_DATA;
    hdr->tpl_hdr.tpl_data.xmit_seq = p->xmit_seq;
    hdr->tpl_hdr.tpl_data.raw_len = len;
    ++p->xmit_seq;
    p->xmit_seq = p->xmit_seq & MAX_SEQ_NUMBER;
    p->xmit_head += len;

    return 0;
}
