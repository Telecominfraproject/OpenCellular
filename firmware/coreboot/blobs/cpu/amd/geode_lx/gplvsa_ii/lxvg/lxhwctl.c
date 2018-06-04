/*
* Copyright (c) 2007-2008 Advanced Micro Devices,Inc. ("AMD").
*
* This library is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 2.1 of the
* License, or (at your option) any later version.
*
* This code is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General
* Public License along with this library; if not, write to the
* Free Software Foundation, Inc., 59 Temple Place, Suite 330,
* Boston, MA 02111-1307 USA
*/
//	  This module contains the routines that access the LX hardware.


#include "lxvg.h"
#include "vsa2.h"
#include "vr.h"
#include "pci.h"

// This is required here because of inclusion problems in LXVG.h
extern Hardware SystemInfo;


//---------------------------------------------------------------------------
// hw_initialize
//
// This routine performs the device dependent initialization.
//---------------------------------------------------------------------------

void hw_initialize(unsigned short config)
{
	unsigned long base,tbase;
	struct mValue mVal;
	unsigned short i;

	// Find the list of devices so we can get to their MSRs.  If any one
	// device is missing, bail out because we don't know how to handle
	// partial systems.

	if (FALSE == msrInit())
	{
		VGState = SF_DISABLED;
		return;
	}


	// Initialize the MSRs for the graphics processor
	hw_gp_msr_init();

	// Initialize the MSRs for the video generator
	hw_vg_msr_init();

	// Initialize the MSRs for the display filter
	hw_df_msr_init();

	// Initialize the MSRs for the display filter
	hw_vip_msr_init();

	// Initialize the DOT PLL MSR in the MCP
	hw_mcp_msr_init();

	// Compute graphics memory requirement from config.	 The following strips
	// off the PLL bypass bit and shifts the number of 1MB hunks up
	// to the point where it is a size in bytes.  Base is used as a temp.
	base = (unsigned long)(config & MEM_SIZE_MASK) << 20;

	// Now compute the memory size mask we will use when we return the contents of
	// BAR0
	VGdata.pci_fb_mask = 0;
	tbase = base;
	for (i=0;i<32,tbase!=0L;i++)
	{
		VGdata.pci_fb_mask |= (1L << i);
		tbase = tbase >> 1;
	}
	VGdata.pci_fb_mask = ~(VGdata.pci_fb_mask);

	// ALLOCATE DESCRIPTORS
	vga_config_addr = SYS_ALLOCATE_RESOURCE(RESOURCE_MEMORY, BAR0, base, PCI_DEV_ID, ID_MC);	// Graphics memory
	SYS_ALLOCATE_RESOURCE(RESOURCE_MMIO, BAR1, SIZE16K, PCI_DEV_ID, ID_GP);		// GP registers
	SYS_ALLOCATE_RESOURCE(RESOURCE_MMIO, BAR2, SIZE16K, PCI_DEV_ID, ID_VG);		// VG registers
	SYS_ALLOCATE_RESOURCE(RESOURCE_MMIO, BAR3, SIZE16K, PCI_DEV_ID, ID_DF);		// DF registers
	SYS_ALLOCATE_RESOURCE(RESOURCE_MMIO, BAR4, SIZE16K, PCI_DEV_ID, ID_VIP);	// VIP registers

	// Modify vga_config_addr to point to the beginning of the header, because all the accesses
	// to the header are based on the beginning of the header rather than BAR0.
	vga_config_addr -= BAR0;

	// SET BASE ADDRESS VALUES.

	// The base addresses are all relative to the framebuffer base address which
	// is defined to be on the next 256MB boundary above the SYSMGR.  Initially,
	// it was hard coded to be at 0x50000000, and now it should show up at 0x90000000.
	framebuffer_base = (SYS_LOGICAL_TO_PHYSICAL(0) & 0xF0000000) + 0x10000000;
	WRITE_PCI_DWORD_NO_TRAP(vga_config_addr+BAR0, framebuffer_base);	// Frame buffer address

	GPregister_base = framebuffer_base - 0x00004000;
	WRITE_PCI_DWORD_NO_TRAP(vga_config_addr+BAR1, GPregister_base);	// Graphics processor register space

	VGregister_base = GPregister_base - 0x00004000;
	WRITE_PCI_DWORD_NO_TRAP(vga_config_addr+BAR2, VGregister_base);	// Video generator register space

	DFregister_base = VGregister_base - 0x00004000;
	WRITE_PCI_DWORD_NO_TRAP(vga_config_addr+BAR3, DFregister_base);	// Display filter register space

	VIPregister_base = DFregister_base - 0x00004000;
	WRITE_PCI_DWORD_NO_TRAP(vga_config_addr+BAR4, VIPregister_base);	// Display filter register space

	// Turn on the descriptors
	WRITE_PCI_BYTE(vga_config_addr+0x04,PCI_MEM_SPACE);

	// Unlock the display controller registers
	saveLock = read_vg_32(DC_UNLOCK);
	write_vg_32(DC_UNLOCK, DC_UNLOCK_VALUE);

	// Set up the DV Address offset in the DC_DV_CTL register to the offset from frame
	// buffer descriptor.  First, get the frame buffer descriptor so we can set the
	// DV Address Offset in the DV_CTL register.  Because this is a pointer to real
	// silicon memory, we don't need to do this whenever we change the framebuffer BAR,
	// so it isn't included in the hw_fb_map_init routine.
	SYS_MBUS_DESCRIPTOR((unsigned short)(vga_config_addr+BAR0),(void *)&mVal);
	mVal.high &= DESC_OFFSET_MASK;
	mVal.high <<= 4;
	mVal.high += framebuffer_base;	// Watch for overflow issues here...
	write_vg_32(DC_DV_CTL, mVal.high);

	// Initialize the frame buffer base realated stuff.
	hw_fb_map_init(framebuffer_base);

	// CLEAR Video Generator START ADDRESS VALUES
	write_vg_32(DC_FB_ST_OFFSET, 0L);
	write_vg_32(DC_CB_ST_OFFSET, 0L);
	write_vg_32(DC_CURS_ST_OFFSET, 0L);
	write_vg_32(DC_UNLOCK, saveLock);


	// Put VIP input and output sections into reset state with default values.
	write_vip_32(VIP_CTRL1, 0x42000001);

	// Set flags indicating the hardware registers are available and that we are not
	// disabled.  Among other things, the SF_SECONDARY flag controls the hardware
	// register locking and unlocking at the top of the message handling loop.
	VGState |= SF_SECONDARY;
	VGState &= ~SF_DISABLED;

	return;
}


