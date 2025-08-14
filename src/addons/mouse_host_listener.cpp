#include "addons/mouse_host_listener.h"
#include "storagemanager.h"
#include "drivermanager.h"
#include "class/hid/hid_host.h"

void MouseHostListener::setup() {}

void MouseHostListener::mount(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len) {
	uint8_t protocol = tuh_hid_interface_protocol(dev_addr, instance);
	if (protocol == HID_ITF_PROTOCOL_MOUSE) {
		_mouse_host_mounted = true;
		_mouse_dev_addr = dev_addr;
		_mouse_instance = instance;
	} else {
		_mouse_host_mounted = false;
	}
}

void MouseHostListener::unmount(uint8_t dev_addr) {
	if (_mouse_host_mounted && dev_addr == _mouse_dev_addr) {
		_mouse_host_mounted = false;
		_mouse_dev_addr = 0;
		_mouse_instance = 0;
		mouseActive = false;
		mouseX = 0;
		mouseY = 0;
		mouseZ = 0;
		hasNewMovementData = false;
		mouseLeftButton = false;
		mouseRightButton = false;
		mouseMiddleButton = false;
	}
}

void MouseHostListener::report_received(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
	if (!_mouse_host_mounted) return;
	if (dev_addr != _mouse_dev_addr || instance != _mouse_instance) return;

	uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
	if (itf_protocol == HID_ITF_PROTOCOL_MOUSE) {
		hid_mouse_report_t const *mouse_report = (hid_mouse_report_t const *)report;
		mouseLeftButton = (mouse_report->buttons & MOUSE_BUTTON_LEFT) != 0;
		mouseRightButton = (mouse_report->buttons & MOUSE_BUTTON_RIGHT) != 0;
		mouseMiddleButton = (mouse_report->buttons & MOUSE_BUTTON_MIDDLE) != 0;
		mouseX = mouse_report->x;
		mouseY = mouse_report->y;
		mouseZ = mouse_report->wheel;
		hasNewMovementData = (mouseX != 0 || mouseY != 0 || mouseZ != 0);
		mouseActive = (hasNewMovementData || mouseLeftButton || mouseRightButton || mouseMiddleButton);
	}
}

void MouseHostListener::process() {
	Gamepad *gamepad = Storage::getInstance().GetGamepad();

	if (_mouse_host_mounted && gamepad) {
		gamepad->auxState.sensors.mouse.position.enabled = true;

		gamepad->auxState.sensors.mouse.leftButton = mouseLeftButton;
		gamepad->auxState.sensors.mouse.rightButton = mouseRightButton;
		gamepad->auxState.sensors.mouse.middleButton = mouseMiddleButton;

		if (hasNewMovementData) {
			gamepad->auxState.sensors.mouse.position.x = mouseX;
			gamepad->auxState.sensors.mouse.position.y = mouseY;
			gamepad->auxState.sensors.mouse.position.z = mouseZ;
		}

		gamepad->auxState.sensors.mouse.position.active = mouseActive;
	}
}

void MouseHostListener::clearMovementData() {
	mouseX = 0;
	mouseY = 0;
	mouseZ = 0;
	hasNewMovementData = false;

	Gamepad *gamepad = Storage::getInstance().GetGamepad();
	if (gamepad) {
		gamepad->auxState.sensors.mouse.position.x = 0;
		gamepad->auxState.sensors.mouse.position.y = 0;
		gamepad->auxState.sensors.mouse.position.z = 0;
	}

	mouseActive = (mouseLeftButton || mouseRightButton || mouseMiddleButton);
}
