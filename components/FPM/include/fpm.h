#ifndef __FPM_H__
#define __FPM_H__

#define FPM_EVT_BLOCKED     1
#define FPM_EVT_GRANTED     2
#define FPM_EVT_ENROLL      3

#define FPM_ERR_NONE        0
#define FPM_ERR_IDENTIFY    1
#define FPM_ERR_ENROLL      2
#define FPM_ERR_DUPLICATED  3
#define FPM_ERR_TIMEOUT     4
#include <stdint.h>
typedef void (*fpm_handler_t)(int event, int index,
                              uint8_t *data, int len,
                              int error, void *user_data);

int fpm_init(int timeout, int security, int identify_retries,
             fpm_handler_t func, void *user_data);
void fpm_release(void);

int fpm_set_enroll(uint16_t index);
int fpm_get_enroll(void);
int fpm_cancel_enroll(void);

int fpm_set_template(uint16_t index, uint8_t *data);
int fpm_delete_template(uint16_t index);
int fpm_delete_all(void);
bool fpm_is_busy(void);

#endif  /* __FPM_H__ */
