/*
 * This file is part of the coreboot project.
 *
 * Copyright 2017 Google Inc.
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
#include <variant/sku.h>

/* Pad configuration in ramstage */
/* Leave eSPI pins untouched from default settings */
static const struct pad_config gpio_table[] = {
	/* A0  : RCIN# ==> NC(TP22) */
	PAD_CFG_NC(GPP_A0),
	/* A1  : ESPI_IO0 */
	/* A2  : ESPI_IO1 */
	/* A3  : ESPI_IO2 */
	/* A4  : ESPI_IO3 */
	/* A5  : ESPI_CS# */
	/* A6  : SERIRQ ==> NC(TP24) */
	PAD_CFG_NC(GPP_A6),
	/* A7  : PIRQA# ==> NC(TP15) */
	PAD_CFG_NC(GPP_A7),
	/* A8  : CLKRUN# ==> NC(TP23) */
	PAD_CFG_NC(GPP_A8),
	/* A9  : ESPI_CLK */
	/* A10 : CLKOUT_LPC1 ==> NC */
	PAD_CFG_NC(GPP_A10),
	/* A11 : PME# ==> NC(TP46) */
	PAD_CFG_NC(GPP_A11),
	/* A12 : BM_BUSY# ==> NC */
	PAD_CFG_NC(GPP_A12),
	/* A13 : SUSWARN# ==> SUSWARN#_R */
	PAD_CFG_NF(GPP_A13, NONE, DEEP, NF1),
	/* A14 : ESPI_RESET# */
	/* A15 : SUSACK# ==> SUSACK# */
	PAD_CFG_NF(GPP_A15, NONE, DEEP, NF1),
	/* A16 : SD_1P8_SEL ==> NC */
	PAD_CFG_NC(GPP_A16),
	/* A17 : SD_PWR_EN# ==> NC */
	PAD_CFG_NC(GPP_A17),
	/* A18 : ISH_GP0 ==> EMMC_RST#L_R_SOC (unstuffed) */
	PAD_CFG_NC(GPP_A18),
	/* A19 : ISH_GP1 ==> NC */
	PAD_CFG_NC(GPP_A19),
	/* A20 : ISH_GP2 ==> NC */
	PAD_CFG_NC(GPP_A20),
	/* A21 : ISH_GP3 ==> NC */
	PAD_CFG_NC(GPP_A21),
	/* A22 : ISH_GP4 ==> NC */
	PAD_CFG_NC(GPP_A22),
	/* A23 : ISH_GP5 ==> PCH_SPK_EN */
	PAD_CFG_GPO(GPP_A23, 1, DEEP),

	/* B0  : CORE_VID0 ==> NC(T3) */
	PAD_CFG_NC(GPP_B0),
	/* B1  : CORE_VID1 ==> NC(T4) */
	PAD_CFG_NC(GPP_B1),
	/* B2  : VRALERT# ==> NC */
	PAD_CFG_NC(GPP_B2),
	/* B3  : CPU_GP2 ==> TOUCHSCREEN_RST# */
	PAD_CFG_GPO(GPP_B3, 0, DEEP),
	/* B4  : CPU_GP3 ==> EN_PP3300_DX_TOUCHSCREEN */
	PAD_CFG_GPO(GPP_B4, 0, DEEP),
	/* B5  : SRCCLKREQ0# ==> NC */
	PAD_CFG_NC(GPP_B5),
	/* B6  : SRCCLKREQ1# ==> CLKREQ_PCIE#1 */
	PAD_CFG_NF(GPP_B6, NONE, DEEP, NF1),
	/* B7  : SRCCLKREQ2# ==> CLKREQ_PCIE#2 */
	PAD_CFG_NF(GPP_B7, NONE, DEEP, NF1),
	/* B8  : SRCCLKREQ3# ==> CLKREQ_PCIE#3 */
	PAD_CFG_NF(GPP_B8, NONE, DEEP, NF1),
	/* B9  : SRCCLKREQ4# ==> WLAN_PE_RST_AP */
	PAD_CFG_GPO(GPP_B9, 0, RSMRST),
	/* B10 : SRCCLKREQ5# ==> NC */
	PAD_CFG_NC(GPP_B10),
	/* B11 : EXT_PWR_GATE# ==> NC */
	PAD_CFG_NC(GPP_B11),
	/* B12 : SLP_S0# ==> PM_SLP_R_S0# */
	PAD_CFG_NF(GPP_B12, NONE, DEEP, NF1),
	/* B13 : PLTRST# ==> PLT_RST#_PCH */
	PAD_CFG_NF(GPP_B13, NONE, DEEP, NF1),
	/* B14 : SPKR ==> EC_GPP_B14 (rsvd for later) */
	PAD_CFG_NC(GPP_B14),
	/* B15 : GSPI0_CS# ==> PCH_SPI_H1_3V3_CS# */
	PAD_CFG_NF(GPP_B15, NONE, DEEP, NF1),
	/* B16 : GSPI0_CLK ==> PCH_SPI_H1_3V3_CLK */
	PAD_CFG_NF(GPP_B16, NONE, DEEP, NF1),
	/* B17 : GSPI0_MISO ==> PCH_SPI_H1_3V3_MISO */
	PAD_CFG_NF(GPP_B17, NONE, DEEP, NF1),
	/* B18 : GSPI0_MOSI ==> PCH_SPI_H1_3V3_MOSI */
	PAD_CFG_NF(GPP_B18, NONE, DEEP, NF1),
	/* B19 : GSPI1_CS# ==> NC(TP26) */
	PAD_CFG_NC(GPP_B19),
	/* B20 : GSPI1_CLK ==> NC(TP27) */
	PAD_CFG_NC(GPP_B20),
	/* B21 : GSPI1_MISO ==> NC(TP28) */
	PAD_CFG_NC(GPP_B21),
	/* B22 : GSPI1_MOSI ==> NC(TP30) */
	PAD_CFG_NC(GPP_B22),
	/* B23 : SM1ALERT# ==> SOC_SML1ALERT# (unstuffed) */
	PAD_CFG_NC(GPP_B23),

	/* C0  : SMBCLK ==> SOC_SMBCLK */
	PAD_CFG_NF(GPP_C0, NONE, DEEP, NF1),
	/* C1  : SMBDATA ==> SOC_SMBDATA */
	PAD_CFG_NF(GPP_C1, NONE, DEEP, NF1),
	/* C2  : SMBALERT# ==> NC(TP917) */
	PAD_CFG_NC(GPP_C2),
	/* C3  : SML0CLK ==> TOUCHSCREEN_DIS# */
	PAD_CFG_GPO(GPP_C3, 0, DEEP),
	/* C4  : SML0DATA ==> NC */
	PAD_CFG_NC(GPP_C4),
	/* C5  : SML0ALERT# ==> SOC_SML0ALERT# (unstuffed) */
	PAD_CFG_NC(GPP_C5),
	/* C6  : SM1CLK ==> EC_IN_RW_OD */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_C6, 20K_PU, DEEP),
	/* C7  : SM1DATA ==> TRACKPAD_DISABLE# */
	PAD_CFG_GPO(GPP_C7, 1, DEEP),
	/* C8  : UART0_RXD ==> NC(TP31) */
	PAD_CFG_NC(GPP_C8),
	/* C9  : UART0_TXD ==> NC(TP32) */
	PAD_CFG_NC(GPP_C9),
	/* C10 : UART0_RTS# ==> EN_PP3300_DX_CAM1 */
	PAD_CFG_GPO(GPP_C10, 1, DEEP),
	/* C11 : UART0_CTS# ==> EN_PP3300_DX_CAM2 */
	PAD_CFG_GPO(GPP_C11, 1, DEEP),
	/* C12 : UART1_RXD ==> PCH_MEM_CONFIG0 */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_C12, NONE, DEEP),
	/* C13 : UART1_TXD ==> PCH_MEM_CONFIG1 */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_C13, NONE, DEEP),
	/* C14 : UART1_RTS# ==> PCH_MEM_CONFIG2 */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_C14, NONE, DEEP),
	/* C15 : UART1_CTS# ==> PCH_MEM_CONFIG3 */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_C15, NONE, DEEP),
	/* C16 : I2C0_SDA ==> I2C_0_SDA */
	PAD_CFG_NF(GPP_C16, NONE, DEEP, NF1),
	/* C17 : I2C0_SCL ==> I2C_0_SCL */
	PAD_CFG_NF(GPP_C17, NONE, DEEP, NF1),
	/* C18 : I2C1_SDA ==> I2C_1_SDA */
	PAD_CFG_NF(GPP_C18, NONE, DEEP, NF1),
	/* C19 : I2C1_SCL ==> I2C_1_SCL */
	PAD_CFG_NF(GPP_C19, NONE, DEEP, NF1),
	/* C20 : UART2_RXD ==> PCHRX_SERVOTX_UART */
	PAD_CFG_NF(GPP_C20, NONE, DEEP, NF1),
	/* C21 : UART2_TXD ==> PCHTX_SERVORX_UART */
	PAD_CFG_NF(GPP_C21, NONE, DEEP, NF1),
	/* C22 : UART2_RTS# ==> NC(TP926) */
	PAD_CFG_NC(GPP_C22),
	/* C23 : UART2_CTS# ==> PCH_WP */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_C23, 20K_PU, DEEP),

	/* D0  : SPI1_CS# ==> DDR_CHB_EN (for debugging) */
	PAD_CFG_NC(GPP_D0),
	/* D1  : SPI1_CLK ==> PEN_IRQ# */
	PAD_CFG_GPI_APIC(GPP_D1, NONE, PLTRST),
	/* D2  : SPI1_MISO ==> PEN_PDCT# */
	PAD_CFG_GPI_APIC(GPP_D2, NONE, PLTRST),
	/* D3  : SPI1_MOSI ==> PEN_RST# */
	PAD_CFG_GPO(GPP_D3, 0, DEEP),
	/* D4  : FASHTRIG ==> NC */
	PAD_CFG_NC(GPP_D4),
	/* D5  : ISH_I2C0_SDA ==> NC */
	PAD_CFG_NC(GPP_D5),
	/* D6  : ISH_I2C0_SCL ==> NC */
	PAD_CFG_NC(GPP_D6),
	/* D7  : ISH_I2C1_SDA ==> NC */
	PAD_CFG_NC(GPP_D7),
	/* D8  : ISH_I2C1_SCL ==> NC */
	PAD_CFG_NC(GPP_D8),
	/* D9  : ISH_SPI_CS# ==> HP_IRQ_GPIO */
	PAD_CFG_GPI_APIC(GPP_D9, NONE, PLTRST),
	/* D10 : ISH_SPI_CLK ==> SPKR_RST_L (unstuffed) */
	PAD_CFG_NC(GPP_D10),
	/* D11 : ISH_SPI_MISO ==> DCI_CLK (debug header) */
	PAD_CFG_NC(GPP_D11),
	/* D12 : ISH_SPI_MOSI ==> DCI_DATA (debug header) */
	PAD_CFG_NC(GPP_D12),
	/* D13 : ISH_UART0_RXD ==> H1_BOOT_UART_RX (unstuffed) */
	PAD_CFG_NC(GPP_D13),
	/* D14 : ISH_UART0_TXD ==> H1_BOOT_UART_TX (unstuffed) */
	PAD_CFG_NC(GPP_D14),
	/* D15 : ISH_UART0_RTS# ==> NC */
	PAD_CFG_NC(GPP_D15),
	/* D16 : ISH_UART0_CTS# ==> NC */
	PAD_CFG_NC(GPP_D16),
	/* D18 : DMIC_DATA1 ==> SOC_DMIC_DATA1_R */
	PAD_CFG_NF(GPP_D18, NONE, DEEP, NF1),
	/* D19 : DMIC_CLK0 ==> SOC_DMIC_CLK0_R */
	PAD_CFG_NF(GPP_D19, NONE, DEEP, NF1),
	/* D20 : DMIC_DATA0 ==> SOC_DMIC_DATA0_R */
	PAD_CFG_NF(GPP_D20, NONE, DEEP, NF1),
	/* D21 : SPI1_IO2 ==> DDR_CHA_EN (debugging) */
	PAD_CFG_NC(GPP_D21),
	/* D22 : SPI1_IO3 ==> BOOT_BEEP_OVERRIDE */
	PAD_CFG_GPO(GPP_D22, 1, DEEP),
	/* D23 : I2S_MCLK ==> I2S_1_MCLK */
	PAD_CFG_NF(GPP_D23, NONE, DEEP, NF1),

	/* E0  : SATAXPCI0 ==> H1_PCH_INT_ODL */
	PAD_CFG_GPI_APIC_INVERT(GPP_E0, NONE, PLTRST),
	/* E1  : SATAXPCIE1 ==> EMR_GARAGE_DET# - for wake event */
	PAD_CFG_GPI_ACPI_SCI(GPP_E1, NONE, DEEP, INVERT),
	/* E2  : SATAXPCIE2 ==> WLAN_OFF# */
	PAD_CFG_GPO(GPP_E2, 1, DEEP),
	/* E3  : CPU_GP0 ==> TRACKPAD_INT# */
	PAD_CFG_GPI_APIC(GPP_E3, NONE, PLTRST),
	/* E4  : SATA_DEVSLP0 ==> BT_OFF# */
	PAD_CFG_GPO(GPP_E4, 1, DEEP),
	/* E5  : SATA_DEVSLP1 ==> NC(TP928) */
	PAD_CFG_NC(GPP_E5),
	/* E6  : SATA_DEVSLP2 ==> NC(TP915) */
	PAD_CFG_NC(GPP_E6),
	/* E7  : CPU_GP1 ==> TOUCHSCREEN_INT# */
	PAD_CFG_GPI_APIC(GPP_E7, NONE, PLTRST),
	/* E8  : SATALED# ==> EMR_GARAGE_DET# - for notification */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_E8, NONE, DEEP),
	/* E9  : USB2_OCO# ==> USB_C0_OC# */
	PAD_CFG_NF(GPP_E9, NONE, DEEP, NF1),
	/* E10 : USB2_OC1# ==> USB_C1_OC# */
	PAD_CFG_NF(GPP_E10, NONE, DEEP, NF1),
	/* E11 : USB2_OC2# ==> USB_A0_OC# */
	PAD_CFG_NF(GPP_E11, NONE, DEEP, NF1),
	/* E12 : USB2_OC3# ==> NC */
	PAD_CFG_NC(GPP_E12),
	/* E13 : DDPB_HPD0 ==> USB_C0_DP_HPD */
	PAD_CFG_NF(GPP_E13, 20K_PD, DEEP, NF1),
	/* E14 : DDPC_HPD1 ==> USB_C1_DP_HPD */
	PAD_CFG_NF(GPP_E14, 20K_PD, DEEP, NF1),
	/* E15 : DDPD_HPD2 ==> DDR_SEL */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_E15, NONE, DEEP),
	/* E16 : DDPE_HPD3 ==> TRACKPAD_INT# */
	PAD_CFG_GPI_ACPI_SCI(GPP_E16, NONE, DEEP, INVERT),
	/* E17 : EDP_HPD ==> EDP_HPD */
	PAD_CFG_NF(GPP_E17, NONE, DEEP, NF1),
	/* E18 : DDPB_CTRLCLK ==> SOC_DP1_CTRL_CLK */
	PAD_CFG_NF(GPP_E18, NONE, DEEP, NF1),
	/* E19 : DDPB_CTRLDATA ==> SOC_DP1_CTRL_DATA */
	PAD_CFG_NF(GPP_E19, NONE, DEEP, NF1),
	/* E20 : DDPC_CTRLCLK ==> SOC_DP2_CTRL_CLK */
	PAD_CFG_NF(GPP_E20, NONE, DEEP, NF1),
	/* E21 : DDPC_CTRLDATA ==> SOC_DP2_CTRL_DATA */
	PAD_CFG_NF(GPP_E21, NONE, DEEP, NF1),
	/* E22 : DDPD_CTRLCLK ==> WLAN_PCIE_WAKE# */
	PAD_CFG_GPI_ACPI_SCI(GPP_E22, NONE, DEEP, INVERT),
	/* E23 : DDPD_CTRLDATA ==> NC(TP17)*/
	PAD_CFG_NC(GPP_E23),

	/* The next 4 pads are for bit banging the amplifiers, default to I2S */
	/* F0  : I2S2_SCLK ==> I2S2_2_BCLK_R */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_F0, NONE, DEEP),
	/* F1  : I2S2_SFRM ==> I2S2_2_FS_LRC */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_F1, NONE, DEEP),
	/* F2  : I2S2_TXD ==> I2S2_2_TX_DAC */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_F2, NONE, DEEP),
	/* F3  : I2S2_RXD ==> NC */
	PAD_CFG_NC(GPP_F3),
	/* F4  : I2C2_SDA ==> I2C_2_SDA */
	PAD_CFG_NF_1V8(GPP_F4, NONE, DEEP, NF1),
	/* F5  : I2C2_SCL ==> I2C_2_SCL */
	PAD_CFG_NF_1V8(GPP_F5, NONE, DEEP, NF1),
	/* F6  : I2C3_SDA ==> I2C_3_SDA */
	PAD_CFG_NF_1V8(GPP_F6, NONE, DEEP, NF1),
	/* F7  : I2C3_SCL ==> I2C_3_SCL */
	PAD_CFG_NF_1V8(GPP_F7, NONE, DEEP, NF1),
	/* F8  : I2C4_SDA ==> I2C_4_SDA (unstuffed) */
	PAD_CFG_NC(GPP_F8),
	/* F9  : I2C4_SCL ==> I2C_4_SCL (unstuffed) */
	PAD_CFG_NC(GPP_F9),
	/* F10 : I2C5_SDA ==> NC */
	PAD_CFG_NC(GPP_F10),
	/* F11 : I2C5_SCL ==> NC */
	PAD_CFG_NC(GPP_F11),
	/* F12 : EMMC_CMD ==> EMMC_1_CMD */
	PAD_CFG_NF(GPP_F12, NONE, DEEP, NF1),
	/* F13 : EMMC_DATA0 ==> EMMC_1_D0 */
	PAD_CFG_NF(GPP_F13, NONE, DEEP, NF1),
	/* F14 : EMMC_DATA1 ==> EMMC_1_D1 */
	PAD_CFG_NF(GPP_F14, NONE, DEEP, NF1),
	/* F15 : EMMC_DATA2 ==> EMMC_1_D2 */
	PAD_CFG_NF(GPP_F15, NONE, DEEP, NF1),
	/* F16 : EMMC_DATA3 ==> EMMC_1_D3 */
	PAD_CFG_NF(GPP_F16, NONE, DEEP, NF1),
	/* F17 : EMMC_DATA4 ==> EMMC_1_D4 */
	PAD_CFG_NF(GPP_F17, NONE, DEEP, NF1),
	/* F18 : EMMC_DATA5 ==> EMMC_1_D5 */
	PAD_CFG_NF(GPP_F18, NONE, DEEP, NF1),
	/* F19 : EMMC_DATA6 ==> EMMC_1_D6 */
	PAD_CFG_NF(GPP_F19, NONE, DEEP, NF1),
	/* F20 : EMMC_DATA7 ==> EMMC_1_D7 */
	PAD_CFG_NF(GPP_F20, NONE, DEEP, NF1),
	/* F21 : EMMC_RCLK ==> EMMC_1_RCLK */
	PAD_CFG_NF(GPP_F21, NONE, DEEP, NF1),
	/* F22 : EMMC_CLK ==> EMMC_1_CLK */
	PAD_CFG_NF(GPP_F22, NONE, DEEP, NF1),
	/* F23 : RSVD ==> NC */
	PAD_CFG_NC(GPP_F23),

	/* G0  : SD_CMD ==> NC */
	PAD_CFG_NC(GPP_G0),
	/* G1  : SD_DATA0 ==> NC */
	PAD_CFG_NC(GPP_G1),
	/* G2  : SD_DATA1 ==> NC */
	PAD_CFG_NC(GPP_G2),
	/* G3  : SD_DATA2 ==> NC */
	PAD_CFG_NC(GPP_G3),
	/* G4  : SD_DATA3 ==> NC */
	PAD_CFG_NC(GPP_G4),
	/* G5  : SD_CD# ==> NC */
	PAD_CFG_NC(GPP_G5),
	/* G6  : SD_CLK ==> NC */
	PAD_CFG_NC(GPP_G6),
	/* G7  : SD_WP ==> SD_WP (not needed) */
	PAD_CFG_NC(GPP_G7),

	/* GPD0: BATLOW# ==> PCH_BATLOW# */
	PAD_CFG_NF(GPD0, NONE, DEEP, NF1),
	/* GPD1: ACPRESENT ==> EC_PCH_ACPRESENT */
	PAD_CFG_NF(GPD1, NONE, DEEP, NF1),
	/* GPD2: LAN_WAKE# ==> EC_PCH_WAKE_R# */
	PAD_CFG_NF(GPD2, NONE, DEEP, NF1),
	/* GPD3: PWRBTN# ==> EC_PCH_PWR_BTN_R_BTN# */
	PAD_CFG_NF(GPD3, 20K_PU, DEEP, NF1),
	/* GPD4: SLP_S3# ==> SLP_S3# */
	PAD_CFG_NF(GPD4, NONE, DEEP, NF1),
	/* GPD5: SLP_S4# ==> SLP_S4# */
	PAD_CFG_NF(GPD5, NONE, DEEP, NF1),
	/* GPD6: SLP_A# ==> NC(TP44) */
	PAD_CFG_NC(GPD6),
	/* GPD7: RSVD ==> NC */
	PAD_CFG_NC(GPD7),
	/* GPD8: SUSCLK ==> PCH_SUSCLK */
	PAD_CFG_NF(GPD8, NONE, DEEP, NF1),
	/* GPD9: SLP_WLAN# ==> NC(TP41) */
	PAD_CFG_NC(GPD9),
	/* GPD10: SLP_S5# ==> NC(TP38) */
	PAD_CFG_NC(GPD10),
	/* GPD11: LANPHYC ==> NC */
	PAD_CFG_NC(GPD11),
};

