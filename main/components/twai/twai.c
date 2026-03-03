#include "twai.h"

static const char TAG[] = "TWAI";

QueueHandle_t twai_show_queue;
twai_node_handle_t node_hdl;

void initialize_twai(){
    ESP_LOGI(TAG, "Initializing TWAI");

    node_hdl = NULL;
    twai_onchip_node_config_t node_config = {
        .io_cfg.tx = CANTX_PIN,             // TWAI TX GPIO pin
        .io_cfg.rx = CANRX_PIN,             // TWAI RX GPIO pin
        .bit_timing.bitrate = 250000,  // 250 kbps bitrate
        .tx_queue_depth = 5,        // Transmit queue depth set to 5
    };

    // Create a queue for printing received messages
    twai_show_queue = xQueueCreate(1, sizeof(CANMessage));
    xTaskCreate(received_message_task, "ReceivedMessageTask", 4096, NULL, 1, NULL);

    // Create a new TWAI controller driver instance
    ESP_LOGI(TAG, "Creating TWAI Node");
    ESP_ERROR_CHECK(twai_new_node_onchip(&node_config, &node_hdl));
    
    twai_event_callbacks_t user_cbs = {
    .on_rx_done = twai_rx_cb,
    };
    ESP_ERROR_CHECK(twai_node_register_event_callbacks(node_hdl, &user_cbs, NULL));
    
    // Start the TWAI controller
    ESP_LOGI(TAG, "Starting TWAI Controller");
    ESP_ERROR_CHECK(twai_node_enable(node_hdl));
    

    // Create a queue for sending messages
    xTaskCreate(send_message_task, "SendMessageTask", 4096, NULL, 1, NULL);
}

bool twai_rx_cb(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx)
{
    uint8_t recv_buff[8];
    twai_frame_t rx_frame = {
        .buffer = recv_buff,
        .buffer_len = sizeof(recv_buff),
    };
    if (ESP_OK == twai_node_receive_from_isr(handle, &rx_frame)) {
        //print the recieved frame
        CANMessage msg;
        msg.CANMessageID = rx_frame.header.id;
        memcpy(msg.CANData, rx_frame.buffer, rx_frame.buffer_len);
        xQueueSendFromISR(twai_show_queue, &msg, NULL);
    }
    return false;
}

void received_message_task(void *arg){
    if (twai_show_queue != NULL) {
        while (1) {
            CANMessage msg;
            if (xQueueReceive(twai_show_queue, &msg, portMAX_DELAY) == pdTRUE) {
                printf("Received CAN message with ID: %lu, Data: ", msg.CANMessageID);
            }
        }
    }
}

void send_message_task (void *args){
    // Example data to send
    uint8_t send_buff[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01, 0x02, 0x03}; 

    twai_frame_t tx_frame = {
        .header.id = 0x1,           // Message ID
        .header.ide = false,         // Use 29-bit extended ID format
        .buffer = send_buff,        // Pointer to data to transmit
        .buffer_len = sizeof(send_buff),  // Length of data to transmit 
    };

    while (1)   
    {       
        ESP_ERROR_CHECK(twai_node_transmit(node_hdl, &tx_frame, 0));
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay for 1 second
    }   
}