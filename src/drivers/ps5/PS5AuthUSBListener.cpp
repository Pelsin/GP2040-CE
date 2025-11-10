#include "tusb.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "class/hid/hid.h"
#include "class/hid/hid_host.h"
#include "drivers/ps5/PS5Auth.h"
#include "drivers/ps5/PS5AuthUSBListener.h"
#include "mbedtls/des.h"
#include "peripheralmanager.h"
#include "usbhostmanager.h"
#include "display/ui/screens/PS5DebugScreen.h"

// DES Secret Key
static const unsigned char DES_SECRET_KEY[16] = {
		0x5C, 0x28, 0xE3, 0x05, 0x97, 0xC5, 0xAD, 0x04,
		0x9E, 0x5D, 0x19, 0xC3, 0x25, 0x40, 0x5B, 0x9D};

void PS5AuthUSBListener::setup()
{
	ps_dev_addr = 0xFF;
	ps_instance = 0xFF;
	ps5AuthData = nullptr;
	resetHostData();
}

void PS5AuthUSBListener::resetHostData()
{
	awaiting_cb = false;
	memset(auth_buffer, 0, sizeof(auth_buffer));
	memset(key_buffer, 0, sizeof(key_buffer));

	if (ps5AuthData != nullptr)
	{
		ps5AuthData->dongle_ready = false;
		ps5AuthData->ps5_auth_state = PS5State::ps5_no_auth;
		ps5AuthData->keys_ready = false;
		memset(ps5AuthData->auth_data_received, 0, sizeof(ps5AuthData->auth_data_received));
		memset(ps5AuthData->auth_data_encrypted, 0, sizeof(ps5AuthData->auth_data_encrypted));
		memset(ps5AuthData->encrypted_keys, 0, sizeof(ps5AuthData->encrypted_keys));
		memset(ps5AuthData->incount, 0, sizeof(ps5AuthData->incount));
		memset(ps5AuthData->aes_cmac, 0, sizeof(ps5AuthData->aes_cmac));
	}
}

// Is there a better way to do this?
void PS5AuthUSBListener::performDESEncryption(uint8_t *data)
{
	mbedtls_des_context des_ctx1;
	mbedtls_des_context des_ctx2;

	mbedtls_des_init(&des_ctx1);
	mbedtls_des_init(&des_ctx2);

	mbedtls_des_setkey_enc(&des_ctx1, &DES_SECRET_KEY[0]);
	mbedtls_des_setkey_enc(&des_ctx2, &DES_SECRET_KEY[8]);

	mbedtls_des_crypt_ecb(&des_ctx1, &data[0], &data[0]);
	mbedtls_des_crypt_ecb(&des_ctx2, &data[8], &data[8]);

	mbedtls_des_free(&des_ctx1);
	mbedtls_des_free(&des_ctx2);
}

void PS5AuthUSBListener::startAuthentication()
{
	if (!ps5AuthData || !ps5AuthData->dongle_ready || ps5AuthData->ps5_auth_state != PS5State::ps5_no_auth)
		return;

	// Get the authentication data
	PS5DebugScreen::addMessage("GET AUTH");
	if (!tuh_hid_get_report(ps_dev_addr, ps_instance, 0x01, HID_REPORT_TYPE_INPUT, auth_buffer, 64))
	{
		PS5DebugScreen::addMessage("GET fail");
		awaiting_cb = false;
		return;
	}

	ps5AuthData->ps5_auth_state = PS5State::ps5_auth_requested;
	awaiting_cb = true;
}

bool PS5AuthUSBListener::requestAuthentication()
{
	// if (!dongle_ready)
	// {
	// 	PS5DebugScreen::addMessage("MAGS5 not ready");
	// 	return false;
	// }

	// // Reset to no_auth to allow startAuthentication to run
	// if (auth_state != PS5State::ps5_no_auth)
	// {
	// 	PS5DebugScreen::addMessage("Reset auth state");
	// 	auth_state = PS5State::ps5_no_auth;
	// 	awaiting_cb = false;
	// }

	// startAuthentication();
	return true;
}

