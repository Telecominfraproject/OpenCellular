/*
 * This file is part of the coreboot project.
 *
 * Copyright 2017 Intel Corporation.
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

#include <baseboard/gpio.h>
#include <baseboard/variants.h>
#include <commonlib/helpers.h>
#include <compiler.h>

/* Pad configuration in ramstage*/
static const struct pad_config gpio_table[] = {
	/* GPPC */
	/* A0  : RCINB_TIME_SYNC_1 */
	/* A1  : ESPI_IO_0 */
	/* A2  : ESPI_IO_1 */
	/* A3  : ESPI_IO_2 */
	/* A4  : ESPI_IO_3 */
	/* A5  : ESPI_CSB */
	/* A6  : SERIRQ */
	/* A7  : PRIQAB_GSP10_CS1B */
	PAD_CFG_GPI_SCI_HIGH(GPP_A7, UP_20K, DEEP, EDGE_SINGLE),
	/* A8  : CLKRUNB */
	PAD_CFG_GPO(GPP_A8, 1, PLTRST),
	/* A9  : CLKOUT_LPC_0_ESPI_CLK */
	/* A10 : CLKOUT_LPC_1 */
	/* A11 : PMEB_GSP11_CS1B */
	PAD_CFG_GPI_SCI_LOW(GPP_A11, UP_20K, DEEP, LEVEL),
	/* A12 : BM_BUSYB_ISH__GP_6 */
	/* A13 : SUSWARNB_SUSPWRDNACK */
	PAD_CFG_GPO(GPP_A13, 1, PLTRST),
	/* A14 : SUS_STATB_ESPI_RESETB */
	/* A15 : SUSACKB */
	PAD_CFG_GPO(GPP_A15, 1, PLTRST),
	/* A16 : SD_1P8_SEL */
	PAD_CFG_GPO(GPP_A16, 0, PLTRST),
	/* A17 : SD_VDD1_PWR_EN_B_ISH_GP_7 */
	/* A18 : ISH_GP_0 */
	PAD_CFG_NF(GPP_A18, UP_20K, DEEP, NF1),
	/* A19 : ISH_GP_1 */
	PAD_CFG_NF(GPP_A19, UP_20K, DEEP, NF1),
	/* A20 : aduio codec irq  */
	PAD_CFG_GPI_APIC_LOW(GPP_A20, NONE, DEEP),
	/* A21 : ISH_GP_3 */
	PAD_CFG_NF(GPP_A21, UP_20K, DEEP, NF1),
	/* A22 : ISH_GP_4 */
	PAD_CFG_NF(GPP_A22, UP_20K, DEEP, NF1),
	/* A23 : ISH_GP_5 */
	PAD_CFG_NF(GPP_A23, UP_20K, DEEP, NF1),

	/* B0  : CORE_VID_0 */
	/* B1  : CORE_VID_1 */
	/* B2  : VRALERTB */
	PAD_CFG_GPI_APIC(GPP_B2, NONE, DEEP, LEVEL, NONE),
	/* B3  : CPU_GP_2 */
	PAD_CFG_GPI_APIC(GPP_B3, NONE, PLTRST, LEVEL, NONE),
	/* B4  : CPU_GP_3 */
	PAD_CFG_GPO(GPP_B4, 1, DEEP),
	/* B5  : SRCCLKREQB_0 */
	/* B6  : SRCCLKREQB_1 */
	/* B7  : SRCCLKREQB_2 */
	/* B8  : SRCCLKREQB_3 */
	/* B9  : SRCCLKREQB_4 */
	/* B10 : SRCCLKREQB_5 */
	/* B11 : EXT_PWR_GATEB */
	PAD_CFG_NF(GPP_B11, NONE, DEEP, NF1),
	/* B12 : SLP_S0B */
	/* B13 : PLTRSTB */
	/* B14 : SPKR */
	PAD_CFG_GPO(GPP_B14, 1, PLTRST),
	/* B15 : GSPI0_CS0B */
	PAD_CFG_GPO(GPP_B15, 0, DEEP),
	/* B16 : GSPI0_CLK */
	PAD_CFG_GPI_APIC(GPP_B16, NONE, PLTRST, LEVEL, NONE),
	/* B17 : GSPI0_MISO */
	PAD_CFG_GPO(GPP_B17, 1, PLTRST),
	/* B18 : GSPI0_MOSI */
	PAD_CFG_GPO(GPP_B18, 1, PLTRST),
	/* B19 : GSPI1_CS0B */
	/* B20 : GSPI1_CLK_NFC_CLK */
	/* B21 : GSPI1_MISO_NFC_CLKREQ */
	/* B22 : GSP1_MOSI */
	/* B23 : SML1ALERTB_PCHHOTB */
	PAD_CFG_GPO(GPP_B23, 1, DEEP),

	/* C0  : SMBCLK */
	/* C1  : SMBDATA */
	/* C2  : SMBALERTB */
	PAD_CFG_GPO(GPP_C2, 1, DEEP),
	/* C3  : SML0CLK */
	/* C4  : SML0DATA */
	/* C5  : SML0ALERTB */
	PAD_CFG_GPI_SCI_LOW(GPP_C5, NONE, DEEP, LEVEL),
	/* C6  : SML1CLK */
	/* C7  : SML1DATA */
	/* C8  : UART0_RXD */
	PAD_CFG_GPI_APIC(GPP_C8, UP_20K, DEEP, LEVEL, INVERT),
	/* C9  : UART0_TXD */
	PAD_CFG_GPI_SCI_LOW(GPP_C9, UP_20K, PLTRST, EDGE_SINGLE),
	/* C10 : UART0_RTSB */
	PAD_CFG_GPO(GPP_C10, 0, PLTRST),
	/* C11 : UART0_CTSB */
	PAD_CFG_GPI_SCI_LOW(GPP_C11, UP_20K, DEEP, LEVEL),
	/* C12 : UART1_RXD_ISH_UART1_RXD */
	PAD_CFG_GPO(GPP_C12, 1, PLTRST),
	/* C13 : UART1_RXD_ISH_UART1_TXD */
	/* C14 : UART1_RXD_ISH_UART1_RTSB */
	/* C15 : UART1_RXD_ISH_UART1_CTSB */
	PAD_CFG_GPO(GPP_C15, 1, PLTRST),
	/* C16 : I2C0_SDA */
	/* C17 : I2C0_SCL */
	/* C18 : I2C1_SDA */
	/* C19 : I2C1_SCL */
	/* C20 : UART2_RXD */
	/* C21 : UART2_TXD */
	/* C22 : UART2_RTSB */
	/* C23 : UART2_CTSB */

	/* D0  : SPI1_CSB_BK_0 */
	/* D1  : SPI1_CLK_BK_1 */
	/* D2  : SPI1_MISO_IO_1_BK_2 */
	/* D3  : SPI1_MOSI_IO_0_BK_3 */
	/* D4  : IMGCLKOUT_0_BK_4 */
	/* D5  : ISH_I2C0_SDA */
	/* D6  : ISH_I2C0_SCL */
	/* D7  : ISH_I2C1_SDA */
	/* D8  : ISH_I2C1_SCL */
	/* D9  : ISH_SPI_CSB */
	PAD_CFG_GPO(GPP_D9, 1, PLTRST),
	/* D10 : ISH_SPI_CLK */
	PAD_CFG_GPI_APIC(GPP_D10, NONE, PLTRST, EDGE_SINGLE, NONE),
	/* D11 : ISH_SPI_MISO_GP_BSSB_CLK */
	PAD_CFG_GPI_SCI_LOW(GPP_D11, NONE, DEEP, LEVEL),
	/* D12 : ISH_SPI_MOSI_GP_BSSB_DI */
	/* D13 : ISH_UART0_RXD_SML0BDATA */
	PAD_CFG_GPO(GPP_D13, 1, DEEP),
	/* D14 : ISH_UART0_TXD_SML0BCLK */
	PAD_CFG_GPO(GPP_D14, 1, PLTRST),
	/* D15 : ISH_UART0_RTSB_GPSPI2_CS1B */
	/* D16 : ISH_UART0_CTSB_SML0BALERTB */
	PAD_CFG_GPI_SCI_HIGH(GPP_D16, NONE, DEEP, LEVEL),
	/* D17 : DMIC_CLK_1_SNDW3_CLK */
	PAD_CFG_NF(GPP_D17, UP_20K, DEEP, NF1),
	/* D18 : DMIC_DATA_1_SNDW3_DATA */
	PAD_CFG_NF(GPP_D18, UP_20K, DEEP, NF1),
	/* D19 : DMIC_CLK_0_SNDW4_CLK */
	PAD_CFG_NF(GPP_D19, UP_20K, DEEP, NF1),
	/* D20 : DMIC_DATA_0_SNDW4_DATA */
	PAD_CFG_NF(GPP_D20, UP_20K, DEEP, NF1),
	/* D21 : SPI1_IO_2 */
	PAD_CFG_NF(GPP_D21, NONE, PLTRST, NF1),
	/* D22 : SPI1_IO_3 */
	PAD_CFG_NF(GPP_D22, NONE, PLTRST, NF1),
	/* D23 : SPP_MCLK */
	PAD_CFG_NF(GPP_D23, NONE, DEEP, NF1),
	/* E0  : SATAXPCIE_0_SATAGP_0 */
#if IS_ENABLED(CONFIG_BOARD_INTEL_CANNONLAKE_RVPY)
	PAD_CFG_NF(GPP_E0, UP_20K, DEEP, NF1),
#endif
	/* E1  : SATAXPCIE_1_SATAGP_1 */
	/* E2  : SATAXPCIE_2_SATAGP_2 */
	PAD_CFG_GPI(GPP_E2, UP_20K, PLTRST),
	/* E3  : CPU_GP_0 */
	PAD_CFG_GPI_SMI(GPP_E3, NONE, PLTRST, EDGE_SINGLE, NONE),
	/* E4  : SATA_DEVSLP_0 */
	PAD_CFG_NF(GPP_E4, NONE, DEEP, NF1),
	/* E5  : SATA_DEVSLP_1 */
	/* E6  : SATA_DEVSLP_2 */
	PAD_CFG_GPI_SCI(GPP_E6, NONE, DEEP, OFF, NONE),
	/* E7  : CPU_GP_1 */
	PAD_CFG_GPI_INT(GPP_E7, NONE, PLTRST, EDGE_SINGLE),
	/* E8  : SATA_LEDB */
	/* E9  : USB2_OCB_0_GP_BSSB_CLK */
	/* E10 : USB2_OCB_1_GP_BSSB_DI */
	/* E11 : USB2_OCB_2 */
	/* E12 : USB2_OCB_3 */
	/* E13 : DDSP_HPD_0_DISP_MISC_0 */
	/* E14 : DDSP_HPD_0_DISP_MISC_1 */
	/* E15 : DDSP_HPD_0_DISP_MISC_2 */
	/* E16 : EMMC_EN */
	PAD_CFG_GPO(GPP_E16, 1, PLTRST),
	/* E17 : EDP_HPD_DISP_MISC_4 */
	/* E18 : DDPB_CTRLCLK */
	/* E19 : DDPB_CTRLDATA */
	/* E20 : DDPC_CTRLCLK */
	/* E21 : DDPC_CTRLDATA */
	/* E22 : DDPD_CTRLCLK */
	/* E23 : DDPD_CTRLDATA */

	/* F0  : CNV_GNSS_PA_BLANKING */
	PAD_CFG_GPI(GPP_F0, NONE, PLTRST),
	/* F1  : CNV_GNSS_FAT */
	PAD_CFG_TERM_GPO(GPP_F1, 1, UP_20K, DEEP),
	/* F2  : CNV_GNSS_SYSCK */
	PAD_CFG_TERM_GPO(GPP_F2, 1, UP_20K, PLTRST),
	/* F3  : GPP_F_3 */
	PAD_CFG_TERM_GPO(GPP_F3, 0, UP_20K, PLTRST),
	/* F4  : CNV_BRI_DT_UART0_RTSB */
	/* F5  : CNV_BRI_RSP_UART0_RXD */
	/* F6  : CNV_RGI_DT_UART0_TXD */
	/* F7  : CNV_RGI_DT_RSP_UART9_CTSB */
	/* F8  : CNV_MFUART2_RXD */
	PAD_CFG_NF(GPP_F8, UP_20K, DEEP, NF1),
	/* F9  : CNV_MFUART2_TXD */
	PAD_CFG_NF(GPP_F9, UP_20K, DEEP, NF1),
	/* F10 : GPP_F_10 */
	PAD_CFG_GPO(GPP_F10, 1, PLTRST),
	/* F11 : EMMC_CMD */
	/* F12 : EMMC_DATA0 */
	/* F13 : EMMC_DATA1 */
	/* F14 : EMMC_DATA2 */
	/* F15 : EMMC_DATA3 */
	/* F16 : EMMC_DATA4 */
	/* F17 : EMMC_DATA5 */
	/* F18 : EMMC_DATA6 */
	/* F19 : EMMC_DATA9 */
	/* F20 : EMMC_RCLK */
	/* F21 : EMMC_CLK */
	/* F22 : EMMC_RESETB */
	/* F23 : BIOS_REC */
	PAD_CFG_GPI(GPP_F23, UP_20K, DEEP),
	/* G0  : SD3_D2 */
	/* G1  : SD3_D0_SD4_RCLK_P */
	/* G2  : SD3_D1_SD4_RCLK_N */
	/* G3  : SD3_D2 */
	/* G4  : SD3_D3 */
	/* G5  : SD3_CDB */
	PAD_CFG_NF(GPP_G5, UP_20K, DEEP, NF1),
	/* G6  : SD3_CLK */
	/* G7  : SD3_WP */
	PAD_CFG_NF(GPP_G7, DN_20K, DEEP, NF1),

	/* H0  : SSP2_SCLK */
	/* H1  : SSP2_SFRM */
	/* H2  : SSP2_TXD */
	/* H3  : SSP2_RXD */
	/* H4  : I2C2_SDA */
	/* H5  : I2C2_SCL */
	/* H6  : I2C3_SDA */
	PAD_CFG_NF(GPP_H6, UP_2K, DEEP, NF1),
	/* H7  : I2C3_SCL */
	PAD_CFG_NF(GPP_H7, UP_2K, DEEP, NF1),
	/* H8  : I2C4_SDA */
	/* H9  : I2C4_SCL */
	/* H10 : I2C5_SDA_ISH_I2C2_SDA */
	PAD_CFG_GPO(GPP_H10, 1, PLTRST),
	/* H11 : I2C5_SCL_ISH_I2C2_SCL */
	PAD_CFG_GPO(GPP_H11, 1, PLTRST),
	/* H12 : M2_SKT2_CFG_0_DFLEXIO_0 */
	PAD_CFG_GPO(GPP_H12, 1, PLTRST),
	/* H13 : M2_SKT2_CFG_1_DFLEXIO_1 */
	PAD_CFG_GPO(GPP_H13, 1, PLTRST),
	/* H14 : M2_SKT2_CFG_2 */
	PAD_CFG_GPO(GPP_H14, 0, PLTRST),
	/* H15 : M2_SKT2_CFG_3 */
	PAD_CFG_GPO(GPP_H15, 1, PLTRST),
	/* H16 : CAM5_PWR_EN */
	PAD_CFG_GPO(GPP_H16, 1, PLTRST),
	/* H17 : CAM5_FLASH_STROBE */
	PAD_CFG_GPO(GPP_H17, 1, PLTRST),
	/* H18 : BOOTMPC */
	/* H19 : TIMESYNC_0 */
	PAD_CFG_GPO(GPP_H19, 1, PLTRST),
	/* H20 : IMGCLKOUT_1 */
	/* H21 : GPPC_H_21 */
	/* H22 : GPPC_H_22 */
	PAD_CFG_GPO(GPP_H22, 1, PLTRST),
	/* H23 : GPPC_H_23 */

	/* GPD */
	/* GPD_0  : BATLOWB */
	/* GPD_1  : ACPRESENT */
	/* GPD_2  : LAN_WAKEB */
	/* GPD_3  : PWRBTNB */
	/* GPD_4  : SLP_S3B */
	/* GPD_5  : SLP_S4B */
	/* GPD_6  : SLP_AB */
	/* GPD_7  : GPD_7 */
	/* GPD-8  : SUSCLK */
	/* GPD-9  : SLP_WLANB */
	/* GPD-10 : SLP_5B */
	/* GPD_11 : LANPHYPC */
};

/* Early pad configuration in bootblock */
static const struct pad_config early_gpio_table[] = {


};

const struct pad_config *__weak variant_gpio_table(size_t *num)
{
	*num = ARRAY_SIZE(gpio_table);
	return gpio_table;
}

const struct pad_config *__weak
	variant_early_gpio_table(size_t *num)
{
	*num = ARRAY_SIZE(early_gpio_table);
	return early_gpio_table;
}

static const struct cros_gpio cros_gpios[] = {
        CROS_GPIO_REC_AL(CROS_GPIO_VIRTUAL, CROS_GPIO_DEVICE_NAME),
};

const struct cros_gpio * __weak variant_cros_gpios(size_t *num)
{
        *num = ARRAY_SIZE(cros_gpios);
        return cros_gpios;
}
