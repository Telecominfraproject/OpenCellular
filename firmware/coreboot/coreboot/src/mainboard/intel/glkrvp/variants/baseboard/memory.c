/*
 * This file is part of the coreboot project.
 *
 * Copyright 2017 Intel Corp.
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

#include <baseboard/variants.h>
#include <compiler.h>
#include <gpio.h>
#include <soc/meminit.h>
#include <variant/gpio.h>

const struct lpddr4_swizzle_cfg baseboard_lpddr4_swizzle = {
	/* CH0_DQA[0:31] SoC pins -> U22 LPDDR4 module pins */
	.phys[LP4_PHYS_CH0A] = {
		/* DQA[0:7] pins of LPDDR4 module. */
		.dqs[LP4_DQS0] = { 6, 7, 5, 4, 3, 1, 0, 2 },
		/* DQA[8:15] pins of LPDDR4 module. */
		.dqs[LP4_DQS1] = { 12, 10, 11, 13, 14, 8, 9, 15 },
		/* DQB[0:7] pins of LPDDR4 module with offset of 16. */
		.dqs[LP4_DQS2] = { 16, 22, 23, 20, 18, 17, 19, 21 },
		/* DQB[7:15] pins of LPDDR4 module with offset of 16. */
		.dqs[LP4_DQS3] = { 30, 28, 29, 25, 24, 26, 27, 31 },
	},
	.phys[LP4_PHYS_CH0B] = {
		/* DQA[0:7] pins of LPDDR4 module. */
		.dqs[LP4_DQS0] = { 7, 3, 5, 2, 6, 0, 1, 4 },
		/* DQA[8:15] pins of LPDDR4 module. */
		.dqs[LP4_DQS1] = { 9, 14, 12, 13, 10, 11, 8, 15 },
		/* DQB[0:7] pins of LPDDR4 module with offset of 16. */
		.dqs[LP4_DQS2] = { 20, 22, 23, 16, 19, 17, 18, 21 },
		/* DQB[7:15] pins of LPDDR4 module with offset of 16. */
		.dqs[LP4_DQS3] = { 28, 24, 26, 27, 29, 30, 31, 25 },
	},
	.phys[LP4_PHYS_CH1A] = {
		/* DQA[0:7] pins of LPDDR4 module. */
		.dqs[LP4_DQS0] = { 2, 1, 6, 7, 5, 4, 3, 0 },
		/* DQA[8:15] pins of LPDDR4 module. */
		.dqs[LP4_DQS1] = { 11, 10, 8, 9, 12, 15, 13, 14 },
		/* DQB[0:7] pins of LPDDR4 module with offset of 16. */
		.dqs[LP4_DQS2] = { 17, 23, 19, 16, 21, 22, 20, 18 },
		/* DQB[7:15] pins of LPDDR4 module with offset of 16. */
		.dqs[LP4_DQS3] = { 31, 29, 26, 25, 28, 27, 24, 30 },
	},
	.phys[LP4_PHYS_CH1B] = {
		/* DQA[0:7] pins of LPDDR4 module. */
		.dqs[LP4_DQS0] = { 4, 3, 7, 5, 6, 1, 0, 2 },
		/* DQA[8:15] pins of LPDDR4 module. */
		.dqs[LP4_DQS1] = { 15, 9, 8, 11, 14, 13, 12, 10 },
		/* DQB[0:7] pins of LPDDR4 module with offset of 16. */
		.dqs[LP4_DQS2] = { 20, 23, 22, 21, 18, 19, 16, 17 },
		/* DQB[7:15] pins of LPDDR4 module with offset of 16. */
		.dqs[LP4_DQS3] = { 25, 28, 30, 31, 26, 27, 24, 29 },
	},
};

static const struct lpddr4_sku skus[] = {
	/*
	 * K4F6E304HB-MGCJ - both logical channels While the parts
	 * are listed at 16Gb there are 2 ranks per channel so indicate
	 * the density as 8Gb per rank.
	 */
	[0] = {
		.speed = LP4_SPEED_2400,
		.ch0_rank_density = LP4_8Gb_DENSITY,
		.ch1_rank_density = LP4_8Gb_DENSITY,
		.ch0_dual_rank = 1,
		.ch1_dual_rank = 1,
		.part_num = "K4F6E304HB-MGCJ",
	},
	/* K4F8E304HB-MGCJ - both logical channels  */
	[1] = {
		.speed = LP4_SPEED_2400,
		.ch0_rank_density = LP4_8Gb_DENSITY,
		.ch1_rank_density = LP4_8Gb_DENSITY,
		.part_num = "K4F8E304HB-MGCJ",
	},
	/*
	 * MT53B512M32D2NP-062WT:C - both logical channels. While the parts
	 * are listed at 16Gb there are 2 ranks per channel so indicate
	 * the density as 8Gb per rank.
	 */
	[2] = {
		.speed = LP4_SPEED_2400,
		.ch0_rank_density = LP4_8Gb_DENSITY,
		.ch1_rank_density = LP4_8Gb_DENSITY,
		.ch0_dual_rank = 1,
		.ch1_dual_rank = 1,
		.part_num = "MT53B512M32D2NP",
		.disable_periodic_retraining = 1,
	},
	/* MT53B256M32D1NP-062 WT:C - both logical channels */
	[3] = {
		.speed = LP4_SPEED_2400,
		.ch0_rank_density = LP4_8Gb_DENSITY,
		.ch1_rank_density = LP4_8Gb_DENSITY,
		.part_num = "MT53B256M32D1NP",
		.disable_periodic_retraining = 1,
	},
	/*
	 * H9HCNNNBPUMLHR-NLE - both logical channels. While the parts
	 * are listed at 16Gb there are 2 ranks per channel so indicate the
	 * density as 8Gb per rank.
	 */
	[4] = {
		.speed = LP4_SPEED_2400,
		.ch0_rank_density = LP4_8Gb_DENSITY,
		.ch1_rank_density = LP4_8Gb_DENSITY,
		.ch0_dual_rank = 1,
		.ch1_dual_rank = 1,
		.part_num = "H9HCNNNBPUMLHR",
	},
	/* H9HCNNN8KUMLHR-NLE - both logical channels */
	[5] = {
		.speed = LP4_SPEED_2400,
		.ch0_rank_density = LP4_8Gb_DENSITY,
		.ch1_rank_density = LP4_8Gb_DENSITY,
		.part_num = "H9HCNNN8KUMLHR",
	},
};

static const struct lpddr4_cfg lp4cfg = {
	.skus = skus,
	.num_skus = ARRAY_SIZE(skus),
	.swizzle_config = &baseboard_lpddr4_swizzle,
};

const struct lpddr4_cfg * __weak variant_lpddr4_config(void)
{
	return &lp4cfg;
}

size_t __weak variant_memory_sku(void)
{
	return 0;
}
