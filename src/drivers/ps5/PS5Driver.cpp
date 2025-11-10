/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#include "drivers/ps5/PS5Driver.h"
#include "drivers/ps5/PS5Descriptors.h"
#include "drivers/ps5/PS5Auth.h"
#include "drivers/shared/driverhelper.h"
#include "display/ui/screens/PS5DebugScreen.h"
#include "storagemanager.h"
#include "usbhostmanager.h"
#include "GamepadState.h"
#include "tusb.h"

#include <cstring>

// Compile-time verification that DualSenseInputReport is exactly 64 bytes
static_assert(sizeof(DualSenseInputReport) == 64, "DualSenseInputReport must be exactly 64 bytes");

static constexpr uint8_t output_0x03[] = {
		0x21,
		0x28,
		0x03,
		0xC3,
		0x00,
		0x2C,
		0x56,
		0x01,
		0x00,
		0xD0,
		0x07,
		0x00,
		0x80,
		0x04,
		0x00,
		0x00,
		0x80,
		0x0D,
		0x0D,
		0x84,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
};

// Initialize DualSense driver
PS5Driver::PS5Driver()
{
	last_report_counter = 0;
	last_axis_counter = 0;

	led_red = 0;
	led_green = 0;
	led_blue = 255;

	rumble_left = 0;
	rumble_right = 0;

	gyro_x = gyro_y = gyro_z = 0;
	accel_x = accel_y = accel_z = 0;
	sensor_timestamp = 0;

	memset(touchpad_data, 0, sizeof(touchpad_data));

	ps5AuthDriver = nullptr;
	ps5AuthData = nullptr;
	awaiting_encryption = false;
	memset(&pending_report, 0, sizeof(pending_report));

	// Initialize USB class driver
	class_driver = {
#if CFG_TUSB_DEBUG >= 2
			.name = "DualSense",
#endif
			.init = hidd_init,
			.reset = hidd_reset,
			.open = hidd_open,
			.control_xfer_cb = hidd_control_xfer_cb,
			.xfer_cb = hidd_xfer_cb,
			.sof = NULL};
}

PS5Driver::~PS5Driver()
{
	// Clean up PS5 auth driver if it was created
	if (ps5AuthDriver != nullptr)
	{
		delete ps5AuthDriver;
		ps5AuthDriver = nullptr;
		ps5AuthData = nullptr;
	}
}

void PS5Driver::initialize()
{
	// Initialize PS5 authentication driver for TGS/MAGS5 adapter support
	GamepadOptions &gamepadOptions = Storage::getInstance().getGamepadOptions();

	ps5AuthDriver = nullptr;
	ps5AuthData = nullptr;

	// Create PS5Auth driver if USB authentication is enabled
	if (gamepadOptions.ps5AuthType == InputModeAuthType::INPUT_MODE_AUTH_TYPE_USB)
	{
		ps5AuthDriver = new PS5Auth(gamepadOptions.ps5AuthType);
		// If authentication driver can be initialized (USB enabled, etc.)
		if (ps5AuthDriver != nullptr && ps5AuthDriver->available())
		{
			ps5AuthDriver->initialize();
			ps5AuthData = ps5AuthDriver->getAuthData();
			// Note: Listener is registered automatically by GP2040Aux via get_usb_auth_listener()
		}
	}
}

bool PS5Driver::process(Gamepad *gamepad)
{
	if (tud_suspended())
	{
		tud_remote_wakeup();
	}

	if (ps5AuthDriver != nullptr)
	{
		ps5AuthDriver->process();

		if (awaiting_encryption && ps5AuthData != nullptr)
		{
			uint8_t encrypted_keys[12];
			uint8_t incount[4];
			uint8_t aes_cmac[8];

			if (ps5AuthData->keys_ready)
			{
				// Keys are ready in ps5AuthData, send the buffered report
				sendBufferedReport();
				awaiting_encryption = false;
			}
		}
	}

	// Build and send DualSense report (or buffer it if encryption is needed)
	if (!awaiting_encryption)
	{
		sendReport(gamepad);
	}

	return true;
}

