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

/**
 * set hardware H-bridge driver
 */
#define BTS7960


#define ALARM_CODE 0b0101100
#define TEST_CODE 0b01011011

#define GPIO_PWM0A_OUT 19   //Set GPIO 19 as PWM0A
#define GPIO_PWM0B_OUT 18   //Set GPIO 18 as PWM0B


#ifdef BTS7960
    #define GPIO_R_EN 16    //set enable signals
    #define GPIO_L_EN 17
    #define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_R_EN) | (1ULL<<GPIO_L_EN))
#endif

void mcpwm_gpio_initialize();

/**
 * @brief Configure whole MCPWM module
 */
void mcpwm_config(void *arg);

/**
 * @brief Main function to send vlf messages.
 */
void start_vlf_tx( void *arg);
void vlf_tx_one(void *arg);
void vlf_tx_zero(void *arg);
