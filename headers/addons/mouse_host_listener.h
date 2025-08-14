#ifndef _MouseHostListener_H
#define _MouseHostListener_H

#include "usblistener.h"
#include "gamepad.h"
#include "class/hid/hid.h"

class MouseHostListener : public USBListener
{
public:
	virtual void setup();
	virtual void mount(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len);
	virtual void xmount(uint8_t dev_addr, uint8_t instance, uint8_t controllerType, uint8_t subtype) {}
	virtual void unmount(uint8_t dev_addr);
	virtual void report_received(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len);
	virtual void report_sent(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {}
	virtual void process();
	virtual void set_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) {}
	virtual void get_report_complete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) {}
	virtual void set_report(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint8_t const *report, uint16_t len) {}
	virtual void clearMovementData();
	private:
	bool _mouse_host_mounted = false;
	uint8_t _mouse_dev_addr = 0;
	uint8_t _mouse_instance = 0;

	bool mouseActive = false;
	int16_t mouseX = 0;
	int16_t mouseY = 0;
	int16_t mouseZ = 0;
	bool mouseLeftButton = false;
	bool mouseRightButton = false;
	bool mouseMiddleButton = false;

	bool hasNewMovementData = false;
};

#endif // _MouseHostListener_H
