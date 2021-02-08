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

#include "bleserver.h"

#include "uartapp.h"

#include "jsonapp.h"

static uint32_t ble_add_char_pos;
uint8_t adv_config_done = 0;

#define BLE_SERVICE_UUID_SIZE ESP_UUID_LEN_128
// Add more UUIDs for more then one Service
uint8_t ble_service_uuid128[BLE_SERVICE_UUID_SIZE] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
	// 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E,
    // 0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xAB, 0xCD, 0x00, 0x00,
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x0A, 0x18, 0x00, 0x00,
    //0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

uint8_t adv_service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00,
    //second uuid, 32bit, [12], [13], [14], [15] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

uint8_t ble_manufacturer[BLE_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};


// The length of adv data must be less than 31 bytes
//static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
//adv data
esp_ble_adv_data_t ble_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = BLE_MANUFACTURER_DATA_LEN, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = (uint8_t *)ble_manufacturer, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
// scan response data
esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    //.min_interval = 0x0006,
    //.max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};



esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

uint8_t char1_str[] = {0x11,0x22,0x33};

esp_attr_value_t gatts_demo_char1_val =
{
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len     = sizeof(char1_str),
    .attr_value   = char1_str,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;


struct gatts_profile_inst gl_profile = {
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
};

struct gatts_char_inst {
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t char_perm;
	esp_gatt_char_prop_t char_property;
	esp_attr_value_t *char_val;
    esp_attr_control_t *char_control;
    uint16_t char_handle;
    esp_gatts_cb_t char_read_callback;
	esp_gatts_cb_t char_write_callback;
    esp_bt_uuid_t descr_uuid;
    esp_gatt_perm_t descr_perm;
	esp_attr_value_t *descr_val;
    esp_attr_control_t *descr_control;
    uint16_t descr_handle;
    esp_gatts_cb_t descr_read_callback;
	esp_gatts_cb_t descr_write_callback;
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
struct gatts_char_inst gl_char[GATTS_CHAR_NUM] = {
		{
				.char_uuid.len = ESP_UUID_LEN_128, // RX
				.char_uuid.uuid.uuid128 =  { 0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xAB, 0xCD, 0x00, 0x00 },
				.char_perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
				.char_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY,
				.char_val = &gatts_demo_char1_val,
				.char_control = NULL,
				.char_handle = 0,
				.char_read_callback=char1_read_handler,
				.char_write_callback=char1_write_handler
		}
};





 void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {

    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done == 0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done == 0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTS_TAG, "Advertising start failed\n");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTS_TAG, "Advertising stop failed\n");
        } else {
            ESP_LOGI(GATTS_TAG, "Stop adv successfully\n");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(GATTS_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);

        break;
    default:
        break;
    }
}

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
void gatts_add_char() {

	ESP_LOGI(GATTS_TAG, "gatts_add_char %d\n", GATTS_CHAR_NUM);
	for (uint32_t pos=0;pos<GATTS_CHAR_NUM;pos++) {
		if (gl_char[pos].char_handle==0) {
			ESP_LOGI(GATTS_TAG, "ADD pos %d handle %d service %d\n", pos,gl_char[pos].char_handle,gl_profile.service_handle);
			ble_add_char_pos=pos;
			esp_ble_gatts_add_char(gl_profile.service_handle, &gl_char[pos].char_uuid,
								   gl_char[pos].char_perm,gl_char[pos].char_property,gl_char[pos].char_val, gl_char[pos].char_control);
			break;
		}
	}
}