void PS5Driver::sendReport(Gamepad *gamepad)
{
	DualSenseInputReport report;
	memset(&report, 0, sizeof(report));

	report.report_id = DUALSENSE_REPORT_ID_INPUT;

	// Analog sticks (0-255, center at 128)
	report.left_stick_x = gamepad->state.lx >> 8;
	report.left_stick_y = gamepad->state.ly >> 8;
	report.right_stick_x = gamepad->state.rx >> 8;
	report.right_stick_y = gamepad->state.ry >> 8;

	// Analog triggers (0-255)
	report.l2_trigger = (gamepad->state.buttons & GAMEPAD_MASK_L2) ? 0xFF : 0x00;
	report.r2_trigger = (gamepad->state.buttons & GAMEPAD_MASK_R2) ? 0xFF : 0x00;

	// Sequence number
	static uint8_t sequence = 0;
	report.sequence_number = sequence++;

	// Button array (4 bytes) matching PS5 reference layout
	// Byte 0: D-pad (lower 4 bits) + buttons (upper 4 bits)
	uint8_t hat_value;
	switch (gamepad->state.dpad & GAMEPAD_MASK_DPAD)
	{
	case GAMEPAD_MASK_UP:
		hat_value = GAMEPAD_HAT_UP;
		break;
	case GAMEPAD_MASK_UP | GAMEPAD_MASK_RIGHT:
		hat_value = GAMEPAD_HAT_UPRIGHT;
		break;
	case GAMEPAD_MASK_RIGHT:
		hat_value = GAMEPAD_HAT_RIGHT;
		break;
	case GAMEPAD_MASK_DOWN | GAMEPAD_MASK_RIGHT:
		hat_value = GAMEPAD_HAT_DOWNRIGHT;
		break;
	case GAMEPAD_MASK_DOWN:
		hat_value = GAMEPAD_HAT_DOWN;
		break;
	case GAMEPAD_MASK_DOWN | GAMEPAD_MASK_LEFT:
		hat_value = GAMEPAD_HAT_DOWNLEFT;
		break;
	case GAMEPAD_MASK_LEFT:
		hat_value = GAMEPAD_HAT_LEFT;
		break;
	case GAMEPAD_MASK_UP | GAMEPAD_MASK_LEFT:
		hat_value = GAMEPAD_HAT_UPLEFT;
		break;
	default:
		hat_value = GAMEPAD_HAT_NOTHING;
		break;
	}

	report.buttons[0] = hat_value & 0x0F; // Lower 4 bits: D-pad
	// Upper 4 bits of buttons[0]: Square, Cross, Circle, Triangle
	if (gamepad->state.buttons & GAMEPAD_MASK_B1)
		report.buttons[0] |= (1 << 4); // Square
	if (gamepad->state.buttons & GAMEPAD_MASK_B2)
		report.buttons[0] |= (1 << 5); // Cross
	if (gamepad->state.buttons & GAMEPAD_MASK_B3)
		report.buttons[0] |= (1 << 6); // Circle
	if (gamepad->state.buttons & GAMEPAD_MASK_B4)
		report.buttons[0] |= (1 << 7); // Triangle

	// Byte 1: L1, R1, L2, R2, Create, Options, L3, R3
	report.buttons[1] = 0;
	if (gamepad->state.buttons & GAMEPAD_MASK_L1)
		report.buttons[1] |= 0x01; // L1
	if (gamepad->state.buttons & GAMEPAD_MASK_R1)
		report.buttons[1] |= 0x02; // R1
	if (gamepad->state.buttons & GAMEPAD_MASK_L2)
		report.buttons[1] |= 0x04; // L2
	if (gamepad->state.buttons & GAMEPAD_MASK_R2)
		report.buttons[1] |= 0x08; // R2
	if (gamepad->state.buttons & GAMEPAD_MASK_S1)
		report.buttons[1] |= 0x10; // Create
	if (gamepad->state.buttons & GAMEPAD_MASK_S2)
		report.buttons[1] |= 0x20; // Options
	if (gamepad->state.buttons & GAMEPAD_MASK_L3)
		report.buttons[1] |= 0x40; // L3
	if (gamepad->state.buttons & GAMEPAD_MASK_R3)
		report.buttons[1] |= 0x80; // R3

	// Byte 2: PS, Touchpad, Mute (+ padding)
	report.buttons[2] = 0;
	if (gamepad->state.buttons & GAMEPAD_MASK_A1)
		report.buttons[2] |= 0x01; // PS button
	if (gamepad->state.buttons & GAMEPAD_MASK_A2)
		report.buttons[2] |= 0x02; // Touchpad

	// Byte 3: Connection/Battery status
	// Real DualSense USB: 0x13 = 0001 0011
	// Bit 0: USB data (1)
	// Bit 1: USB power (1)
	// Bit 4: Battery charging (1)
	// This indicates: USB connected, powered, charging
	report.buttons[3] = 0x13;

	// Reserved increment field (real DualSense uses this for something)
	memset(report.reserved_increment, 0, sizeof(report.reserved_increment));

	// Motion sensors (set to neutral/zero values)
	// Gyro: 0 means no rotation (neutral position)
	report.gyro[0] = 0; // Pitch
	report.gyro[1] = 0; // Yaw
	report.gyro[2] = 0; // Roll

	report.accel[0] = 0;		// X axis
	report.accel[1] = 8192; // Y axis - 1g gravity = 9.80665 m/s²
	report.accel[2] = 0;		// Z axis

	// Sensor timestamp (increment each report)
	static uint32_t sensor_time = 0;
	report.sensor_timestamp = sensor_time++;

	// Touchpad data: Set both touch points to "not touching" (ID >= 0x80, coordinates = 0)
	uint8_t *touchpad_bytes = (uint8_t *)&report.touchpad[0];
	memset(touchpad_bytes, 0, 8); // Zero all touchpad bytes
	touchpad_bytes[0] = 0x80;			// Touch 1 ID: not touching
	touchpad_bytes[4] = 0x80;			// Touch 2 ID: not touching

	// Reserved fields
	memset(report.reserved1, 0, sizeof(report.reserved1));

	// Status byte: bit 0 = headphone connected, bit 1 = haptics enabled, bit 2-3 = unknown
	// For USB connection, this is typically 0x00 or specific flags
	report.status = 0x00;
	memset(report.reserved2, 0, sizeof(report.reserved2));

	if (ps5AuthData != nullptr && ps5AuthData->dongle_ready && ps5AuthData->ps5_auth_state == PS5State::ps5_key_encryption_ready)
	{
		sendReportPS5Auth(&report);
	}
	else
	{
		sendReportNormal(&report);
	}
}

