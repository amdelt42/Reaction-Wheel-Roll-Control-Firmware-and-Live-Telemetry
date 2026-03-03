/*
    Delvin Thiel
    Reaction Wheel Test PCB - Test Code
*/

// header files
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "blink.h"
#include "twai.h"
#include "spi.h"
#include "imu.h"

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);  

    initialize_blink();

    initialize_twai();

    initialize_spi();

    initialize_imu();
    
    // loop
    while (1) {
        vTaskDelay(portMAX_DELAY);
    }
}


