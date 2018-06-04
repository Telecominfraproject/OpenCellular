/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2016 Rockchip, Inc.
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

#include <arch/io.h>
#include <assert.h>
#include <console/console.h>
#include <delay.h>
#include <soc/clock.h>
#include <soc/grf.h>
#include <soc/soc.h>
#include <soc/usb.h>

/* SuperSpeed over Type-C is hard. We don't care about speed in firmware: just
 * gate off the SuperSpeed lines to have an unimpaired USB 2.0 connection. */
static void isolate_tcphy(struct rk3399_tcphy *tcphy)
{
	write32(&tcphy->isolation_ctrl,
		TCPHY_ISOLATION_CTRL_EN |
		TCPHY_ISOLATION_CTRL_CMN_EN |
		TCPHY_ISOLATION_CTRL_MODE_SEL |
		TCPHY_ISOLATION_CTRL_LN_EN(7) |
		TCPHY_ISOLATION_CTRL_LN_EN(6) |
		TCPHY_ISOLATION_CTRL_LN_EN(5) |
		TCPHY_ISOLATION_CTRL_LN_EN(4) |
		TCPHY_ISOLATION_CTRL_LN_EN(3) |
		TCPHY_ISOLATION_CTRL_LN_EN(2) |
		TCPHY_ISOLATION_CTRL_LN_EN(1) |
		TCPHY_ISOLATION_CTRL_LN_EN(0));
}

static void tcphy_cfg_24m(struct rk3399_tcphy *tcphy)
{
	u32 i;

	/* cmn_ref_clk_sel = 3, select the 24Mhz for clk parent
	 * cmn_psm_clk_dig_div = 2, set the clk division to 2 */
	write32(&tcphy->pma_cmn_ctrl1, 2 << 10 | 3 << 4);
	for (i = 0; i < 4; i++) {
		/* The following PHY configuration assumes a
		 * 24 MHz reference clock */
		write32(&tcphy->lane[i].xcvr_diag_lane_fcm_en_mgn, 0x90);
		write32(&tcphy->lane[i].tx_rcvdet_en_tmr, 0x960);
		write32(&tcphy->lane[i].tx_rcvdet_st_tmr, 0x30);
	}

	clrsetbits_le32(&tcphy->cmn_diag_hsclk_sel,
			TCPHY_CMN_HSCLK_PLL_MASK, TCPHY_CMN_HSCLK_PLL_CONFIG);
}

static void tcphy_phy_init(struct rk3399_tcphy *tcphy)
{
	u32 i;

	tcphy_cfg_24m(tcphy);

	for (i = 0; i < 4; i++) {
		/* Enable transmitter reset pull down override for all lanes*/
		write32(&tcphy->lane[i].tx_diag_tx_drv, 0x2000);
		/* Disable transmitter low current mode, disable TX
		 * driver common mode, disable TX post-emphasis*/
		write32(&tcphy->lane[i].tx_psc_a2, 0x0000);
	}

	isolate_tcphy(tcphy);
}

static void reset_dwc3(struct rockchip_usb_dwc3 *dwc3)
{
	/* Before Resetting PHY, put Core in Reset */
	setbits_le32(&dwc3->ctl, DWC3_GCTL_CORESOFTRESET);
	/* Assert USB3 PHY reset */
	setbits_le32(&dwc3->usb3pipectl, DWC3_GUSB3PIPECTL_PHYSOFTRST);
	/* Assert USB2 PHY reset */
	setbits_le32(&dwc3->usb2phycfg, DWC3_GUSB2PHYCFG_PHYSOFTRST);
}

