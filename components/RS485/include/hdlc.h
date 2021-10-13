/*
 * HDLC module
 */

#if !defined(__HDLC_H__)
#define __HDLC_H__
#include "stdbool.h"
#define HDLC_MAX_SIZE(x)  (((x + 4) * 10) / 8)

#define HDLC_MAXFRAME_LEN_RX  512
#define HDLC_MAXFRAME_LEN_TX  512

typedef void (*hdlc_frame_handler_t)(void *user_data, unsigned char ok, 
                                     unsigned char *pkt, 
                                     unsigned short len);
typedef void (*hdlc_underflow_handler_t)(void *user_data);

typedef struct
{
    unsigned long bit_buf;
    unsigned long byte_in_progress;
    
    hdlc_frame_handler_t frame_handler;
    void *user_data;
    
    unsigned short len;
    unsigned char flag_seen;
    unsigned char num_bits;	    
    unsigned char buffer[HDLC_MAXFRAME_LEN_RX + 2];

} hdlc_rx_state_t;


typedef struct
{
    unsigned long idle_byte;
    unsigned short len;
    unsigned short pos;
    unsigned char num_bits;
    unsigned char buffer[HDLC_MAXFRAME_LEN_TX + 2];
    unsigned char bits;

} hdlc_tx_state_t;


unsigned short crc_itu16_calc(unsigned char *buf, unsigned short len, 
                              unsigned short crc);

unsigned short crc_itu16_append(unsigned char *buf, unsigned short len);

unsigned char crc_itu16_check(unsigned char *buf, unsigned short len);


void hdlc_rx_init(hdlc_rx_state_t *s,
                  hdlc_frame_handler_t handler,
                  void *user_data);
void hdlc_rx_byte(hdlc_rx_state_t *s, unsigned char new_byte);


void hdlc_tx_init(hdlc_tx_state_t *s);

void hdlc_tx_frame(hdlc_tx_state_t *s, unsigned char *frame, 
                   unsigned short len);

unsigned char hdlc_tx_getbyte(hdlc_tx_state_t *s);

#endif  /* __HDLC_H__ */