void PS5AuthUSBListener::process()
{
	if (!ps5AuthData || !ps5AuthData->dongle_ready)
		return;

	if (awaiting_cb)
		return;

	switch (ps5AuthData->ps5_auth_state)
	{
	case PS5State::ps5_no_auth:
		startAuthentication();
		break;
	case PS5State::ps5_auth_requested:
		// Waiting for get_report_complete callback with auth data
		break;

	case PS5State::ps5_auth_complete:
		// PS5DebugScreen::addMessage("Auth complete!");
		// Authentication is complete, ready for key encryption
		// ps5AuthData->ps5_auth_state = PS5State::ps5_key_encryption_ready;
		// PS5DebugScreen::addMessage("Key enc ready");
		break;

	case PS5State::ps5_key_encryption_ready:

		break;

	default:
		break;
	}
}

// This is probably all wrong, havent made it work yet
bool PS5AuthUSBListener::sendKeysForEncryption(const uint8_t *keys_12byte, const uint8_t *battery_3byte, const uint8_t *touchpad_9byte)
{
	if (!ps5AuthData || !ps5AuthData->dongle_ready || ps5AuthData->ps5_auth_state != PS5State::ps5_key_encryption_ready || awaiting_cb)
	{
		return false;
	}

	// Byte0 = 0x02 (report ID)
	// Byte1 = 0x04 (key encryption data type)
	// Byte2 = 0x00
	// Byte3 = 0x18 (24 bytes length)
	// Byte4-15 = 12-byte keys
	// Byte16-18 = 3-byte battery info
	// Byte19-27 = 9-byte touchpad data
	memset(key_buffer, 0, sizeof(key_buffer));
	key_buffer[0] = 0x02;
	key_buffer[1] = 0x04;
	key_buffer[2] = 0x00;
	key_buffer[3] = 0x18; // 24 bytes
	memcpy(&key_buffer[4], keys_12byte, 12);
	memcpy(&key_buffer[16], battery_3byte, 3);
	memcpy(&key_buffer[19], touchpad_9byte, 9);

	if (!tuh_hid_set_report(ps_dev_addr, ps_instance, 0x02, HID_REPORT_TYPE_OUTPUT, key_buffer, 48))
	{
		PS5DebugScreen::addMessage("SEND FAIL!");
		return false;
	}

	// PS5DebugScreen::addMessage("SEND OK");

	awaiting_cb = true;
	ps5AuthData->keys_ready = false;
	return true;
}

bool PS5AuthUSBListener::getEncryptedKeys(uint8_t *keys_12byte, uint8_t *incount_4byte, uint8_t *aes_cmac_8byte)
{
	if (!ps5AuthData || !ps5AuthData->keys_ready)
		return false;

	memcpy(keys_12byte, ps5AuthData->encrypted_keys, 12);
	memcpy(incount_4byte, ps5AuthData->incount, 4);
	memcpy(aes_cmac_8byte, ps5AuthData->aes_cmac, 8);

	return true;
}

void PS5AuthUSBListener::mount(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len)
{
	if (ps5AuthData && ps5AuthData->dongle_ready == true)
	{
		return;
	}

	uint16_t vid, pid;
	if (!tuh_vid_pid_get(dev_addr, &vid, &pid))
	{
		PS5DebugScreen::addMessage("No VID/PID");
		return;
	}

	bool isPS5Dongle = false;

	// MAGS5 has same VID/PID as DualSense
	// We should ideally check some other identifier to distinguish from real DualSense, not sure how yet
	if (vid == 0x054C && pid == 0x0CE6)
	{
		isPS5Dongle = true;
		PS5DebugScreen::addMessage("MAGS5 VID/PID match");
	}

	if (!isPS5Dongle)
	{
		PS5DebugScreen::addMessage("Not MAGS5");
		return;
	}

	ps_dev_addr = dev_addr;
	ps_instance = instance;
	resetHostData();
	if (ps5AuthData)
	{
		ps5AuthData->dongle_ready = true;
	}
}