// Initialize the graphics processor MSR registers.
void hw_gp_msr_init(void)
{
	// Initialize the configuration MSR - 0x00000000 00000010
	msrModify(msrIdx_GP, MBD_MSR_CONFIG, MSR_CLR_ALL, MSR_CLR_ALL_BUT_PID, 0, GP_DEF_PRI);

	// Initialize the SMI MSR.	Clear and disable all SMIs - 0x00000001 00000001
	msrModify(msrIdx_GP, MBD_MSR_SMI, MSR_CLR_ALL, MSR_CLR_ALL, GP_SMI_CLR, GP_SMI_DIS);

	return;
}

// Initialize the video input port MSR registers.
void hw_vip_msr_init(void)
{
	msrModify(msrIdx_VIP, MBD_MSR_CONFIG, MSR_CLR_ALL, MSR_CLR_ALL_BUT_PID, 0L, VIP_DEF_PRI);

	// Initialize the SMI MSR.	Clear and disable all SMIs - 0x00000000 7FFF7FFF
	msrModify(msrIdx_VIP, MBD_MSR_SMI, MSR_CLR_ALL, MSR_CLR_ALL, 0L, 0x7FFF7FFF);

	return;
}

// Initialize the video generator MSR registers.
void hw_vg_msr_init(void)
{
	// Initialize the configuration MSR - 0x00000000 00000320
	msrModify(msrIdx_VG, MBD_MSR_CONFIG, MSR_CLR_ALL, MSR_CLR_ALL_BUT_PID, 0, VG_DEF_PRI);

	// Initialize the SMI MSR.	Clear and disable all SMIs - 0x0000001f 0000001f
	msrModify(msrIdx_VG, MBD_MSR_SMI, MSR_CLR_ALL, MSR_CLR_ALL, VG_SMI_DIS, VG_SMI_DIS);

	// Initialize the DELAY MSR.
	msrModify(msrIdx_VG, MBD_MSR_DELAY, MSR_CLR_ALL, MSR_CLR_ALL, 0L, 0x00000302);

	// Turn off the bad VG fetch state machine hardware fix and the video FIFO watermarks
	msrModify(msrIdx_VG, MBD_MSR_SPARE, MSR_CLR_ALL, MSR_CLR_ALL, 0L, 0x00000042);

	return;
}

