#include "blink.h"
void initialize_blink(){
    // configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),     // Select GPIO 1
        .mode = GPIO_MODE_OUTPUT,              // Set as output
        .pull_up_en = GPIO_PULLUP_DISABLE,     // Disable pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Disable pull-down
        .intr_type = GPIO_INTR_DISABLE         // Disable interrupts
    };
    gpio_config(&io_conf);

    xTaskCreate(blink_task,"blink",32,NULL,1,NULL);
}

void blink_task(){
    while(1){
        // turn LED ON
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(1000)); // delay 1 second

        // turn LED OFF
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(1000)); // delay 1 second
    }
}