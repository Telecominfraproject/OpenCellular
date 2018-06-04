/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015-2016 Advanced Micro Devices, Inc.
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

#include <soc/southbridge.h>
#include <stdlib.h>
#include <soc/gpio.h>

#include "gpio.h"

/*
 * As a rule of thumb, GPIO pins used by coreboot should be initialized at
 * bootblock while GPIO pins used only by the OS should be initialized at
 * ramstage.
 */
static const struct soc_amd_gpio gpio_set_stage_reset[] = {
	/* NFC PU */
	PAD_GPO(GPIO_64, HIGH),
	/* PCIe presence detect */
	PAD_GPI(GPIO_69, PULL_UP),
	/* MUX for Power Express Eval */
	PAD_GPI(GPIO_116, PULL_DOWN),
	/* SD power */
	PAD_GPO(GPIO_119, HIGH),
	/* GPIO_136 - UART0_FCH_RX_DEBUG_RX */
	PAD_NF(GPIO_136, UART0_RXD, PULL_NONE),
	/* GPIO_137 - UART0_FCH_DEBUG_RTS */
	PAD_NF(GPIO_137, UART0_RTS_L, PULL_NONE),
	/* GPIO_138 - UART0_FCH_TX_DEBUG_RX */
	PAD_NF(GPIO_138, UART0_TXD, PULL_NONE),
	/* GPIO_142 - UART1_FCH_RTS */
	PAD_NF(GPIO_142, UART1_RTS_L, PULL_NONE),
	/* GPIO_143 - UART1_FCH_TX */
	PAD_NF(GPIO_143, UART1_TXD, PULL_NONE),
};

static const struct soc_amd_gpio gpio_set_stage_ram[] = {
	/* BT radio disable */
	PAD_GPO(GPIO_14, HIGH),
	/* NFC wake */
	PAD_GPO(GPIO_65, HIGH),
	/* Webcam */
	PAD_GPO(GPIO_66, HIGH),
	/* GPS sleep */
	PAD_GPO(GPIO_70, HIGH),
};

const struct soc_amd_gpio *early_gpio_table(size_t *size)
{
	*size = ARRAY_SIZE(gpio_set_stage_reset);
	return gpio_set_stage_reset;
}

const struct soc_amd_gpio *gpio_table(size_t *size)
{
	*size = ARRAY_SIZE(gpio_set_stage_ram);
	return gpio_set_stage_ram;
}
