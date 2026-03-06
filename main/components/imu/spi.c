#include "spi.h"
#include <freertos/task.h>

static const char TAG[] = "SPI";

/* Definitions for the externs declared in spi.h */
spi_device_handle_t spi_handle;

spi_bus_config_t bus_config = {
    .mosi_io_num = SPI_MOSI_PIN,
    .miso_io_num = SPI_MISO_PIN,
    .sclk_io_num = SPI_CLK_PIN,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 64,
};

spi_device_interface_config_t device_config = {
    .clock_speed_hz = 10 * 1000 * 1000, // 10 MHz
    .duty_cycle_pos = 128, // 50% duty cycle
    .mode = 0, // SPI mode 0
    .spics_io_num = SPI_CS_PIN,
    .queue_size = 1,
};

void initialize_spi(void){
    ESP_LOGI(TAG, "Initializing SPI");
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_DISABLED));
    
    ESP_LOGD(TAG, "Adding IMU-Device");
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &device_config, &spi_handle));

    ESP_LOGI(TAG, "SPI configuration complete");
}



