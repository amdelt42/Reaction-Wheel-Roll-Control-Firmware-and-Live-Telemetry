#include "spi.h"
#include "driver/ledc.h"
#include <driver/gpio.h>

typedef struct {
    float x;
    float y;
    float z;
} gyro_data_t;

void initialize_imu(void);

esp_err_t imu_read(uint8_t reg, uint8_t *data, size_t len);
esp_err_t imu_write(uint8_t reg, uint8_t data);

esp_err_t imu_read_gyro(gyro_data_t *gyro);

void imu_print_gyro_once(void);

void imu_set_bits(uint8_t reg, uint8_t start_bit, uint8_t width, uint8_t value);

void imu_clkin_init(void);