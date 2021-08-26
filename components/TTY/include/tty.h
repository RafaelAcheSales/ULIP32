#ifndef __TTY_H__
#define __TTY_H__

#define TTY_NUM_DEV     4


typedef void (*tty_func_t)(int tty, const char *msg,
                           int len, void *user_data);

int tty_init(void);
void tty_release(void);
int tty_open(int tty, tty_func_t func, void *user_data);
int tty_close(int tty);
int tty_write(int tty, unsigned char *data, int len);
int tty_tx_fifo_size(int tty);
void tty_set_baudrate(int tty, int baudrate);
void tty_set_parity(int tty, int mode);

#endif  /* __TTY_H__ */
