/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
 * Copyright (C) 2014 Sage Electronic Engineering, LLC.
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

#include <stdlib.h>
#include <soc/gpio.h>
#include "irqroute.h"

/* NCORE GPIOs */
static const struct soc_gpio_map gpncore_gpio_map[] = {
	GPIO_FUNC2,	/* GPIO 0 */
	GPIO_FUNC2,	/* GPIO 1 */
	GPIO_FUNC2,	/* GPIO 2 */
	GPIO_FUNC2,	/* GPIO 3 */
	GPIO_FUNC2,	/* GPIO 4 */
	GPIO_FUNC2,	/* GPIO 5 */
	GPIO_FUNC2,	/* GPIO 6 */
	GPIO_FUNC2,	/* GPIO 7 */
	GPIO_FUNC2,	/* GPIO 8 */
	GPIO_FUNC2,	/* GPIO 9 */
	GPIO_FUNC2,	/* GPIO 10 */
	GPIO_FUNC2,	/* GPIO 11 */
	GPIO_FUNC2,	/* GPIO 12 */
	GPIO_FUNC2,	/* GPIO 13 */
	GPIO_FUNC2,	/* GPIO 14 */
	GPIO_FUNC2,	/* GPIO 15 */
	GPIO_FUNC2,	/* GPIO 16 */
	GPIO_FUNC2,	/* GPIO 17 */
	GPIO_FUNC2,	/* GPIO 18 */
	GPIO_FUNC2,	/* GPIO 19 */
	GPIO_FUNC2,	/* GPIO 20 */
	GPIO_FUNC2,	/* GPIO 21 */
	GPIO_FUNC2,	/* GPIO 22 */
	GPIO_FUNC2,	/* GPIO 23 */
	GPIO_FUNC2,	/* GPIO 24 */
	GPIO_FUNC2,	/* GPIO 25 */
	GPIO_FUNC2,	/* GPIO 26 */
	GPIO_END
};