/* Early pad configuration in bootblock */
static const struct pad_config early_gpio_table[] = {
	/* B15 : GSPI0_CS# ==> PCH_SPI_H1_3V3_CS# */
	PAD_CFG_NF(GPP_B15, NONE, DEEP, NF1),
	/* B16 : GSPI0_CLK ==> PCH_SPI_H1_3V3_CLK */
	PAD_CFG_NF(GPP_B16, NONE, DEEP, NF1),
	/* B17 : GSPI0_MISO ==> PCH_SPI_H1_3V3_MISO */
	PAD_CFG_NF(GPP_B17, NONE, DEEP, NF1),
	/* B18 : GSPI0_MOSI ==> PCH_SPI_H1_3V3_MOSI */
	PAD_CFG_NF(GPP_B18, NONE, DEEP, NF1),

	/* Ensure UART pins are in native mode for H1. */
	/* C20 : UART2_RXD ==> PCHRX_SERVOTX_UART */
	PAD_CFG_NF(GPP_C20, NONE, DEEP, NF1),
	/* C21 : UART2_TXD ==> PCHTX_SERVORX_UART */
	PAD_CFG_NF(GPP_C21, NONE, DEEP, NF1),

	/* C23 : UART2_CTS# ==> PCH_WP */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_C23, 20K_PU, DEEP),

	/* E0  : SATAXPCI0 ==> H1_PCH_INT_ODL */
	PAD_CFG_GPI_APIC_INVERT(GPP_E0, NONE, PLTRST),
};