void PS5Driver::sendReportNormal(const DualSenseInputReport *report)
{
	// Send report directly via USB without encryption
	if (tud_hid_ready())
	{
		tud_hid_report(0, report, sizeof(DualSenseInputReport));
	}
}

void PS5Driver::sendReportPS5Auth(const DualSenseInputReport *report)
{
	// PS5 authenticated mode: Need to encrypt keys via MAGS5 before sending?
	memcpy(&pending_report, report, sizeof(DualSenseInputReport));

	// Extract 12-byte keys, battery, and touchpad data from report
	uint8_t keys_12byte[12];
	uint8_t battery_3byte[3];
	uint8_t touchpad_9byte[9];

	// TODO: Map the actual button/analog data to 12-byte keys format
	// For now, send zeros as placeholder
	memset(keys_12byte, 0, 12);

	// Battery info (3 bytes) - use status byte
	battery_3byte[0] = report->status;
	battery_3byte[1] = 0;
	battery_3byte[2] = 0;

	// Touchpad data (9 bytes) - extract from report
	const uint8_t *touchpad_bytes = (const uint8_t *)&report->touchpad[0];
	memcpy(touchpad_9byte, touchpad_bytes, 8);
	touchpad_9byte[8] = 0; // 9th byte padding

	// Send to TGS for encryption and wait for response (via PS5Auth interface)
	if (ps5AuthDriver->sendKeysForEncryption(keys_12byte, battery_3byte, touchpad_9byte))
	{
		awaiting_encryption = true;
	}
	else
	{
		// Encryption request failed, fall back to normal send
		sendReportNormal(report);
	}
}

