#ifndef __RS485_H__
#define __RS485_H__
#include "stdbool.h"
#define PSH_DATA        1
#define ACK_DATA        2
#define RXMIT_DATA      4
#define TIMEOUT_DATA    8

typedef void (*func_rs485_t)(unsigned char from_addr,
                             unsigned char ok, unsigned char control,
                             unsigned char *frame, 
                             unsigned short len, 
                             void *user_data);


int rs485_init(int pinout, unsigned char hwaddr,
               unsigned char xmit_retries,
               unsigned short xmit_timeout,
               func_rs485_t func, void *user_data);

void rs485_release(void);


int rs485_tx_frame(unsigned char hw_addr_dest,
                   unsigned char *frame,
                   unsigned short len);
                   
#endif /* __RS485_H__ */
