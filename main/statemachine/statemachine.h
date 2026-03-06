#ifndef SM_H
#define SM_H

#include "config.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

typedef enum {
    PID  = 0,
    TWAI = 1,
} subsystem_t;

extern QueueHandle_t sm_queue;

void initialize_sm(void);
void statemachine_task(void *arg);
void set_enabled(subsystem_t sys, bool en);

#endif // SM_H