void gatts_check_add_char(esp_bt_uuid_t char_uuid, uint16_t attr_handle) {

	ESP_LOGI(GATTS_TAG, "gatts_check_add_char %d\n", attr_handle);
	if (attr_handle != 0) {
		if (char_uuid.len == ESP_UUID_LEN_16) {
			ESP_LOGI(GATTS_TAG, "Char UUID16: %x", char_uuid.uuid.uuid16);
		} else if (char_uuid.len == ESP_UUID_LEN_32) {
			ESP_LOGI(GATTS_TAG, "Char UUID32: %x", char_uuid.uuid.uuid32);
		} else if (char_uuid.len == ESP_UUID_LEN_128) {
			ESP_LOGI(GATTS_TAG, "Char UUID128: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", char_uuid.uuid.uuid128[0],
					 char_uuid.uuid.uuid128[1], char_uuid.uuid.uuid128[2], char_uuid.uuid.uuid128[3],
					 char_uuid.uuid.uuid128[4], char_uuid.uuid.uuid128[5], char_uuid.uuid.uuid128[6],
					 char_uuid.uuid.uuid128[7], char_uuid.uuid.uuid128[8], char_uuid.uuid.uuid128[9],
					 char_uuid.uuid.uuid128[10], char_uuid.uuid.uuid128[11], char_uuid.uuid.uuid128[12],
					 char_uuid.uuid.uuid128[13], char_uuid.uuid.uuid128[14], char_uuid.uuid.uuid128[15]);
		} else {
			ESP_LOGE(GATTS_TAG, "Char UNKNOWN LEN %d\n", char_uuid.len);
		}

		ESP_LOGI(GATTS_TAG, "FOUND Char pos %d handle %d\n", ble_add_char_pos,attr_handle);
		gl_char[ble_add_char_pos].char_handle=attr_handle;

		// is there a descriptor to add ?
		if (gl_char[ble_add_char_pos].descr_uuid.len!=0 && gl_char[ble_add_char_pos].descr_handle==0) {
			ESP_LOGI(GATTS_TAG, "ADD Descr pos %d handle %d service %d\n", ble_add_char_pos,gl_char[ble_add_char_pos].descr_handle,gl_profile.service_handle);
			esp_ble_gatts_add_char_descr(gl_profile.service_handle, &gl_char[ble_add_char_pos].descr_uuid,
					gl_char[ble_add_char_pos].descr_perm, gl_char[ble_add_char_pos].descr_val, gl_char[ble_add_char_pos].descr_control);
		} else {
			gatts_add_char();
		}
	}
}

void gatts_check_add_descr(esp_bt_uuid_t descr_uuid, uint16_t attr_handle) {

	ESP_LOGI(GATTS_TAG, "gatts_check_add_descr %d\n", attr_handle);
	if (attr_handle != 0) {
		if (descr_uuid.len == ESP_UUID_LEN_16) {
			ESP_LOGI(GATTS_TAG, "Char UUID16: %x", descr_uuid.uuid.uuid16);
		} else if (descr_uuid.len == ESP_UUID_LEN_32) {
			ESP_LOGI(GATTS_TAG, "Char UUID32: %x", descr_uuid.uuid.uuid32);
		} else if (descr_uuid.len == ESP_UUID_LEN_128) {
			ESP_LOGI(GATTS_TAG, "Char UUID128: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", descr_uuid.uuid.uuid128[0],
					 descr_uuid.uuid.uuid128[1], descr_uuid.uuid.uuid128[2], descr_uuid.uuid.uuid128[3],
					 descr_uuid.uuid.uuid128[4], descr_uuid.uuid.uuid128[5], descr_uuid.uuid.uuid128[6],
					 descr_uuid.uuid.uuid128[7], descr_uuid.uuid.uuid128[8], descr_uuid.uuid.uuid128[9],
					 descr_uuid.uuid.uuid128[10], descr_uuid.uuid.uuid128[11], descr_uuid.uuid.uuid128[12],
					 descr_uuid.uuid.uuid128[13], descr_uuid.uuid.uuid128[14], descr_uuid.uuid.uuid128[15]);
		} else {
			ESP_LOGE(GATTS_TAG, "Descriptor UNKNOWN LEN %d\n", descr_uuid.len);
		}
		ESP_LOGI(GATTS_TAG, "FOUND Descriptor pos %d handle %d\n", ble_add_char_pos,attr_handle);
		gl_char[ble_add_char_pos].descr_handle=attr_handle;
	}
	gatts_add_char();
}



void char1_read_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
	ESP_LOGI(GATTS_TAG, "char1_read_handler %d\n", param->read.handle);

	esp_gatt_rsp_t rsp;
	memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
	rsp.attr_value.handle = param->read.handle;

	if (gl_char[0].char_val!=NULL) {
		ESP_LOGI(GATTS_TAG, "char1_read_handler char_val %d\n",gl_char[0].char_val->attr_len);
		rsp.attr_value.len = gl_char[0].char_val->attr_len;
		for (uint32_t pos=0;pos<gl_char[0].char_val->attr_len&&pos<gl_char[0].char_val->attr_max_len;pos++) {
			printf("gl_char: %c\n", gl_char[0].char_val->attr_value[pos]);
			rsp.attr_value.value[pos] = gl_char[0].char_val->attr_value[pos];
		}
	}
	ESP_LOGI(GATTS_TAG, "char1_read_handler esp_gatt_rsp_t\n");
	esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
								ESP_GATT_OK, &rsp);
}

