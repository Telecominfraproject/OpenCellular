/*
 * This file is part of the coreboot project.
 *
 * Copyright 2015 Google, Inc.
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

#ifndef _SOC_NHLT_H_
#define _SOC_NHLT_H_

#include <nhlt.h>

#define NHLT_VID 0x8086
#define NHLT_DID_DMIC 0xae20
#define NHLT_DID_BT 0xae30
#define NHLT_DID_SSP 0xae34

/*
 * Skylake NHLT link types. These values are to be used for the hwlink
 * fields in the functions below to specify which link a device is on.
 */

enum {
	AUDIO_LINK_SSP0,
	AUDIO_LINK_SSP1,
	AUDIO_LINK_SSP2, /* Only Bluetooth supported on SSP2. */
	AUDIO_LINK_DMIC,
};

/*
 * Add a dmic array composed of the provided number of channels. The skylake
 * SoC currently only supports dmic arrays on the dmic signals. Either 2
 * or 4 channel arrays are supported. Returns 0 on success, < 0 on error.
 */
int nhlt_soc_add_dmic_array(struct nhlt *nhlt, int num_channels);

/*
 * Add nau88l25 headset codec on provided SSP link. Return 0 on succes, < 0
 * on error.
 */
int nhlt_soc_add_nau88l25(struct nhlt *nhlt, int hwlink);

/*
 * Add ssm4567 smart amplifiers in stereo configuration on provided SSP link.
 * Return 0 on success, < 0 on error.
 */
int nhlt_soc_add_ssm4567(struct nhlt *nhlt, int hwlink);

/*
 * Add max98357a amplifier in stereo configuration on provide SSP link.
 * Return 0 on success, < 0 on error.
 */
int nhlt_soc_add_max98357(struct nhlt *nhlt, int hwlink);

/*
 * Add max98373 amplifier in stereo configuration on provided SSP link.
 * Return 0 on success, < 0 on error.
 */
int nhlt_soc_add_max98373(struct nhlt *nhlt, int hwlink);

/*
 * Add rt5514 DSP on provided SSP link. Return 0 on success, < 0 on error.
 */
int nhlt_soc_add_rt5514(struct nhlt *nhlt, int hwlink, int num_channels);

/*
 * Add rt5663 headset codec on provided SSP link. Return 0 on success, < 0
 * on error.
 */
int nhlt_soc_add_rt5663(struct nhlt *nhlt, int hwlink);

/*
 * Add max98927 amplifier in stereo configuration on provided SSP link.
 * Return 0 on success, < 0 on error.
 */
int nhlt_soc_add_max98927(struct nhlt *nhlt, int hwlink);

/*
 * Add da7219 headset codec on provided SSP link. Return 0 on success, < 0
 * on error.
 */
int nhlt_soc_add_da7219(struct nhlt *nhlt, int hwlink);

#endif
