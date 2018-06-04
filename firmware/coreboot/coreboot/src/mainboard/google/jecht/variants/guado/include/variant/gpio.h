/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Google Inc.
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

#ifndef GUADO_GPIO_H
#define GUADO_GPIO_H

#include <soc/gpio.h>

static const struct gpio_config mainboard_gpio_config[] = {
	PCH_GPIO_UNUSED,        /* 0: UNUSED */
	PCH_GPIO_UNUSED,        /* 1: UNUSED */
	PCH_GPIO_UNUSED,        /* 2: UNUSED */
	PCH_GPIO_UNUSED,        /* 3: UNUSED */
	PCH_GPIO_NATIVE,        /* 4: NATIVE: I2C0_SDA_GPIO4 */
	PCH_GPIO_NATIVE,        /* 5: NATIVE: I2C0_SCL_GPIO5 */
	PCH_GPIO_NATIVE,        /* 6: NATIVE: I2C1_SDA_GPIO6 */
	PCH_GPIO_NATIVE,        /* 7: NATIVE: I2C1_SCL_GPIO7 */
	PCH_GPIO_ACPI_SCI,      /* 8: LAN_WAKE_L_Q */
	PCH_GPIO_OUT_HIGH,      /* 9: PP3300_WLAN_EN */
	PCH_GPIO_ACPI_SCI,      /* 10: WLAN_WAKE_L_Q */
	PCH_GPIO_UNUSED,        /* 11: SMBALERT */
	PCH_GPIO_INPUT_INVERT,  /* 12: RECOVERY_L */
	PCH_GPIO_UNUSED,        /* 13: UNUSED */
	PCH_GPIO_UNUSED,        /* 14: UNUSED */
	PCH_GPIO_UNUSED,        /* 15: UNUSED (STRAP) */
	PCH_GPIO_UNUSED,        /* 16: UNUSED */
	PCH_GPIO_OUT_HIGH,      /* 17: PP3300_VP8_EN */
	PCH_GPIO_UNUSED,        /* 18: UNUSED */
	PCH_GPIO_UNUSED,        /* 19: UNUSED */
	PCH_GPIO_NATIVE,        /* 20: NATIVE: CLK_PCIE_REQ2# */
	PCH_GPIO_NATIVE,        /* 21: NATIVE: CLK_PCIE_REQ3# */
	PCH_GPIO_NATIVE,        /* 22: NATIVE: CLK_PCIE_REQ4# */
	PCH_GPIO_UNUSED,        /* 23: UNUSED */
	PCH_GPIO_OUT_HIGH,      /* 24: WLAN_OFF_L */
	PCH_GPIO_UNUSED,        /* 25: UNUSED */
	PCH_GPIO_OUT_HIGH,      /* 26: USB_CTL_1 */
	PCH_GPIO_UNUSED,        /* 27: UNUSED */
	PCH_GPIO_OUT_LOW,      /* 28: USB_ILIM_SEL */
	PCH_GPIO_UNUSED,        /* 29: UNUSED */
	PCH_GPIO_NATIVE,        /* 30: NATIVE: PCH_SUSPWRACK_L */
	PCH_GPIO_NATIVE,        /* 31: NATIVE: PCH_ACPRESENT */
	PCH_GPIO_NATIVE,        /* 32: NATIVE: CLKRUN# */
	PCH_GPIO_NATIVE,        /* 33: NATIVE: DEVSLP0 */
	PCH_GPIO_ACPI_SMI,      /* 34: EC_SMI_L */
	PCH_GPIO_ACPI_SMI,      /* 35: PCH_NMI_DBG_L (route in NMI_EN) */
	PCH_GPIO_ACPI_SCI,      /* 36: EC_SCI_L */
	PCH_GPIO_UNUSED,        /* 37: UNUSED */
	PCH_GPIO_UNUSED,        /* 38: UNUSED */
	PCH_GPIO_UNUSED,        /* 39: UNUSED */
	PCH_GPIO_NATIVE,        /* 40: NATIVE: USB_OC0# */
	PCH_GPIO_NATIVE,        /* 41: NATIVE: USB_OC1# */
	PCH_GPIO_NATIVE,        /* 42: NATIVE: USB_OC2# */
	PCH_GPIO_NATIVE,        /* 43: NATIVE: USB_OC3# */
	PCH_GPIO_UNUSED,        /* 44: UNUSED */
	PCH_GPIO_OUT_HIGH,      /* 45: PP5000_CODEC_EN */
	PCH_GPIO_OUT_HIGH,      /* 46: BT_DISABLE_L */
	PCH_GPIO_OUT_HIGH,      /* 47: USB1_PWR_EN */
	PCH_GPIO_UNUSED,        /* 48: UNUSED */
	PCH_GPIO_OUT_HIGH,      /* 49: POWER_LED */
	PCH_GPIO_OUT_HIGH,      /* 50: VP8_DISABLE_L */
	PCH_GPIO_UNUSED,        /* 51: UNUSED */
	PCH_GPIO_UNUSED,        /* 52: UNUSED */
	PCH_GPIO_UNUSED,        /* 53: UNUSED */
	PCH_GPIO_UNUSED,        /* 54: UNUSED */
	PCH_GPIO_UNUSED,        /* 55: UNUSED */
	PCH_GPIO_OUT_HIGH,      /* 56: USB2_PWR_EN */
	PCH_GPIO_OUT_HIGH,      /* 57: USB3_PWR_EN */
	PCH_GPIO_INPUT,         /* 58: PCH_SPI_WP_D */
	PCH_GPIO_OUT_HIGH,      /* 59: PP3300_LAN_EN */
	PCH_GPIO_NATIVE,        /* 60: NATIVE: SMB0ALERT# */
	PCH_GPIO_UNUSED,        /* 61: UNUSED */
	PCH_GPIO_UNUSED,        /* 62: UNUSED */
	PCH_GPIO_NATIVE,        /* 63: NATIVE: PCH_SLP_S5_L */
	PCH_GPIO_UNUSED,        /* 64: UNUSED */
	PCH_GPIO_UNUSED,        /* 65: UNUSED */
	PCH_GPIO_UNUSED,        /* 66: UNUSED (STRAP) */
	PCH_GPIO_UNUSED,        /* 67: UNUSED */
	PCH_GPIO_UNUSED,        /* 68: UNUSED */
	PCH_GPIO_UNUSED,        /* 69: UNUSED */
	PCH_GPIO_UNUSED,        /* 70: UNUSED */
	PCH_GPIO_NATIVE,        /* 71: NATIVE: MODPHY_EN */
	PCH_GPIO_UNUSED,        /* 72: UNUSED */
	PCH_GPIO_UNUSED,        /* 73: UNUSED */
	PCH_GPIO_NATIVE,        /* 74: NATIVE: SMB_ME1_DAT */
	PCH_GPIO_NATIVE,        /* 75: NATIVE: SMB_ME1_CLK */
	PCH_GPIO_UNUSED,        /* 76: UNUSED */
	PCH_GPIO_UNUSED,        /* 77: UNUSED */
	PCH_GPIO_UNUSED,        /* 78: UNUSED */
	PCH_GPIO_UNUSED,        /* 79: UNUSED */
	PCH_GPIO_UNUSED,        /* 80: UNUSED */
	PCH_GPIO_NATIVE,        /* 81: NATIVE: SPKR */
	PCH_GPIO_NATIVE,        /* 82: NATIVE: EC_RCIN_L */
	PCH_GPIO_UNUSED,        /* 83: UNUSED */
	PCH_GPIO_UNUSED,        /* 84: UNUSED */
	PCH_GPIO_UNUSED,        /* 85: UNUSED */
	PCH_GPIO_UNUSED,        /* 86: UNUSED (STRAP) */
	PCH_GPIO_UNUSED,        /* 87: UNUSED */
	PCH_GPIO_UNUSED,        /* 88: UNUSED */
	PCH_GPIO_UNUSED,        /* 89: UNUSED */
	PCH_GPIO_UNUSED,        /* 90: UNUSED */
	PCH_GPIO_UNUSED,        /* 91: UNUSED */
	PCH_GPIO_UNUSED,        /* 92: UNUSED */
	PCH_GPIO_UNUSED,        /* 93: UNUSED */
	PCH_GPIO_UNUSED,        /* 94: UNUSED */
	PCH_GPIO_END
};

#endif
