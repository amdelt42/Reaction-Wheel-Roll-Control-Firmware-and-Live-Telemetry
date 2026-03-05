#include "pid.h"

static const char TAG[] = "PID";

static inline float clamp(float val, float lo, float hi) 
{ 
    if (val > hi) return hi; 
    if (val < lo) return lo; 
    return val; 
}

 pid_t *s_pid = NULL;  

void initialize_pid(pid_t *pid, float kp, float ki, float kd, float set_pnt, float out_min, float out_max)
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

    ESP_LOGI(TAG, "PID configuration complete");
}

// compute 
float pid_compute(pid_t *pid, float measurement, float dt) 
{ 
    if (dt <= 0.0f) return 0.0f; 
    float error = pid->setpoint - measurement; 

    // proportional 
    float p_term = pid->kp * error; 

    // derivative 
    float d_term = -pid->kd * (measurement - pid->prev_measurement) / dt; 
    pid->prev_measurement = measurement; 

    // anti-windup check
    float raw_output = p_term + pid->ki * pid->integral + d_term;

    // integral
    if (raw_output < pid->output_max && raw_output > pid->output_min) {
        pid->integral += error * dt;
        pid->integral = clamp(pid->integral, -pid->integral_max, pid->integral_max); 
    }

    float i_term = pid->ki * pid->integral; 
    float output = clamp(p_term + i_term + d_term, pid->output_min, pid->output_max); 

    ESP_LOGD(TAG, "e=%.4f P=%.4f I=%.4f D=%.4f -> out=%.4f", error, p_term, i_term, d_term, output); 
    return output; 
}

// reset
void pid_reset(pid_t *pid)
{
    pid->integral         = 0.0f;
    pid->prev_measurement = 0.0f;
    ESP_LOGD(TAG, "PID reset");
}
