/*
    Delvin Thiel
    Reaction Wheel Test PCB - Test Code
*/

// header files
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config.h"
#include "blink.h"
#include "twai.h"
#include "spi.h"
#include "imu.h"
#include "pid.h"

static pid_t roll_pid;

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);  

    initialize_blink();
    initialize_twai();
    initialize_spi();
    initialize_imu();
    initialize_pid(&roll_pid, PID_KP, PID_KI, PID_KD, PID_SETPOINT, -PID_OUTPUT_MAX, PID_OUTPUT_MAX);
    
    // loop
    while (1) {
        vTaskDelay(portMAX_DELAY);
    }
}