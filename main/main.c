/*
    Delvin Thiel
    Reaction Wheel Test PCB - Test Code
*/

// header files
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "nvs_flash.h"
#include "config.h"
#include "blink.h"
#include "twai.h"
#include "spi.h"
#include "imu.h"
#include "pid.h"
#include "wifi.h"
#include "statemachine.h"

static const char *TAG = "MAIN";
static pid_t roll_pid;

void app_main(void)
{
    
    ESP_LOGI(TAG, "Startup..");
    ESP_LOGI(TAG, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);  

    // NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // global shared resources
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // drivers
    initialize_blink();
    initialize_twai();
    initialize_spi();
    initialize_imu();
    initialize_pid(&roll_pid, PID_KP, PID_KI, PID_KD, PID_SETPOINT, -PID_OUTPUT_MAX, PID_OUTPUT_MAX);
    initialize_wifi();
    initialize_sm();

    // loop
    while (1) {
        vTaskDelay(portMAX_DELAY);
    }
}