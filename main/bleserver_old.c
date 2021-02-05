// /*
//    This example code is in the Public Domain (or CC0 licensed, at your option.)
//    Unless required by applicable law or agreed to in writing, this
//    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
//    CONDITIONS OF ANY KIND, either express or implied.
// */

// /****************************************************************************
// *
// * BLE GATT server. It can send adv data, be connected by client.
// * * Client demo will enable gatt_server's notify after connection. The two devices will then exchange
// * data.
// *
// ****************************************************************************/

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/event_groups.h"
// #include "esp_system.h"
// #include "esp_log.h"
// #include "nvs_flash.h"
// #include "esp_bt.h"

// #include "esp_gap_ble_api.h"
// #include "esp_gatts_api.h"
// #include "esp_bt_defs.h"
// #include "esp_bt_main.h"
// #include "esp_gatt_common_api.h"

// #include "sdkconfig.h"

// #include "bleserver.h"

// #include "uartapp.h"

// #include "jsonapp.h"
// //08-ca-23-16-85-0e-46-0a-9b-20-6b-bb-03-4e-d3-b3

//  void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
// {
//     switch (event) {

//     case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
//         adv_config_done &= (~adv_config_flag);
//         if (adv_config_done == 0){
//             esp_ble_gap_start_advertising(&adv_params);
//         }
//         break;
//     case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
//         adv_config_done &= (~scan_rsp_config_flag);
//         if (adv_config_done == 0){
//             esp_ble_gap_start_advertising(&adv_params);
//         }
//         break;
//     case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
//         //advertising start complete event to indicate advertising start successfully or failed
//         if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
//             ESP_LOGE(GATTS_TAG, "Advertising start failed\n");
//         }
//         break;
//     case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
//         if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
//             ESP_LOGE(GATTS_TAG, "Advertising stop failed\n");
//         } else {
//             ESP_LOGI(GATTS_TAG, "Stop adv successfully\n");
//         }
//         break;
//     case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
//          ESP_LOGI(GATTS_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
//                   param->update_conn_params.status,
//                   param->update_conn_params.min_int,
//                   param->update_conn_params.max_int,
//                   param->update_conn_params.conn_int,
//                   param->update_conn_params.latency,
//                   param->update_conn_params.timeout);

//         break;
//     default:
//         break;
//     }
// }

// void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
//     esp_gatt_status_t status = ESP_GATT_OK;
//     if (param->write.need_rsp){
//         if (param->write.is_prep){
//             if (prepare_write_env->prepare_buf == NULL) {
//                 prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE*sizeof(uint8_t));
//                 prepare_write_env->prepare_len = 0;
//                 if (prepare_write_env->prepare_buf == NULL) {
//                     ESP_LOGE(GATTS_TAG, "Gatt_server prep no mem\n");
//                     status = ESP_GATT_NO_RESOURCES;
//                 }
//             } else {
//                 if(param->write.offset > PREPARE_BUF_MAX_SIZE) {
//                     status = ESP_GATT_INVALID_OFFSET;
//                 } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
//                     status = ESP_GATT_INVALID_ATTR_LEN;
//                 }
//             }

//             esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
//             gatt_rsp->attr_value.len = param->write.len;
//             gatt_rsp->attr_value.handle = param->write.handle;
//             gatt_rsp->attr_value.offset = param->write.offset;
//             gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
//             memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
//             esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
//             if (response_err != ESP_OK){
//                ESP_LOGE(GATTS_TAG, "Send response error\n");
//             }
//             free(gatt_rsp);
//             if (status != ESP_GATT_OK){
//                 return;
//             }
//             memcpy(prepare_write_env->prepare_buf + param->write.offset,
//                    param->write.value,
//                    param->write.len);
//             prepare_write_env->prepare_len += param->write.len;

//         }else{
//             esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
//         }
//     }
// }

// void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
//     if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC){
//         esp_log_buffer_hex(GATTS_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
//     }else{
//         ESP_LOGI(GATTS_TAG,"ESP_GATT_PREP_WRITE_CANCEL");
//     }
//     if (prepare_write_env->prepare_buf) {
//         free(prepare_write_env->prepare_buf);
//         prepare_write_env->prepare_buf = NULL;
//     }
//     prepare_write_env->prepare_len = 0;
// }

