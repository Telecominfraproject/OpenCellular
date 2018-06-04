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

#ifndef __SOC_ROCKCHIP_RK3399_GRF_H__
#define __SOC_ROCKCHIP_RK3399_GRF_H__

#include <soc/addressmap.h>
#include <soc/soc.h>
#include <types.h>

struct rk3399_grf_regs {
	u32 reserved[0x800];
	u32 usb3_perf_con0;
	u32 usb3_perf_con1;
	u32 usb3_perf_con2;
	u32 usb3_perf_rd_max_latency_num;
	u32 usb3_perf_rd_latency_samp_num;
	u32 usb3_perf_rd_latency_acc_num;
	u32 usb3_perf_rd_axi_total_byte;
	u32 usb3_perf_wr_axi_total_byte;
	u32 usb3_perf_working_cnt;
	u32 reserved1[0x103];
	u32 usb3otg0_con0;
	u32 usb3otg0_con1;
	u32 reserved2[2];
	u32 usb3otg1_con0;
	u32 usb3otg1_con1;
	u32 reserved3[2];
	u32 usb3otg0_status_lat0;
	u32 usb3otg0_status_lat1;
	u32 usb3otg0_status_cb;
	u32 reserved4;
	u32 usb3otg1_status_lat0;
	u32 usb3otg1_status_lat1;
	u32 usb3ogt1_status_cb;
	u32 reserved5[0x6e5];
	u32 pcie_perf_con0;
	u32 pcie_perf_con1;
	u32 pcie_perf_con2;
	u32 pcie_perf_rd_max_latency_num;
	u32 pcie_perf_rd_latency_samp_num;
	u32 pcie_perf_rd_laterncy_acc_num;
	u32 pcie_perf_rd_axi_total_byte;
	u32 pcie_perf_wr_axi_total_byte;
	u32 pcie_perf_working_cnt;
	u32 reserved6[0x37];
	u32 usb20_host0_con0;
	u32 usb20_host0_con1;
	u32 reserved7[2];
	u32 usb20_host1_con0;
	u32 usb20_host1_con1;
	u32 reserved8[2];
	u32 hsic_con0;
	u32 hsic_con1;
	u32 reserved9[6];
	u32 grf_usbhost0_status;
	u32 grf_usbhost1_Status;
	u32 grf_hsic_status;
	u32 reserved10[0xc9];
	u32 hsicphy_con0;
	u32 reserved11[3];
	u32 usbphy_ctrl[2][26 + 6]; /* 26 PHY regs, 6 reserved padding regs */
	u32 reserved13[0x729];
	u32 soc_con9;
	u32 reserved14[0x0a];
	u32 soc_con20;
	u32 soc_con21;
	u32 soc_con22;
	u32 soc_con23;
	u32 soc_con24;
	u32 soc_con25;
	u32 soc_con26;
	u32 reserved15[0xf65];
	u32 cpu_con[4];
	u32 reserved16[0x1c];
	u32 cpu_status[6];
	u32 reserved17[0x1a];
	u32 a53_perf_con[4];
	u32 a53_perf_rd_mon_st;
	u32 a53_perf_rd_mon_end;
	u32 a53_perf_wr_mon_st;
	u32 a53_perf_wr_mon_end;
	u32 a53_perf_rd_max_latency_num;
	u32 a53_perf_rd_latency_samp_num;
	u32 a53_perf_rd_laterncy_acc_num;
	u32 a53_perf_rd_axi_total_byte;
	u32 a53_perf_wr_axi_total_byte;
	u32 a53_perf_working_cnt;
	u32 a53_perf_int_status;
	u32 reserved18[0x31];
	u32 a72_perf_con[4];
	u32 a72_perf_rd_mon_st;
	u32 a72_perf_rd_mon_end;
	u32 a72_perf_wr_mon_st;
	u32 a72_perf_wr_mon_end;
	u32 a72_perf_rd_max_latency_num;
	u32 a72_perf_rd_latency_samp_num;
	u32 a72_perf_rd_laterncy_acc_num;
	u32 a72_perf_rd_axi_total_byte;
	u32 a72_perf_wr_axi_total_byte;
	u32 a72_perf_working_cnt;
	u32 a72_perf_int_status;
	u32 reserved19[0x7f6];
	u32 soc_con5;
	u32 soc_con6;
	u32 reserved20[0x779];
	u32 gpio2a_iomux;
	union {
		u32 iomux_spi2;
		u32 gpio2b_iomux;
	};
	union {
		u32 gpio2c_iomux;
		u32 iomux_spi5;
	};
	u32 gpio2d_iomux;
	union {
		u32 gpio3a_iomux;
		u32 iomux_spi0;
	};
	u32 gpio3b_iomux;
	u32 gpio3c_iomux;
	union {
		u32 iomux_i2s0;
		u32 gpio3d_iomux;
	};
	union {
		u32 iomux_i2sclk;
		u32 gpio4a_iomux;
	};
	union {
		u32 iomux_sdmmc;
		u32 iomux_uart2a;
		u32 gpio4b_iomux;
	};
	union {
		u32 iomux_pwm_0;
		u32 iomux_pwm_1;
		u32 iomux_uart2b;
		u32 iomux_uart2c;
		u32 iomux_edp_hotplug;
		u32 gpio4c_iomux;
	};
	u32 gpio4d_iomux;
	u32 reserved21[4];
	u32 gpio2_p[3][4];
	u32 reserved22[4];
	u32 gpio2_sr[3][4];
	u32 reserved23[4];
	u32 gpio2_smt[3][4];
	u32 reserved24[(0xe130 - 0xe0ec)/4 - 1];
	u32 gpio4b_e01;
	u32 gpio4b_e2;
	u32 reserved24a[(0xe200 - 0xe134)/4 - 1];
	u32 soc_con0;
	u32 soc_con1;
	u32 soc_con2;
	u32 soc_con3;
	u32 soc_con4;
	u32 soc_con5_pcie;
	u32 reserved25;
	u32 soc_con7;
	u32 soc_con8;
	u32 soc_con9_pcie;
	u32 reserved26[0x1e];
	u32 soc_status[6];
	u32 reserved27[0x32];
	u32 ddrc0_con0;
	u32 ddrc0_con1;
	u32 ddrc1_con0;
	u32 ddrc1_con1;
	u32 reserved28[0xac];
	u32 io_vsel;
	u32 saradc_testbit;
	u32 tsadc_testbit_l;
	u32 tsadc_testbit_h;
	u32 reserved29[0x6c];
	u32 chip_id_addr;
	u32 reserved30[0x1f];
	u32 fast_boot_addr;
	u32 reserved31[0x1df];
	u32 emmccore_con[12];
	u32 reserved32[4];
	u32 emmccore_status[4];
	u32 reserved33[0x1cc];
	u32 emmcphy_con[7];
	u32 reserved34;
	u32 emmcphy_status;
};
check_member(rk3399_grf_regs, emmcphy_status, 0xf7a0);

