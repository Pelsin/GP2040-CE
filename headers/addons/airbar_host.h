#ifndef _AirBarHost_H
#define _AirBarHost_H

#include "gpaddon.h"

#ifndef AIRBAR_HOST_ENABLED
#define AIRBAR_HOST_ENABLED 0
#endif

// AirBarHost Module Name
#define AirBarHostName "AirBarHost"

class AirBarHostAddon : public GPAddon {
public:
    virtual bool available();
    virtual void setup();
    virtual void process() {}
    virtual void preprocess();
    virtual void postprocess(bool sent) {}
    virtual void reinit() {}
    virtual std::string name() { return AirBarHostName; }
private:
};

#endif  // _AirBarHost_H