void PS5Driver::sendBufferedReport()
{
	// Get encrypted keys from PS5 auth data
	if (ps5AuthData == nullptr || !ps5AuthData->keys_ready)
	{
		// Keys not ready, fall back to normal send
		sendReportNormal(&pending_report);
		return;
	}

	// Send the auth_buffer directly as Report 0x01
	// The auth_buffer from MAGS5 contains the full 64-byte auth structure
	// that needs to be forwarded to the PS5
	uint8_t report_with_auth[64];

	// Copy the auth buffer (which already includes proper structure)
	memcpy(report_with_auth, ps5AuthData->auth_buffer, 64);

	// Ensure report ID is 0x01
	report_with_auth[0] = DUALSENSE_REPORT_ID_INPUT;

	// Debug: Show first few bytes being sent to PS5
	char debug_msg[32];
	snprintf(debug_msg, sizeof(debug_msg), "TX->PS5:%02X %02X %02X %02X",
					 report_with_auth[1], report_with_auth[2],
					 report_with_auth[3], report_with_auth[4]);
	PS5DebugScreen::addMessage(debug_msg);

	// Send the auth report to PS5
	if (tud_hid_ready())
	{
		tud_hid_report(0, report_with_auth, 64);
	}
}

uint16_t PS5Driver::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
	if (report_type != HID_REPORT_TYPE_FEATURE)
	{
		return -1;
	}
	uint16_t responseLen = 0;
	switch (report_id)
	{
	case DualSenseAuthReport::DUAL_SENSE_DEFINITION:
	{
		if (reqlen < sizeof(output_0x03))
		{
			return -1;
		}
		responseLen = MAX(reqlen, sizeof(output_0x03));
		memcpy(buffer, output_0x03, responseLen);
		PS5DebugScreen::addMessage("DEF REQ");
		return responseLen;
	}
	case DualSenseAuthReport::DUAL_SENSE_CALIBRATION:
	{
		// I think this is similar to PS4's calibration report but for DualSense
		static uint8_t calibration_data[40] = {
				// Gyro pitch bias, yaw bias, roll bias (int16_t each)
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				// Gyro pitch+, pitch-, yaw+, yaw-, roll+, roll- (int16_t each)
				0x00, 0x20, 0x00, 0xE0, 0x00, 0x20, 0x00, 0xE0, 0x00, 0x20, 0x00, 0xE0,
				// Gyro speed+ and speed- (int16_t each)
				0x00, 0x20, 0x00, 0xE0,
				// Accel X+, X-, Y+, Y-, Z+, Z- (int16_t each)
				0x00, 0x20, 0x00, 0xE0, 0x00, 0x20, 0x00, 0xE0, 0x00, 0x20, 0x00, 0xE0,
				// Unknown/padding to 40 bytes
				0x00, 0x00};
		memcpy(buffer, calibration_data, MIN(reqlen, sizeof(calibration_data)));
		return MIN(reqlen, sizeof(calibration_data));
	}
	case DualSenseAuthReport::DUAL_SENSE_FIRMWARE_VERSION:
	{
		PS5DebugScreen::addMessage("Firmware ver");

		// Pulled from real my own DualSense controller - Pelsin
		static uint8_t firmware_data[63] = {
				0x20, 0x4A, 0x75, 0x6C, 0x20, 0x20,
				0x34, 0x20, 0x32, 0x30, 0x32, 0x35,
				0x31, 0x30, 0x3A, 0x31, 0x30, 0x3A,
				0x33, 0x32, 0x02, 0x00, 0x04, 0x00,
				0x13, 0x03, 0x00, 0x00, 0x2A, 0x00,
				0x10, 0x01, 0x41, 0x0A, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x30, 0x06, 0x00, 0x00,
				0x2A, 0x00, 0x01, 0x00, 0x0A, 0x00,
				0x02, 0x00, 0x06, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00};
		memcpy(buffer, firmware_data, MIN(reqlen, sizeof(firmware_data)));
		return MIN(reqlen, sizeof(firmware_data));
	}
	// Test?
	case DualSenseAuthReport::DUAL_SENSE_GET_TEST_RESULT:
	{
		// Report 0x81 - Test result - 63 bytes, what should we return here?
		PS5DebugScreen::addMessage("Test result");
		memset(buffer, 0, reqlen);
		buffer[0] = 0x81;

		return reqlen;
	}
	case DualSenseAuthReport::DUAL_SENSE_GET_SIGNATURE_NONCE:
		PS5DebugScreen::addMessage("SIG NONCE REQ");
		return 63;
	case DualSenseAuthReport::DUAL_SENSE_GET_SIGNING_STATE:
		PS5DebugScreen::addMessage("SIGNING STATE REQ");
		return 15;
	}
	return reqlen;

	// PS5 is requesting authentication data - forward to MAGS5
	// if (report_id == 0xF8 && ps5AuthListener != nullptr && ps5AuthListener->isMounted())
	// {
	// 	// Trigger authentication request to MAGS5
	// 	if (ps5AuthListener->requestAuthentication())
	// 	{
	// 		// Wait for auth data - for now return placeholder
	// 		PS5DebugScreen::addMessage("Fwd to MAGS5");
	// 	}
	// 	memset(buffer, 0, reqlen);
	// 	buffer[0] = report_id;
	// 	return reqlen;
	// }
}

