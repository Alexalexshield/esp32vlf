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

#include "vlftx.h"
#include "bleserver.h"


void app_main()
{

    ble_server_init();

    printf("Testing MCPWM...\n");
    xTaskCreate(mcpwm_config, "mcpwm_config", 4096, NULL, 5, NULL);
    static const uint8_t *vlf_message = TEST_CODE;
    for(;;){
    xTaskCreate(start_vlf_tx, "start_vlf", 4096, (void*)vlf_message, 5, NULL);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    }

}

