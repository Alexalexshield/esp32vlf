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


    // gpio_pad_select_gpio(GPIO_R_EN);
    // gpio_pad_select_gpio(GPIO_L_EN);
    // gpio_set_direction(GPIO_R_EN, GPIO_MODE_OUTPUT);
    // gpio_set_direction(GPIO_L_EN, GPIO_MODE_OUTPUT);
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;        //disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT;    //set as output mode
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;//bit mask of the pins that you want to set,e.g.GPIO16/19
    io_conf.pull_down_en = 1;     //enable pull-down mode
    io_conf.pull_up_en = 0;     //disable pull-up mode
    gpio_config(&io_conf);    //configure GPIO with the given settings

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
    pwm_config.frequency = 8000;    //frequency = 1000Hz
    pwm_config.cmpr_a = 50.0;       //duty cycle of PWMxA = 60.0%
    pwm_config.cmpr_b = 50.0;       //duty cycle of PWMxb = 50.0%
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_1;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);   //Configure PWM0A & PWM0B with above settings
    //4. deadtime configuration
    //comment if you don't want to use deadtime submodule
    //add rising edge delay or falling edge delay. There are 8 different types, each explained in mcpwm_deadtime_type_t in mcpwm.h
    mcpwm_deadtime_enable(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, 100, 100);   //Enable deadtime on PWM0A and PWM0B with red = (1000)*100ns on PWM0A

    vTaskDelay(300 / portTICK_PERIOD_MS);

    vTaskDelete(NULL);
}

void start_vlf_tx(void *arg){
    uint8_t message = (uint8_t*)arg;
    //uint8_t mask = 0b00000001;

    uint8_t mask[8] = {0,0,1,0,1,1,0,0};
    for (int i = 0; i < 8; i++){
        if (mask[i]==1)
        {
            printf("Send..1\n");
            xTaskCreate(vlf_tx_one, "vlf_tx_one", 4096, NULL, 5, NULL);
        }
        else
        {        
            printf("Send..0\n");
            xTaskCreate(vlf_tx_zero, "vlf_tx_zero", 4096, NULL, 5, NULL);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    // for (int i = 8; i >= 0; i--){
    //     mask = mask << i;
    //     if (message & mask)
    //     {
    //         mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
    //     }
    //     else
    //     {
    //         mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
    //     }
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
    vTaskDelete( NULL );
}
void vlf_tx_one(void *arg){
    gpio_set_level(GPIO_R_EN, 1);//mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
    gpio_set_level(GPIO_L_EN, 1);//mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
    vTaskDelete( NULL );
}
void vlf_tx_zero(void *arg){
    gpio_set_level(GPIO_R_EN, 0);//mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
    gpio_set_level(GPIO_L_EN, 0);//mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
    vTaskDelete( NULL );
}