/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015 Rockchip Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __DESIGNWARE_H__
#define __DESIGNWARE_H__

#include "udc.h"

struct usbdev_ctrl *dwc2_udc_init(device_descriptor_t *dd);

#endif
