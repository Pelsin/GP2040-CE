#ifndef _AirBarHostListener_H
#define _AirBarHostListener_H

#include "usblistener.h"
#include "gamepad.h"
#include "host/usbh.h"

// Neonode AirBar 15.6 USB identifiers
#define AIRBAR_VID              0x1536  // 5430 decimal
#define AIRBAR_PID              0x0101  // 257 decimal

// HID report constants (from AirBar HID report descriptor)
#define AIRBAR_REPORT_ID        0x03
#define AIRBAR_X_MAX            3452
#define AIRBAR_Y_MAX            1942
#define AIRBAR_MAX_CONTACTS     2
#define AIRBAR_CONTACT_SIZE     9       // bytes per contact: 1(tip+id) + 2(x) + 2(y) + 2(w) + 2(h)
#define AIRBAR_CONTACTS_OFFSET  4       // after: reportId(1) + contactCount(1) + scanTime(2)
#define AIRBAR_MIN_REPORT_LEN   (AIRBAR_CONTACTS_OFFSET + AIRBAR_MAX_CONTACTS * AIRBAR_CONTACT_SIZE)

class AirBarHostListener : public USBListener {
public:
    virtual void setup();
    virtual void mount(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len);
    virtual void xmount(uint8_t dev_addr, uint8_t instance, uint8_t controllerType, uint8_t subtype) {}
    virtual void unmount(uint8_t dev_addr);
    virtual void report_received(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
    virtual void report_sent(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {}
    virtual void set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) {}
    virtual void get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) {}
    void process();

private:
    // Map a coordinate value linearly into [GAMEPAD_JOYSTICK_MIN, GAMEPAD_JOYSTICK_MAX]
    uint16_t mapAxis(uint16_t value, uint16_t range_max);

    bool     _airbar_mounted;
    uint8_t  _airbar_dev_addr;
    uint8_t  _airbar_instance;

    GamepadState _airbar_state;
};

#endif  // _AirBarHostListener_H
