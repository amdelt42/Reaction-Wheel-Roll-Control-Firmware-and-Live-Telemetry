#ifndef CTWAI_H
#define CTWAI_H

#include "config.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <string.h>
#include "esp_log.h"

extern QueueHandle_t twai_show_queue;
extern twai_node_handle_t node_hdl;

typedef struct 
{
    uint32_t CANMessageID;
    uint8_t CANData[8];
} CANMessage;

void initialize_twai();
bool twai_rx_cb(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx);
void received_message_task(void *arg);
void send_message(float pid_value);

void float_to_bytes(float val, uint8_t *buf);

#endif /* TWAI_H */