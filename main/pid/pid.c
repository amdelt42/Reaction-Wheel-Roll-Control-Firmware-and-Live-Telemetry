#include "pid.h"
#include "twai.h"   // twai_send_motor_cmd() — adapt to your frame format

static const char TAG[] = "PID";

QueueHandle_t pid_queue = NULL;

static pid_t *s_pid = NULL;  

static inline float clamp(float val, float lo, float hi)
{
    if (val > hi) return hi;
    if (val < lo) return lo;
    return val;
}

void initialize_pid(pid_t *pid, float kp, float ki, float kd,
                    float set_pnt, float out_min, float out_max)
{
    ESP_LOGI(TAG, "Initializing PID");

    pid->kp  = kp;
    pid->ki  = ki;
    pid->kd  = kd;

    pid->integral         = 0.0f;
    pid->integral_max     = 50.0f;
    pid->prev_measurement = 0.0f;

    pid->setpoint   = set_pnt;
    pid->output_min = out_min;
    pid->output_max = out_max;

    s_pid = pid;

    ESP_LOGD(TAG, "Kp=%.3f Ki=%.3f Kd=%.3f SP=%.3f", kp, ki, kd, set_pnt);

    pid_queue = xQueueCreate(4, sizeof(pid_msg_t));

    ESP_LOGD(TAG, "Starting PID task");

    ESP_LOGI(TAG, "PID configuration complete");
}

// reset
void pid_reset(pid_t *pid)
{
    pid->integral         = 0.0f;
    pid->prev_measurement = 0.0f;
    ESP_LOGD(TAG, "PID reset");
}