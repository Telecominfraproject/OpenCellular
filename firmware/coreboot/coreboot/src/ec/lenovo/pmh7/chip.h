/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2011 Sven Schnelle <svens@stackframe.org>
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

#ifndef EC_LENOVO_PMH7_CHIP_H
#define EC_LENOVO_PMH7_CHIP_H

struct ec_lenovo_pmh7_config {
	int backlight_enable:1;
	int dock_event_enable:1;
};

#endif /* EC_LENOVO_PMH7_CHIP_H */