const struct pad_config *variant_gpio_table(size_t *num)
{
	*num = ARRAY_SIZE(gpio_table);
	return gpio_table;
}

const struct pad_config *variant_early_gpio_table(size_t *num)
{
	*num = ARRAY_SIZE(early_gpio_table);
	return early_gpio_table;
}

static const struct pad_config nami_default_sku_gpio_table[] = {
	/* D17 : DMIC_CLK1 ==> SOC_DMIC_CLK1 */
	PAD_CFG_NF(GPP_D17, NONE, DEEP, NF1),
};

static const struct pad_config vayne_sku_gpio_table[] = {
	/* D17 : DMIC_CLK1 ==> NC */
	PAD_CFG_NC(GPP_D17),
};

const struct pad_config *variant_sku_gpio_table(size_t *num)
{
	uint32_t sku_id = variant_board_sku();
	const struct pad_config *board_gpio_tables;
	switch (sku_id) {
	case SKU_1_VAYNE:
	case SKU_2_VAYNE:
		*num = ARRAY_SIZE(vayne_sku_gpio_table);
		board_gpio_tables = vayne_sku_gpio_table;
		break;
	default:
		*num = ARRAY_SIZE(nami_default_sku_gpio_table);
		board_gpio_tables = nami_default_sku_gpio_table;
		break;
	}
	return board_gpio_tables;
}
