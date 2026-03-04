#ifndef PID_H
#define PID_H

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

typedef struct {
    float measurement;  // rad/s
    float dt;           // s
} pid_msg_t;

typedef struct {
    float kp;
    float ki;
    float kd;

    float integral;
    float integral_max;      // symmetric clamp: ±integral_max
    float prev_measurement;  // derivative on measurement, not error

    float setpoint;

    float output_min;
    float output_max;
} pid_t;

extern pid_t *s_pid;

void  initialize_pid(pid_t *pid, float kp, float ki, float kd, float set_pnt, float out_min, float out_max);
float pid_compute(pid_t *pid, float measurement, float dt);
void  pid_reset(pid_t *pid);  
#endif // PID_H