void PS5AuthUSBListener::unmount(uint8_t dev_addr)
{
	PS5DebugScreen::addMessage("MAGS5 disc");
	ps_dev_addr = 0xFF;
	ps_instance = 0xFF;
	resetHostData();
	if (ps5AuthData)
	{
		ps5AuthData->dongle_ready = false;
	}
}

void PS5AuthUSBListener::set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{
	if (!ps5AuthData || !ps5AuthData->dongle_ready || (dev_addr != ps_dev_addr))
	{
		return;
	}

	// Check which SET completed based on auth state
	if (ps5AuthData->ps5_auth_state == PS5State::ps5_auth_requested)
	{
		// Authentication data sent
		PS5DebugScreen::addMessage("Auth sent");
		awaiting_cb = false;
	}
	else if (ps5AuthData->ps5_auth_state == PS5State::ps5_auth_complete)
	{
		// Authentication encrypted data sent and ACKed
		// PS5DebugScreen::addMessage("Auth SUCCESS");
		awaiting_cb = false;
	}
	else if (ps5AuthData->ps5_auth_state == PS5State::ps5_key_encryption_ready)
	{
		awaiting_cb = false;
		return;
		// Key encryption data sent
		PS5DebugScreen::addMessage("Key enc sent");
		awaiting_cb = false;

		// Try GET_REPORT to retrieve the encrypted response (similar to authentication flow)
		if (!tuh_hid_get_report(ps_dev_addr, ps_instance, 0x02, HID_REPORT_TYPE_INPUT, key_buffer, 64))
		{
			PS5DebugScreen::addMessage("GET enc fail");
		}
		else
		{
			PS5DebugScreen::addMessage("GET enc req");
			awaiting_cb = true;
		}
	}
	else
	{
		awaiting_cb = false;
	}
}

void PS5AuthUSBListener::get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{
	if (!ps5AuthData || !ps5AuthData->dongle_ready || (dev_addr != ps_dev_addr) || (instance != ps_instance))
	{
		return;
	}

	awaiting_cb = false;
	char len_msg[20];
	snprintf(len_msg, sizeof(len_msg), "GET:%d", len);
	PS5DebugScreen::addMessage(len_msg);

	// Check which GET completed based on auth state
	if (ps5AuthData->ps5_auth_state == PS5State::ps5_auth_requested)
	{
		// First GET - check for authentication data: auth_buffer[7] = 0x08, auth_buffer[12] = 0x10
		if (len == 64 && auth_buffer[7] == 0x08 && auth_buffer[12] == 0x10)
		{
			PS5DebugScreen::addMessage("Auth Data OK");

			memcpy(ps5AuthData->auth_data_received, &auth_buffer[13], 16);
			memcpy(ps5AuthData->auth_data_encrypted, ps5AuthData->auth_data_received, 16);

			performDESEncryption(ps5AuthData->auth_data_encrypted);

			uint8_t set_buffer[48];
			memset(set_buffer, 0, sizeof(set_buffer));
			set_buffer[0] = 0x02;
			set_buffer[1] = 0x08;
			set_buffer[2] = 0x00;
			set_buffer[3] = 0x10;
			memcpy(&set_buffer[4], ps5AuthData->auth_data_encrypted, 16);

			if (len >= 48)
			{
				memcpy(&set_buffer[20], &auth_buffer[29], 19);
			}

			PS5DebugScreen::addMessage("Auth enc");
			if (!tuh_hid_set_report(ps_dev_addr, ps_instance, 0x02, HID_REPORT_TYPE_OUTPUT, set_buffer, 48))
			{
				PS5DebugScreen::addMessage("SET fail");
				return;
			}

			awaiting_cb = true;
			ps5AuthData->ps5_auth_state = PS5State::ps5_auth_complete;
		}
		else
		{
			PS5DebugScreen::addMessage("Bad auth!");
		}
	}
	else if (ps5AuthData->ps5_auth_state == PS5State::ps5_key_encryption_ready)
	{
		// GET encrypted keys response
		PS5DebugScreen::addMessage("Got enc keys");

		// Show bytes for debugging
		if (len >= 13)
		{
			char byte_msg[20];
			snprintf(byte_msg, sizeof(byte_msg), "B7:%02X B12:%02X", key_buffer[7], key_buffer[12]);
			PS5DebugScreen::addMessage(byte_msg);
		}

		// Check for key encryption response format
		if (len >= 37 && key_buffer[7] == 0x04 && key_buffer[12] == 0x28)
		{
			PS5DebugScreen::addMessage("Key OK GET");

			// Extract encrypted keys and related data
			memcpy(ps5AuthData->encrypted_keys, &key_buffer[13], 12);
			memcpy(ps5AuthData->incount, &key_buffer[25], 4);
			memcpy(ps5AuthData->aes_cmac, &key_buffer[29], 8);

			ps5AuthData->keys_ready = true;
			PS5DebugScreen::addMessage("Keys done");
		}
	}
}

