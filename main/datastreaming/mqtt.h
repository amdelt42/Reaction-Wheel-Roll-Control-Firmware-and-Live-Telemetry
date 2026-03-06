#ifndef MQTT_H
#define MQTT_H

#include "config.h"
#include <mqtt_client.h>
#include <esp_log.h>
#include <string.h>
#include "cJSON.h"

// rocket/state values
typedef enum {
    ROCKET_STATE_IDLE     = 0,
    ROCKET_STATE_ARMED    = 1,
    ROCKET_STATE_LAUNCH   = 2,
    ROCKET_STATE_ABORT    = 3,
} rocket_state_t;

extern volatile rocket_state_t rocket_state;

// MQTT
void initialize_mqtt(void);
void mqtt_publish(const char *topic, const char *payload, int len);

#endif // MQTT_H