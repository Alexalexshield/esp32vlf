/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/****************************************************************************
*
* BLE GATT server. It can send adv data, be connected by client.
* * Client demo will enable gatt_server's notify after connection. The two devices will then exchange
* data.
*
****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "sdkconfig.h"

#include "jsonapp.h"

#define GATTS_TAG "VLF_TX_TAG"

#define BLE_PROFILE_APP_ID 0

#define GATTS_SERVICE_UUID   0x0001
#define GATTS_CHAR_NUM		1
#define GATTS_NUM_HANDLE     1+(3*GATTS_CHAR_NUM)
#define BLE_MANUFACTURER_DATA_LEN  4
#define GATTS_CHAR_VAL_LEN_MAX		22


#define BLE_DEVICE_NAME            "VLF_TX"
#define TEST_MANUFACTURER_DATA_LEN  17

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40

#define PREPARE_BUF_MAX_SIZE 1024

#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)





// Tasks
void bleAdvt_task(void *pvParameters);          // Advertisement Test
void bleServer_task(void *pvParameters);        // GATT Server
// Functions
void ble_server_init();
void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
void char1_read_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param); 
void char1_write_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param); 
void descr1_read_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);



// void gatts_add_char();
// void gatts_check_add_char(esp_bt_uuid_t char_uuid, uint16_t attr_handle);
// void gatts_check_add_descr(esp_bt_uuid_t descr_uuid, uint16_t attr_handle);

// void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
// void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);