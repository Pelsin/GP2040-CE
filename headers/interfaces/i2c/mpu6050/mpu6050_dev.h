#ifndef _MPU6050DEVICE_H_
#define _MPU6050DEVICE_H_

#include <vector>

#include "i2cdevicebase.h"
#include "MPU6050.h"

class MPU6050Device : public MPU6050, public I2CDeviceBase {
public:
    MPU6050Device() {}
    MPU6050Device(PeripheralI2C *i2cController, uint8_t addr = 0x68)
        : MPU6050(i2cController, addr) {}

    // MPU-6050: AD0=GND → 0x68, AD0=VCC → 0x69
    std::vector<uint8_t> getDeviceAddresses() const override {
        return {0x68, 0x69};
    }
};

#endif // _MPU6050DEVICE_H_
