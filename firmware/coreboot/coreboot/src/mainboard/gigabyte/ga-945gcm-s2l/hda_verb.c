/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2016 Arthur Heymans <arthur@aheymans.xyz>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <device/azalia_device.h>

const u32 cim_verb_data[] = {
	/* coreboot specific header */
	0x10ec0662,     /* Vendor ID */
	0x1458a002,     /* Subsystem ID */
	0x00000009,     /* Number of entries */

	/* Pin Widget Verb Table */
	AZALIA_PIN_CFG(0, 0x14, 0x01014010),
	AZALIA_PIN_CFG(0, 0x15, 0x411111f0),
	AZALIA_PIN_CFG(0, 0x16, 0x411111f0),
	AZALIA_PIN_CFG(0, 0x18, 0x01a19830),
	AZALIA_PIN_CFG(0, 0x19, 0x02a19c31),
	AZALIA_PIN_CFG(0, 0x1a, 0x0181303f),
	AZALIA_PIN_CFG(0, 0x1b, 0x02214c1f),
	AZALIA_PIN_CFG(0, 0x1c, 0x593301f0),
	AZALIA_PIN_CFG(0, 0x1d, 0x4005c603),
	AZALIA_PIN_CFG(0, 0x1e, 0x014b6120),
};

const u32 pc_beep_verbs[0] = {};
AZALIA_ARRAY_SIZES;
