#ifndef SPI_H
#define SPI_H

#include <driver/spi_master.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include "config.h"
#include "esp_log.h"

extern spi_device_handle_t spi_handle;
extern spi_bus_config_t bus_config;
extern spi_device_interface_config_t device_config;

void initialize_spi(void);

#endif /* SPI_H */