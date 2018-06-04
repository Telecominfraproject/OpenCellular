/*
 * This file is part of the coreboot project.
 *
 * Copyright 2016 Rockchip Inc.
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

#ifndef __SOC_ROCKCHIP_RK3399_ADDRESSMAP_H__
#define __SOC_ROCKCHIP_RK3399_ADDRESSMAP_H__

#define MAX_DRAM_ADDRESS	0xF8000000
#define PMUGRF_BASE		0xff320000
#define PMUSGRF_BASE		0xff330000
#define PMUCRU_BASE		0xff750000
#define CRU_BASE		0xff760000
#define GRF_BASE		0xff770000
#define TIMER0_BASE		0xff850000
#define EMMC_BASE		0xfe330000
#define SDMMC_BASE		0xfe320000

#define GPIO0_BASE		0xff720000
#define GPIO1_BASE		0xff730000
#define GPIO2_BASE		0xff780000
#define GPIO3_BASE		0xff788000
#define GPIO4_BASE		0xff790000

#define I2C0_BASE		0xff3c0000
#define I2C1_BASE		0xff110000
#define I2C2_BASE		0xff120000
#define I2C3_BASE		0xff130000
#define I2C4_BASE		0xff3d0000
#define I2C5_BASE		0xff140000
#define I2C6_BASE		0xff150000
#define I2C7_BASE		0xff160000
#define I2C8_BASE		0xff3e0000

#define UART0_BASE		0xff180000
#define UART1_BASE		0xff190000
#define UART2_BASE		0xff1a0000
#define UART3_BASE		0xff1b0000
#define UART4_BASE		0xff370000

#define SPI0_BASE		0xff1c0000
#define SPI1_BASE		0xff1d0000
#define SPI2_BASE		0xff1e0000
#define SPI3_BASE		0xff350000
#define SPI4_BASE		0xff1f0000
#define	SPI5_BASE		0xff200000

#define TSADC_BASE		0xff260000
#define SARADC_BASE		0xff100000
#define RK_PWM_BASE		0xff420000
#define EDP_BASE		0xff970000
#define MIPI0_BASE		0xff960000
#define MIPI1_BASE		0xff968000

#define VOP_BIG_BASE		0xff900000 /* corresponds to vop_id 0 */
#define VOP_LIT_BASE		0xff8f0000 /* corresponds to vop_id 1 */


#define DDRC0_BASE_ADDR		0xffa80000
#define SERVER_MSCH0_BASE_ADDR	0xffa84000
#define DDRC1_BASE_ADDR		0xffa88000
#define SERVER_MSCH1_BASE_ADDR	0xffa8c000
#define CIC_BASE_ADDR		0xff620000

#define USB_OTG0_DWC3_BASE	0xfe80c100
#define USB_OTG1_DWC3_BASE	0xfe90c100
#define USB_OTG0_TCPHY_BASE	0xff7c0000
#define USB_OTG1_TCPHY_BASE	0xff800000

#define IC_BASES  { I2C0_BASE, I2C1_BASE, I2C2_BASE, I2C3_BASE,		\
			I2C4_BASE, I2C5_BASE, I2C6_BASE, I2C7_BASE, I2C8_BASE }

#endif /* __SOC_ROCKCHIP_RK3399_ADDRESSMAP_H__ */
