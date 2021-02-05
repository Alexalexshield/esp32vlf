#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

#include "cJSON.h"


//JSON CMD
//Objects:  command         8 bits
//          freq            100-36000 Hz
//          power           10-100 %
//          modulation      ask/fsk
//          flag_on_air     false/true


    //json de-serialization test
    // char *uartstr;
    // uartstr = (char*)malloc(90);
    // uartstr = app_json_serialize();
    //  {
    //         "command":      0,
    //         "freq": 8000,
    //         "modulation": "ASK",
    //         "flag_on_air": 1
    //  }
    // app_json_deserialize(uartstr);


char *app_json_serialize(void);
void app_json_deserialize(char *arg);