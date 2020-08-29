/* MCPWM and GPIO setup for vlf signals transmition
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
#include "vlftx.h"

void mcpwm_gpio_initialize()
{
    printf("initializing mcpwm gpio...\n");
    mcpwm_pin_config_t pin_config = {
        .mcpwm0a_out_num = GPIO_PWM0A_OUT,
        .mcpwm0b_out_num = GPIO_PWM0B_OUT,
    };
    mcpwm_set_pin(MCPWM_UNIT_0, &pin_config);
}

/**
 * @brief Configure whole MCPWM module
 */
void mcpwm_config(void *arg)
{
    //1. mcpwm gpio initialization
    mcpwm_gpio_initialize();

    //2. initialize mcpwm configuration
    printf("Configuring Initial Parameters of mcpwm...\n");
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 1000;    //frequency = 1000Hz
    pwm_config.cmpr_a = 50.0;       //duty cycle of PWMxA = 60.0%
    pwm_config.cmpr_b = 50.0;       //duty cycle of PWMxb = 50.0%
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_1;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);   //Configure PWM0A & PWM0B with above settings
    //4. deadtime configuration
    //comment if you don't want to use deadtime submodule
    //add rising edge delay or falling edge delay. There are 8 different types, each explained in mcpwm_deadtime_type_t in mcpwm.h
    mcpwm_deadtime_enable(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, 0, 0);   //Enable deadtime on PWM0A and PWM0B with red = (1000)*100ns on PWM0A

   // vTaskDelete(NULL);
}

void start_vlf_tx(uint8_t message){
    uint8_t mask = 0b00000001;
    for (int i = 8; i >= 0; i--){
        mask = mask << i;
        if (message & mask)
        {
            mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
        }
        else
        {
            mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
void init_vlf_tx()
{
    printf("Testing MCPWM...\n");
    xTaskCreate(mcpwm_config, "mcpwm_example_config", 4096, NULL, 5, NULL);
}