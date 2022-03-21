#include <driver/gpio.h>
#include <rom/ets_sys.h>
#include <osapi.h>

#include "gpio_drv.h"

typedef struct gpio_intr {
    int intr;
    int gpio;
    int state;
    int flags;
    gpio_func_t func;
    void *user_data;
} gpio_intr_t;

static gpio_intr_t gpio_intr[GPIO_NUM_INTERRUPT] = {
    {
        .intr = 0,
        .gpio = -1,
        .state = GPIO_PIN_INTR_DISABLE,
        .flags = 0,
        .func = NULL,
        .user_data = NULL,
    },
    {
        .intr = 1,
        .gpio = -1,
        .state = GPIO_PIN_INTR_DISABLE,
        .flags = 0,
        .func = NULL,
        .user_data = NULL,
    },
    {
        .intr = 2,
        .gpio = -1,
        .state = GPIO_PIN_INTR_DISABLE,
        .flags = 0,
        .func = NULL,
        .user_data = NULL,
    },
    {
        .intr = 3,
        .gpio = -1,
        .state = GPIO_PIN_INTR_DISABLE,
        .flags = 0,
        .func = NULL,
        .user_data = NULL,
    },
    {
        .intr = 4,
        .gpio = -1,
        .state = GPIO_PIN_INTR_DISABLE,
        .flags = 0,
        .func = NULL,
        .user_data = NULL,
    }
};


IRAM_ATTR
static void gpio_interrupt_handler(void *arg)
{
    gpio_intr_t *p = (gpio_intr_t *)arg;

    /* Disable interrupt */
    if (p->flags & GPIO_INTR_DISABLED)
        gpio_intr_disable(p->gpio);
    if (p->func)
        p->func(p->intr, p->user_data);
}

int gpio_drv_init(void)
{
    gpio_install_isr_service(0);

    return 0;
}

void gpio_drv_release(void)
{
    gpio_intr_t *p;
    int i;
    
    for (i = 0; i < GPIO_NUM_INTERRUPT; i++) {
        p = &gpio_intr[i];
        if (p->gpio != -1)
            gpio_interrupt_close(i);
    }
    gpio_uninstall_isr_service();
}

int gpio_interrupt_open(int intr, int gpio, int state,
                        int flags, gpio_func_t func,
                        void *user_data)
{
    gpio_intr_t *p;

    if (intr >= GPIO_NUM_INTERRUPT) return -1;

    p = &gpio_intr[intr];
    p->gpio = gpio;
    p->state = state;
    p->flags = flags;
    p->func = func;
    p->user_data = user_data;

    gpio_set_intr_type(gpio, state);
    gpio_isr_handler_add(gpio, gpio_interrupt_handler, (void *)p);

    return 0;
}

int gpio_interrupt_close(int intr)
{
    gpio_intr_t *p;

    if (intr >= GPIO_NUM_INTERRUPT) return -1;

    p = &gpio_intr[intr];
    if (p->gpio == -1) return -1;

    gpio_isr_handler_remove(p->gpio);
    p->gpio = -1;
    p->flags = 0;
    p->func = NULL;
    p->user_data = NULL;
    

    return 0;
}

void gpio_interrupt_enable(int intr)
{
    gpio_intr_t *p;

    if (intr >= GPIO_NUM_INTERRUPT) return;

    p = &gpio_intr[intr];
    if (p->gpio == -1) return;
    gpio_intr_enable(p->gpio);
}

void gpio_interrupt_disable(int intr)
{
    gpio_intr_t *p;

    if (intr >= GPIO_NUM_INTERRUPT) return;

    p = &gpio_intr[intr];
    if (p->gpio == -1) return;
    gpio_intr_disable(p->gpio);
}
