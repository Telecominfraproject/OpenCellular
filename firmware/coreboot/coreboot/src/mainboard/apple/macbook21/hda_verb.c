/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Vladimir Serbinenko.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <device/azalia_device.h>

const u32 cim_verb_data[] = {
	/* coreboot specific header */
	0x83847680,	/* Codec Vendor / Device ID: SigmaTel STAC9221 A1 */
#if IS_ENABLED(CONFIG_BOARD_APPLE_MACBOOK11) || \
	IS_ENABLED(CONFIG_BOARD_APPLE_MACBOOK21)
	0x106b2200,	/* Subsystem ID  */
	0x0000000B,	/* Number of 4 dword sets */

	/* NID 0x01: Subsystem ID.  */
	AZALIA_SUBVENDOR(0x0, 0x106B2200),

	/* NID 0x0A.  */
	AZALIA_PIN_CFG(0x0, 0x0A, 0x0321E21F),

	/* NID 0x0B.  */
	AZALIA_PIN_CFG(0x0, 0x0B, 0x03A1E02E),

	/* NID 0x0C.  */
	AZALIA_PIN_CFG(0x0, 0x0C, 0x9017E110),

	/* NID 0x0D.  */
	AZALIA_PIN_CFG(0x0, 0x0D, 0x9017E11F),

	/* NID 0x0E.  */
	AZALIA_PIN_CFG(0x0, 0x0E, 0x400000FE),

	/* NID 0x0F  */
	AZALIA_PIN_CFG(0x0, 0x0F, 0x0381E020),

	/* NID 0x10  */
	AZALIA_PIN_CFG(0x0, 0x10, 0x1345E230),

	/* NID 0x11  */
	AZALIA_PIN_CFG(0x0, 0x11, 0x13C5E240),

	/* NID 0x15  */
	AZALIA_PIN_CFG(0x0, 0x15, 0x400000FC),

	/* NID 0x1B.  */
	AZALIA_PIN_CFG(0x0, 0x1B, 0x400000FB),
#else /* CONFIG_BOARD_APPLE_IMAC52 */
	0x106b0f00,	/* Subsystem ID  */
	0x0000000b,	/* Number of 4 dword sets */

	/* NID 0x01: Subsystem ID.  */
	AZALIA_SUBVENDOR(0x0, 0x106b0f00),

	/* NID 0x0A.  */
	AZALIA_PIN_CFG(0x0, 0x0a, 0x012be032),

	/* NID 0x0B.  */
	AZALIA_PIN_CFG(0x0, 0x0b, 0x90afe111),

	/* NID 0x0C.  */
	AZALIA_PIN_CFG(0x0, 0x0c, 0x9017e131),

	/* NID 0x0D.  */
	AZALIA_PIN_CFG(0x0, 0x0d, 0x4080e10f),

	/* NID 0x0E.  */
	AZALIA_PIN_CFG(0x0, 0x0e, 0x40f0e00f),

	/* NID 0x0F  */
	AZALIA_PIN_CFG(0x0, 0x0f, 0x018be021),

	/* NID 0x10  */
	AZALIA_PIN_CFG(0x0, 0x10, 0x114bf033),

	/* NID 0x11  */
	AZALIA_PIN_CFG(0x0, 0x11, 0x11cbc022),

	/* NID 0x15  */
	AZALIA_PIN_CFG(0x0, 0x15, 0x4080e10f),

	/* NID 0x1B.  */
	AZALIA_PIN_CFG(0x0, 0x1b, 0x4080e10f),
#endif

};

const u32 pc_beep_verbs[0] = {};

AZALIA_ARRAY_SIZES;