/* SCORE GPIOs (GPIO_S0_SC_XX)*/
static const struct soc_gpio_map gpscore_gpio_map[] = {
	GPIO_FUNC1,		/* GPIO_S0_SC[000]		SATA_GP[0]			-				-				- */
	GPIO_FUNC2,		/* GPIO_S0_SC[001]		SATA_GP[1]			SATA_DEVSLP[0]	-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[002]		SATA_LED#			-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[003]		PCIE_CLKREQ[0]#		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[004]		PCIE_CLKREQ[1]#		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[005]		PCIE_CLKREQ[2]#		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[006]		PCIE_CLKREQ[3]#		-				-				- */
	GPIO_NC,		/* GPIO_S0_SC[007]		RESERVED			SD3_WP			-				- */
	GPIO_FUNC2,		/* GPIO_S0_SC[008]		I2S0_CLK			HDA_RST#		-				- */
	GPIO_FUNC2,		/* GPIO_S0_SC[009]		I2S0_FRM			HDA_SYNC		-				- */
	GPIO_FUNC2,		/* GPIO_S0_SC[010]		I2S0_DATAOUT		HDA_CLK			-				- */
	GPIO_FUNC2,		/* GPIO_S0_SC[011]		I2S0_DATAIN			HDA_SDO			-				- */
	GPIO_FUNC2,		/* GPIO_S0_SC[012]		I2S1_CLK			HDA_SDI[0]		-				- */
	GPIO_NC,		/* GPIO_S0_SC[013]		I2S1_FRM			HDA_SDI[1]		-				- */
	GPIO_DEFAULT,	/* GPIO_S0_SC[014]		I2S1_DATAOUT		RESERVED		-				- */
	GPIO_DEFAULT,	/* GPIO_S0_SC[015]		I2S1_DATAIN			RESERVED		-				- */
	GPIO_NC,		/* GPIO_S0_SC[016]		MMC1_CLK			-				MMC1_45_CLK		- */
	GPIO_NC,		/* GPIO_S0_SC[017]		MMC1_D[0]			-				MMC1_45_D[0]	- */
	GPIO_NC,		/* GPIO_S0_SC[018]		MMC1_D[1]			-				MMC1_45_D[1]	- */
	GPIO_NC,		/* GPIO_S0_SC[019]		MMC1_D[2]			-				MMC1_45_D[2]	- */
	GPIO_NC,		/* GPIO_S0_SC[020]		MMC1_D[3]			-				MMC1_45_D[3]	- */
	GPIO_NC,		/* GPIO_S0_SC[021]		MMC1_D[4]			-				MMC1_45_D[4]	- */
	GPIO_NC,		/* GPIO_S0_SC[022]		MMC1_D[5]			-				MMC1_45_D[5]	- */
	GPIO_NC,		/* GPIO_S0_SC[023]		MMC1_D[6]			-				MMC1_45_D[6]	- */
	GPIO_NC,		/* GPIO_S0_SC[024]		MMC1_D[7]			-				MMC1_45_D[7]	- */
	GPIO_NC,		/* GPIO_S0_SC[025]		MMC1_CMD			-				MMC1_45_CMD		- */
	GPIO_NC,		/* GPIO_S0_SC[026]		MMC1_RST#			SATA_DEVSLP[0]	MMC1_45_RST#	- */
	GPIO_FUNC1,		/* GPIO_S0_SC[027]		SD2_CLK				-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[028]		SD2_D[0]			-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[029]		SD2_D[1]			-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[030]		SD2_D[2]			-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[031]		SD2_D[3]_CD#		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[032]		SD2_CMD				-				-				- */
	GPIO_NC,		/* GPIO_S0_SC[033]		SD3_CLK				-				-				- */
	GPIO_NC,		/* GPIO_S0_SC[034]		SD3_D[0]			-				-				- */
	GPIO_NC,		/* GPIO_S0_SC[035]		SD3_D[1]			-				-				- */
	GPIO_NC,		/* GPIO_S0_SC[036]		SD3_D[2]			-				-				- */
	GPIO_NC,		/* GPIO_S0_SC[037]		SD3_D[3]			-				-				- */
	GPIO_NC,		/* GPIO_S0_SC[038]		SD3_CD#				-				-				- */
	GPIO_NC,		/* GPIO_S0_SC[039]		SD3_CMD				-				-				- */
	GPIO_NC,		/* GPIO_S0_SC[040]		SD3_1P8EN			-				-				- */
	GPIO_NC,		/* GPIO_S0_SC[041]		SD3_PWREN#			-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[042]		ILB_LPC_AD[0]		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[043]		ILB_LPC_AD[1]		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[044]		ILB_LPC_AD[2]		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[045]		ILB_LPC_AD[3]		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[046]		ILB_LPC_FRAME#		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[047]		ILB_LPC_CLK[0]		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[048]		ILB_LPC_CLK[1]		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[049]		ILB_LPC_CLKRUN#		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[050]		ILB_LPC_SERIRQ		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[051]		PCU_SMB_DATA		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[052]		PCU_SMB_CLK			-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[053]		PCU_SMB_ALERT#		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[054]		ILB_8254_SPKR		RESERVED		-				- */
	GPIO_DEFAULT,	/* GPIO_S0_SC[055]		RESERVED			-				-				- */
	GPIO_DEFAULT,	/* GPIO_S0_SC[056]		RESERVED			-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[057]		PCU_UART_TXD		-				-				- */
	GPIO_DEFAULT,	/* GPIO_S0_SC[058]		RESERVED			-				-				- */
	GPIO_DEFAULT,	/* GPIO_S0_SC[059]		RESERVED			-				-				- */
	GPIO_DEFAULT,	/* GPIO_S0_SC[060]		RESERVED			-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[061]		PCU_UART_RXD		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[062]		LPE_I2S2_CLK		SATA_DEVSLP[1]	RESERVED		- */
	GPIO_FUNC1,		/* GPIO_S0_SC[063]		LPE_I2S2_FRM		RESERVED		-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[064]		LPE_I2S2_DATAIN		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[065]		LPE_I2S2_DATAOUT	-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[066]		SIO_SPI_CS#			-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[067]		SIO_SPI_MISO		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[068]		SIO_SPI_MOSI		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[069]		SIO_SPI_CLK			-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[070]		SIO_UART1_RXD		RESERVED		-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[071]		SIO_UART1_TXD		RESERVED		-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[072]		SIO_UART1_RTS#		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[073]		SIO_UART1_CTS#		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[074]		SIO_UART2_RXD		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[075]		SIO_UART2_TXD		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[076]		SIO_UART2_RTS#		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[077]		SIO_UART2_CTS#		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[078]		SIO_I2C0_DATA		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[079]		SIO_I2C0_CLK		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[080]		SIO_I2C1_DATA		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[081]		SIO_I2C1_CLK		RESERVED		-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[082]		SIO_I2C2_DATA		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[083]		SIO_I2C2_CLK		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[084]		SIO_I2C3_DATA		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[085]		SIO_I2C3_CLK		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[086]		SIO_I2C4_DATA		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[087]		SIO_I2C4_CLK		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[088]		SIO_I2C5_DATA		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[089]		SIO_I2C5_CLK		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[090]		SIO_I2C6_DATA		ILB_NMI			-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[091]		SIO_I2C6_CLK		SD3_WP			-				- */
	GPIO_FUNC1,		/* RESERVED				GPIO_S0_SC[092]		-				-				- */
	GPIO_FUNC1,		/* RESERVED				GPIO_S0_SC[093]		-				-				- */
	GPIO_DEFAULT,	/* GPIO_S0_SC[094]		SIO_PWM[0]			-				-				- */
	GPIO_DEFAULT,	/* GPIO_S0_SC[095]		SIO_PWM[1]			-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[096]		PMC_PLT_CLK[0]		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[097]		PMC_PLT_CLK[1]		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[098]		PMC_PLT_CLK[2]		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[099]		PMC_PLT_CLK[3]		-				-				- */
	GPIO_FUNC1,		/* GPIO_S0_SC[100]		PMC_PLT_CLK[4]		-				-				- */
	GPIO_DEFAULT,	/* GPIO_S0_SC[101]		PMC_PLT_CLK[5]		-				-				- */
	GPIO_END
};

