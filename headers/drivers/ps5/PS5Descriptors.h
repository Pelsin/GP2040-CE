/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2025 OpenStickCommunity (gp2040-ce.info)
 */

#pragma once

#include <stdint.h>

// DualSense Controller (PS5)
#define DUALSENSE_VENDOR_ID         0x054C
#define DUALSENSE_PRODUCT_ID        0x0CE6

#define DUALSENSE_ENDPOINT_SIZE     64

/**************************************************************************
 *
 *  Endpoint Buffer Configuration
 *
 **************************************************************************/

#define ENDPOINT0_SIZE              64
#define DUALSENSE_FEATURES_SIZE     64

#define GAMEPAD_INTERFACE           0
#define GAMEPAD_ENDPOINT            1
#define GAMEPAD_SIZE                64

#define LSB(n) (n & 255)
#define MSB(n) ((n >> 8) & 255)

// HAT report (4 bits)
#define GAMEPAD_HAT_UP          0x00
#define GAMEPAD_HAT_UPRIGHT     0x01
#define GAMEPAD_HAT_RIGHT       0x02
#define GAMEPAD_HAT_DOWNRIGHT   0x03
#define GAMEPAD_HAT_DOWN        0x04
#define GAMEPAD_HAT_DOWNLEFT    0x05
#define GAMEPAD_HAT_LEFT        0x06
#define GAMEPAD_HAT_UPLEFT      0x07
#define GAMEPAD_HAT_NOTHING     0x08

// DualSense Report IDs
#define DUALSENSE_REPORT_ID_INPUT       0x01
#define DUALSENSE_REPORT_ID_OUTPUT_USB  0x02
#define DUALSENSE_REPORT_ID_FEATURE_F1  0xF1
#define DUALSENSE_REPORT_ID_FEATURE_F2  0xF2
#define DUALSENSE_REPORT_ID_FEATURE_05  0x05
#define DUALSENSE_REPORT_ID_FEATURE_08  0x08
#define DUALSENSE_REPORT_ID_FEATURE_09  0x09
#define DUALSENSE_REPORT_ID_FEATURE_20  0x20

// DualSense HID Report Descriptor
static const uint8_t dualsense_report_descriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)

    0x85, 0x01,        //   Report ID (1)

    // Sticks (Left X, Left Y, Right X, Right Y) - 4 bytes
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x35,        //   Usage (Rz)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data,Var,Abs)

    // Triggers (L2, R2) - 2 bytes
    0x09, 0x33,        //   Usage (Rx)
    0x09, 0x34,        //   Usage (Ry)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x02,        //   Input (Data,Var,Abs)

    // Sequence number - 1 byte
    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined)
    0x09, 0x20,        //   Usage (Vendor Usage 0x20)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x02,        //   Input (Data,Var,Abs)

    // Buttons (32 bits / 4 bytes): D-pad + 14 buttons + padding
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x39,        //   Usage (Hat switch)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x07,        //   Logical Maximum (7)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x42,        //   Input (Data,Var,Abs,Null State)

    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (Button 1)
    0x29, 0x0E,        //   Usage Maximum (Button 14)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x0E,        //   Report Count (14)
    0x81, 0x02,        //   Input (Data,Var,Abs)

    0x75, 0x0E,        //   Report Size (14) - 14 bits padding for alignment
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x03,        //   Input (Const,Var,Abs) - padding

    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined)
    0x09, 0x21,        //   Usage (Vendor Usage 0x21)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x34,        //   Report Count (52)
    0x81, 0x02,        //   Input (Data,Var,Abs)

    // Output Report (for LED/rumble)
    0x85, 0x02,        //   Report ID (2)
    0x09, 0x21,        //   Usage (Vendor Usage 0x21)
    0x95, 0x3F,        //   Report Count (63)
    0x91, 0x02,        //   Output (Data,Var,Abs)

    0xC0,              // End Collection
};


static const char *dualsense_string_descriptors[] = {
    (char[]){0x09, 0x04},                   // 0: Language
    "Sony Interactive Entertainment",       // 1: Manufacturer
    "DualSense Wireless Controller",        // 2: Product
    "000000000001",                         // 3: Serial Number
};

static uint8_t dualsense_device_desc[] = {
    0x12,        // bLength
    0x01,        // bDescriptorType (Device)
    0x00, 0x02,  // bcdUSB 2.00
    0x00,        // bDeviceClass (Use class information in the Interface Descriptors)
    0x00,        // bDeviceSubClass
    0x00,        // bDeviceProtocol
    0x40,        // bMaxPacketSize0 64
    LSB(DUALSENSE_VENDOR_ID), MSB(DUALSENSE_VENDOR_ID),   // idVendor
    LSB(DUALSENSE_PRODUCT_ID), MSB(DUALSENSE_PRODUCT_ID), // idProduct
    0x00, 0x02,  // bcdDevice 2.00
    0x01,        // iManufacturer (String Index)
    0x02,        // iProduct (String Index)
    0x03,        // iSerialNumber (String Index)
    0x01         // bNumConfigurations 1
};

