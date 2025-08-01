#pragma once

#include <stdint.h>
#include "tusb.h"

#define KEYBOARD_KEY_REPORT_ID 0x01
#define KEYBOARD_MULTIMEDIA_REPORT_ID 0x02
#define KEYBOARD_MOUSE_REPORT_ID 0x03

#define KEYBOARD_MULTIMEDIA_NEXT_TRACK  0XE8
#define KEYBOARD_MULTIMEDIA_PREV_TRACK  0XE9
#define KEYBOARD_MULTIMEDIA_STOP 	    0XF0
#define KEYBOARD_MULTIMEDIA_PLAY_PAUSE  0XF1
#define KEYBOARD_MULTIMEDIA_MUTE 	    0XF2
#define KEYBOARD_MULTIMEDIA_VOLUME_UP   0XF3
#define KEYBOARD_MULTIMEDIA_VOLUME_DOWN 0XF4

/// Standard HID Boot Protocol Keyboard Report.
typedef struct
{
	uint8_t reportId = KEYBOARD_KEY_REPORT_ID;
	uint8_t keycode[32]; /**< Key codes of the currently pressed keys. */
	uint8_t multimedia;
} KeyboardReport;

/// Standard HID Mouse Report.
typedef struct
{
	uint8_t reportId = KEYBOARD_MOUSE_REPORT_ID;
	uint8_t buttons;
	int8_t x;
	int8_t y;
	int8_t wheel;
} MouseReport;

static const uint8_t keyboard_string_language[]    = { 0x09, 0x04 };
static const uint8_t keyboard_string_manfacturer[] = "Open Stick Community";
static const uint8_t keyboard_string_product[]     = "GP2040-CE (Keyboard)";
static const uint8_t keyboard_mouse_string_product[] = "GP2040-CE (Keyboard+Mouse)";
static const uint8_t keyboard_string_version[]     = "1.1";

static const uint8_t *keyboard_string_descriptors[] __attribute__((unused)) =
{
	keyboard_string_language,
	keyboard_string_manfacturer,
	keyboard_string_product,
	keyboard_string_version
};

static const uint8_t *keyboard_mouse_string_descriptors[] __attribute__((unused)) =
{
	keyboard_string_language,
	keyboard_string_manfacturer,
	keyboard_mouse_string_product,
	keyboard_string_version
};

static const uint8_t keyboard_device_descriptor[] =
{
	sizeof(tusb_desc_device_t),	// bLength
	TUSB_DESC_DEVICE,			// bDescriptorType
	0x10, 0x01,					// bcdUSB
	0x00,						// bDeviceClass
	0x00,						// bDeviceSubClass
	0x00,						// bDeviceProtocol
	64,							// bMaxPacketSize0
	0xfe, 0xca,					// idVendor
	0x01, 0x00,					// idProduct
	0x00, 0x01,					// bcdDevice
	0x01,						// iManufacturer
	0x02,						// iProduct
	0x00,						// iSerialNumber
	0x01						// bNumConfigurations
};

enum
{
	ITF_NUM_HID_KEYBOARD,
	ITF_NUM_TOTAL_KEYBOARD
};

#define  CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

#define EPNUM_HID   0x81

static const uint8_t keyboard_report_descriptor[] =
	{
		0x05, 0x01, // Usage Page (Generic Desktop),
		0x09, 0x06, // Usage (Keyboard),
		0xA1, 0x01, 	// Collection (Application),

		// Report ID (1)
		0x85, KEYBOARD_KEY_REPORT_ID,
		// Keys
		0x05, 0x07,			 // Usage Page (Key Codes),
		0x19, 0x00,			 // Usage Minimum (0),
		0x2A, 0xFF, 0x00, 	 // Usage Maximum (255),
		0x15, 0x00,			 // Logical Minimum (0),
		0x25, 0x01,			 // Logical Maximum (1),
		0x75, 0x01,			 // Report Size (1),
		0x96, 0x00, 0x01,	 // Report Count (256),
		0x81, 0x02,			 // Input (Data, Variable, Absolute), Key byte (1-32)
		0xC0,			// End Collection
		0x05, 0x0C, //Usage Page (Consumer Devices)
		0x09, 0x01, //Usage (Consumer Control)
		0xA1, 0x01, 	//Collection (Application)
		
		//Report ID (2)
		0x85, KEYBOARD_MULTIMEDIA_REPORT_ID,
		0x05, 0x0C,			 //Usage Page (Consumer Devices)
		0x15, 0x00,			 //Logical Minimum (0)
		0x25, 0x01,			 //Logical Maximum (1)
		0x75, 0x01,			 //Report Size (1)
		0x95, 0x07,			 //Report Count (7)
		0x09, 0xB5,			 //Usage (Scan Next Track)
		0x09, 0xB6,			 //Usage (Scan Previous Track)
		0x09, 0xB7,			 //Usage (Stop)
		0x09, 0xCD,			 //Usage (Play/Pause)
		0x09, 0xE2,			 //Usage (Mute)
		0x09, 0xE9,			 //Usage (Volume Increment)
		0x09, 0xEA,			 //Usage (Volume Decrement)
		0x81, 0x02,			 //Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
		0x95, 0x01,			 //Report Count (1)
		0x81, 0x01,			 //Input (Const,Ary,Abs)
		0xC0,			//End Collection
};