static void setup_dwc3(struct rockchip_usb_dwc3 *dwc3)
{
	u32 usb2phycfg = read32(&dwc3->usb2phycfg);
	u32 ctl = read32(&dwc3->ctl);

	/* Ensure reset_dwc3() has been called before this. */
	assert(ctl & DWC3_GCTL_CORESOFTRESET);

	/* Clear USB3 PHY reset (oddly enough, this is really necessary). */
	clrbits_le32(&dwc3->usb3pipectl, DWC3_GUSB3PIPECTL_PHYSOFTRST);

	/* Clear USB2 PHY and core reset. */
	usb2phycfg &= ~DWC3_GUSB2PHYCFG_PHYSOFTRST;
	ctl &= ~DWC3_GCTL_CORESOFTRESET;

	/* We are hard-coding DWC3 core to Host Mode */
	ctl &= ~DWC3_GCTL_PRTCAP_MASK;
	ctl |= DWC3_GCTL_PRTCAP_HOST;

	/*
	 * Configure USB phy interface of DWC3 core.
	 * For Rockchip rk3399 SOC DWC3 core:
	 * 1. Clear U2_FREECLK_EXITS.
	 * 2. Select UTMI+ PHY with 16-bit interface.
	 * 3. Set USBTRDTIM to the corresponding value
	 * according to the UTMI+ PHY interface.
	 */
	usb2phycfg &= ~(DWC3_GUSB2PHYCFG_U2_FREECLK_EXISTS |
			DWC3_GUSB2PHYCFG_USB2TRDTIM_MASK |
			DWC3_GUSB2PHYCFG_PHYIF_MASK);
	usb2phycfg |= DWC3_GUSB2PHYCFG_PHYIF(1) |
		      DWC3_GUSB2PHYCFG_USBTRDTIM(USBTRDTIM_UTMI_16_BIT);

	write32(&dwc3->usb2phycfg, usb2phycfg);
	write32(&dwc3->ctl, ctl);
}

void reset_usb_otg0(void)
{
	printk(BIOS_DEBUG, "Starting DWC3 and TCPHY reset for USB OTG0\n");

	/* Keep whole USB OTG0 controller in reset, then
	 * configure controller to work in USB 2.0 only mode. */
	write32(&cru_ptr->softrst_con[18], RK_SETBITS(1 << 5));
	write32(&rk3399_grf->usb3otg0_con1, RK_CLRSETBITS(0xf << 12, 1 << 0));
	write32(&cru_ptr->softrst_con[18], RK_CLRBITS(1 << 5));

	tcphy_phy_init(rockchip_usb_otg0_phy);

	/* Clear TCPHY0 reset */
	write32(&cru_ptr->softrst_con[9], RK_CLRBITS(1 << 5));

	reset_dwc3(rockchip_usb_otg0_dwc3);
}

void reset_usb_otg1(void)
{
	printk(BIOS_DEBUG, "Starting DWC3 and TCPHY reset for USB OTG1\n");

	/* Keep whole USB OTG1 controller in reset, then
	 * configure controller to work in USB 2.0 only mode. */
	write32(&cru_ptr->softrst_con[18], RK_SETBITS(1 << 6));
	write32(&rk3399_grf->usb3otg1_con1, RK_CLRSETBITS(0xf << 12, 1 << 0));
	write32(&cru_ptr->softrst_con[18], RK_CLRBITS(1 << 6));

	tcphy_phy_init(rockchip_usb_otg1_phy);

	/* Clear TCPHY1 reset */
	write32(&cru_ptr->softrst_con[9], RK_CLRBITS(1 << 13));

	reset_dwc3(rockchip_usb_otg1_dwc3);
}

void setup_usb_otg0(void)
{
	/* Clear pipe reset */
	write32(&cru_ptr->softrst_con[9], RK_CLRBITS(1 << 4));

	setup_dwc3(rockchip_usb_otg0_dwc3);
	printk(BIOS_DEBUG, "DWC3 and TCPHY setup for USB OTG0 finished\n");
}

void setup_usb_otg1(void)
{
	/* Clear pipe reset */
	write32(&cru_ptr->softrst_con[9], RK_CLRBITS(1 << 12));

	setup_dwc3(rockchip_usb_otg1_dwc3);
	printk(BIOS_DEBUG, "DWC3 and TCPHY setup for USB OTG1 finished\n");
}
