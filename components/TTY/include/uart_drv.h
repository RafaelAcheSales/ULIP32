#ifndef __UART_DRV_H__
#define __UART_DRV_H__
/* UART Echo Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
typedef enum {
    BIT_RATE_300 = 300,
    BIT_RATE_600 = 600,
    BIT_RATE_1200 = 1200,
    BIT_RATE_2400 = 2400,
    BIT_RATE_4800 = 4800,
    BIT_RATE_9600   = 9600,
    BIT_RATE_19200  = 19200,
    BIT_RATE_38400  = 38400,
    BIT_RATE_57600  = 57600,
    BIT_RATE_74880  = 74880,
    BIT_RATE_115200 = 115200,
    BIT_RATE_230400 = 230400,
    BIT_RATE_460800 = 460800,
    BIT_RATE_921600 = 921600,
    BIT_RATE_1843200 = 1843200,
    BIT_RATE_3686400 = 3686400,
} UartBautRate;

typedef struct
{
    int baud_rate;
    int data_bits;
    int parity;
    int stop_bit;
    int flow_control;
    int source_clk;
    int uart_id;
    int tx_pin;
    int rx_pin;
    int cts;
    int rts;
}uart_dev;

uart_isr_handle_t handlers[4];
#define BUF_SIZE (2048)
#define QUEUE_SIZE 20

// }
static void uart_initialize(uart_dev* device)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = device->baud_rate,
        .data_bits = device->data_bits,
        .parity    = device->parity,
        .stop_bits = device->stop_bit,
        .flow_ctrl = device->flow_control,
        .source_clk = device->source_clk,
    };
    //int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_param_config(device->uart_id, &uart_config));
    ESP_LOGI("uart", "seted params");
    ESP_ERROR_CHECK(uart_set_pin(device->uart_id, device->tx_pin, device->rx_pin, device->rts, device->cts));
    ESP_LOGI("uart", "seted pins");

    // Configure a temporary buffer for the incoming data
}

void uart_open(int uart_n) {
    uart_driver_install(uart_n, BUF_SIZE, BUF_SIZE, QUEUE_SIZE, NULL, 0);
}
void uart_close(int uart_n) {
    // uart_isr_free(uart_n);
    if (uart_is_driver_installed(uart_n))
        uart_driver_delete(uart_n);
}
#endif

