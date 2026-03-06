#ifndef BLINK_H
#define BLINK_H

#include "config.h"
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "mqtt.h"

void initialize_blink(void);
extern void blink_task(void *arg);
extern void set_blink_pattern(rocket_state_t state);

#endif // BLINK_H