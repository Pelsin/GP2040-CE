// MPU6050 Library
// category=Signal Input/Output

#include "MPU6050.h"

#include <cstring>
#include <pico/time.h>

void MPU6050::writeRegister(uint8_t reg, uint8_t value) {
    uc[0] = reg;
    uc[1] = value;
    i2c->write(address, uc, 2);
}

uint8_t MPU6050::readRegister(uint8_t reg) {
    i2c->readRegister(address, reg, uc, 1);
    return uc[0];
}

void MPU6050::begin(uint8_t accelRange, uint8_t gyroRange) {
    // Wake from sleep, use PLL with X-axis gyro as clock source (recommended by InvenSense).
    // After setting CLKSEL, wait 100 ms for the PLL to lock and gyro to stabilise
    // before writing any further configuration registers.
    writeRegister(MPU6050_REG_PWR_MGMT_1, MPU6050_CLK_PLL_XGYRO);
    sleep_ms(100);

    // Sample rate = gyroscope output rate / (1 + SMPLRT_DIV)
    // DLPF active → gyro output rate = 1 kHz; 1000 / (1 + 9) = 100 Hz
    writeRegister(MPU6050_REG_SMPLRT_DIV, 0x09);

    // DLPF_CFG = 2: Accel bandwidth 94 Hz, Gyro bandwidth 98 Hz
    writeRegister(MPU6050_REG_CONFIG, 0x02);

    // Gyroscope full-scale range
    writeRegister(MPU6050_REG_GYRO_CONFIG, gyroRange);

    // Accelerometer full-scale range
    writeRegister(MPU6050_REG_ACCEL_CONFIG, accelRange);
}

bool MPU6050::readRawData(MPU6050RawData &out) {
    // Read 14 bytes starting at ACCEL_XOUT_H:
    //   [0-1]  Accel X  [2-3]  Accel Y  [4-5]  Accel Z
    //   [6-7]  Temp     (ignored)
    //   [8-9]  Gyro X   [10-11] Gyro Y  [12-13] Gyro Z
    int16_t ret = i2c->readRegister(address, MPU6050_REG_ACCEL_XOUT_H, uc, 14);
    if (ret < 0) return false;

    out.accelX = (int16_t)((uc[0]  << 8) | uc[1]);
    out.accelY = (int16_t)((uc[2]  << 8) | uc[3]);
    out.accelZ = (int16_t)((uc[4]  << 8) | uc[5]);
    // uc[6]/uc[7] = temperature — skip
    out.gyroX  = (int16_t)((uc[8]  << 8) | uc[9]);
    out.gyroY  = (int16_t)((uc[10] << 8) | uc[11]);
    out.gyroZ  = (int16_t)((uc[12] << 8) | uc[13]);

    return true;
}

uint8_t MPU6050::whoAmI() {
    return readRegister(MPU6050_REG_WHO_AM_I);
}
