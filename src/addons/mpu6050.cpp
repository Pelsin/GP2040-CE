#include "addons/mpu6050input.h"
#include "storagemanager.h"
#include "helper.h"
#include "config.pb.h"

bool MPU6050Input::available() {
    const MPU6050Options &options =
        Storage::getInstance().getAddonOptions().mpu6050Options;
    if (!options.enabled) return false;

    mpu = new MPU6050Device();
    PeripheralI2CScanResult result =
        PeripheralManager::getInstance().scanForI2CDevice(mpu->getDeviceAddresses());

    if (result.address > -1) {
        mpu->setAddress((uint8_t)result.address);
        mpu->setI2C(PeripheralManager::getInstance().getI2C(result.block));
        return true;
    }

    delete mpu;
    return false;
}

void MPU6050Input::setup() {
    uIntervalMS = 10;   // 100 Hz polling
    nextTimer   = getMillis();

    mpu->begin();

    // Set enabled flags once at setup so the driver sees them from frame 1
    const MPU6050Options &options =
        Storage::getInstance().getAddonOptions().mpu6050Options;
    Gamepad *gamepad = Storage::getInstance().GetGamepad();
    gamepad->auxState.sensors.accelerometer.enabled = options.accelEnabled;
    gamepad->auxState.sensors.gyroscope.enabled     = options.gyroEnabled;
}

void MPU6050Input::process() {
    // Always write last-read values so the driver has fresh data every frame
    Gamepad *gamepad = Storage::getInstance().GetGamepad();
    const MPU6050Options &options =
        Storage::getInstance().getAddonOptions().mpu6050Options;

    if (options.accelEnabled) {
        gamepad->auxState.sensors.accelerometer.active = true;
        gamepad->auxState.sensors.accelerometer.x = rawData.accelX;
        gamepad->auxState.sensors.accelerometer.y = rawData.accelY;   // chip Z (gravity) → PS4 Y
        gamepad->auxState.sensors.accelerometer.z = rawData.accelZ;
    }

    if (options.gyroEnabled) {
        gamepad->auxState.sensors.gyroscope.active = true;
        gamepad->auxState.sensors.gyroscope.x = rawData.gyroX;   // Pitch
        gamepad->auxState.sensors.gyroscope.y = rawData.gyroY;   // Yaw
        gamepad->auxState.sensors.gyroscope.z = rawData.gyroZ;   // Roll
    }

    // Rate-limit I2C reads to ~100 Hz
    if (getMillis() < nextTimer) return;
    nextTimer = getMillis() + uIntervalMS;
    mpu->readRawData(rawData);
}