//  void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
//     switch (event) {
//     case ESP_GATTS_REG_EVT:
//         ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
//         gl_profile_tab[PROFILE_A_APP_ID].service_id.is_primary = true;
//         gl_profile_tab[PROFILE_A_APP_ID].service_id.id.inst_id = 0x00;
//         gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
//         gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_A;
//         esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(BLE_DEVICE_NAME);
//         if (set_dev_name_ret){
//             ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
//         }

//         //config adv data
//         esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
//         if (ret){
//             ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x", ret);
//         }
//         adv_config_done |= adv_config_flag;
//         //config scan response data
//         ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
//         if (ret){
//             ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x", ret);
//         }
//         adv_config_done |= scan_rsp_config_flag;

//         esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_A_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_A);

//         break;
//     case ESP_GATTS_READ_EVT: {
//         ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
//         esp_gatt_rsp_t rsp;
//         memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
//         rsp.attr_value.handle = param->read.handle;
//         rsp.attr_value.len = 4;
//         rsp.attr_value.value[0] = 0xde;
//         rsp.attr_value.value[1] = 0xed;
//         rsp.attr_value.value[2] = 0xbe;
//         rsp.attr_value.value[3] = 0xef;
//         esp_ble_gatts_send_response(gatts_if,
//                                     param->read.conn_id, 
//                                     param->read.trans_id,
//                                     ESP_GATT_OK, &rsp);
//         break;
//     }
//     case ESP_GATTS_WRITE_EVT: {
//         ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
//         if (!param->write.is_prep){
//             ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
//             esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);

//         app_json_deserialize((char *)param->write.value);

// //ALIVE SENDING DATA THROUGH TUE BLE SERVER TO THE IOS BLE TERMINAL
//         esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
//                                                 strlen((const char*)param->write.value) + sizeof(""), param->write.value, false);
//         }

//         example_write_event_env(gatts_if, &a_prepare_write_env, param);
//         break;
//     }
//     case ESP_GATTS_EXEC_WRITE_EVT:
//         ESP_LOGI(GATTS_TAG,"ESP_GATTS_EXEC_WRITE_EVT");
//         esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
//         example_exec_write_event_env(&a_prepare_write_env, param);
//         break;
//     case ESP_GATTS_MTU_EVT:
//         ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
//         break;
//     case ESP_GATTS_UNREG_EVT:
//         break;
//     case ESP_GATTS_CREATE_EVT:
//         ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
//         gl_profile_tab[PROFILE_A_APP_ID].service_handle = param->create.service_handle;
//         gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
//         gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_A;

//         esp_ble_gatts_start_service(gl_profile_tab[PROFILE_A_APP_ID].service_handle);
//         a_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
//         esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_A_APP_ID].service_handle, &gl_profile_tab[PROFILE_A_APP_ID].char_uuid,
//                                                         ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
//                                                         a_property,
//                                                         &gatts_demo_char1_val, NULL);
//         if (add_char_ret){
//             ESP_LOGE(GATTS_TAG, "add char failed, error code =%x",add_char_ret);
//         }
//         break;
//     case ESP_GATTS_ADD_INCL_SRVC_EVT:
//         break;
//     case ESP_GATTS_ADD_CHAR_EVT: {
//         uint16_t length = 0;
//         const uint8_t *prf_char;

//         ESP_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
//                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
//         gl_profile_tab[PROFILE_A_APP_ID].char_handle = param->add_char.attr_handle;
//         gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
//         gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG; // vozmojno :ESP_GATT_UUID_CHAR_SRVR_CONFIG
//         esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle,  &length, &prf_char);
//         if (get_attr_ret == ESP_FAIL){
//             ESP_LOGE(GATTS_TAG, "ILLEGAL HANDLE");
//         }

