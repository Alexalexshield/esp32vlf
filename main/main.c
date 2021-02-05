/* MCPWM basic config example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
 * This example will show you how to use each submodule of MCPWM unit.
 * The example can't be used without modifying the code first.
 * Edit the macros at the top of mcpwm_example_basic_config.c to enable/disable the submodules which are used in the example.
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

#include "jsonapp.h"
#include "vlftx.h"
#include "bleserver.h"
#include "uartapp.h"


vlf_config_t *ptr_vlf_configuration;
vlf_config_t vlf_configuration;


void app_main()
{
    uart_init();
    xTaskCreate(rx_task, "uart_rx_task", RX_BUF_SIZE*2, NULL, configMAX_PRIORITIES, NULL);

    printf("Testing MCPWM...\n");
    xTaskCreate(mcpwm_config, "mcpwm_config", 4096, NULL, 5, NULL);

    ble_server_init();

    ptr_vlf_configuration = &vlf_configuration;  
   
    for(;;){

        if (vlf_configuration.flag_on_air)
        {
            //xTaskCreate(start_vlf_tx, "start_vlf", 4096, (void*)vlf_configuration.freq, 5, NULL);
            xTaskCreate(start_vlf_tx, "start_vlf", 4096, NULL, 5, NULL);
            ptr_vlf_configuration->flag_on_air=false;
            vTaskDelay(1000 / portTICK_PERIOD_MS);

            
        // uint8_t temp_value[] = {2,4,5,7,8,9,12,14,124,22};
        // esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
        //                                         sizeof(temp_value), temp_value, false);
        
        }
        vTaskDelay(100); 
    }
}