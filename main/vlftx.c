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

char ALARM_CODE[] = {'0','0','1','0','1','1','0','0',0x00};
char TEST_CODE[]  = "00101101";
char TEST_POWER[] = "11111111";
char QUICK_SEARCH[] = "101001011010111001010001";

extern vlf_config_t vlf_configuration;

void mcpwm_gpio_initialize()
{
    printf("initializing mcpwm gpio...\n");
    mcpwm_pin_config_t pin_config = {
        .mcpwm0a_out_num = GPIO_PWM0A_OUT,
        .mcpwm0b_out_num = GPIO_PWM0B_OUT,
    };
    mcpwm_set_pin(MCPWM_UNIT_0, &pin_config);
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
    pwm_config.cmpr_a = 50.0;       //duty cycle of PWMxA = 50.0%
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

void set_vlf_configuration(vlf_config_t vlf)
{
    mcpwm_config_t pwm_config;
    pwm_config.frequency = vlf.freq;    //frequency = 1000Hz
    pwm_config.cmpr_a = vlf.power;       //duty cycle of PWMxA 
    pwm_config.cmpr_b = vlf.power;       //duty cycle of PWMxb 
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_1;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);   //Configure PWM0A & PWM0B with above settings
    int power = 150; //100 ->100ns //vlf.power*100/vlf_freq; %complex equation to tune power!!!!!
    mcpwm_deadtime_enable(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, power, power);   //Enable deadtime on PWM0A and PWM0B with red = (1000)*100ns on PWM0A
}

void start_vlf_tx(void *arg){
    char *p = NULL;
    switch(vlf_configuration.command){
        case(ALARM): p = ALARM_CODE; break;
        case(TEST): p = TEST_CODE; break;
        case(POWER): p = TEST_POWER; break;
        case(QUICKSEARCH): p = QUICK_SEARCH; break;
        default: break;//ESP_LOGI("JSON", "unknown command\n"); 
    }

    if (p != NULL){
        for (uint8_t i = 0; i < strlen(p); i++){
            if (*(p+i)=='1')
            {
                printf("Send..1\n");
                xTaskCreate(vlf_tx_one_ask, "vlf_tx_one", 4096, NULL, 5, NULL);
            }
            else
            {        
                printf("Send..0\n");
                xTaskCreate(vlf_tx_zero_ask, "vlf_tx_zero", 4096, NULL, 5, NULL);
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
    xTaskCreate(vlf_tx_zero_ask, "vlf_tx_zero", 4096, NULL, 5, NULL);
    vTaskDelete( NULL );
}

void vlf_tx_one_ask(void *arg){
    gpio_set_level(GPIO_R_EN, 1);
    gpio_set_level(GPIO_L_EN, 1);
    vTaskDelete( NULL );
}
void vlf_tx_zero_ask(void *arg){
    gpio_set_level(GPIO_R_EN, 0);
    gpio_set_level(GPIO_L_EN, 0);
    vTaskDelete( NULL );
}