struct rk3399_pmugrf_regs {
	union {
		u32 iomux_pwm_3a;
		u32 gpio0a_iomux;
	};
	u32 gpio0b_iomux;
	u32 reserved0[2];
	union {
		u32 spi1_rxd;
		u32 tsadc_int;
		u32 gpio1a_iomux;
	};
	union {
		u32 spi1_csclktx;
		u32 iomux_pwm_3b;
		u32 iomux_i2c0_sda;
		u32 gpio1b_iomux;
	};
	union {
		u32 iomux_pwm_2;
		u32 iomux_i2c0_scl;
		u32 gpio1c_iomux;
	};
	u32 gpio1d_iomux;
	u32 reserved1[8];
	u32 gpio0_p[2][4];
	u32 reserved3[8];
	u32 gpio0a_e;
	u32 reserved4;
	u32 gpio0b_e;
	u32 reserved5[5];
	u32 gpio1a_e;
	u32 reserved6;
	u32 gpio1b_e;
	u32 reserved7;
	u32 gpio1c_e;
	u32 reserved8;
	u32 gpio1d_e;
	u32 reserved9[0x11];
	u32 gpio0l_sr;
	u32 reserved10;
	u32 gpio1l_sr;
	u32 gpio1h_sr;
	u32 reserved11[4];
	u32 gpio0a_smt;
	u32 gpio0b_smt;
	u32 reserved12[2];
	u32 gpio1a_smt;
	u32 gpio1b_smt;
	u32 gpio1c_smt;
	u32 gpio1d_smt;
	u32 reserved13[8];
	u32 gpio0l_he;
	u32 reserved14;
	u32 gpio1l_he;
	u32 gpio1h_he;
	u32 reserved15[4];
	u32 soc_con0;
	u32 reserved16[9];
	u32 soc_con10;
	u32 soc_con11;
	u32 reserved17[0x24];
	u32 pmupvtm_con0;
	u32 pmupvtm_con1;
	u32 pmupvtm_status0;
	u32 pmupvtm_status1;
	u32 grf_osc_e;
	u32 reserved18[0x2b];
	u32 os_reg0;
	u32 os_reg1;
	u32 os_reg2;
	u32 os_reg3;
};
check_member(rk3399_pmugrf_regs, os_reg3, 0x30c);