// Initialize the display filter MSR registers.
void hw_mcp_msr_init(void)
{
	unsigned long orValLo, orValHi;
	struct mValue mVal;
	unsigned char i;

	// Initialize the DOT PLL MSR.	We need to default to a known clock so
	// the PLL doesn't exceed its limits, and if the bypass bit is set, we
	// need to also set the power down bit.
	if (vReg[VG_CONFIG] & VG_CFG_BYPASS)
	{
		// Bypassed, so just set the bypass and power down bits.
		orValLo = DOTPLL_BYPASS | DOTPLL_PDBIT;
		orValHi = 0L;
	}
	else
	{
		orValLo = DOTPLL_RESET;
		orValHi = 0x0000216C;	// 28.322MHz
	}

	msrModify(msrIdx_MCP, MCP_DOTPLL, MSR_CLR_ALL, MSR_CLR_ALL, orValHi, orValLo);

	// LEDA has indicated that there may be up to a 42 clock delay from the time the
	// DOTPLL comes out of powerdown until the lock bit is guaranteed to go low.  In
	// order to make sure we don't read an errant value, we need to delay a bit.
	for (i=0;i<8;i++)
		outp(0xed,0xf5);

	// Wait for lock (maybe)
	msrRead(msrIdx_MCP, MCP_DOTPLL, &mVal);
	while (!(mVal.low & DOTPLL_LOCKBIT))
	{
		// Read the current contents...
		msrRead(msrIdx_MCP, MCP_DOTPLL, &mVal);
	};

	msrModify(msrIdx_MCP, MCP_DOTPLL, 0, MSR_CLR_ALL, 0, 0);		// Clear the reset bit

	return;
}

// Initialize the display filter MSR registers.
void hw_df_msr_init(void)
{
	unsigned long orVal;
	unsigned long mDiv, mMul, pSpd, mSpd, mbClk;
	struct mValue mVal;

	// Get a colletion of information necessary to compute the divisor for the
	// CONFIG MSR.	This includes the processor speed, the CPU multiplier and
	// divisor and the MBus multiplier and divisor.	 There are two different
	// versions of the computation.	 The commented out version doesn't depend
	// on the bootstrap pin, and the other starts with a known PCI speed based
	// on that pin.
	msrRead(msrIdx_MCP, MCP_SYSPLL, &mVal);		// Returns the divisors...

	pSpd = (mVal.low & 0x00000008)?66:33;

	mDiv = ((mVal.high & 0x00000040) >> 6) + 1;
	mMul = ((mVal.high & 0x00000F80) >> 7) + 1;

	mbClk = (pSpd * mMul) / mDiv;

	mSpd = (mbClk / 14) & 0x0000003F;

	// Initialize the configuration MSR
	orVal = DF_DEF_PRI;
	orVal |= (mSpd << 8);
	msrModify(msrIdx_DF, MBD_MSR_CONFIG, MSR_CLR_ALL, MSR_CLR_ALL_BUT_PID, 0, orVal);

	return;
}



//---------------------------------------------------------------------------
// hw_fb_map_init
//
// This routine initializes the hardware pointers that are specifically
// related to the framebuffer address.
//---------------------------------------------------------------------------
void hw_fb_map_init(unsigned long fbLoc)
{
	unsigned long ltemp;

	// Insist upon a certain alignment...
	fbLoc &= VGdata.pci_fb_mask;

	// Set the MBus Memory Offset register.
	write_vg_32(PHY_MEM_OFFSET, fbLoc);

	// Set GP2 base offset - different from Redcloud.  There are both source
	// and destination fields the need to be set.
	ltemp = fbLoc | (fbLoc >> 10);
	write_gp_32(GP2_BASE_OFFSET, ltemp);

	return;
}