// Composite Keyboard + Mouse HID Report Descriptor
static const uint8_t keyboard_mouse_report_descriptor[] =
{
	// Keyboard
	0x05, 0x01, // Usage Page (Generic Desktop),
	0x09, 0x06, // Usage (Keyboard),
	0xA1, 0x01, 	// Collection (Application),

	// Report ID (1)
	0x85, KEYBOARD_KEY_REPORT_ID,
	// Keys
	0x05, 0x07,			 // Usage Page (Key Codes),
	0x19, 0x00,			 // Usage Minimum (0),
	0x2A, 0xFF, 0x00, 	 // Usage Maximum (255),
	0x15, 0x00,			 // Logical Minimum (0),
	0x25, 0x01,			 // Logical Maximum (1),
	0x75, 0x01,			 // Report Size (1),
	0x96, 0x00, 0x01,	 // Report Count (256),
	0x81, 0x02,			 // Input (Data, Variable, Absolute), Key byte (1-32)
	0xC0,			// End Collection
	0x05, 0x0C, //Usage Page (Consumer Devices)
	0x09, 0x01, //Usage (Consumer Control)
	0xA1, 0x01, 	//Collection (Application)
	
	//Report ID (2)
	0x85, KEYBOARD_MULTIMEDIA_REPORT_ID,
	0x05, 0x0C,			 //Usage Page (Consumer Devices)
	0x15, 0x00,			 //Logical Minimum (0)
	0x25, 0x01,			 //Logical Maximum (1)
	0x75, 0x01,			 //Report Size (1)
	0x95, 0x07,			 //Report Count (7)
	0x09, 0xB5,			 //Usage (Scan Next Track)
	0x09, 0xB6,			 //Usage (Scan Previous Track)
	0x09, 0xB7,			 //Usage (Stop)
	0x09, 0xCD,			 //Usage (Play/Pause)
	0x09, 0xE2,			 //Usage (Mute)
	0x09, 0xE9,			 //Usage (Volume Increment)
	0x09, 0xEA,			 //Usage (Volume Decrement)
	0x81, 0x02,			 //Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
	0x95, 0x01,			 //Report Count (1)
	0x81, 0x01,			 //Input (Const,Ary,Abs)
	0xC0,			//End Collection

	// Mouse
	0x05, 0x01,        // Usage Page (Generic Desktop)
	0x09, 0x02,        // Usage (Mouse)
	0xA1, 0x01,        // Collection (Application)
	0x85, KEYBOARD_MOUSE_REPORT_ID, // Report ID (3)
	0x09, 0x01,        //   Usage (Pointer)
	0xA1, 0x00,        //   Collection (Physical)
	0x05, 0x09,        //     Usage Page (Button)
	0x19, 0x01,        //     Usage Minimum (0x01)
	0x29, 0x03,        //     Usage Maximum (0x03)
	0x15, 0x00,        //     Logical Minimum (0)
	0x25, 0x01,        //     Logical Maximum (1)
	0x95, 0x03,        //     Report Count (3)
	0x75, 0x01,        //     Report Size (1)
	0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x95, 0x01,        //     Report Count (1)
	0x75, 0x05,        //     Report Size (5)
	0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
	0x09, 0x30,        //     Usage (X)
	0x09, 0x31,        //     Usage (Y)
	0x15, 0x81,        //     Logical Minimum (-127)
	0x25, 0x7F,        //     Logical Maximum (127)
	0x75, 0x08,        //     Report Size (8)
	0x95, 0x02,        //     Report Count (2)
	0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
	0x09, 0x38,        //     Usage (Wheel)
	0x95, 0x01,        //     Report Count (1)
	0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
	0xC0,              //   End Collection
	0xC0,              // End Collection
};

// Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
static const uint8_t keyboard_hid_descriptor[] =
{
	0x09,								 	  // bLength
	0x21,								 	  // bDescriptorType (HID)
	0x11, 0x01,							 	  // bcdHID 1.11
	0x00,								 	  // bCountryCode
	0x01,									  // bNumDescriptors
	0x22,									  // bDescriptorType[0] (HID)
	sizeof(keyboard_report_descriptor), 0x00, // wDescriptorLength[0] 90
};

static const uint8_t keyboard_configuration_descriptor[] =
{
	// Config number, interface count, string index, total length, attribute, power in mA
	TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL_KEYBOARD, 0, CONFIG_TOTAL_LEN, 32, 100),

	// Interface number, string index, protocol, report descriptor len, EP Out & In address, size & polling interval
	TUD_HID_DESCRIPTOR(ITF_NUM_HID_KEYBOARD, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(keyboard_report_descriptor), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 1)
};

// Composite Keyboard + Mouse Configuration Descriptor
static const uint8_t keyboard_mouse_configuration_descriptor[] =
{
	// Config number, interface count, string index, total length, attribute, power in mA
	TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL_KEYBOARD, 0, CONFIG_TOTAL_LEN, 32, 100),

	// Interface number, string index, protocol, report descriptor len, EP Out & In address, size & polling interval
	TUD_HID_DESCRIPTOR(ITF_NUM_HID_KEYBOARD, 0, HID_ITF_PROTOCOL_NONE, sizeof(keyboard_mouse_report_descriptor), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 1)
};