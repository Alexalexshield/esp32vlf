#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"


#include "jsonapp.h"


static const int RX_BUF_SIZE = 1024;
static const int TX_BUF_SIZE = 1024;

#define TXD_PIN (GPIO_NUM_4)
#define RXD_PIN (GPIO_NUM_5)

void uart_init(void);

int sendData(const char* logName, const char* data);

void rx_task(void *arg);