struct rk3399_pmusgrf_regs {
	u32 ddr_rgn_con[35];
	u32 reserved[0x1fe5];
	u32 soc_con8;
	u32 soc_con9;
	u32 soc_con10;
	u32 soc_con11;
	u32 soc_con12;
	u32 soc_con13;
	u32 soc_con14;
	u32 soc_con15;
	u32 reserved1[3];
	u32 soc_con19;
	u32 soc_con20;
	u32 soc_con21;
	u32 soc_con22;
	u32 reserved2[0x29];
	u32 perilp_con[9];
	u32 reserved4[7];
	u32 perilp_status;
	u32 reserved5[0xfaf];
	u32 soc_con0;
	u32 soc_con1;
	u32 reserved6[0x3e];
	u32 pmu_con[9];
	u32 reserved7[0x17];
	u32 fast_boot_addr;
	u32 reserved8[0x1f];
	u32 efuse_prg_mask;
	u32 efuse_read_mask;
	u32 reserved9[0x0e];
	u32 pmu_slv_con0;
	u32 pmu_slv_con1;
	u32 reserved10[0x771];
	u32 soc_con3;
	u32 soc_con4;
	u32 soc_con5;
	u32 soc_con6;
	u32 soc_con7;
	u32 reserved11[8];
	u32 soc_con16;
	u32 soc_con17;
	u32 soc_con18;
	u32 reserved12[0xdd];
	u32 slv_secure_con0;
	u32 slv_secure_con1;
	u32 reserved13;
	u32 slv_secure_con2;
	u32 slv_secure_con3;
	u32 slv_secure_con4;
};
check_member(rk3399_pmusgrf_regs, slv_secure_con4, 0xe3d4);

static struct rk3399_grf_regs * const rk3399_grf = (void *)GRF_BASE;
static struct rk3399_pmugrf_regs * const rk3399_pmugrf = (void *)PMUGRF_BASE;
static struct rk3399_pmusgrf_regs * const rk3399_pmusgrf = (void *)PMUSGRF_BASE;

#define UART2A_SEL	RK_CLRSETBITS(3 << 10, 0 << 10)
#define UART2B_SEL	RK_CLRSETBITS(3 << 10, 1 << 10)
#define UART2C_SEL	RK_CLRSETBITS(3 << 10, 2 << 10)
#define PWM3_SEL_A	RK_CLRBITS(1 << 5)
#define PWM3_SEL_B	RK_SETBITS(1 << 5)

#define IOMUX_UART2A	RK_CLRSETBITS(3 << 2 | 3 << 0, 2 << 2 | 2 << 0)
#define IOMUX_UART2B	RK_CLRSETBITS(3 << 2 | 3 << 0, 2 << 2 | 2 << 0)
#define IOMUX_UART2C	RK_CLRSETBITS(3 << 8 | 3 << 6, 1 << 8 | 1 << 6)
#define IOMUX_SPI0	RK_CLRSETBITS(0xff << 8, \
				      2 << 14 | 2 << 12 | 2 << 10 | 2 << 8)
#define IOMUX_SPI1_RX	RK_CLRSETBITS(3 << 14, 2 << 14)
#define IOMUX_SPI1_CSCLKTX	RK_CLRSETBITS(0x3f << 0, 2 << 4 |\
					      2 << 2 | 2 << 0)
#define IOMUX_SPI2	RK_CLRSETBITS(0xff << 2, 1 << 8 | 1 << 6 |\
				      1 << 4 | 1 << 2)
#define IOMUX_SPI5     RK_CLRSETBITS(0xff << 8, \
				     2 << 14 | 2 << 12 | 2 << 10 | 2 << 8)
#define IOMUX_SDMMC	RK_CLRSETBITS(0xfff, 1 << 10 | 1 << 8 | 1 << 6 |\
					     1 << 4 | 1 << 2 | 1 << 0)
#define IOMUX_I2C0_SCL	RK_CLRSETBITS(3 << 0, 2 << 0)
#define IOMUX_I2C0_SDA	RK_CLRSETBITS(3 << 14, 2 << 14)

#define IOMUX_I2S0_SD0	RK_SETBITS(1 << 14 | 1 << 6 | 1 << 4 | 1 << 2 | 1 << 0)
#define IOMUX_I2SCLK	RK_SETBITS(1 << 0)

#define IOMUX_PWM_0	RK_SETBITS(1 << 4)
#define IOMUX_PWM_1	RK_SETBITS(1 << 12)
#define IOMUX_PWM_2	RK_SETBITS(1 << 6)
#define IOMUX_PWM_3_A	RK_SETBITS(1 << 12)
#define IOMUX_PWM_3_B	RK_SETBITS(1 << 12)
#define IOMUX_TSADC_INT	RK_CLRSETBITS(3 << 12, 1 << 12)
#define IOMUX_EDP_HOTPLUG	RK_CLRSETBITS(3 << 14, 2 << 14)
#endif	/* __SOC_ROCKCHIP_RK3399_GRF_H__ */
