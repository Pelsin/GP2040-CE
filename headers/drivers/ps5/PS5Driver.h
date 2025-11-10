/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2025 OpenStickCommunity (gp2040-ce.info)
 */

#pragma once

#include "gpdriver.h"
#include "drivers/ps5/PS5Descriptors.h"

// Forward declarations
class PS5Auth;
class USBListener;
struct PS5AuthData;

typedef enum
{
	DUAL_SENSE_DEFINITION = 0x03,
	DUAL_SENSE_CALIBRATION = 0x05,				 // Calibration data (IMU sensors)
	DUAL_SENSE_FIRMWARE_VERSION = 0x20,		 // Firmware version and date
	DUAL_SENSE_SET_TEST_COMMAND = 0x80,		 // Set test command
	DUAL_SENSE_GET_TEST_RESULT = 0x81,		 // Get test result
	DUAL_SENSE_SET_AUTH_PAYLOAD = 0xF0,		 // Set Auth Payload
	DUAL_SENSE_GET_SIGNATURE_NONCE = 0xF1, // Get Signature Nonce
	DUAL_SENSE_GET_SIGNING_STATE = 0xF2,	 // Get Signing State
} DualSenseAuthReport;

class PS5Driver : public GPDriver
{
public:
	PS5Driver();
	virtual ~PS5Driver();
	virtual void initialize();
	virtual bool process(Gamepad *gamepad);
	virtual void initializeAux() { initialize(); }
	virtual void processAux() {}
	virtual uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
	virtual void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);
	virtual bool vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request);
	virtual const uint16_t *get_descriptor_string_cb(uint8_t index, uint16_t langid);
	virtual const uint8_t *get_descriptor_device_cb();
	virtual const uint8_t *get_hid_descriptor_report_cb(uint8_t itf);
	virtual const uint8_t *get_descriptor_configuration_cb(uint8_t index);
	virtual const uint8_t *get_descriptor_device_qualifier_cb();
	virtual uint16_t GetJoystickMidValue();
	virtual USBListener *get_usb_auth_listener();

private:
	void sendReport(Gamepad *gamepad);
	void sendReportNormal(const DualSenseInputReport *report);	// Send report directly to USB
	void sendReportPS5Auth(const DualSenseInputReport *report); // Send report via PS5 auth (TGS encryption)
	void sendBufferedReport();																	// Send buffered report with encrypted keys
	uint8_t last_report_counter;
	uint32_t last_axis_counter;

	PS5Auth *ps5AuthDriver;							 // PS5 Authentication Driver
	PS5AuthData *ps5AuthData;						 // PS5 Authentication Data (shared with PS5Auth)
	DualSenseInputReport pending_report; // Buffer for report waiting for encryption
	bool awaiting_encryption;						 // Flag indicating we're waiting for encrypted keys

	// Touch data
	uint8_t touchpad_data[9];

	// LED/Rumble state
	uint8_t led_red;
	uint8_t led_green;
	uint8_t led_blue;
	uint8_t rumble_left;
	uint8_t rumble_right;

	// Motion sensor state
	int16_t gyro_x, gyro_y, gyro_z;
	int16_t accel_x, accel_y, accel_z;
	uint32_t sensor_timestamp;
};
