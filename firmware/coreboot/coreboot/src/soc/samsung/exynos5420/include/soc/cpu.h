/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2010 Samsung Electronics
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

#ifndef CPU_SAMSUNG_EXYNOS5420_CPU_H
#define CPU_SAMSUNG_EXYNOS5420_CPU_H

#include <arch/io.h>
#include <symbols.h>

#define EXYNOS5_SRAM_BASE		0x02020000

/* Base address registers */
#define EXYNOS5420_GPIO_PART6_BASE	0x03860000 /* Z0 */
#define EXYNOS5_PRO_ID			0x10000000
#define EXYNOS5_CLOCK_BASE		0x10010000
#define EXYNOS5_POWER_BASE		0x10040000
#define EXYNOS5_SYSREG_BASE		0x10050000
#define EXYNOS5_TZPC1_DECPROT1SET	0x10110810
#define EXYNOS5_MULTI_CORE_TIMER_BASE	0x101C0000
#define EXYNOS5_WATCHDOG_BASE		0x101D0000
#define EXYNOS5_ACE_SFR_BASE            0x10830000
#define EXYNOS5_DMC_PHY0_BASE		0x10C00000
#define EXYNOS5_DMC_PHY1_BASE		0x10C10000
#define EXYNOS5420_DMC_DREXI_0		0x10C20000
#define EXYNOS5420_DMC_DREXI_1		0x10C30000
#define EXYNOS5420_DMC_TZASC_0		0x10D40000
#define EXYNOS5420_DMC_TZASC_1		0x10D50000
#define EXYNOS5420_USB_DRD0_XHCI_BASE	0x12000000
#define EXYNOS5420_USB_DRD0_DWC3_BASE	0x1200C100
#define EXYNOS5420_USB_DRD0_PHY_BASE	0x12100000
#define EXYNOS5_USB_HOST_EHCI_BASE	0x12110000
#define EXYNOS5_USB_HOST_PHY_BASE	0x12130000
#define EXYNOS5_MMC_BASE		0x12200000
#define EXYNOS5_MSHC_BASE		0x12240000
#define EXYNOS5_SROMC_BASE		0x12250000
#define EXYNOS5420_USB_DRD1_XHCI_BASE	0x12400000
#define EXYNOS5420_USB_DRD1_DWC3_BASE	0x1240C100
#define EXYNOS5420_USB_DRD1_PHY_BASE	0x12500000
#define EXYNOS5_UART0_BASE		0x12C00000
#define EXYNOS5_UART1_BASE		0x12C10000
#define EXYNOS5_UART2_BASE		0x12C20000
#define EXYNOS5_UART3_BASE		0x12C30000
#define EXYNOS5_I2C_BASE		0x12C60000
#define EXYNOS5_SPI0_BASE		0x12D20000
#define EXYNOS5_SPI1_BASE		0x12D30000
#define EXYNOS5_SPI2_BASE		0x12D40000
#define EXYNOS5_I2S_BASE		0x12D60000
#define EXYNOS5420_I2C_8910_BASE	0x12E00000
#define EXYNOS5_UART_ISP_BASE		0x13190000
#define EXYNOS5_SPI_ISP_BASE		0x131A0000
#define EXYNOS5420_GPIO_PART1_BASE	0x13400000 /* Y0 */
#define EXYNOS5420_GPIO_PART2_BASE	0x13400C00 /* X0..3 */
#define EXYNOS5420_GPIO_PART3_BASE	0x13410000 /* C0..4, D1, Y0..6 */
#define EXYNOS5420_GPIO_PART4_BASE	0x14000000 /* E0..1, F0..1, G0..2, J4 */
#define EXYNOS5420_GPIO_PART5_BASE	0x14010000 /* A0..2, B0..4, H0 */
#define EXYNOS5420_MIPI_DSIM_BASE	0x14500000
#define EXYNOS5_DP0_BASE		0x14510000
#define EXYNOS5_DP1_BASE		0x145B0000

/* Marker values stored at the bottom of IRAM stack by SPL */
#define EXYNOS5_SPL_MARKER	0xb004f1a9	/* hexspeak word: bootflag */

#define RST_FLAG_VAL			0xfcba0d10

#define EXYNOS5_SPI_NUM_CONTROLLERS	5
#define EXYNOS_I2C_MAX_CONTROLLERS	8

extern struct tmu_info exynos5420_tmu_info;

/* TODO clean up defines. */
#define FB_SIZE_KB  4096
#define RAM_BASE_KB ((uintptr_t)_dram/KiB)
#define RAM_SIZE_KB (CONFIG_DRAM_SIZE_MB << 10UL)

static inline u32 get_fb_base_kb(void)
{
	return RAM_BASE_KB + RAM_SIZE_KB - FB_SIZE_KB;
}

/* Procedures to setup Exynos5420 CPU */
void exynos5420_config_smp(void);

#endif /* CPU_SAMSUNG_EXYNOS5420_CPU_H */
