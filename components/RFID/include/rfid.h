#ifndef __RFID_H__
#define __RFID_H__
#include "stdbool.h"
#define RFID_EVT_CHECK      1
#define RFID_EVT_MIFARE     2
#define RFID_EVT_NFC        3
#define RFID_EVT_PANIC      4

#define RFID_BE_FORMAT      0
#define RFID_LE_FORMAT      1

typedef int (*rfid_handler_t)(int event, const char *data,
                              int len, void *user_data);


int rfid_init(int timeout, int retries, bool nfc, int panic_timeout,
              int format, rfid_handler_t func, void *user_data);
 void rfid_release(void);

#endif  /* __RFID_H__ */
