// MPU6050 Library
// category=Signal Input/Output

// Written for GP2040-CE
// Interfaces the InvenSense MPU-6050 6-axis IMU (3-axis accel + 3-axis gyro) over I2C.

#ifndef _MPU6050_H_
#define _MPU6050_H_

#include "peripheral_i2c.h"

// ---- Register Map ----
#define MPU6050_REG_SMPLRT_DIV      0x19
#define MPU6050_REG_CONFIG          0x1A
#define MPU6050_REG_GYRO_CONFIG     0x1B
#define MPU6050_REG_ACCEL_CONFIG    0x1C
#define MPU6050_REG_ACCEL_XOUT_H    0x3B
#define MPU6050_REG_GYRO_XOUT_H     0x43
#define MPU6050_REG_PWR_MGMT_1      0x6B
#define MPU6050_REG_WHO_AM_I        0x75

// ---- GYRO_CONFIG FS_SEL ----
#define MPU6050_GYRO_FS_250         0x00  // ±250  °/s  (131   LSB/°/s)
#define MPU6050_GYRO_FS_500         0x08  // ±500  °/s  (65.5  LSB/°/s)
#define MPU6050_GYRO_FS_1000        0x10  // ±1000 °/s  (32.8  LSB/°/s)
#define MPU6050_GYRO_FS_2000        0x18  // ±2000 °/s  (16.4  LSB/°/s)

// ---- ACCEL_CONFIG AFS_SEL ----
#define MPU6050_ACCEL_FS_2G         0x00  // ±2 g  (16384 LSB/g)
#define MPU6050_ACCEL_FS_4G         0x08  // ±4 g  (8192  LSB/g)
#define MPU6050_ACCEL_FS_8G         0x10  // ±8 g  (4096  LSB/g)
#define MPU6050_ACCEL_FS_16G        0x18  // ±16 g (2048  LSB/g)

// ---- PWR_MGMT_1 ----
#define MPU6050_SLEEP_BIT           0x40
#define MPU6050_CLK_PLL_XGYRO       0x01  // PLL with X-axis gyro reference

// ---- WHO_AM_I expected value ----
#define MPU6050_WHO_AM_I_VAL        0x68

struct MPU6050RawData {
    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
};

class MPU6050 {
public:
    MPU6050() : i2c(nullptr), address(0x68) {}
    MPU6050(PeripheralI2C *i2cController, uint8_t addr = 0x68)
        : i2c(i2cController), address(addr) {}

    // Initialise the sensor: wake from sleep, configure clock, full-scale ranges
    void begin(uint8_t accelRange = MPU6050_ACCEL_FS_2G,
               uint8_t gyroRange  = MPU6050_GYRO_FS_500);

    // Read all six axes in one burst (14 bytes from 0x3B, skips temp)
    bool readRawData(MPU6050RawData &out);

    // Returns the WHO_AM_I register value (should be 0x68)
    uint8_t whoAmI();

    void setI2C(PeripheralI2C *i2cController) { i2c = i2cController; }
    void setAddress(uint8_t addr) { address = addr; }

protected:
    PeripheralI2C *i2c;
    uint8_t address;

private:
    uint8_t uc[14];

    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
};

#endif // _MPU6050_H_
