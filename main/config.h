// LED
#define LED_PIN 1

// CAN
#define CANTX_PIN 48
#define CANRX_PIN 47 

// SPI
#define SPI_MOSI_PIN 13
#define SPI_MISO_PIN 11
#define SPI_CLK_PIN 12
#define SPI_CS_PIN 10

// ICM-42686-P IMU
#define REG_WHO_AM_I 0x75
#define REG_PWR_MGMT0 0x4E
#define REG_GYRO_CONFIG0 0x4F
#define REG_ACCEL_CONFIG0 0x50

#define REG_GYRO_XOUT_H 0x25
#define REG_GYRO_XOUT_L 0x26
#define REG_GYRO_YOUT_H 0x27
#define REG_GYRO_YOUT_L 0x28
#define REG_GYRO_ZOUT_H 0x29
#define REG_GYRO_ZOUT_L 0x2A

#define GYRO_SCALE (1.0f / 16.4f) // LSB/dps for ±2000 dps
#define DEG_TO_RAD (3.14159265359f / 180.0f)

#define IMU_INT_PIN 9
#define IMU_FSYNC_PIN 21