void char1_write_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
	ESP_LOGI(GATTS_TAG, "char1_write_handler %d\n", param->write.handle);

	if (gl_char[0].char_val!=NULL) {
		ESP_LOGI(GATTS_TAG, "char1_write_handler char_val %d\n",param->write.len);
		gl_char[0].char_val->attr_len = param->write.len;
		for (uint32_t pos=0;pos<param->write.len;pos++) {
			gl_char[0].char_val->attr_value[pos]=param->write.value[pos];
		}
		ESP_LOGI(GATTS_TAG, "char1_write_handler %.*s", gl_char[0].char_val->attr_len, (char*)gl_char[0].char_val->attr_value);
	}
}
 void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    esp_err_t ret;
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d", param->reg.status, param->reg.app_id);
        gl_profile.service_id.is_primary = true;
        gl_profile.service_id.id.inst_id = 0x00;
        gl_profile.service_id.id.uuid.len = ESP_UUID_LEN_128;
        for (uint8_t pos=0;pos<ESP_UUID_LEN_128;pos++) {
        	gl_profile.service_id.id.uuid.uuid.uuid128[pos]=ble_service_uuid128[pos];
        }

        esp_ble_gap_set_device_name(BLE_DEVICE_NAME);
        ret=esp_ble_gap_config_adv_data(&ble_adv_data);
        ESP_LOGI(GATTS_TAG, "esp_ble_gap_config_adv_data %d", ret);

        esp_ble_gatts_create_service(gatts_if, &gl_profile.service_id, GATTS_NUM_HANDLE);
                break;
    case ESP_GATTS_READ_EVT: {
        ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        
        esp_gatt_rsp_t rsp;
        esp_ble_gatts_send_response(gatts_if,
                                    param->read.conn_id, 
                                    param->read.trans_id,
                                    ESP_GATT_OK, &rsp);

        //esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile.char_handle,
                                                // strlen((const char*)param->write.value) + sizeof(""), param->write.value, false);

        gatts_check_callback(event, gatts_if, param);
        break;
    }
    case ESP_GATTS_WRITE_EVT: {
        ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
        
        esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);

        // esp_gatt_rsp_t rsp;
        // esp_ble_gatts_send_response(gatts_if,
        //                             param->read.conn_id, 
        //                             param->read.trans_id,
        //                             ESP_GATT_OK, &rsp);


       // app_json_deserialize((char *)param->write.value);
//ALIVE SENDING DATA THROUGH TUE BLE SERVER TO THE IOS BLE TERMINAL
        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile.char_handle,
                                                strlen((const char*)param->write.value) + sizeof(""), param->write.value, false);
        
        
        gatts_check_callback(event, gatts_if, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
    case ESP_GATTS_MTU_EVT:
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gl_profile.service_handle = param->create.service_handle;
        gl_profile.char_uuid.len = gl_char[0].char_uuid.len;
        //gl_profile.char_uuid.uuid.uuid16 = gl_char[0].char_uuid.uuid.uuid16;

        for (uint8_t pos=0;pos<ESP_UUID_LEN_128;pos++) {
        	gl_profile.char_uuid.uuid.uuid128[pos] = gl_char[0].char_uuid.uuid.uuid128[pos];
        }

        esp_ble_gatts_start_service(gl_profile.service_handle);
        gatts_add_char();

        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
        ESP_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status 0x%X,  attr_handle %d, service_handle %d\n",
        param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        gl_profile.char_handle = param->add_char.attr_handle;     
        if (param->add_char.status==ESP_GATT_OK) {
        	gatts_check_add_char(param->add_char.char_uuid,param->add_char.attr_handle);
        }
        break;
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT char, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT desc, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        if (param->add_char_descr.status==ESP_GATT_OK) {
        	gatts_check_add_descr(param->add_char.char_uuid,param->add_char.attr_handle);
        }
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x: \n",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gl_profile.conn_id = param->connect.conn_id;
        break;
        // esp_ble_conn_update_params_t conn_params = {0};
        // memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        // /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        // conn_params.latency = 0;
        // conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
        // conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
        // conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
        // ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
        //          param->connect.conn_id,
        //          param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
        //          param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        // gl_profile_tab[PROFILE_A_APP_ID].conn_id = param->connect.conn_id;
        // //start sent the update connection parameters to the peer device.
        // esp_ble_gap_update_conn_params(&conn_params);
        // break;
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK){
            esp_log_buffer_hex(GATTS_TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}


void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile.gatts_if = gatts_if;
        } else {
            ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d\n",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    gatts_profile_event_handler(event, gatts_if, param);
}

void ble_server_init()
{
    esp_err_t ret;

    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ESP_LOGE(GATTS_TAG, "gap register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gatts_app_register(BLE_PROFILE_APP_ID);
    if (ret){
        ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
        return;
    }
    
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }
    return;
}