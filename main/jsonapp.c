#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

#include "vlftx.h"
#include "uartapp.h"
#include "cJSON.h"
#include "jsonapp.h"

static const char *TAG = "JSON";
extern vlf_config_t vlf_configuration;

char *app_json_serialize(void){
	ESP_LOGI(TAG, "Serialize.....");
	cJSON *root;
	root = cJSON_CreateObject();
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	cJSON_AddNumberToObject(root, "command", 1);
	cJSON_AddNumberToObject(root, "freq", 8000);
	cJSON_AddNumberToObject(root, "power", 100);
	cJSON_AddStringToObject(root, "modulation", "ASK"); //"FSK"
	cJSON_AddTrueToObject(root, "flag_on_air");
	// cJSON_AddFalseToObject(root, "flag_false");
	char *my_json_string = cJSON_Print(root);
	ESP_LOGI(TAG, "my_json_string\n%s",my_json_string);
	cJSON_Delete(root);
    return my_json_string;
}

void app_json_deserialize(char * arg){
	ESP_LOGI(TAG, "Deserialize.....");
	cJSON *root2 = cJSON_Parse(arg);
	if (root2 == NULL){
		ESP_LOGI(TAG, "invalid command");
		return;
	}
	if (cJSON_GetObjectItem(root2,"command") != NULL)
	{
		vlf_configuration.command = cJSON_GetObjectItem(root2,"command")->valueint;
		ESP_LOGI(TAG, "command=%d",vlf_configuration.command);

	}
	if (cJSON_GetObjectItem(root2,"freq") != NULL)
	{
		vlf_configuration.freq = cJSON_GetObjectItem(root2,"freq")->valueint;
		ESP_LOGI(TAG, "frequency=%d",vlf_configuration.freq);
	}
	if (cJSON_GetObjectItem(root2,"power") != NULL)
	{
		vlf_configuration.power = cJSON_GetObjectItem(root2,"power")->valueint;
		ESP_LOGI(TAG, "power=%d",vlf_configuration.power);
	}
	// if (cJSON_GetObjectItem(root2,"modulation") != NULL)
	// {
	// 	char *modulation = cJSON_GetObjectItem(root2,"modulation")->valuestring;
	// 	ESP_LOGI(TAG, "modulation=%s", modulation);
	// 	vlf_configuration.modulation = modulation;
	// }
	if (cJSON_GetObjectItem(root2,"flag_on_air") != NULL)
	{
		vlf_configuration.flag_on_air = cJSON_GetObjectItem(root2,"flag_on_air")->valueint;
		ESP_LOGI(TAG, "flag_on_air set=%d",(int) vlf_configuration.flag_on_air);
	}

	// bool flag_false = cJSON_GetObjectItem(root2,"flag_false")->valueint;
	
    //update PWMCHARACTERISTICS
    set_vlf_configuration(vlf_configuration);
    
    cJSON_Delete(root2);
}