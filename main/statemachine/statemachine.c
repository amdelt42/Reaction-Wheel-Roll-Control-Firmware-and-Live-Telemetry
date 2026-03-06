#include "statemachine.h"
#include "mqtt.h"
#include "pid.h"
#include "imu.h"
#include "blink.h"

static const char TAG[] = "SM";
static rocket_state_t last_state = 255; //invalid state for init
QueueHandle_t sm_queue = NULL;

void initialize_sm(void)
{
    ESP_LOGI(TAG, "Initializing SM");
    sm_queue = xQueueCreate(5, sizeof(rocket_state_t));
    
    rocket_state_t init_state = ROCKET_STATE_IDLE;
    xQueueSend(sm_queue, &init_state, 0);  
    
    xTaskCreate(statemachine_task, "sm_task", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "SM configuration complete");
}

void statemachine_task(void *arg)
{   
    while (1)
    {   
        rocket_state_t state;
        xQueueReceive(sm_queue, &state, portMAX_DELAY);

        if (last_state == ROCKET_STATE_ABORT) 
        {
            ESP_LOGW("ABORT", "RESET required", state);
            continue;
        }
        
        if (state != last_state) {
            //ESP_LOGI(TAG, "State change: %d → %d", last_state, state);
            last_state = state;

            switch (state)
            {
            case ROCKET_STATE_IDLE:
                // PID off, IMU off, motor idle
                ESP_LOGI(TAG, "IDLE state triggered");
                set_enabled(PID, false);
                set_enabled(TWAI, false);
                imu_stop();
                set_blink_pattern(state);
                break;

            case ROCKET_STATE_ARMED:
                // PID on, IMU on, motor idle
                ESP_LOGI(TAG, "ARMED state triggered");
                set_enabled(PID, true);
                set_enabled(TWAI, false);
                imu_start();
                set_blink_pattern(state);
                break;

            case ROCKET_STATE_LAUNCH:
                // PID on, IMU on, motor active
                ESP_LOGI(TAG, "LAUNCH state triggered");
                set_enabled(PID, true);
                set_enabled(TWAI, true);
                set_blink_pattern(state);
                break;

            case ROCKET_STATE_ABORT:
                // PID off, IMU off, motor idle (+trigger on MQTT disconnect)
                ESP_LOGI(TAG, "ABORT state triggered");
                set_enabled(PID, false);
                set_enabled(TWAI, false);
                imu_stop();
                set_blink_pattern(state);
                break;
            }
        }
    }
}

void set_enabled(subsystem_t sys, bool en)
{
    switch (sys)
    {
    case PID:
        pid_enabled = en;
        ESP_LOGD(TAG, "PID %s", en ? "enabled" : "disabled");
        break;
    case TWAI:
        twai_enabled = en;
        ESP_LOGD(TAG, "TWAI %s", en ? "enabled" : "disabled");
        break;
    }
}