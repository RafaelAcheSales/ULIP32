#ifndef __RS485_H__
#define __RS485_H__
#include "stdbool.h"
#define PSH_DATA        1
#define ACK_DATA        2
#define RXMIT_DATA      4
#define TIMEOUT_DATA    8

#define RS485_CMD_POLLING       0x00
#define RS485_CMD_STATUS        0x01
#define RS485_CMD_RELAY         0x02
#define RS485_CMD_ALARM         0x03
#define RS485_CMD_PANIC         0x04
#define RS485_CMD_ADDUSER       0x05
#define RS485_CMD_DELUSER       0x06
#define RS485_CMD_PROBEUSER     0x07
#define RS485_CMD_ERASEUSER     0x08
#define RS485_CMD_ERASEALL      0x09
#define RS485_CMD_REBOOT        0x0A
#define RS485_CMD_GETCONFIG     0x0B
#define RS485_CMD_SETCONFIG     0x0C
#define RS485_CMD_REQUEST       0x0D
#define RS485_CMD_BEEP          0x0E
#define RS485_CMD_GETDATETIME   0x0F
#define RS485_CMD_SETDATETIME   0x10
#define RS485_CMD_TEMPERATURE   0x11
#define RS485_CMD_GAS           0x12
#define RS485_CMD_PIR           0x13
#define RS485_CMD_TELEMETRY     0x14
#define RS485_CMD_LEVEL         0x15
#define RS485_CMD_VOLUME        0x16
#define RS485_CMD_DELTELEMETRY  0x17
#define RS485_CMD_LUMINOSITY    0x18
#define RS485_CMD_POWER         0x19
#define RS485_CMD_UPDATE        0x20
#define RS485_CMD_LOOP          0x21

#define RS485_DOOR_REQUEST      1
#define RS485_CARD_REQUEST      2
#define RS485_QRCODE_REQUEST    3
#define RS485_RFCODE_REQUEST    4
#define RS485_PROBE_REQUEST     5
#define RS485_ERASE_REQUEST     6
#define RS485_USER_REQUEST      7

#define RS485_DOOR_RELAY        1
#define RS485_DOOR_SENSOR       2

#define RS485_CARD_EVENT        1
#define RS485_QRCODE_EVENT      2
#define RS485_RFCODE_EVENT      3

#define RS485_BLOCKED           0
#define RS485_GRANTED           1
#define RS485_PANIC             2
#define RS485_DETECTED          3

#define RS485_CONTROL_CLOSE     0
#define RS485_CONTROL_OPEN      1
#define RS485_CONTROL_HOLD      2

#define RS485_SENSOR_DISABLED   0
#define RS485_SENSOR_NORMAL     1
#define RS485_SENSOR_ALARM      2

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