//         ESP_LOGI(GATTS_TAG, "the gatts demo char length = %x\n", length);
//         for(int i = 0; i < length; i++){
//             ESP_LOGI(GATTS_TAG, "prf_char[%x] =%x\n",i,prf_char[i]);
//         }
//         esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_A_APP_ID].service_handle, &gl_profile_tab[PROFILE_A_APP_ID].descr_uuid,
//                                                                 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
//         if (add_descr_ret){
//             ESP_LOGE(GATTS_TAG, "add char descr failed, error code =%x", add_descr_ret);
//         }
//         break;
//     }
//     case ESP_GATTS_ADD_CHAR_DESCR_EVT:
//         gl_profile_tab[PROFILE_A_APP_ID].descr_handle = param->add_char_descr.attr_handle;
//         ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
//                  param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
//         break;
//     case ESP_GATTS_DELETE_EVT:
//         break;
//     case ESP_GATTS_START_EVT:
//         ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
//                  param->start.status, param->start.service_handle);
//         break;
//     case ESP_GATTS_STOP_EVT:
//         break;
//     case ESP_GATTS_CONNECT_EVT: {
//         esp_ble_conn_update_params_t conn_params = {0};
//         memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
//         /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
//         conn_params.latency = 0;
//         conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
//         conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
//         conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
//         ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
//                  param->connect.conn_id,
//                  param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
//                  param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
//         gl_profile_tab[PROFILE_A_APP_ID].conn_id = param->connect.conn_id;
//         //start sent the update connection parameters to the peer device.
//         esp_ble_gap_update_conn_params(&conn_params);

//         break;
//     }
//     case ESP_GATTS_DISCONNECT_EVT:
//         ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
//         esp_ble_gap_start_advertising(&adv_params);
//         break;
//     case ESP_GATTS_CONF_EVT:
//         ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
//         if (param->conf.status != ESP_GATT_OK){
//             esp_log_buffer_hex(GATTS_TAG, param->conf.value, param->conf.len);
//         }
//         break;
//     case ESP_GATTS_OPEN_EVT:
//     case ESP_GATTS_CANCEL_OPEN_EVT:
//     case ESP_GATTS_CLOSE_EVT:
//     case ESP_GATTS_LISTEN_EVT:
//     case ESP_GATTS_CONGEST_EVT:
//     default:
//         break;
//     }
// }


// void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
// {
//     /* If event is register event, store the gatts_if for each profile */
//     if (event == ESP_GATTS_REG_EVT) {
//         if (param->reg.status == ESP_GATT_OK) {
//             gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
//         } else {
//             ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d\n",
//                     param->reg.app_id,
//                     param->reg.status);
//             return;
//         }
//     }

//     /* If the gatts_if equal to profile A, call profile A cb handler,
//      * so here call each profile's callback */
//     do {
//         int idx;
//         for (idx = 0; idx < PROFILE_NUM; idx++) {
//             if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
//                     gatts_if == gl_profile_tab[idx].gatts_if) {
//                 if (gl_profile_tab[idx].gatts_cb) {
//                     gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
//                 }
//             }
//         }
//     } while (0);
// }

// void ble_server_init()
// {
//     esp_err_t ret;

//     // Initialize NVS.
//     ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK( ret );

//     ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

//     esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
//     ret = esp_bt_controller_init(&bt_cfg);
//     if (ret) {
//         ESP_LOGE(GATTS_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
//         return;
//     }

//     ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
//     if (ret) {
//         ESP_LOGE(GATTS_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
//         return;
//     }
//     ret = esp_bluedroid_init();
//     if (ret) {
//         ESP_LOGE(GATTS_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
//         return;
//     }
//     ret = esp_bluedroid_enable();
//     if (ret) {
//         ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
//         return;
//     }

//     ret = esp_ble_gatts_register_callback(gatts_event_handler);
//     if (ret){
//         ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
//         return;
//     }
//     ret = esp_ble_gap_register_callback(gap_event_handler);
//     if (ret){
//         ESP_LOGE(GATTS_TAG, "gap register error, error code = %x", ret);
//         return;
//     }
//     ret = esp_ble_gatts_app_register(PROFILE_A_APP_ID);
//     if (ret){
//         ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
//         return;
//     }
    
//     esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
//     if (local_mtu_ret){
//         ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
//     }

//    return;
// }