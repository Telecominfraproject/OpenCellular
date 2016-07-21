/*
 * This file is part of the coreboot project.
 *
 * Copyright 2016 Google Inc.
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

#ifndef _SOC_APOLLOLAKE_MEMINIT_H_
#define _SOC_APOLLOLAKE_MEMINIT_H_

#include <stddef.h>
#include <stdint.h>

/*
 * LPDDR4 helper routines for configuring the memory UPD for LPDDR4 operation.
 * There are 4 physical LPDDR4 channels each 32-bits wide. There are 2 logical
 * channels using 2 physical channels together to form a 64-bit interface to
 * memory for each logical channel.
 */

enum {
	LP4_PHYS_CH0A,
	LP4_PHYS_CH0B,
	LP4_PHYS_CH1A,
	LP4_PHYS_CH1B,
	LP4_NUM_PHYS_CHANNELS,
};

/* Logical channel identification. */
enum {
	LP4_LCH0,
	LP4_LCH1,
};

/*
 * The DQs within a physical channel can be bit-swizzled within each byte.
 * Within a channel the bytes can be swapped, but the DQs need to be routed
 * with the corresponding DQS (strobe).
 */
enum {
	LP4_DQS0,
	LP4_DQS1,
	LP4_DQS2,
	LP4_DQS3,
	LP4_NUM_BYTE_LANES,
	DQ_BITS_PER_DQS = 8,
};

enum {
				/* RL-tRCD-tRP */
	LP4_SPEED_1600 = 1600,	/* 14-15-15 */
	LP4_SPEED_2133 = 2133,	/* 20-20-20 */
	LP4_SPEED_2400 = 2400,	/* 24-22-22 */
};

/* LPDDR4 module density in bits. */
enum {
	LP4_8Gb_DENSITY = 2,
	LP4_12Gb_DESNITY,
	LP4_16Gb_DENSITY,
};

/* Provide bit swizzling per DQS and byte swapping within a channel. */
struct lpddr4_chan_swizzle_cfg {
	uint8_t dqs[LP4_NUM_BYTE_LANES][DQ_BITS_PER_DQS];
};

struct lpddr4_swizzle_cfg {
	struct lpddr4_chan_swizzle_cfg phys[LP4_NUM_PHYS_CHANNELS];
};

struct FSP_M_CONFIG;

/*
 * Initialize default LPDDR4 settings with provided speed. No logical channels
 * are enabled. Subsequent calls to logical channel enabling are required.
 */
void meminit_lpddr4(struct FSP_M_CONFIG *cfg, int speed);

/*
 * Enable logical channel providing the full lpddr4_swizzle_config to
 * fill in per channel swizzle definitions. This assumes a 64-bit wide
 * memory width per logical channel -- i.e. 2 physical channels are configured
 * to the memory reference code.
 */
void meminit_lpddr4_enable_channel(struct FSP_M_CONFIG *cfg, int logical_chan,
					int device_density, int dual_rank,
					const struct lpddr4_swizzle_cfg *scfg);

struct lpddr4_sku {
	int speed;
	int ch0_density;
	int ch1_density;
	int ch0_dual_rank;
	int ch1_dual_rank;
};

struct lpddr4_cfg {
	const struct lpddr4_sku *skus;
	size_t num_skus;
	const struct lpddr4_swizzle_cfg *swizzle_config;
};

/*
 * Initialize LPDDR4 settings by the provided lpddr4_cfg information and sku id.
 * The sku id is an index into the sku array within the lpddr4_cfg struct.
 */
void meminit_lpddr4_by_sku(struct FSP_M_CONFIG *cfg,
				const struct lpddr4_cfg *lpcfg, size_t sku_id);

#endif /* _SOC_APOLLOLAKE_MEMINIT_H_ */
