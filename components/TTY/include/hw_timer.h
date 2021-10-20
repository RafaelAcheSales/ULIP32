#ifndef __HW_TIMER_H__
#define __HW_TIMER_H__
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/timer.h"
#include "esp_log.h"

#define DIV_CLK 2
#define US_TO_RTC_TIMER_TICKS(t)          \
    ((t) ?          \
     (((t) > 0x35A) ?            \
      (((t) >> 2) * ((APB_CLK_FREQ >> (DIV_CLK/2)) / 250000) + ((t)&0x3) * ((APB_CLK_FREQ >> (DIV_CLK/2)) / 1000000)) : \
      (((t) *(APB_CLK_FREQ>>(DIV_CLK/2))) / 1000000)) :    \
         0)

// void test();
// void hw_timer_init();

// void hw_timer_set_func(void (*user_hw_timer_cb_set)(void));

// void hw_timer_arm(unsigned int val, bool req);

// void hw_timer_disarm(void);

// #endif  /* __HW_TIMER_H__ */


 


typedef struct {
    int timer_group;
    int timer_idx;
    int alarm_interval;
    bool auto_reload;
} example_timer_info_t;


/**
 * @brief A sample structure to pass events from the timer ISR to task
 *
 */
typedef struct {
    example_timer_info_t info;
    uint64_t timer_counter_value;
} example_timer_event_t;




typedef enum {          // timer provided mode
    DIVDED_BY_1   = 1,  // timer clock
    DIVDED_BY_16  = 16,  // divided by 16
    DIVDED_BY_256 = 256,  // divided by 256
} TIMER_PREDIVED_MODE;

typedef enum {          // timer interrupt mode
    TM_LEVEL_INT = 1,   // level interrupt
    TM_EDGE_INT  = 0,   // edge interrupt
} TIMER_INT_MODE;

static void (* user_hw_timer_cb)(void) = NULL;
static void (* user_sw_timer_cb)(void) = NULL;
static const char * TAG = "hardware timer: ";
static xQueueHandle s_timer_queue;
bool frc1_auto_load = false;


static bool IRAM_ATTR hw_timer_isr_cb(void *args)
{
    //ESP_LOGI(TAG, "hw timer callback");

    BaseType_t high_task_awoken = pdFALSE;
    example_timer_info_t *info = (example_timer_info_t *) args;

    uint64_t timer_counter_value = timer_group_get_counter_value_in_isr(info->timer_group, info->timer_idx);

    // /* Prepare basic event data that will be then sent back to task */
    example_timer_event_t evt = {
        .info.timer_group = info->timer_group,
        .info.timer_idx = info->timer_idx,
        .info.auto_reload = info->auto_reload,
        .info.alarm_interval = info->alarm_interval,
        .timer_counter_value = timer_counter_value
    };

    // if (!info->auto_reload) {
    //     timer_counter_value += info->alarm_interval * TIMER_SCALE;
    //     timer_group_set_alarm_value_in_isr(info->timer_group, info->timer_idx, timer_counter_value);
    // }

    // /* Now just send the event data back to the main program task */
    xQueueSendFromISR(s_timer_queue, &evt, &high_task_awoken);
    //ESP_LOGI(TAG, "sent from ISR");

    return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
}


void timer_isr_task() {
    ESP_LOGI(TAG, "task running");
    
    s_timer_queue = xQueueCreate(10, sizeof(example_timer_event_t));
    //example_tg_timer_init(TIMER_GROUP_1, TIMER_0, false, 5);

    while (1) {
        example_timer_event_t evt;
        
        xQueueReceive(s_timer_queue, &evt, NULL);
        if (evt.timer_counter_value == NULL) {
            ets_printf("deleted task hwtimer\n");
            
            vTaskDelete(NULL);
        }
        switch (evt.info.timer_group)
        
        {
        case 0:
            if (user_hw_timer_cb != NULL) {
                (*(user_hw_timer_cb))();
            }
            break;
        case 1:
            if (user_sw_timer_cb != NULL) {
                (*(user_sw_timer_cb))();
            }
            break;
        default:
            printf("timer group not supported\n");
            break;
        }
        
    }
}

void hw_timer_disarm(int timer_group)
{
    
    timer_pause(timer_group, TIMER_0);
}


void hw_timer_arm(unsigned int val, bool auto_reload, int timer_group)
{

    timer_set_auto_reload(timer_group, TIMER_0, auto_reload);
    timer_set_alarm_value(timer_group, TIMER_0, US_TO_RTC_TIMER_TICKS(val));
    timer_start(timer_group, TIMER_0);
    
}


void hw_timer_set_func(void (* user_hw_timer_cb_set)(void))
{
    user_hw_timer_cb = user_hw_timer_cb_set;
}
void sw_timer_set_func(void (* user_sw_timer_cb_set)(void))
{
    user_sw_timer_cb = user_sw_timer_cb_set;
}

static void hw_timer_init(int timer_group)
{
    
    ESP_LOGI(TAG, "initializing timer...");
    timer_config_t config = {
        .divider = DIV_CLK,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = true,
    }; // default clock source is APB

    timer_init(timer_group, TIMER_0, &config);

    timer_enable_intr(timer_group, TIMER_0);


    example_timer_info_t *timer_info = calloc(1, sizeof(example_timer_info_t));
    timer_info->timer_group = timer_group;
    timer_info->timer_idx = TIMER_0;
    timer_info->auto_reload = true;
    timer_info->alarm_interval = 1;
    timer_isr_callback_add(timer_group, TIMER_0, hw_timer_isr_cb, timer_info, 0);
    
    ESP_LOGI(TAG, "callback added");
    static uint8_t ucParameterToPass;
    TaskHandle_t xHandle = NULL;

    // Create the task, storing the handle.  Note that the passed parameter ucParameterToPass
    // must exist for the lifetime of the task, so in this case is declared static.  If it was just an
    // an automatic stack variable it might no longer exist, or at least have been corrupted, by the time
    // the new task attempts to access it.
    ESP_LOGI(TAG, "creating task");
    xTaskCreate( timer_isr_task, "timer task", 8192, &ucParameterToPass, 1 | portPRIVILEGE_BIT , &xHandle );
    configASSERT( xHandle );
    ESP_LOGI(TAG, "task created");

 // Use the handle to delete the task.

}
void hw_timer_release() {
    timer_deinit(TIMER_GROUP_0, TIMER_0);
    // timer_deinit(TIMER_GROUP_1, TIMER_0);
    example_timer_info_t info = {
        .alarm_interval = NULL,
        .auto_reload = NULL,
        .timer_group = NULL,
        .timer_idx = NULL
    };
    example_timer_event_t evt;
    evt.info = info;
    evt.timer_counter_value = NULL;
    xQueueSend(s_timer_queue, &evt, NULL);

}


#endif