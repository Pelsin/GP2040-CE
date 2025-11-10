#include "host/usbh.h"
#include "class/hid/hid.h"
#include "class/hid/hid_host.h"
#include "drivers/ps5/PS5Auth.h"
#include "drivers/ps5/PS5AuthUSBListener.h"
#include "peripheralmanager.h"
#include "storagemanager.h"
#include "usbhostmanager.h"

#include "enums.pb.h"

void PS5Auth::initialize() {
    if (!available()) {
        return;
    }

    listener = nullptr;
    ps5AuthData.dongle_ready = false;
    ps5AuthData.passthrough_state = GPAuthState::auth_idle_state;
    ps5AuthData.ps5_auth_state = PS5State::ps5_no_auth;
    ps5AuthData.keys_ready = false;

    memset(ps5AuthData.auth_data_received, 0, sizeof(ps5AuthData.auth_data_received));
    memset(ps5AuthData.auth_data_encrypted, 0, sizeof(ps5AuthData.auth_data_encrypted));
    memset(ps5AuthData.encrypted_keys, 0, sizeof(ps5AuthData.encrypted_keys));
    memset(ps5AuthData.incount, 0, sizeof(ps5AuthData.incount));
    memset(ps5AuthData.aes_cmac, 0, sizeof(ps5AuthData.aes_cmac));

    if (authType == InputModeAuthType::INPUT_MODE_AUTH_TYPE_USB) {
        listener = new PS5AuthUSBListener();
        ((PS5AuthUSBListener*)listener)->setup();
        ((PS5AuthUSBListener*)listener)->setAuthData(&ps5AuthData);
    }
    // Note: PS5 doesn't have a key-based mode like PS4, only USB dongle mode
}

bool PS5Auth::available() {
    if (authType == InputModeAuthType::INPUT_MODE_AUTH_TYPE_USB) {
        return (PeripheralManager::getInstance().isUSBEnabled(0));
    }
    return false;
}

void PS5Auth::process() {
    if (authType == InputModeAuthType::INPUT_MODE_AUTH_TYPE_USB) {
        ((PS5AuthUSBListener*)listener)->process();
    }
}

void PS5Auth::resetAuth() {
    if (authType == InputModeAuthType::INPUT_MODE_AUTH_TYPE_USB) {
        ((PS5AuthUSBListener*)listener)->resetHostData();
    }
    ps5AuthData.passthrough_state = GPAuthState::auth_idle_state;
    ps5AuthData.ps5_auth_state = PS5State::ps5_no_auth;
    ps5AuthData.keys_ready = false;
}

bool PS5Auth::sendKeysForEncryption(const uint8_t* keys_12byte, const uint8_t* battery_3byte, const uint8_t* touchpad_9byte) {
    if (authType == InputModeAuthType::INPUT_MODE_AUTH_TYPE_USB && listener != nullptr) {
        return ((PS5AuthUSBListener*)listener)->sendKeysForEncryption(keys_12byte, battery_3byte, touchpad_9byte);
    }
    return false;
}
