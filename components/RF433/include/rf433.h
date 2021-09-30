#ifndef __RF433_H__
#define __RF433_H__
#include "stdbool.h"
#include "stdint.h"
#define RF433_EVT_CHECK    1
#define RF433_EVT_CODE     2
#define RF433_EVT_PANIC    3

typedef int (*rf433_handler_t)(int event, const char *data, int len,
                               uint16_t sync, uint8_t button,
                               uint8_t status, void *user_data);

int rf433_init(bool rolling_code, bool button_code,
               int panic_timeout, rf433_handler_t func,
               void *user_data);
void rf433_release(void);

#endif  /* __RF433_H__ */