void PS5Driver::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
	if (report_type == HID_REPORT_TYPE_OUTPUT && report_id == DUALSENSE_REPORT_ID_OUTPUT_USB)
	{
		if (bufsize >= sizeof(DualSenseOutputReport))
		{
			const DualSenseOutputReport *output = (const DualSenseOutputReport *)buffer;

			// Update LED colors
			led_red = output->led_red;
			led_green = output->led_green;
			led_blue = output->led_blue;

			// Update rumble
			rumble_left = output->rumble_left;
			rumble_right = output->rumble_right;
		}
	}
	else if (report_type == HID_REPORT_TYPE_FEATURE)
	{
		// No idea if we need to handle feature reports for DualSense like PS4Auth does
	}
}

bool PS5Driver::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
	return false;
}

const uint16_t *PS5Driver::get_descriptor_string_cb(uint8_t index, uint16_t langid)
{
	const char *value = (const char *)dualsense_string_descriptors[index];
	return getStringDescriptor(value, index);
}

const uint8_t *PS5Driver::get_descriptor_device_cb()
{
	return dualsense_device_desc;
}

const uint8_t *PS5Driver::get_hid_descriptor_report_cb(uint8_t itf)
{
	return dualsense_report_descriptor;
}

const uint8_t *PS5Driver::get_descriptor_configuration_cb(uint8_t index)
{
	return dualsense_configuration_desc;
}

const uint8_t *PS5Driver::get_descriptor_device_qualifier_cb()
{
	return nullptr;
}

uint16_t PS5Driver::GetJoystickMidValue()
{
	return GAMEPAD_JOYSTICK_MID;
}

USBListener *PS5Driver::get_usb_auth_listener()
{
	// Return PS5 auth listener if it exists (for TGS/MAGS5 adapter support)
	if (ps5AuthDriver != nullptr)
	{
		return ps5AuthDriver->getListener();
	}
	return nullptr;
}
