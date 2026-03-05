#ifndef MQTT_H
#define MQTT_H

#include "config.h"
#include <mqtt_client.h>
#include <esp_log.h>

// MQTT
void initialize_mqtt(void);
void mqtt_publish(const char *topic, const char *payload);
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void log_error_if_nonzero(const char *message, int error_code);

#endif // MQTT_H


