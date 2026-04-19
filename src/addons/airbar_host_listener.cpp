#include "addons/airbar_host_listener.h"
#include "storagemanager.h"
#include "host/usbh.h"
#include "gamepad/GamepadState.h"

#include <algorithm>

#define DEV_ADDR_NONE 0xFF

// Linear map: value in [0, range_max] → [GAMEPAD_JOYSTICK_MIN, GAMEPAD_JOYSTICK_MAX]
uint16_t AirBarHostListener::mapAxis(uint16_t value, uint16_t range_max) {
    if (range_max == 0) return GAMEPAD_JOYSTICK_MID;
    uint32_t mapped = ((uint32_t)value * GAMEPAD_JOYSTICK_MAX) / range_max;
    return (uint16_t)std::clamp<uint32_t>(mapped, GAMEPAD_JOYSTICK_MIN, GAMEPAD_JOYSTICK_MAX);
}

void AirBarHostListener::setup() {
    _airbar_mounted   = false;
    _airbar_dev_addr  = DEV_ADDR_NONE;
    _airbar_instance  = 0;

    _airbar_state.lx = GAMEPAD_JOYSTICK_MID;
    _airbar_state.ly = GAMEPAD_JOYSTICK_MID;
    _airbar_state.rx = GAMEPAD_JOYSTICK_MID;
    _airbar_state.ry = GAMEPAD_JOYSTICK_MID;
    _airbar_state.buttons = 0;
    _airbar_state.dpad    = 0;
    _airbar_state.lt = 0;
    _airbar_state.rt = 0;
}

void AirBarHostListener::mount(uint8_t dev_addr, uint8_t instance,
                               uint8_t const* desc_report, uint16_t desc_len) {
    if (_airbar_mounted) return;

    uint16_t vid = 0;
    uint16_t pid = 0;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    if (vid == AIRBAR_VID && pid == AIRBAR_PID) {
        _airbar_mounted  = true;
        _airbar_dev_addr = dev_addr;
        _airbar_instance = instance;

        // Reset sticks to centre on (re)connect
        _airbar_state.lx = GAMEPAD_JOYSTICK_MID;
        _airbar_state.ly = GAMEPAD_JOYSTICK_MID;
        _airbar_state.rx = GAMEPAD_JOYSTICK_MID;
        _airbar_state.ry = GAMEPAD_JOYSTICK_MID;
    }
}

void AirBarHostListener::unmount(uint8_t dev_addr) {
    if (_airbar_mounted && _airbar_dev_addr == dev_addr) {
        _airbar_mounted  = false;
        _airbar_dev_addr = DEV_ADDR_NONE;
        _airbar_instance = 0;

        // Centre sticks when device is removed
        _airbar_state.lx = GAMEPAD_JOYSTICK_MID;
        _airbar_state.ly = GAMEPAD_JOYSTICK_MID;
        _airbar_state.rx = GAMEPAD_JOYSTICK_MID;
        _airbar_state.ry = GAMEPAD_JOYSTICK_MID;
    }
}

void AirBarHostListener::report_received(uint8_t dev_addr, uint8_t instance,
                                         uint8_t const* report, uint16_t len) {
    if (!_airbar_mounted) return;
    if (dev_addr != _airbar_dev_addr || instance != _airbar_instance) return;

    // Expect: reportId(1) + contactCount(1) + scanTime(2) + 2 * contact(9)
    if (len < AIRBAR_MIN_REPORT_LEN) return;
    if (report[0] != AIRBAR_REPORT_ID) return;

    // Reset sticks to centre; active touches will override below
    _airbar_state.lx = GAMEPAD_JOYSTICK_MID;
    _airbar_state.ly = GAMEPAD_JOYSTICK_MID;
    _airbar_state.rx = GAMEPAD_JOYSTICK_MID;
    _airbar_state.ry = GAMEPAD_JOYSTICK_MID;

    const uint16_t X_HALF = AIRBAR_X_MAX / 2;

    for (uint8_t i = 0; i < AIRBAR_MAX_CONTACTS; i++) {
        uint16_t offset = AIRBAR_CONTACTS_OFFSET + i * AIRBAR_CONTACT_SIZE;

        bool tipSwitch = (report[offset] & 0x01) != 0;
        if (!tipSwitch) continue;

        uint16_t x = (uint16_t)(report[offset + 1]) | ((uint16_t)(report[offset + 2]) << 8);
        uint16_t y = (uint16_t)(report[offset + 3]) | ((uint16_t)(report[offset + 4]) << 8);

        uint16_t mapped_y = mapAxis(y, AIRBAR_Y_MAX);

        if (x < X_HALF) {
            // Left half → left stick
            _airbar_state.lx = mapAxis(x, X_HALF);
            _airbar_state.ly = mapped_y;
        } else {
            // Right half → right stick (re-normalise X within the right half)
            uint16_t x_right = x - X_HALF;
            _airbar_state.rx = mapAxis(x_right, AIRBAR_X_MAX - X_HALF);
            _airbar_state.ry = mapped_y;
        }
    }
}

void AirBarHostListener::process() {
    if (!_airbar_mounted) return;

    Gamepad *gamepad = Storage::getInstance().GetGamepad();
    gamepad->hasLeftAnalogStick  = true;
    gamepad->hasRightAnalogStick = true;

    gamepad->state.lx = _airbar_state.lx;
    gamepad->state.ly = _airbar_state.ly;
    gamepad->state.rx = _airbar_state.rx;
    gamepad->state.ry = _airbar_state.ry;
}
