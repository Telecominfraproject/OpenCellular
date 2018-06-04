/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Intel Corporation. All Rights Reserved.
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

#include <soc/gpio.h>

static const struct pad_config gpio_table[] = {
	PAD_CFG_NF(GPIO_134, NATIVE, DEEP, NF2), /* ISH_I2C0_SDA/IO-OD */
	PAD_CFG_NF(GPIO_135, NATIVE, DEEP, NF2), /* ISH_I2C0_SCL/IO-OD */
	PAD_CFG_NF(GPIO_136, NATIVE, DEEP, NF2), /* ISH_I2C1_SDA/IO-OD */
	PAD_CFG_NF(GPIO_137, NATIVE, DEEP, NF2), /* ISH_I2C1_SCL/IO-OD */

	PAD_CFG_NF(GPIO_0, NATIVE, DEEP, NF1),
	PAD_CFG_NF(GPIO_1, NATIVE, DEEP, NF1),
	PAD_CFG_NF(GPIO_2, NATIVE, DEEP, NF1),
	PAD_CFG_NF(GPIO_3, NATIVE, DEEP, NF1),
	PAD_CFG_NF(GPIO_4, NATIVE, DEEP, NF1),
	PAD_CFG_NF(GPIO_5, NATIVE, DEEP, NF1),
	PAD_CFG_NF(GPIO_6, NATIVE, DEEP, NF1),
	PAD_CFG_NF(GPIO_7, NATIVE, DEEP, NF1),
	PAD_CFG_NF(GPIO_8, NATIVE, DEEP, NF1),

	/* EXP_I2C_SDA and I2C_PSS_SDA and I2C_2_SDA_IOEXP */
	PAD_CFG_NF(GPIO_7, NATIVE, DEEP, NF1),
	/* EXP_I2C_SCL and I2C_PSS_SCL and I2C_2_SCL_IOEXP */
	PAD_CFG_NF(GPIO_8, NATIVE, DEEP, NF1),

	PAD_CFG_GPO(GPIO_152, 0, DEEP), /* PERST# */
	PAD_CFG_GPO(GPIO_19, 1, DEEP), /* PFET */
	PAD_CFG_GPO(GPIO_13, 0, DEEP), /* PERST# */
	PAD_CFG_GPO(GPIO_17, 1, DEEP), /* PFET */
	PAD_CFG_GPO(GPIO_15, 0, DEEP), /* PERST# */

	PAD_CFG_NF(GPIO_210, NATIVE, DEEP, NF1), /* CLKREQ# */

	PAD_CFG_NF(SMB_CLK, NATIVE, DEEP, NF1),
	PAD_CFG_NF(SMB_DATA, NATIVE, DEEP, NF1),
};
