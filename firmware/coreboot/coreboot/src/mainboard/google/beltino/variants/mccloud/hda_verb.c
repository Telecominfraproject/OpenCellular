/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef HDA_VERB_H
#define HDA_VERB_H

#include <stdlib.h>

#include <device/azalia_device.h>

const u32 cim_verb_data[] = {
	/* coreboot specific header */
	0x10ec0283,	// Codec Vendor / Device ID: Realtek ALC283
	0x10ec0283,	// Subsystem ID
	0x0000000e,	// Number of jacks (NID entries)

	0x0017ff00,	// Function Reset
	0x0017ff00,	// Double Function Reset
	0x000F0000,	// Pad - get vendor id
	0x000F0002,	// Pad - get revision id

	/* NID 0x01, HDA Codec Subsystem ID Verb Table */
	AZALIA_SUBVENDOR(0x0, 0x11790670),

	/* Pin Widget Verb Table */

	/* Pin Complex (NID 0x12) DMIC - Disabled */
	AZALIA_PIN_CFG(0x0, 0x12, 0x411111f0),

	/* Pin Complex (NID 0x14) SPKR-OUT PORTD - Disabled */
	AZALIA_PIN_CFG(0x0, 0x14, 0x401111f0),

	/* Pin Complex (NID 0x17) MONO Out - Disabled */
	AZALIA_PIN_CFG(0x0, 0x17, 0x411111f0),

	/* Pin Complex (NID 0x18) MIC1 PORTB */
	// group 1, cap 1
	// black, jack detect
	// mic in, analog
	// connector, left panel
	AZALIA_PIN_CFG(0x0, 0x19, 0x03a71011),

	/* Pin Complex (NID 0x19) MIC2 PORTF - Disabled */
	AZALIA_PIN_CFG(0x0, 0x18, 0x411111f0),

	/* Pin Complex (NID 0x1A) LINE1 PORTC - Disabled */
	AZALIA_PIN_CFG(0x0, 0x1a, 0x411111f0),

	/* Pin Complex (NID 0x1B) LINE2 PORTE - Disabled */
	AZALIA_PIN_CFG(0x0, 0x1a, 0x411111f0),

	/* Pin Complex (NID 0x1d) PCBeep */
	// eapd low on ex-amp, laptop, custom enable
	// mute spkr on hpout
	// pcbeep en able, checksum
	// no physical, internal
	AZALIA_PIN_CFG(0x0, 0x1d, 0x4015812d),

	/* Pin Complex (NID 0x1E) SPDIF-OUT - Disabled */
	AZALIA_PIN_CFG(0x0, 0x1e, 0x411111f0),

	/* Pin Complex (NID 0x21) HPOUT PORT-I */
	// group1,
	// black, jack detect
	// HPOut, 1/8 stereo
	// connector, left panel
	AZALIA_PIN_CFG(0x0, 0x21, 0x0321101f),

	/* Undocumented settings from Realtek (needed for beep_gen) */
	/* Widget node 0x20 */
	0x02050010,
	0x02040c20,
	0x0205001b,
	0x0204081b,

	/* Tuned jack detection */
	0x02050043,
	0x0204A614,
	0x02050047,
	0x02049470,
};

const u32 pc_beep_verbs[] = {
	0x00170500,	/* power up everything (codec, dac, adc, mixers)  */
	0x01470740,	/* enable speaker out */
	0x01470c02,	/* set speaker EAPD pin */
	0x0143b01f,	/* unmute speaker */
	0x00c37100,	/* unmute mixer nid 0xc input 1 */
	0x00b37410,	/* unmute mixer nid 0xb beep input and set volume */
};


#endif /* HDA_VERB_H */
AZALIA_ARRAY_SIZES;