void PS5AuthUSBListener::report_sent(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len)
{
	if (!ps5AuthData->dongle_ready || (dev_addr != ps_dev_addr) || (instance != ps_instance))
	{
		return;
	}

	// PS5DebugScreen::addMessage("Report sent");
	awaiting_cb = false;

	// After sending key encryption via interrupt OUT, wait for interrupt IN response
	// PS5DebugScreen::addMessage("Wait INT IN");
}

void PS5AuthUSBListener::report_received(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len)
{
	if (!ps5AuthData || !ps5AuthData->dongle_ready || (dev_addr != ps_dev_addr) || (instance != ps_instance))
	{
		return;
	}

	PS5DebugScreen::addMessage("*** RR CALLED ***");

	// Always log to see if interrupt endpoint is working at all
	char int_msg[20];
	snprintf(int_msg, sizeof(int_msg), "INT:%d st:%d", len, (int)ps5AuthData->ps5_auth_state);
	PS5DebugScreen::addMessage(int_msg);

	// Debug: Show first bytes if we have data
	if (len >= 13)
	{
		char byte_msg[20];
		snprintf(byte_msg, sizeof(byte_msg), "B7:%02X B12:%02X", report[7], report[12]);
		PS5DebugScreen::addMessage(byte_msg);
	}

	// Check for key encryption response
	// Byte 7 (index 7) = 0x04 (key encryption data type)
	// Byte 12 (index 12) = 0x28 (40 bytes length)
	// Bytes 13-24 (indices 13-24) = 12-byte encrypted keys
	// Bytes 25-28 (indices 25-28) = 4-byte incount
	// Bytes 29-36 (indices 29-36) = 8-byte AES CMAC
	if (ps5AuthData->ps5_auth_state == PS5State::ps5_key_encryption_ready && len >= 37)
	{
		if (report[7] == 0x04 && report[12] == 0x28)
		{
			PS5DebugScreen::addMessage("Key OK INT");

			// Extract encrypted keys and related data
			memcpy(ps5AuthData->encrypted_keys, &report[13], 12);
			memcpy(ps5AuthData->incount, &report[25], 4);
			memcpy(ps5AuthData->aes_cmac, &report[29], 8);

			ps5AuthData->keys_ready = true;
			PS5DebugScreen::addMessage("Keys done");
		}
		else
		{
			char bad_msg[20];
			snprintf(bad_msg, sizeof(bad_msg), "Bad:%02X %02X", report[7], report[12]);
			PS5DebugScreen::addMessage(bad_msg);
		}
	}

	// Re-queue the next transfer to keep receiving data
	// tuh_hid_receive_report(dev_addr, instance);
}
