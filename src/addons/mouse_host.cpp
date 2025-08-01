#include "addons/mouse_host.h"
#include "addons/mouse_host_listener.h"
#include "storagemanager.h"
#include "drivermanager.h"
#include "usbhostmanager.h"
#include "peripheralmanager.h"
#include "class/hid/hid_host.h"
#include "pio_usb.h"

bool MouseHostAddon::available()
{
	// Check if mouse passthrough is enabled for keyboard input mode
	const GamepadOptions &options = Storage::getInstance().getGamepadOptions();

	// Enable mouse host addon when:
	// 1. Mouse passthrough is enabled AND
	// 2. We're in keyboard input mode AND
	// 3. USB is enabled
	return options.keyboardModeMousePassthrough &&
				 (options.inputMode == INPUT_MODE_KEYBOARD) &&
				 PeripheralManager::getInstance().isUSBEnabled(0);
}

void MouseHostAddon::setup()
{
	listener = new MouseHostListener();
	((MouseHostListener *)listener)->setup();
}

void MouseHostAddon::preprocess()
{
	((MouseHostListener *)listener)->process();
}

void MouseHostAddon::postprocess(bool sent)
{
	// Only clear movement data if a HID report was actually sent
	if (sent)
	{
		((MouseHostListener *)listener)->clearMovementData();
	}
}
