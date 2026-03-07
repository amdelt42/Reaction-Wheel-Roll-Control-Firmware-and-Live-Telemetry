#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// LED
#define LED_PIN 1

// CAN
#define CANTX_PIN 48
#define CANRX_PIN 47 

#define ODRIVE_MSG(cmd)     ((ODRIVE_NODE_ID << 5) | (cmd))
#define ODRIVE_HEARTBEAT    ODRIVE_MSG(0x01)
#define ODRIVE_ENCODER      ODRIVE_MSG(0x09)
#define ODRIVE_IQ           ODRIVE_MSG(0x14)
#define ODRIVE_BUS_VI       ODRIVE_MSG(0x17)
#define ODRIVE_NODE_ID  0  

// SPI
#define SPI_MOSI_PIN 13
#define SPI_MISO_PIN 11
#define SPI_CLK_PIN 12
#define SPI_CS_PIN 10

// ICM-42686-P IMU
#define IMU_INT_PIN 9
#define IMU_CLKIN_PIN 21
#define REG_WHO_AM_I 0x75

#define REG_PWR_MGMT0 0x4E
#define REG_GYRO_CONFIG0 0x4F
#define REG_ACCEL_CONFIG0 0x50

#define REG_INTF_CONFIG1 0x4D
#define REG_INTF_CONFIG5 0x7B

#define REG_INT_CONFIG 0x14
#define REG_INT_SOURCE0 0x65
#define REG_INT_STATUS 0x2D

#define REG_GYRO_XOUT_H 0x25
#define REG_GYRO_XOUT_L 0x26
#define REG_GYRO_YOUT_H 0x27
#define REG_GYRO_YOUT_L 0x28
#define REG_GYRO_ZOUT_H 0x29
#define REG_GYRO_ZOUT_L 0x2A

#define GYRO_SCALE (1.0f / 16.4f) // LSB/dps for ±2000 dps
#define DEG_TO_RAD (3.14159265359f / 180.0f)
#define IMU_MAX_READ_LEN 16

// PID
#define PID_KP ((float)1.0)
#define PID_KI ((float)0.0)
#define PID_KD ((float)0.0)
#define PID_SETPOINT ((float)0.0)
#define PID_OUTPUT_MAX ((float)500.0)

// WIFI
#define WIFI_SSID "bim_mobile_wifi"
#define WIFI_PASS "mypookiesotootie"

#define PUB_INTERVAL_US  10000LL

#endif // CONFIG_H