#ifndef _MouseHost_H
#define _MouseHost_H

#include "gpaddon.h"

#ifndef MOUSE_HOST_ENABLED
#define MOUSE_HOST_ENABLED 0
#endif

#ifndef MOUSE_HOST_PIN_DPLUS
#define MOUSE_HOST_PIN_DPLUS -1
#endif

#ifndef MOUSE_HOST_PIN_5V
#define MOUSE_HOST_PIN_5V -1
#endif

// MouseHost Module Name
#define MouseHostName "MouseHost"

class MouseHostAddon : public GPAddon
{
public:
	virtual bool available();
	virtual void setup();			// MouseHost Setup
	virtual void process() {} // MouseHost Process
	virtual void preprocess();
	virtual void postprocess(bool sent);
	virtual void reinit() {}
	virtual std::string name() { return MouseHostName; }

private:
};

#endif // _MouseHost_H