static uint8_t dualsense_configuration_desc[] = {
    0x09,        // bLength
    0x02,        // bDescriptorType (Configuration)
    0x29, 0x00,  // wTotalLength 41
    0x01,        // bNumInterfaces 1
    0x01,        // bConfigurationValue
    0x00,        // iConfiguration (String Index)
    0xC0,        // bmAttributes Self Powered
    0xFA,        // bMaxPower 500mA

    // Interface Descriptor
    0x09,        // bLength
    0x04,        // bDescriptorType (Interface)
    0x00,        // bInterfaceNumber 0
    0x00,        // bAlternateSetting
    0x02,        // bNumEndpoints 2
    0x03,        // bInterfaceClass (HID)
    0x00,        // bInterfaceSubClass
    0x00,        // bInterfaceProtocol
    0x00,        // iInterface (String Index)

    // HID Descriptor
    0x09,        // bLength
    0x21,        // bDescriptorType (HID)
    0x11, 0x01,  // bcdHID 1.11
    0x00,        // bCountryCode
    0x01,        // bNumDescriptors
    0x22,        // bDescriptorType (Report)
    LSB(sizeof(dualsense_report_descriptor)), MSB(sizeof(dualsense_report_descriptor)),

    // Endpoint Descriptor (IN)
    0x07,        // bLength
    0x05,        // bDescriptorType (Endpoint)
    0x81,        // bEndpointAddress (IN/D2H)
    0x03,        // bmAttributes (Interrupt)
    0x40, 0x00,  // wMaxPacketSize 64
    0x01,        // bInterval 1 (unit depends on device speed)

    // Endpoint Descriptor (OUT)
    0x07,        // bLength
    0x05,        // bDescriptorType (Endpoint)
    0x02,        // bEndpointAddress (OUT/H2D)
    0x03,        // bmAttributes (Interrupt)
    0x40, 0x00,  // wMaxPacketSize 64
    0x01,        // bInterval 1 (unit depends on device speed)
};

typedef struct __attribute__((packed)) {
    uint8_t report_id;           // Byte 0: 0x01
    uint8_t left_stick_x;        // Byte 1: Left stick X
    uint8_t left_stick_y;        // Byte 2: Left stick Y
    uint8_t right_stick_x;       // Byte 3: Right stick X
    uint8_t right_stick_y;       // Byte 4: Right stick Y
    uint8_t l2_trigger;          // Byte 5: L2 analog trigger
    uint8_t r2_trigger;          // Byte 6: R2 analog trigger
    uint8_t sequence_number;     // Byte 7: Sequence counter (relative to data, not including report_id)
    uint8_t buttons[4];          // Bytes 8-11: Button state (4 bytes)

    uint8_t reserved_increment[4]; // Reserved?

    int16_t gyro[3];             // Bytes 16-21: gyro[0] at 16-17, gyro[1] at 18-19, gyro[2] at 20-21
    int16_t accel[3];            // Bytes 22-27: accel[0] at 22-23, accel[1] at 24-25, accel[2] at 26-27
    uint32_t sensor_timestamp;   // Bytes 28-31: Sensor timestamp

    uint8_t reserved_before_touchpad; // Byte 32: Reserved

    // Touchpad data (2 touch points, 4 bytes each)
    uint32_t touchpad[2];        // Bytes 33-40: Touch data for 2 points

    uint8_t reserved1[12];       // Bytes 41-52: Reserved
    uint8_t status;              // Byte 53: Status byte
    uint8_t reserved2[10];       // Bytes 54-63: Reserved (total 64 bytes for 504-bit HID report)
} DualSenseInputReport;

typedef struct __attribute__((packed)) {
    uint8_t report_id;           // 0x02
    uint8_t flags;               // Control flags
    uint8_t reserved[2];         // Reserved
    uint8_t rumble_right;        // Right motor (weak)
    uint8_t rumble_left;         // Left motor (strong)
    uint8_t reserved2[4];        // Reserved
    uint8_t mic_mute_led;        // Mic mute LED
    uint8_t trigger_motor_mode;  // Adaptive trigger motor mode
    uint8_t reserved3[10];       // Reserved
    uint8_t trigger_left[11];    // Left trigger effect data
    uint8_t trigger_right[11];   // Right trigger effect data
    uint8_t reserved4[6];        // Reserved
    uint8_t led_brightness;      // LED brightness
    uint8_t player_leds;         // Player LED bits
    uint8_t led_red;             // LED Red
    uint8_t led_green;           // LED Green
    uint8_t led_blue;            // LED Blue
} DualSenseOutputReport;
