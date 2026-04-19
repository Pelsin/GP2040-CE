#include "addons/airbar_host.h"
#include "addons/airbar_host_listener.h"
#include "storagemanager.h"
#include "peripheralmanager.h"
#include "usbhostmanager.h"

bool AirBarHostAddon::available() {
    return PeripheralManager::getInstance().isUSBEnabled(0);
}

void AirBarHostAddon::setup() {
    listener = new AirBarHostListener();
    ((AirBarHostListener*)listener)->setup();
}

void AirBarHostAddon::preprocess() {
    ((AirBarHostListener*)listener)->process();
}
