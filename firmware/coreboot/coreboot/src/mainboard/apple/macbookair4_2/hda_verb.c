/*
 * This file is part of the coreboot project.
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

#include <device/azalia_device.h>

const u32 cim_verb_data[] = {
	0x10134206, /* Codec Vendor / Device ID: Cirrus */
	0x106b5b00, /* Subsystem ID */

	0x0000000b, /* Number of 4 dword sets */
	/* NID 0x01: Subsystem ID.  */
	AZALIA_SUBVENDOR(0x0, 0x106b5b00),

	/* NID 0x09.  */
	AZALIA_PIN_CFG(0x0, 0x09, 0x012b4030),

	/* NID 0x0a.  */
	AZALIA_PIN_CFG(0x0, 0x0a, 0x400000f0),

	/* NID 0x0b.  */
	AZALIA_PIN_CFG(0x0, 0x0b, 0x90100120),

	/* NID 0x0c.  */
	AZALIA_PIN_CFG(0x0, 0x0c, 0x400000f0),

	/* NID 0x0d.  */
	AZALIA_PIN_CFG(0x0, 0x0d, 0x90a00110),

	/* NID 0x0e.  */
	AZALIA_PIN_CFG(0x0, 0x0e, 0x400000f0),

	/* NID 0x0f.  */
	AZALIA_PIN_CFG(0x0, 0x0f, 0x400000f0),

	/* NID 0x10.  */
	AZALIA_PIN_CFG(0x0, 0x10, 0x400000f0),

	/* NID 0x12.  */
	AZALIA_PIN_CFG(0x0, 0x12, 0x400000f0),

	/* NID 0x15.  */
	AZALIA_PIN_CFG(0x0, 0x15, 0x400000f0),
	0x80862805, /* Codec Vendor / Device ID: Intel */
	0x80860101, /* Subsystem ID */

	0x00000004, /* Number of 4 dword sets */
	/* NID 0x01: Subsystem ID.  */
	AZALIA_SUBVENDOR(0x3, 0x80860101),

	/* NID 0x05.  */
	AZALIA_PIN_CFG(0x3, 0x05, 0x18560010),

	/* NID 0x06.  */
	AZALIA_PIN_CFG(0x3, 0x06, 0x18560010),

	/* NID 0x07.  */
	AZALIA_PIN_CFG(0x3, 0x07, 0x18560010),
};

const u32 pc_beep_verbs[0] = {};

AZALIA_ARRAY_SIZES;
