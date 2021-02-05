#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

#include "vlftx.h"
#include "uartapp.h"
#include "cJSON.h"
#include "jsonapp.h"

extern vlf_config_t vlf_configuration;

void uart_init(void) {
    
    uart_config_t uart_config;
    uart_config.baud_rate = 115200;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.rx_flow_ctrl_thresh = 122;
    uart_config.use_ref_tick = true;

    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    QueueHandle_t uart_queue;
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, TX_BUF_SIZE * 2, 10, &uart_queue, 0);
}

int sendData(const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* uart_data = (uint8_t*) malloc(RX_BUF_SIZE+1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, uart_data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0) {
			uart_data[rxBytes] = 0;
			app_json_deserialize((char*) uart_data);
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, uart_data);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, uart_data, rxBytes, ESP_LOG_INFO);
            
            uart_flush(UART_NUM_1);
        }
    }
    
    free(uart_data);
}