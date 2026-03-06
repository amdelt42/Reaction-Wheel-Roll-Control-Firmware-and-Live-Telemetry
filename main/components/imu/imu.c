#include "imu.h"
#include "pid.h"
#include "mqtt.h"
#include "twai.h"

static const char TAG[] = "IMU";
static int64_t last_time = 0;
static int64_t last_pub = 0;
static float output = 0.0f; 
static TaskHandle_t imu_task_handle = NULL;
bool pid_enabled  = false;
bool twai_enabled = false;
QueueHandle_t imu_int_queue = NULL;

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
    //imu_write(REG_INTF_CONFIG5, 0x00); // reset
    imu_set_bits(REG_INTF_CONFIG5, 1, 2, 0b10);  // CLKIN enable
    
    //imu_write(REG_INTF_CONFIG1, 0x51);
    //imu_set_bits(REG_INTF_CONFIG1, 2, 1, 0b1);   // RTC required

    // Gyro config
    ESP_LOGD(TAG, "Configuring Gyro");
    //imu_write(REG_GYRO_CONFIG0, 0x06); // reset
    imu_set_bits(REG_GYRO_CONFIG0, 5, 3, 0b001);  // ±2000 dps
    imu_set_bits(REG_GYRO_CONFIG0, 0, 4, 0b0110); // 1 kHz

    // Power Config
    ESP_LOGD(TAG, "Configuring PWR");
    //imu_write(REG_PWR_MGMT0, 0x00); // reset
    imu_set_bits(REG_PWR_MGMT0, 2, 2, 0b11);     // Gyro ON
    vTaskDelay(pdMS_TO_TICKS(100));    

    // INT config
    ESP_LOGD(TAG, "Configuring INT");
    //imu_write(REG_INT_CONFIG, 0x00); // reset
    imu_set_bits(REG_INT_CONFIG, 0, 3, 0b111); // push-pull, active high, latched

    //imu_write(REG_INT_SOURCE0, 0x10); // reset
    imu_set_bits(REG_INT_SOURCE0, 3, 1, 0b1); // UI data ready interrupt routed to INT1 

    imu_int_queue = xQueueCreate(10, sizeof(uint32_t));

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,   // active HIGH
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << IMU_INT_PIN),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    ESP_LOGI(TAG, "IMU configuration complete");
}

//WRITE, READ
esp_err_t imu_read(uint8_t start_reg, uint8_t *data, size_t len)
{
    spi_transaction_t t = {0};

    if (len == 0 || data == NULL || len > IMU_MAX_READ_LEN) 
        return ESP_ERR_INVALID_ARG;

    uint8_t tx[IMU_MAX_READ_LEN + 1] = {0};
    uint8_t rx[IMU_MAX_READ_LEN + 1] = {0};

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

//INT, CLKIN
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
void imu_interrupt_init(void)
{
    if (imu_int_queue == NULL)
        imu_int_queue = xQueueCreate(10, sizeof(uint32_t));

    gpio_isr_handler_remove(IMU_INT_PIN);
    gpio_isr_handler_add(IMU_INT_PIN, imu_isr_handler, (void*) IMU_INT_PIN);
}
void imu_task(void *arg)
{
    uint32_t io_num;

    while (1)
    {
        if (xQueueReceive(imu_int_queue, &io_num, portMAX_DELAY))
        {
            // clear latched interrupt
            uint8_t status;
            imu_read(REG_INT_STATUS, &status, 1);

            gyro_data_t gyro;
            if (imu_read_gyro(&gyro) != ESP_OK) continue;

            // debug
            //printf("z:%.2f", gyro.z);

            // compute dt
            int64_t now = esp_timer_get_time();
            if (last_time == 0)
            {
                last_time = now;
                continue;   
            }
            float dt = (now - last_time) / 1e6f;
            last_time = now;

            // pid
            if (s_pid != NULL && pid_enabled)
            {
                output = pid_compute(s_pid, gyro.z, dt);
                //debug
                //printf(" output: %.2f\n", output);
            }

            // publish via mqtt
            if ((now - last_pub) >= PUB_INTERVAL_US) {
                last_pub = now;
                char payload[64];
                int len = snprintf(payload, sizeof(payload),
                    "{\"gz\":%.3f,\"out\":%.3f,\"t\":%"PRId64"}",
                    gyro.z, output, now);
                mqtt_publish("rocket/telemetry", payload, len);
                // TWAI
                if (twai_enabled)
                    send_message(output);
            }
        }
    }
}
void imu_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(imu_int_queue, &gpio_num, NULL);
}

//START, STOP
void imu_start(void)
{
    if (imu_task_handle == NULL)
    {
        last_time = 0;

        xQueueReset(imu_int_queue);
        
        uint8_t dummy;
        imu_read(REG_INT_STATUS, &dummy, 1);
        gpio_intr_enable(IMU_INT_PIN);
        imu_interrupt_init();
        xTaskCreate(imu_task, "imu_task", 4096, NULL, 10, &imu_task_handle);
        ESP_LOGI(TAG, "IMU task started");
    }
}

void imu_stop(void)
{
    if (imu_task_handle != NULL) 
    {
        gpio_intr_disable(IMU_INT_PIN); 
        gpio_isr_handler_remove(IMU_INT_PIN);

        vTaskDelete(imu_task_handle);
        imu_task_handle = NULL;
    }

    ESP_LOGI(TAG, "IMU task stopped");
}
