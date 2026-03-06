#include "mqtt.h"
#include "pid.h"
#include "statemachine.h"

static const char *TAG = "MQTT";
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;

volatile rocket_state_t rocket_state = ROCKET_STATE_IDLE;

// inbound handlers
static void handle_pid_set(const char *data, int len)
{
    cJSON *root = cJSON_ParseWithLength(data, len);
    if (!root) {
        ESP_LOGW(TAG, "pid_set: invalid JSON");
        return;
    }

    cJSON *kp_j = cJSON_GetObjectItem(root, "kp");
    cJSON *ki_j = cJSON_GetObjectItem(root, "ki");
    cJSON *kd_j = cJSON_GetObjectItem(root, "kd");
    cJSON *sp_j = cJSON_GetObjectItem(root, "sp");

    if (s_pid != NULL          &&
        cJSON_IsNumber(kp_j)   &&
        cJSON_IsNumber(ki_j)   &&
        cJSON_IsNumber(kd_j)   &&
        cJSON_IsNumber(sp_j))
    {
        s_pid->kp       = (float)kp_j->valuedouble;
        s_pid->ki       = (float)ki_j->valuedouble;
        s_pid->kd       = (float)kd_j->valuedouble;
        s_pid->setpoint = (float)sp_j->valuedouble;
        pid_reset(s_pid);

        ESP_LOGI(TAG, "PID updated: Kp=%.3f Ki=%.4f Kd=%.3f SP=%.2f",
                 s_pid->kp, s_pid->ki, s_pid->kd, s_pid->setpoint);
    }
    else
    {
        ESP_LOGW(TAG, "pid_set: missing or non-numeric fields (s_pid=%s)",
                 s_pid ? "ok" : "NULL");
    }

    cJSON_Delete(root);
}
static void handle_state_set(const char *data, int len)
{
    char buf[16] = {0};
    int  copy    = (len < 15) ? len : 15;
    memcpy(buf, data, copy);

    rocket_state_t new_state;

    if      (strncmp(buf, "ARMED",    5) == 0) new_state = ROCKET_STATE_ARMED;
    else if (strncmp(buf, "LAUNCH",   6) == 0) new_state = ROCKET_STATE_LAUNCH;
    else if (strncmp(buf, "ABORT",    5) == 0) new_state = ROCKET_STATE_ABORT;
    else                                       new_state = ROCKET_STATE_IDLE;

    rocket_state = new_state;

    if (sm_queue != NULL)
    xQueueSend(sm_queue, &new_state, 0);

    const char *names[] = {"IDLE", "ARMED", "LAUNCH", "ABORT", "RECOVERY"};
    ESP_LOGI(TAG, "State → %s", names[new_state]);
}

// event handler
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected");
        mqtt_connected = true;

        esp_mqtt_client_subscribe(mqtt_client, "rocket/pid_set", 1);
        esp_mqtt_client_subscribe(mqtt_client, "rocket/state",   1);
        ESP_LOGI(TAG, "Subscribed to rocket/pid_set and rocket/state");
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT disconnected");
        mqtt_connected = false;
        break;

    case MQTT_EVENT_DATA:
        // route by topic
        if (strncmp(event->topic, "rocket/pid_set",
                    event->topic_len) == 0)
        {
            handle_pid_set(event->data, event->data_len);
        }
        else if (strncmp(event->topic, "rocket/state",
                         event->topic_len) == 0)
        {
            handle_state_set(event->data, event->data_len);
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error");
        break;

    default:
        break;
    }
}

// mqtt api
void initialize_mqtt(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://10.14.204.35:1883",  
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return;
    }

    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void mqtt_publish(const char *topic, const char *payload, int len)
{
    if (mqtt_connected && mqtt_client != NULL) {
        esp_mqtt_client_publish(mqtt_client, topic, payload, len, 0, 0);
    }
}