#ifndef __GPIO_DRV_H__
#define __GPIO_DRV_H__

#define GPIO_NUM_INTERRUPT  5

/* Flags */
#define GPIO_INTR_DISABLED  1

typedef void (*gpio_func_t)(int intr, void *user_data);

int gpio_drv_init(void);
void gpio_drv_release(void);

/* Interrupt */
int gpio_interrupt_open(int intr, int gpio,
                        int state, int flags,
                        gpio_func_t func,
                        void *user_data);
int gpio_interrupt_close(int intr);
void gpio_interrupt_enable(int intr);
void gpio_interrupt_disable(int intr);

#endif  /* __GPIO_DRV_H__ */