/* SSUS GPIOs (GPIO_S5) */
static const struct soc_gpio_map gpssus_gpio_map[] = {
	GPIO_DEFAULT,	/* GPIO_S5[00]			RESERVED			-				-			- */
	GPIO_DEFAULT,	/* GPIO_S5[01]			RESERVED			RESERVED		RESERVED	PMC_WAKE_PCIE[1]# */
	GPIO_DEFAULT,	/* GPIO_S5[02]			RESERVED			RESERVED		RESERVED	PMC_WAKE_PCIE[2]# */
	GPIO_DEFAULT,	/* GPIO_S5[03]			RESERVED			RESERVED		RESERVED	PMC_WAKE_PCIE[3]# */
	GPIO_DEFAULT,	/* GPIO_S5[04]			RESERVED			RESERVED		RESERVED	RESERVED */
	GPIO_DEFAULT,	/* GPIO_S5[05]			PMC_SUSCLK[1]		RESERVED		RESERVED	RESERVED */
	GPIO_DEFAULT,	/* GPIO_S5[06]			PMC_SUSCLK[2]		RESERVED		RESERVED	RESERVED */
	GPIO_DEFAULT,	/* GPIO_S5[07]			PMC_SUSCLK[3]		RESERVED		RESERVED	RESERVED */
	GPIO_NC,		/* GPIO_S5[08]			RESERVED			RESERVED		RESERVED	RESERVED */
	GPIO_NC,		/* GPIO_S5[09]			RESERVED			RESERVED		RESERVED	RESERVED */
	GPIO_NC,		/* GPIO_S5[10]			RESERVED			RESERVED		RESERVED	- */
	GPIO_DEFAULT,	/* PMC_SUSPWRDNACK		GPIO_S5[11]			-				-			- */
	GPIO_FUNC0,		/* PMC_SUSCLK[0]		GPIO_S5[12]			-				-			- */
	GPIO_FUNC1,		/* RESERVED				GPIO_S5[13]			-				-			- */
	GPIO_FUNC1,		/* RESERVED				GPIO_S5[14]			USB_ULPI_RST#	-			- */
	GPIO_FUNC0,		/* PMC_WAKE_PCIE[0]#	GPIO_S5[15]			-				-			- */
	GPIO_FUNC0,		/* PMC_PWRBTN#			GPIO_S5[16]			-				-			- */
	GPIO_DEFAULT,	/* RESERVED				GPIO_S5[17]			-				-			- */
	GPIO_FUNC1,		/* PMC_SUS_STAT#		GPIO_S5[18]			-				-			- */
	GPIO_FUNC0,		/* USB_OC[0]#			GPIO_S5[19]			-				-			- */
	GPIO_FUNC0,		/* USB_OC[1]#			GPIO_S5[20]			-				-			- */
	GPIO_FUNC0,		/* PCU_SPI_CS[1]#		GPIO_S5[21]			-				-			- */
	GPIO_DEFAULT,	/* GPIO_S5[22]			RESERVED			RESERVED		RESERVED	RESERVED */
	GPIO_DEFAULT,	/* GPIO_S5[23]			RESERVED			RESERVED		RESERVED	RESERVED */
	GPIO_DEFAULT,	/* GPIO_S5[24]			RESERVED			RESERVED		RESERVED	RESERVED */
	GPIO_DEFAULT,	/* GPIO_S5[25]			RESERVED			RESERVED		RESERVED	RESERVED */
	GPIO_DEFAULT,	/* GPIO_S5[26]			RESERVED			RESERVED		RESERVED	RESERVED */
	GPIO_DEFAULT,	/* GPIO_S5[27]			RESERVED			RESERVED		RESERVED	RESERVED */
	GPIO_DEFAULT,	/* GPIO_S5[28]			RESERVED			RESERVED		RESERVED	RESERVED */
	GPIO_DEFAULT,	/* GPIO_S5[29]			RESERVED			RESERVED		RESERVED	RESERVED */
	GPIO_DEFAULT,	/* GPIO_S5[30]			RESERVED			RESERVED		RESERVED	RESERVED */
	GPIO_DEFAULT,	/* GPIO_S5[31]			USB_ULPI_CLK		RESERVED		RESERVED	- */
	GPIO_DEFAULT,	/* GPIO_S5[32]			USB_ULPI_DATA[0]	RESERVED		RESERVED	- */
	GPIO_DEFAULT,	/* GPIO_S5[33]			USB_ULPI_DATA[1]	RESERVED		RESERVED	- */
	GPIO_DEFAULT,	/* GPIO_S5[34]			USB_ULPI_DATA[2]	RESERVED		RESERVED	- */
	GPIO_DEFAULT,	/* GPIO_S5[35]			USB_ULPI_DATA[3]	RESERVED		RESERVED	- */
	GPIO_DEFAULT,	/* GPIO_S5[36]			USB_ULPI_DATA[4]	RESERVED		RESERVED	- */
	GPIO_DEFAULT,	/* GPIO_S5[37]			USB_ULPI_DATA[5]	RESERVED		RESERVED	- */
	GPIO_NC,		/* GPIO_S5[38]			USB_ULPI_DATA[6]	RESERVED		RESERVED	- */
	GPIO_NC,		/* GPIO_S5[39]			USB_ULPI_DATA[7]	RESERVED		RESERVED	- */
	GPIO_NC,		/* GPIO_S5[40]			USB_ULPI_DIR		RESERVED		RESERVED	- */
	GPIO_NC,		/* GPIO_S5[41]			USB_ULPI_NXT		RESERVED		RESERVED	- */
	GPIO_DEFAULT,	/* GPIO_S5[42]			USB_ULPI_STP		RESERVED		RESERVED	- */
	GPIO_DEFAULT,	/* GPIO_S5[43]			USB_ULPI_REFCLK		RESERVED		RESERVED	- */
	GPIO_END
};

static struct soc_gpio_config gpio_config = {
	.ncore = gpncore_gpio_map,
	.score = gpscore_gpio_map,
	.ssus  = gpssus_gpio_map,
	.core_dirq = NULL,
	.sus_dirq = NULL,
};

struct soc_gpio_config* mainboard_get_gpios(void)
{
	return &gpio_config;
}
