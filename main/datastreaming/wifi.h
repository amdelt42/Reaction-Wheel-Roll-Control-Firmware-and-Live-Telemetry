#ifndef WIFI_H
#define WIFI_H

#include "config.h"
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>

// WIFI
void initialize_wifi(void);
static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data);

#endif // WIFI_H


