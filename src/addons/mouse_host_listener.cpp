#include "addons/mouse_host_listener.h"
#include "storagemanager.h"
#include "class/hid/hid.h"
#include "pico/time.h"

// Static instance reference for clearing movement data
static MouseHostListener *staticInstance = nullptr;

void MouseHostListener::setup() {
	// Setup is called automatically when the USB addon is loaded
	staticInstance = this; // Store reference for static access
}

void MouseHostListener::mount(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len) {
	// Assume any device is a mouse - we'll filter in report_received
	_mouse_host_mounted = true;
}

void MouseHostListener::unmount(uint8_t dev_addr) {
	_mouse_host_mounted = false;
	mouseActive = false;
	mouseX = 0;
	mouseY = 0;
	mouseZ = 0;
	hasNewMovementData = false;
	mouseLeftButton = false;
	mouseRightButton = false;
	mouseMiddleButton = false;
}

void MouseHostListener::report_received(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
	if (!_mouse_host_mounted)
		return;

	// Validate this looks like a mouse report
	if (len >= sizeof(hid_mouse_report_t)) {
		hid_mouse_report_t const *mouse_report = (hid_mouse_report_t const *)report;

		// Update button states immediately (these persist)
		mouseLeftButton = (mouse_report->buttons & MOUSE_BUTTON_LEFT) != 0;
		mouseRightButton = (mouse_report->buttons & MOUSE_BUTTON_RIGHT) != 0;
		mouseMiddleButton = (mouse_report->buttons & MOUSE_BUTTON_MIDDLE) != 0;

		// Update movement data - these are deltas that should only be sent once
		mouseX = (int16_t)mouse_report->x;
		mouseY = (int16_t)mouse_report->y;
		mouseZ = (int16_t)mouse_report->wheel;
		hasNewMovementData = (mouseX != 0 || mouseY != 0 || mouseZ != 0);

		// Set active flag if there's actual movement or button activity
		mouseActive = (hasNewMovementData || mouseLeftButton || mouseRightButton || mouseMiddleButton);
	}
}

void MouseHostListener::process() {
	Gamepad *gamepad = Storage::getInstance().GetGamepad();

	if (_mouse_host_mounted && gamepad) {
		// Always enable when mounted
		gamepad->auxState.sensors.mouse.position.enabled = true;

		// Button states persist (these don't need clearing)
		gamepad->auxState.sensors.mouse.leftButton = mouseLeftButton;
		gamepad->auxState.sensors.mouse.rightButton = mouseRightButton;
		gamepad->auxState.sensors.mouse.middleButton = mouseMiddleButton;

		// Only set movement data if there's actually new movement data
		if (hasNewMovementData) {
			gamepad->auxState.sensors.mouse.position.x = mouseX;
			gamepad->auxState.sensors.mouse.position.y = mouseY;
			gamepad->auxState.sensors.mouse.position.z = mouseZ;
		}

		// Always update the active state
		gamepad->auxState.sensors.mouse.position.active = mouseActive;
	}
}

void MouseHostListener::clearMovementData() {
	mouseX = 0;
	mouseY = 0;
	mouseZ = 0;
	hasNewMovementData = false;

	// Also clear the gamepad state so it doesn't persist
	Gamepad *gamepad = Storage::getInstance().GetGamepad();
	if (gamepad) {
		gamepad->auxState.sensors.mouse.position.x = 0;
		gamepad->auxState.sensors.mouse.position.y = 0;
		gamepad->auxState.sensors.mouse.position.z = 0;
	}

	// Recalculate active state based on current button states
	mouseActive = (mouseLeftButton || mouseRightButton || mouseMiddleButton);
}

void MouseHostListener::clearMovementDataStatic() {
	if (staticInstance) {
		staticInstance->clearMovementData();
	}
}
