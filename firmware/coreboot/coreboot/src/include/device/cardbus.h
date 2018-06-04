/* (c) 2005 Linux Networx GPL see COPYING for details */

#ifndef DEVICE_CARDBUS_H
#define DEVICE_CARDBUS_H

#include <device/device.h>

void cardbus_read_resources(device_t dev);
void cardbus_enable_resources(device_t dev);

extern struct device_operations default_cardbus_ops_bus;

#endif /* DEVICE_CARDBUS_H */
