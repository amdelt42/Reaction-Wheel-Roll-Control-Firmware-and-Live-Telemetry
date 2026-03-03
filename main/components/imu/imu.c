#include "imu.h"

static const char TAG[] = "IMU";

void initialize_imu(void){
    ESP_LOGI(TAG, "Initializing IMU");

    uint8_t whoami = 0;
    if (imu_read(REG_WHO_AM_I, &whoami, 1) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read WHO_AM_I");
        return;
    }

    if (whoami != 0x44) {
        ESP_LOGE(TAG, "Unexpected WHO_AM_I");
        return;
    }

    // Start CLKIN 
    ESP_LOGD(TAG, "Initializing CLKIN PIN");
    imu_clkin_init();
    vTaskDelay(pdMS_TO_TICKS(50));

    // Configure CLKIN 
    ESP_LOGD(TAG, "Configuring CLKIN");
    imu_write(REG_INTF_CONFIG5, 0x00);
    imu_set_bits(REG_INTF_CONFIG5, 1, 2, 0b10);  // CLKIN enable
    
    //imu_write(REG_INTF_CONFIG1, 0x51);
    //imu_set_bits(REG_INTF_CONFIG1, 2, 1, 0b1);   // RTC required

    // Gyro config
    ESP_LOGD(TAG, "Configuring Gyro");
    imu_write(REG_GYRO_CONFIG0, 0x06);
    imu_set_bits(REG_GYRO_CONFIG0, 5, 3, 0b001);  // ±2000 dps
    imu_set_bits(REG_GYRO_CONFIG0, 0, 4, 0b0110); // 1 kHz

    // Power Config
    ESP_LOGD(TAG, "Configuring PWR");
    imu_write(REG_PWR_MGMT0, 0x00);
    imu_set_bits(REG_PWR_MGMT0, 2, 2, 0b11);     // Gyro ON
    vTaskDelay(pdMS_TO_TICKS(100));    

    ESP_LOGI(TAG, "IMU configuration complete");
}

esp_err_t imu_read(uint8_t start_reg, uint8_t *data, size_t len)
{
    spi_transaction_t t = {0};

    if (len == 0 || data == NULL) return ESP_ERR_INVALID_ARG;

    // transmit buffer: 1 command byte + len dummy bytes
    uint8_t tx[len + 1];
    uint8_t rx[len + 1];

    tx[0] = start_reg | 0x80;   // set MSB for read
    memset(&tx[1], 0x00, len);   // dummy bytes

    t.length = (len + 1) * 8;    // total bits
    t.tx_buffer = tx;
    t.rx_buffer = rx;

    esp_err_t ret = spi_device_transmit(spi_handle, &t);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read failed from 0x%02X", start_reg);
        return ret;
    }

    // Copy received bytes (skip first dummy/command echo)
    memcpy(data, &rx[1], len);

    // Optional: log only for single-byte reads
    if (len == 1) {
        ESP_LOGD(TAG, "Read 0x%02X from 0x%02X", data[0], start_reg);
    }

    return ESP_OK;
}
esp_err_t imu_write(uint8_t reg, uint8_t data)
{
    spi_transaction_t t = {0};

    uint8_t tx[2] = { reg & 0x7F, data }; 

    t.length   = 16;        
    t.tx_buffer = tx;

    esp_err_t ret = spi_device_transmit(spi_handle, &t);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Write failed to 0x%02X", reg);
        return ret;
    }

    ESP_LOGD(TAG, "Transmitted 0x%02X to 0x%02X", data, reg);
    return ESP_OK;
}

esp_err_t imu_read_gyro(gyro_data_t *gyro)
{
    uint8_t buf[6];
    if (imu_read(REG_GYRO_XOUT_H, buf, 6) != ESP_OK)
        return ESP_FAIL;

    int16_t raw_x = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_y = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_z = (int16_t)((buf[4] << 8) | buf[5]);

    // convert to rad/s 
    gyro->x = raw_x * GYRO_SCALE * DEG_TO_RAD;
    gyro->y = raw_y * GYRO_SCALE * DEG_TO_RAD;
    gyro->z = raw_z * GYRO_SCALE * DEG_TO_RAD;

    return ESP_OK;
}

void imu_print_gyro_once(void)
{
    gyro_data_t gyro;

    if (imu_read_gyro(&gyro) == ESP_OK)
    {
        printf("GYRO [rad/s]: X=%.3f Y=%.3f Z=%.3f\n",
               gyro.x, gyro.y, gyro.z);
    }
    else
    {
        printf("Failed to read gyro\n");
    }
}

void imu_set_bits(uint8_t reg, uint8_t start_bit, uint8_t width, uint8_t value)
{
    // read old value
    uint8_t old;
    imu_read(reg, &old,1);
    
    // insert new value via mask
    uint8_t mask = ((1 << width) - 1) << start_bit;
    uint8_t new_val = (old & ~mask) | ((value << start_bit) & mask);
    imu_write(reg, new_val);
    
    ESP_LOGD(TAG, "Set bits %d:%d of 0x%02X to 0x%02X", start_bit+width-1, start_bit, reg, value);
}

void imu_clkin_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_1_BIT,  // 50% duty
        .freq_hz = 32768,                     // 32.768 kHz
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .gpio_num = IMU_CLKIN_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 1,   // 50% duty for 1-bit resolution
        .hpoint = 0
    };
    ledc_channel_config(&channel);
}
