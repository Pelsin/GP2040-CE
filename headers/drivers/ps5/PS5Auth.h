#ifndef _PS5AUTH_H_
#define _PS5AUTH_H_

#include "drivers/shared/gpauthdriver.h"

// PS5 Authentication State Machine
typedef enum
{
	ps5_no_auth = 0,
	ps5_auth_requested = 1,
	ps5_auth_complete = 2,
	ps5_key_encryption_ready = 3
} PS5State;

// PS5 Auth Data in a single struct
typedef struct PS5AuthData
{
	bool dongle_ready;						 // MAGS5/TGS dongle detected and ready
	GPAuthState passthrough_state; // PS5 Encryption Passthrough State
	PS5State ps5_auth_state;			 // PS5-specific authentication state

	// Authentication data
	uint8_t auth_data_received[16];	 // 16-byte data from device
	uint8_t auth_data_encrypted[16]; // 16-byte encrypted data
	uint8_t auth_buffer[64];				 // Full 64-byte auth buffer from MAGS5 dongle

	// Encrypted key data from device
	uint8_t encrypted_keys[12]; // 12-byte encrypted keys
	uint8_t incount[4];					// 4-byte incount
	uint8_t aes_cmac[8];				// 8-byte AES CMAC
	bool keys_ready;						// Flag indicating encrypted keys are ready
} PS5AuthData;

class PS5Auth : public GPAuthDriver
{
public:
	PS5Auth(InputModeAuthType inType) { authType = inType; }
	virtual void initialize();
	virtual bool available();
	void process();
	PS5AuthData *getAuthData() { return &ps5AuthData; }
	void resetAuth();

	// Helper method to send keys for encryption (delegates to listener)
	bool sendKeysForEncryption(const uint8_t *keys_12byte, const uint8_t *battery_3byte, const uint8_t *touchpad_9byte);

private:
	PS5AuthData ps5AuthData;
};

#endif
