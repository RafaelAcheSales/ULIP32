/* General Purpose Timer example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/timer.h"
#include "driver/gpio.h"
// #define TEST_INTR              1
#define GPIO_OUTPUT            12
#define TIMER_DIVIDER         (2)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#ifdef TEST_INTR
static int count = 0;
#endif
typedef struct {
    int timer_group;
    int timer_idx;
    int alarm_interval;
    bool auto_reload;
} example_timer_info_t;
TaskHandle_t xHandle;
/**
 * @brief A sample structure to pass events from the timer ISR to task
 *
 */
typedef struct {
    example_timer_info_t info;
    uint64_t timer_counter_value;
} example_timer_event_t;

static xQueueHandle s_timer_queue;
static void (* user_hw_timer_cb)(void) = NULL;
/*
 * A simple helper function to print the raw timer counter value
 * and the counter value converted to seconds
 */

static void IRAM_ATTR hw_timer_task(void* arg) {
    while (1) {
        example_timer_event_t evt;
        if (xQueueReceive(s_timer_queue, &evt, portMAX_DELAY)) {
            if (evt.info.alarm_interval == -1) {
                vTaskDelete(NULL);
            }
            if (user_hw_timer_cb != NULL) {
                (*(user_hw_timer_cb))();
            }
    #ifdef TEST_INTR
            gpio_set_level(GPIO_OUTPUT, count & 1);
            count++;
    #endif
        }
    }
}
static bool IRAM_ATTR timer_group_isr_callback(void *args)
{
    BaseType_t high_task_awoken = pdFALSE;
    example_timer_info_t *info = (example_timer_info_t *) args;

    uint64_t timer_counter_value = timer_group_get_counter_value_in_isr(info->timer_group, info->timer_idx);

    /* Prepare basic event data that will be then sent back to task */
    example_timer_event_t evt = {
        .info.timer_group = info->timer_group,
        .info.timer_idx = info->timer_idx,
        .info.auto_reload = info->auto_reload,
        .info.alarm_interval = info->alarm_interval,
        .timer_counter_value = timer_counter_value
    };

    if (!info->auto_reload) {
        timer_counter_value += info->alarm_interval * TIMER_SCALE;
        timer_group_set_alarm_value_in_isr(info->timer_group, info->timer_idx, timer_counter_value);
    }

    /* Now just send the event data back to the main program task */
    xQueueSendFromISR(s_timer_queue, &evt, &high_task_awoken);

    return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
}

/**
 * @brief Initialize selected timer of timer group
 *
 * @param group Timer Group number, index from 0
 * @param timer timer ID, index from 0
 * @param auto_reload whether auto-reload on alarm event
 * @param timer_interval_sec interval of alarm
 */
static void example_tg_timer_init(int group, int timer, bool auto_reload, int timer_interval_usec)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = auto_reload,
    }; // default clock source is APB
    timer_init(group, timer, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(group, timer, 0);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(group, timer, timer_interval_usec * TIMER_SCALE/1000000);
    timer_enable_intr(group, timer);

    example_timer_info_t *timer_info = calloc(1, sizeof(example_timer_info_t));
    timer_info->timer_group = group;
    timer_info->timer_idx = timer;
    timer_info->auto_reload = auto_reload;
    timer_info->alarm_interval = timer_interval_usec;
    timer_isr_callback_add(group, timer, timer_group_isr_callback, timer_info, 0);

    // timer_start(group, timer);
}

void hw_timer_init(int us)
{
#ifdef TEST_INTR
    gpio_config_t config = {
        .intr_type = GPIO_PIN_INTR_DISABLE,
        .pull_down_en = 0,
        .pull_up_en = 0,
        .pin_bit_mask = (1ULL<<GPIO_OUTPUT),
        .mode = GPIO_MODE_OUTPUT
    };
    gpio_config(&config);
#endif
    s_timer_queue = xQueueCreate(10, sizeof(example_timer_event_t));

    example_tg_timer_init(TIMER_GROUP_0, TIMER_0, true, us);
    
    // example_tg_timer_init(TIMER_GROUP_1, TIMER_0, false, 5);
    xTaskCreatePinnedToCore(hw_timer_task, "hw_timer_task", 8192*2, NULL, 12, &xHandle, 1);
    
}
void  hw_timer_set_func(void (* user_hw_timer_cb_set)(void))
{
    user_hw_timer_cb = user_hw_timer_cb_set;
}
void hw_timer_disarm() {
    timer_pause(TIMER_GROUP_0, TIMER_0);
}
void hw_timer_arm() {
    timer_start(TIMER_GROUP_0, TIMER_0);
}
void hw_timer_release() {
    timer_deinit(TIMER_GROUP_0, TIMER_0);
    example_timer_info_t info = {
        .alarm_interval = -1,
        .auto_reload = false,
        .timer_group = -1,
        .timer_idx = -1
        
    };
    example_timer_event_t evt = {
        .info = info,
        .timer_counter_value = 0xffffffffffffffff
    };
    xQueueSend(s_timer_queue, &evt, portMAX_DELAY);
}
