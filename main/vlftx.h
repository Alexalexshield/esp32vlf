/* MCPWM setup for vlf signals transmition
header file
*/

#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_attr.h"
#include "soc/rtc.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

#define ALARM_CODE 0b00101100

#define GPIO_PWM0A_OUT 19   //Set GPIO 19 as PWM0A
#define GPIO_PWM0B_OUT 18   //Set GPIO 18 as PWM0B

void mcpwm_gpio_initialize();

/**
 * @brief Configure whole MCPWM module
 */
void mcpwm_config(void *arg);

/**
 * @brief Main function to send vlf messages.
 */
void start_vlf_tx(uint8_t message);

/**
 * @brief PWM and GPIO initialization
 */
void init_vlf_tx();