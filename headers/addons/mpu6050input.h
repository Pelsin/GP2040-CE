#ifndef _MPU6050Input_H_
#define _MPU6050Input_H_

#include "mpu6050_dev.h"

#include "gpaddon.h"
#include "gamepad/GamepadAuxState.h"
#include "peripheralmanager.h"

#ifndef MPU6050_ENABLED
#define MPU6050_ENABLED 0
#endif

#ifndef MPU6050_ACCEL_ENABLED
#define MPU6050_ACCEL_ENABLED 1
#endif

#ifndef MPU6050_GYRO_ENABLED
#define MPU6050_GYRO_ENABLED 1
#endif

// Addon identifier
#define MPU6050InputName "MPU6050"

class MPU6050Input : public GPAddon {
public:
    virtual bool available();
    virtual void setup();
    virtual void preprocess() {}
    virtual void process();
    virtual void postprocess(bool sent) {}
    virtual void reinit() {}
    virtual std::string name() { return MPU6050InputName; }

private:
    MPU6050Device *mpu;
    MPU6050RawData rawData;
    uint32_t nextTimer;
    uint32_t uIntervalMS;
};

#endif // _MPU6050Input_H_
