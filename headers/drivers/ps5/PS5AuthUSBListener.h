#ifndef _PS5AUTHUSBLISTENER_H_
#define _PS5AUTHUSBLISTENER_H_

#include "usblistener.h"
#include "drivers/ps5/PS5Auth.h"

// Report IDs for PS5 authentication (based on MAGS5 protocol)
#define PS5_FEATURE_REPORT_GET 0xF8 // Feature report for getting auth data
#define PS5_FEATURE_REPORT_SET 0xF9 // Feature report for sending auth data

// Data type indicators in report payload (Byte1/Byte7)
typedef enum
{
	PS5_AUTH_DATA_TYPE = 0x08,		 // Authentication data type
	PS5_KEY_ENCRYPTION_TYPE = 0x04 // Key encryption data type
} PS5AuthDataType;

// Forward declaration
struct PS5AuthData;

class PS5AuthUSBListener : public USBListener
{
public:
	virtual void setup();
	virtual void mount(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len);
	virtual void xmount(uint8_t dev_addr, uint8_t instance, uint8_t controllerType, uint8_t subtype) {}
	virtual void unmount(uint8_t dev_addr);
	virtual void report_received(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len);
	virtual void report_sent(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len);
	virtual void set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len);
	virtual void get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len);
	void process(); // process authentication and key encryption
	void resetHostData();
	void setAuthData(PS5AuthData *authData) { ps5AuthData = authData; }

	// Trigger authentication process
	void startAuthentication();

	// Request authentication from MAGS5 (called when PS5 requests it from us)
	bool requestAuthentication();

	// Check if dongle is ready and authenticated
	bool isReady() const { return ps5AuthData && ps5AuthData->dongle_ready && ps5AuthData->ps5_auth_state == PS5State::ps5_key_encryption_ready; }

	// Check if dongle is mounted
	bool isMounted() const { return ps5AuthData && ps5AuthData->dongle_ready; }

	// Get encrypted key data for sending to console
	bool getEncryptedKeys(uint8_t *keys_12byte, uint8_t *incount_4byte, uint8_t *aes_cmac_8byte);

	// Send key data to device for encryption
	bool sendKeysForEncryption(const uint8_t *keys_12byte, const uint8_t *battery_3byte, const uint8_t *touchpad_9byte);

	// Handle direct endpoint transfer completion (called from C callback)
	void handleTransferComplete(uint8_t ep_addr, uint32_t xferred_bytes);

	// Test DES encryption with known data
	void testDESEncryption();

	// Public access to buffers for direct transfers (needed by C callback)
	uint8_t *getAuthBuffer() { return auth_buffer; }
	uint8_t getDevAddr() const { return ps_dev_addr; }
	uint8_t getInstance() const { return ps_instance; }
	uint8_t getEpIn() const { return ep_in; }
	void setAwaitingCb(bool value) { awaiting_cb = value; }

private:
	void performDESEncryption(uint8_t *data);
	bool host_get_report(uint8_t report_id, void *report, uint16_t len);
	bool host_set_report(uint8_t report_id, void *report, uint16_t len);

	uint8_t ps_dev_addr;			// TinyUSB Address (USB)
	uint8_t ps_instance;			// TinyUSB Instance (USB)
	uint8_t ep_in;						// Interrupt IN endpoint address
	uint8_t ep_out;						// Interrupt OUT endpoint address
	uint8_t itf_num;					// Interface number
	PS5AuthData *ps5AuthData; // PS5 Authentication Data (shared with PS5Auth)
	bool awaiting_cb;					// Waiting for callback

	// Buffers for PS5 authentication
	uint8_t auth_buffer[64]; // Buffer for authentication data
	uint8_t key_buffer[64];	 // Buffer for key encryption
};

#endif // _PS5AUTHUSBLISTENER_H_
