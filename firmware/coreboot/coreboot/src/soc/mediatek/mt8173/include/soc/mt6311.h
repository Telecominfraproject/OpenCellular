/*
 * This file is part of the coreboot project.
 *
 * Copyright 2015 MediaTek Inc.
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

#ifndef __SOC_MEDIATEK_MT8173_MT6311_H__
#define __SOC_MEDIATEK_MT8173_MT6311_H__

void mt6311_probe(uint8_t i2c_num);

enum {
	MT6311_CID = 0x0,
	MT6311_SWCID = 0x1,
	MT6311_GPIO_MODE = 0x04,
	MT6311_TOP_CON = 0x0A,
	MT6311_TOP_RST_CON = 0x15,
	MT6311_TOP_INT_CON = 0x18,
	MT6311_STRUP_CON5 = 0x1F,
	MT6311_EFUSE_DOUT_56_63 = 0x40,
	MT6311_EFUSE_DOUT_64_71 = 0x41,
	MT6311_BUCK_ALL_CON23 = 0x69,
	MT6311_STRUP_ANA_CON1 = 0x6D,
	MT6311_STRUP_ANA_CON2 = 0x6E,
	MT6311_VDVFS1_ANA_CON10 = 0x84,
	MT6311_VDVFS11_CON7 = 0x88,
	MT6311_VDVFS11_CON9 = 0x8A,
	MT6311_VDVFS11_CON10 = 0x8B,
	MT6311_VDVFS11_CON11 = 0x8C,
	MT6311_VDVFS11_CON12 = 0x8D,
	MT6311_VDVFS11_CON13 = 0x8E,
	MT6311_VDVFS11_CON14 = 0x8F,
	MT6311_VDVFS11_CON19 = 0x94,
	MT6311_LDO_CON3 = 0xCF,
};

enum {
	MT6311_E1_CID_CODE = 0x0110,
	MT6311_E2_CID_CODE = 0x0120,
	MT6311_E3_CID_CODE = 0x0130,
};

#endif
