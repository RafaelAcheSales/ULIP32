#ifndef __QRCODE_H__
#define __QRCODE_H__

#define QRCODE_EVT_CHECK    1
#define QRCODE_EVT_QR       2
#define QRCODE_EVT_PANIC    3

typedef int (*qrcode_handler_t)(int event, const char *data,
                                int len, void *user_data);

int qrcode_init(bool led, bool led_alarm, int timeout,
                int panic_timeout, bool dynamic,
                int validity, qrcode_handler_t func,
                void *user_data, int tty);
void qrcode_release(void);
void qrcode_module_initialize(int stage);
bool qrcode_get_dynamic(void);
int qrcode_get_validity(void);

#endif  /* __QRCODE_H__ */
