#include "twai.h"
#include "mqtt.h"

static const char TAG[] = "TWAI";

QueueHandle_t twai_show_queue;
twai_node_handle_t node_hdl;

void initialize_twai()
{
    ESP_LOGI(TAG, "Initializing TWAI");

    node_hdl = NULL;
    twai_onchip_node_config_t node_config = {
        .io_cfg.tx = CANTX_PIN,
        .io_cfg.rx = CANRX_PIN,
        .bit_timing.bitrate = 250000,
        .tx_queue_depth = 5,
    };

    // Create a queue for printing received messages
    twai_show_queue = xQueueCreate(10, sizeof(CANMessage)); 
    xTaskCreate(received_message_task, "ReceivedMessageTask", 4096, NULL, 1, NULL);

    // Create a new TWAI controller driver instance
    ESP_LOGD(TAG, "Creating TWAI Node");
    ESP_ERROR_CHECK(twai_new_node_onchip(&node_config, &node_hdl));

    twai_event_callbacks_t user_cbs = {
        .on_rx_done = twai_rx_cb,
    };
    ESP_ERROR_CHECK(twai_node_register_event_callbacks(node_hdl, &user_cbs, NULL));

    // Start the TWAI controller
    ESP_LOGD(TAG, "Starting TWAI Controller");
    ESP_ERROR_CHECK(twai_node_enable(node_hdl));

    ESP_LOGI(TAG, "TWAI configuration complete");
}

bool twai_rx_cb(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx)
{
    uint8_t recv_buff[8];
    twai_frame_t rx_frame = {
        .buffer = recv_buff,
        .buffer_len = sizeof(recv_buff),
    };
    if (ESP_OK == twai_node_receive_from_isr(handle, &rx_frame)) {
        CANMessage msg;
        msg.CANMessageID = rx_frame.header.id;
        memcpy(msg.CANData, rx_frame.buffer, rx_frame.buffer_len);
        xQueueSendFromISR(twai_show_queue, &msg, NULL);
    }
    return false;
}

void received_message_task(void *arg){
    while (1) {
        CANMessage msg;
        if (xQueueReceive(twai_show_queue, &msg, portMAX_DELAY) != pdTRUE) continue;

        char payload[64];
        int len = 0;

        if (msg.CANMessageID == ODRIVE_HEARTBEAT) {
            uint32_t error;
            uint8_t  state;
            memcpy(&error, &msg.CANData[0], 4);
            memcpy(&state, &msg.CANData[4], 1);
            len = snprintf(payload, sizeof(payload),
                "{\"error\":%lu,\"state\":%u}", error, state);
            mqtt_publish("rocket/odrive/heartbeat", payload, len);

        } else if (msg.CANMessageID == ODRIVE_ENCODER) {
            float vel;
            memcpy(&vel, &msg.CANData[4], 4); 
            float rpm = vel * 60.0f;
            len = snprintf(payload, sizeof(payload), "{\"rpm\":%.1f}", rpm);
            mqtt_publish("rocket/odrive/rpm", payload, len);

        } else if (msg.CANMessageID == ODRIVE_IQ) {
            float iq_measured;
            memcpy(&iq_measured, &msg.CANData[4], 4);
            len = snprintf(payload, sizeof(payload), "{\"iq\":%.3f}", iq_measured);
            mqtt_publish("rocket/odrive/iq", payload, len);

        } else if (msg.CANMessageID == ODRIVE_BUS_VI) {
            float vbus, ibus;
            memcpy(&vbus, &msg.CANData[0], 4);
            memcpy(&ibus, &msg.CANData[4], 4);
            len = snprintf(payload, sizeof(payload),
                "{\"vbus\":%.2f,\"ibus\":%.3f}", vbus, ibus);
            mqtt_publish("rocket/odrive/bus", payload, len);
        }
    }
}

void send_message(float pid_value){
    uint8_t send_buff[4];
    float_to_bytes(pid_value, send_buff);

    twai_frame_t tx_frame = {
        .header.id = 0x00E,
        .header.ide = false,
        .buffer = send_buff,
        .buffer_len = sizeof(send_buff),
    };
    ESP_ERROR_CHECK(twai_node_transmit(node_hdl, &tx_frame, 0));
}

void float_to_bytes(float val, uint8_t *buf) {
    union { float f; uint8_t b[4]; } u;
    u.f = val;
    buf[0] = u.b[3];
    buf[1] = u.b[2];
    buf[2] = u.b[1];
    buf[3] = u.b[0];
}