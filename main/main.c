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
    #if BUILD_MODE == BUILD_DEBUG
    esp_log_level_set("IMU", ESP_LOG_DEBUG);
    #elif BUILD_MODE == BUILD_TEST
        esp_log_level_set("IMU", ESP_LOG_INFO);
    #else
        esp_log_level_set("IMU", ESP_LOG_WARN);
    #endif

    initialize_blink();

    initialize_twai();

    initialize_spi();

    initialize_imu();
    
    // loop
    while (1) {
        imu_print_gyro_once();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
