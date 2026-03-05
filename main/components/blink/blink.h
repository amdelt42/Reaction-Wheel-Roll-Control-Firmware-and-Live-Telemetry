#ifndef BLINK_H
#define BLINK_H

#include "config.h"
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void initialize_blink();
void blink_task();

#endif // BLINK_H