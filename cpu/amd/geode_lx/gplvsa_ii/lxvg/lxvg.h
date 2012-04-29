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

// This is the main header file for LXVG.

// VERSION NUMBERS

#define LXVG_MAJOR_VER			0x00
#define LXVG_MINOR_VER			0x11

// PROCESSOR IDS
// Keep as bitwise flags to make "either" comparisons easier.

#define PROCESSOR_GXM				0x01
#define PROCESSOR_REDC				0x02
#define PROCESSOR_CSTL				0x03

// A pair of LONG generic masks
#define CLR_HIWORD					0x0000FFFF
#define CLR_LOWORD					0xFFFF0000


// Some LXVG configuration flags and masks
#define MEM_SIZE_MASK				0x00FE		// Bits 7:1 (size in 2MB hunks)
#define ADAPTER_PRIMARY				0x0100		// Bit 8

#define PLL_REF_MASK				0x0600		// Bits 10:9
#define PLL_BYPASS					0x0800		// Bit 11
#define		VGDATA_PLL_14MHZ			0x00	// VGdata PLL reference frequency flag value
#define		VGDATA_PLL_48MHZ			0x01	// VGdata PLL reference frequency flag value
#define		VGDATA_PLL_ERROR			0xFF	// Bad PLL or hardware error
#define MONOCHROME_CARD				0x1000		// Bit 12

// SOME REDCLOUD DEFINITIONS

#define FAKE_ADDRESS				0xFFFFFFFF
#define CLASS_MASK					0x000FF000
#define DEVID_MASK					0x00FFFF00

// A memory space enable value for the PCI command register
#define PCI_CMD_REG					0x04
#define PCI_IO_SPACE				(1 << 0)
#define PCI_MEM_SPACE				(1 << 1)

// Some commonly used size definitions
#define SIZE4K						0x00001000	// A 4K range indicator
#define SIZE16K						0x00004000	// A 16K range indicator
#define MASK16K						0xFFFFC00F	// A 16K range mask
#define SIZE32K						0x00008000	// A 32K range indicator
#define SIZE64K						0x00010000	// A 64K range indicator
#define SIZE128K					0x00020000	// A 128K range indicator

// PCI device ID assigned to LX graphics system
#define PCI_DEV_ID					0x2081

// MBus device IDs.	 These IDs will have to be shifted into place by the System
// manager when it goes to access the devices.
#define ID_MBIU						0x01		// Default MBIU device ID
#define ID_MCP						0x02		// MBus Control Processor device ID
#define ID_MC						0x20		// Default Memory Controller device ID
#define ID_GP						0x3D		// Graphics Processor device ID
#define ID_VG						0x3E		// Video Generator device ID
#define ID_DF						0x3F		// Display Filter device ID
#define ID_VIP						0x3C		// Display Filter device ID

// MSR Register Offsets
#define MBD_MSR_CAP			0x2000
#define MBD_MSR_CONFIG		0x2001
#define MBD_MSR_SMI			0x2002
#define MBD_MSR_ERROR		0x2003
#define MBD_MSR_PM			0x2004
#define MBD_MSR_DIAG		0x2005
#define MBD_MSR_SPARE		0x2011
#define MBD_MSR_DELAY		0x2012

// DF specific MSRs
#define DF_MSR_DIAG_DF		0x2010
#define DF_MSR_PAD_SEL		0x2011

// MSR stuff specific to the MCP_DOTPLL
#define MCP_SYSPLL			0x0014
#define MCP_DOTPLL			0x0015
#define DOTPLL_DIV_MASK		0x00007FFF
#define DOTPLL_DIV4			0x00010000

#define DOTPLL_RESET		0x00000001
#define DOTPLL_CAPEN		0x00002000
#define DOTPLL_PDBIT		0x00004000
#define DOTPLL_BYPASS		0x00008000
#define DOTPLL_HALFPIX		0x01000000
#define DOTPLL_LOCKBIT		0x02000000

// MCP Error and SMI MSR error bits related to the DOTPLL reset during power-on bug PBz#3344.
#define MCP_SMI_ERRBIT		0x00010000
#define MCP_DOTPLL_ERRBIT	0x00000004

#define MCP_CHIPREV			0x0017

//
// msrDev indices for the devices that we care about/know about
//
#define msrIdx_MCP		0
#define msrIdx_MC		1
#define msrIdx_GP		2
#define msrIdx_VG		3
#define msrIdx_DF		4
#define msrIdx_VIP		5
#define msrIdx_MBIU		6

// A mask to isolate the offset field of the frame buffer descriptor.
#define DESC_OFFSET_MASK	0x0FFFFF00
#define DV_OFFSET_MASK		0xFFFFF000
#define DV_LINE_1K			0x00000000
#define DV_LINE_2K			0x00000400
#define DV_LINE_4K			0x00000800
#define DV_LINE_8K			0x00000C00

// Structures of MSRs.
//
// Proper usage is as follows:
//
//	MSR access routines (MSRAR) should first check the 'Present' field and :
//
//	1) If the field is set to 'REQ_NOT_FOUND' :
//		- The device was not detected on the MBUS but a request was made to
//			find it at some point since/during initization [ see msrInit() ].
//			Therefore, MSRAR should not attempt to access the device.
//
//	2) If the field is set to 'FOUND' :
//		- The device was detected on the MBUS, the rest of the structure
//		  has been filled in and MSRAR should use the address provided.
//
//	3) If the field is set to 'UNKNOWN' :
//		- There has never been a request to find this device.  MSRAR returns
//		  to caller.  Caller should first call msrInit().
//
//	The address field is set to FAKE_ADDRESS just in case "renegade software" doesn't
//		 check the 'Present' field and just grabs the 'address' field.
//		 FAKE_ADDRESS has been set to a value that should never appear in
//		 a real system.	  It is only meant to protect the system 99%
//		 of the time from badly written software....

typedef struct tagMSR {
	unsigned short	Present;		// Present - Read above under "Structures of MSRs"
	unsigned short	Id;				// Id - Device ID number (from MSR specs)
	unsigned long	Routing;		// Routing - 32-bit address at which 'Id' was found
} MSR;

//
// mValue is used to hold the 64-bit msr data value.
//
typedef struct mValue {
	unsigned long low;
	unsigned long high;
};

// Capabilities bits
#define DF_BOND_MASK		0x000000C0		// CRT bond value
#define BOND_CRT			0x00			// CRT bond value
#define BOND_FP				0x40			// flat panel bond value

#define HALF_MEG			0x00080000			// 512K
#define QRTR_MEG			0x00040000			// 256K
#define MAX_ICON			0x4C00			// 19K

// MSR AND masks used by rc_msrModify.	These masks define bits that are
// always cleared in the register.
#define MSR_CLR_ALL			0xFFFFFFFF		// Clear all bits
#define MSR_CLR_NONE		0L				// Clear no bits
#define MSR_CLR_OUT			0x00008038		// Clear bits 15, 5:3
#define MSR_CLR_ALL_BUT_PID	0xFFFFFFF8		// Clear all bits except [2:0]


// GP
#define GP_SMI_MSK_HI		0xFFFFFFFF		// SMI MSR
#define GP_SMI_MSK_LO		0xFFE0FFE0
#define GP_ERR_MSK_HI		0xFFFFFFFF		// Error MSR
#define GP_ERR_MSK_LO		0xFFF0FFF0
#define GP_PM_MSK_HI		0xFFFFFFFC		// PM MSR
#define GP_PM_MSK_LO		0xFFFFFFF0

// VG
#define VG_SMI_MSK_HI		0xFFFFFFFF		// SMI MSR
#define VG_SMI_MSK_LO		0xFFE0FFE0
#define VG_ERR_MSK_HI		0xFFFFFFFF		// Error MSR
#define VG_ERR_MSK_LO		0xFFF0FFF0
#define VG_PM_MSK_HI		0xFFFFFFFC		// PM MSR
#define VG_PM_MSK_LO		0xFFFFFFF0

// DF
#define DF_SMI_MSK_HI		0xFFFFFFFF		// SMI MSR
#define DF_SMI_MSK_LO		0xFFFCFFFC
#define DF_ERR_MSK_HI		0xFFFFFFFE		// Error MSR
#define DF_ERR_MSK_LO		0xFFFFFFFE
#define DF_PM_MSK_HI		0xFFFFFFE0		// PM MSR
#define DF_PM_MSK_LO		0xF0FFFC00


// MSR bit field AND masks.	 These are added to the above masks to
// clear particular fields

// GP
#define GP_CFG_PRI			0x00000070		// Both priority fields
#define GP_CFG_PID			0x00000007		// PID field
#define GP_DEF_PRI			0x00000010		// Default priorities

#define GP_SMI_CLR			0x00000001		// SMI clear bits
#define GP_SMI_DIS			0x00000001		// SMI disable bits

#define GP_CLK_RQ			0x00000001		// Software clock PM request bits field
#define GP_PM_MODE			0x00000003		// Clock PM mode fields
#define GP_CLK_ON			0x00000000		// Both clocks always on
#define GP_CLK_HW			0x00000001		// Both clocks hardware gated
#define GP_CLK_SW			0x00000002		// Both clocks software gated
#define GP_CLK_BOTH			0x00000003		// Both clocks software and hardware gated

// VG
#define VG_CFG_PRI			0x00000770		// Both priority fields
#define VG_CFG_PID			0x00000007		// PID field
#define VG_DEF_PRI			0x00000720		// Default priorities
#define VG_DEF_PRI_10		0x00000620		// Default priorities for 1.0 parts

// LX 2.0 changes
#define VG_SMI_DIS			0x1001FFFF		// SMI disable and clear bits
#define VG_SMI_ALLNB		0x10000016		// All "Standard" SMIs but VBLANKs

#define VG_SMI_INV_CRTC		0x00000010		// Invalid CRTC SMI disable mask
#define VG_SMI_VBLANK		0x00000009		// Vertical blanks SMI disable mask
#define VG_SMI_ISR0			0x00000004		// Input status register SMI disable mask
#define VG_SMI_MISC_W		0x00000002		// Miscellaneous output register SMI disable mask

#define VG_SMI_ALLCRTC		0x00000060		// All CRTC reads and writes
#define VG_SMI_ALLSEQ		0x00000180		// All SEQ reads and writes
#define VG_SMI_ALLGDC		0x00000600		// All GFX reads and writes
#define VG_SMI_ALLATC		0x00001800		// All ATC reads and writes
#define VG_SMI_ALLDAC		0x00006000		// All DAC reads and writes

#define VG_SMI_CRTC_W		0x00000020		// All CRTC writes
#define VG_SMI_CRTC_R		0x00000040		// All CRTC reads
#define VG_SMI_SEQ_W		0x00000080		// All SEQ writes
#define VG_SMI_SEQ_R		0x00000100		// All SEQ reads
#define VG_SMI_GDC_W		0x00000200		// All GDC writes
#define VG_SMI_GDC_R		0x00000400		// All GDC reads
#define VG_SMI_ATC_W		0x00000800		// All ATC writes
#define VG_SMI_ATC_R		0x00001000		// All ATC reads
#define VG_SMI_DAC_W		0x00002000		// All DAC writes
#define VG_SMI_DAC_R		0x00004000		// All DAC reads
#define VG_SMI_MISC_R		0x00008000		// Miscellaneous output register reads
#define VG_SMI_ISR1_R		0x00010000		// Input status register 1 reads

// VG_DEBUG trap request bits
#define TRAP_ALL_CRTC		0x0100
#define TRAP_ALL_SEQ		0x0200
#define TRAP_ALL_GDC		0x0400
#define TRAP_ALL_ATC		0x0800
#define TRAP_ALL_DAC		0x1000
#define TRAP_MISC_RDS		0x2000
#define TRAP_ISR1_RDS		0x4000
// LX 2.0 changes

#define VG_CLK_RQ			0x00000003		// Software clock PM request bits field
#define VG_PM_MODE			0x0000000F		// Clock PM mode fields
#define VG_CLK_ON			0x00000000		// Both clocks always on
#define VG_CLK_HW			0x00000005		// Both clocks hardware gated
#define VG_CLK_SW			0x0000000A		// Both clocks software gated
#define VG_CLK_BOTH			0x0000000F		// Both clocks software and hardware gated


// DF
#define DF_CFG_PID			0x00000007		// PID field
#define DF_CFG_FMT			0x00000038		// Output format select field
#define DF_CFG_FMBO			0x000000C0		// Output format byte order field
#define DF_CFG_DIV			0x00003F00		// Clock divider field
#define DF_CFG_IUV			0x00004000		// Interchange UV field
#define DF_CFG_FPC			0x00008000		// Simultaneous CRT and panel/VOP bit
#define DF_CFG_PRI			0x00070000		// MBus master priority field
#define DF_DEF_PRI			0x00040000		// Default priority
#define DF_DEF_DIV			0x00003F00		// Default clock divider

//#define DF_SMI_CLR			0x00030000		// SMI clear bits
#define DF_SMI_CLR			0x00000000		// SMI clear bits
#define DF_SMI_DIS			0x00000003		// SMI disable bits

#define DF_CLK_RQ			0x0000001F		// Software clock PM request bits field
#define DF_PM_MODE			0x000003FF		// Clock PM mode fields
#define DF_CLK_ON			0x00000000		// All clocks always on
#define DF_CLK_HW			0x00000155		// All clocks hardware gated
#define DF_CLK_SW			0x000002AA		// All clocks software gated
#define DF_CLK_BOTH			0x000003FF		// All clocks software and hardware gated

#define DF_CLR_CRC			0x80000000		// Clear the CRC select bit

// VIP
#define VIP_CFG_PID			0x00000007		// PID field
#define VIP_PRI_PRI			0x00000070		// MBus master priority field
#define VIP_SEC_PRI			0x00000700		// Default priority
#define VIP_DEF_PRI_10		0x00000630		// Default priority
#define VIP_DEF_PRI			0x00000620		// Default priority

#define VIP_SMI_CLR			0x3FFF0000		// SMI clear bits
#define VIP_SMI_DIS			0x00003FFF		// SMI disable bits

#define VIP_CLK_RQ			0x0000001F		// Software clock PM request bits field
#define VIP_PM_MODE			0x000003FF		// Clock PM mode fields
#define VIP_CLK_ON			0x00000000		// All clocks always on
#define VIP_CLK_HW			0x00000155		// All clocks hardware gated
#define VIP_CLK_SW			0x000002AA		// All clocks software gated
#define VIP_CLK_BOTH		0x000003FF		// All clocks software and hardware gated

//
// SMI event id bits
//
#define EVT_VG_VBLANK		0x00000001
#define EVT_VG_MISC_WR		0x00000002
#define EVT_VG_ISR0_RD		0x00000004
#define EVT_VGA_VBLANK		0x00000008
#define EVT_VG_INV_CRTC		0x00000010
#define EVT_VG_CRTC_W		0x00000020
#define EVT_VG_CRTC_R		0x00000040
#define EVT_VG_SEQ_W		0x00000080
#define EVT_VG_SEQ_R		0x00000100
#define EVT_VG_GFX_W		0x00000200
#define EVT_VG_GFX_R		0x00000400
#define EVT_VG_ATC_W		0x00000800
#define EVT_VG_ATC_R		0x00001000
#define EVT_VG_DAC_W		0x00002000
#define EVT_VG_DAC_R		0x00004000
#define EVT_VG_MISC_R		0x00008000
#define EVT_VG_ISR1_R		0x00010000
#define EVT_VGA_RES_CHG		0x10000000

#define EVT_WRITES			0x00000AB2		// The register write bits
#define EVT_READS			0x0001D554		// The register read bits + ISR0 & Inv CRTC
//#define EVT_GP_SMI0			0x20000000
//#define EVT_DF_SMI0			0x40000000
//#define EVT_DF_SMI1			0x80000000
// Castle 2.0 Defs


//
// GP2 Memory Mapped Register Set
//
#define GP2_DST_OFFSET			0x0000
#define GP2_SRC_OFFSET			0x0004
#define GP2_VEC_ERR				0x0004
#define GP2_STRIDE				0x0008
#define GP2_WID_HEIGHT			0x000C
#define GP2_SRC_COLOR_FG		0x0010
#define GP2_SRC_COLOR_BG		0x0014
#define GP2_PAT_COLOR_0			0x0018
#define GP2_PAT_COLOR_1			0x001C
#define GP2_PAT_COLOR_2			0x0020
#define GP2_PAT_COLOR_3			0x0024
#define GP2_PAT_COLOR_4			0x0028
#define GP2_PAT_COLOR_5			0x002C
#define GP2_PAT_DATA_0			0x0030
#define GP2_PAT_DATA_1			0x0034
#define GP2_RASTER_MODE			0x0038
#define GP2_VECTOR_MODE			0x003C
#define GP2_BLT_MODE			0x0040
#define GP2_BLT_STATUS			0x0044
#define GP2_RESET				0x0044
#define GP2_HST_SRC				0x0048
#define GP2_BASE_OFFSET			0x004C

//
// VG Memory Mapped Register Set
//
#define DC_UNLOCK				0x0000
#define DC_GENERAL_CFG			0x0004
#define DC_DISPLAY_CFG			0x0008
#define DC_ARB_CFG				0x000C
//#define DC_GFX_SCL				0x000C
#define DC_FB_ST_OFFSET			0x0010
#define DC_CB_ST_OFFSET			0x0014
#define DC_CURS_ST_OFFSET		0x0018
//#define DC_ICON_ST_OFFSET			0x001C
#define DC_VID_Y_ST_OFFSET		0x0020
#define DC_VID_U_ST_OFFSET		0x0024
#define DC_VID_V_ST_OFFSET		0x0028
#define DC_DV_TOP				0x002C
//#define DC_VID_SP_ST_OFFSET		0x002C
#define DC_LINE_SIZE			0x0030
#define DC_GFX_PITCH			0x0034
#define DC_VID_YUV_PITCH		0x0038
//#define DC_VID_SP_PITCH			0x003C
#define DC_H_ACTIVE_TIMING		0x0040
#define DC_H_BLANK_TIMING		0x0044
#define DC_H_SYNC_TIMING		0x0048
//#define DC_FP_HSYNC_TIMING		0x004C
#define DC_V_ACTIVE_TIMING		0x0050
#define DC_V_BLANK_TIMING		0x0054
#define DC_V_SYNC_TIMING		0x0058
#define DC_FB_ACTIVE			0x005C
//#define DC_FP_VSYNC_TIMING		0x005C
#define DC_CURSOR_X				0x0060
#define DC_CURSOR_Y				0x0064
//#define DC_ICON_X					0x0068
#define DC_LINE_CNT				0x006C
#define DC_PAL_ADDRESS			0x0070
#define DC_PAL_DATA				0x0074
#define DC_DFIFO_DIAG			0x0078
#define DC_CFIFO_DIAG			0x007C
#define DC_VID_DS_DELTA			0x0080
#define PHY_MEM_OFFSET			0x0084
#define DC_DV_CTL				0x0088
#define DC_ACCESS				0x008C

#define DC_GFX_SCALE			0x0090
#define DC_IRQ_FLT_CTL			0x0094
#define DC_FLT_COEFF1			0x0098
#define DC_FLT_COEFF2			0x009C

#define DC_VBI_EVN_CTL			0x00A0
#define DC_VBI_ODD_CTL			0x00A4
#define DC_VBI_HOR_CTL			0x00A8
#define DC_VBI_LN_ODD			0x00AC
#define DC_VBI_LN_EVN			0x00B0
#define DC_VBI_PITCH			0x00B4
#define DC_VBI_CLR_KEY			0x00B8
#define DC_VBI_CK_MASK			0x00BC
#define DC_VBI_CK_X				0x00C0
#define DC_VBI_CK_Y				0x00C4

#define DC_IRQ					0x00C8
#define DC_GENLK_CTL			0x00D4

#define DC_VID_EVN_Y_ST			0x00D8
#define DC_VID_EVN_U_ST			0x00DC
#define DC_VID_EVN_V_ST			0x00E0

#define DC_VID_EVN_ACT			0x00E4
#define DC_VID_EVN_BLANK		0x00E8
#define DC_VID_EVN_SYNC			0x00EC

#define DC_VGA_CONFIG			0x0100
#define DC_VGA_STATUS			0x0104
//#define DC_VGA_EXTADDR			0x0108

#define DC_UNLOCK_VALUE		0x00004758
#define DC_LOCK_VALUE		0x00000000

//-----------------------------------//
//	DC_GENERAL_CFG Bit Definitions	 //
//-----------------------------------//
// DC_GCFG_CLR_MASK turns off everything but VGA fixed timing enable, VGA enable,
// compression and decompression enables and Display-FIFO Load Enable
#define DC_GCFG_CLR_MASK		0x0004FF00

#define	DC_GCFG_DFLE			0x00000001
#define DC_GCFG_CURE			0x00000002
#define DC_GCFG_ICNE			0x00000004
#define DC_GCFG_VIDE			0x00000008
//#define DC_GCFG_VSPE				0x00000010
#define DC_GCFG_FSSEL			0x00000010
#define DC_GCFG_CMPE			0x00000020
#define DC_GCFG_DECE			0x00000040
#define DC_GCFG_VGAE			0x00000080

#define DC_GCFG_DFIFO_ST		0x00000F00
#define DC_GCFG_DFIFO_END		0x0000F000
#define DC_GCFG_WATERMARKS		0x00007200
#define DC_GCFG_STFM			0x00010000
#define DC_GCFG_FDTY			0x00020000
#define DC_GCFG_VGAFT			0x00040000
#define DC_GCFG_VDSE			0x00080000
#define DC_GCFG_YUVM			0x00100000
//#define DC_GCFG_FTSTR				0x00200000
#define DC_GCFG_FRC8PIX			0x00400000
#define DC_GCFG_SIGSEL			0x00800000
#define DC_GCFG_CLR_CRC			0xF8FFFFFF
//#define DC_GCFG_CLR_CRC_ALL		0xF07FFFFF
#define DC_GCFG_CLR_CRC_ALL		0xF07FFFEF
#define DC_GCFG_SIGE			0x01000000
#define DC_GCFG_SGRE			0x02000000
#define DC_GCFG_SGFR			0x04000000
#define DC_GCFG_CRCMODE			0x08000000
//#define DC_GCFG_GXRFS4			0x08000000


//-----------------------------------//
//	DC_DISPLAY_CFG Bit Definitions	 //
//-----------------------------------//
// DC_DCFG_CLR_MASK turns off everything but display center, color depth fields,
// and scale enable.
#define DC_DCFG_CLR_MASK		0xC0000F00
#define DC_DCFG_BLANK_MASK		0xFFFFFFC0

#define DC_DCFG_TGEN			0x00000001
//#define DC_DCFG_PCKE			0x00000002
//#define DC_DCFG_VCKE			0x00000004
#define DC_DCFG_GDEN			0x00000008
//#define DC_DCFG_VDEN			0x00000010
#define DC_DCFG_VIEN			0x00000020
#define DC_DCFG_TRUP			0x00000040
#define DC_DCFG_SCLE			0x00000080

// Color depth related masks and values
#define DC_DCFG_MODE_MASK		0x00000F00
#define DC_DCFG_16BPP_MODE		0x00000C00
#define DC_DCFG_DISP_MODE		0x00000300

#define DC_DCFG_BPP16			0x00000100
#define DC_DCFG_BPP15			0x00000500
#define DC_DCFG_BPP12			0x00000900
#define DC_DCFG_BPP32			0x00000200

#define DC_DCFG_VFHPSL			0x0000F000
//#define DC_DCFG_PLNR			0x00001000
//#define DC_DCFG_SSLC			0x00002000
//#define DC_DCFG_PXDB			0x00004000
//#define DC_DCFG_LNDB			0x00008000

#define DC_DCFG_VFHPEL			0x000F0000
//#define DC_DCFG_BLNK			0x00010000
//#define DC_DCFG_BKRT			0x00020000
//#define DC_DCFG_RFS4			0x00040000
//#define DC_DCFG_DCEN			0x00080000

//#define DC_DCFG_PIX_PAN		0x00F00000
//#define DC_DCFG_PPC			0x01000000
#define	DC_DCFG_DCEN			0x01000000
#define	DC_DCFG_PALB			0x02000000
//#define DC_DCFG_FRLK			0x04000000
#define	DC_DCFG_VISL			0x08000000
//#define DC_DCFG_A18M			0x40000000
//#define DC_DCFG_A20M			0x80000000

//-----------------------------------//
//	DC_IRQ_FLT_CTL Bit Definitions	 //
//-----------------------------------//
#define	IF_FLT_ADDR_MASK		0x000000FF
#define	IF_HFILT_SEL			0x00000400
#define	IF_INTL_EN				0x00000800
#define	IF_HFILT_EN				0x00001000
#define	IF_VFILT_EN				0x00002000
#define	IF_HALPH_FILT_EN		0x00004000
#define	IF_VALPH_FILT_EN		0x00008000
#define	IF_LIN_CNT_MASK			0x07FF0000
#define	IF_IRQ_EN				0x08000000
#define	IF_INTL_ADDR			0x10000000



//
// DF Memory Mapped Register Set
//
#define	DF_VCFG					0x0000
#define	DF_DCFG					0x0008
#define	DF_VID_X				0x0010
#define	DF_VID_Y				0x0018
#define	DF_VID_SCL				0x0020
#define	DF_VID_CK				0x0028
#define	DF_VID_CM				0x0030
#define	DF_PAL_ADDR				0x0038
#define	DF_PAL_DATA				0x0040

#define	DF_SLR					0x0048

#define	DF_MISC					0x0050
#define	DF_CRT_CS				0x0058

#define	DF_VYS					0x0060
#define	DF_VXS					0x0068

#define	DF_VID_DSC				0x0078
//#define	DF_VID_DCO				0x0080

#define	DF_CRC_SIG				0x0088
#define DF_CRCS_CLR_CRC			0xFFFFFFF8
#define DF_CRCS_SIGE			0x00000001
#define DF_CRCS_SGFR			0x00000004
#define	DF_CRC32_SIG			0x0090

#define	DF_VID_VDE				0x0098
#define	DF_VID_CCK				0x00A0
#define	DF_VID_CCM				0x00A8
#define	DF_VID_CC1				0x00B0
#define	DF_VID_CC2				0x00B8
#define	DF_VID_A1X				0x00C0
#define	DF_VID_A1Y				0x00C8
#define	DF_VID_A1C				0x00D0
#define	DF_VID_A1T				0x00D8
#define	DF_VID_A2X				0x00E0
#define	DF_VID_A2Y				0x00E8
#define	DF_VID_A2C				0x00F0
#define	DF_VID_A2T				0x00F8
#define	DF_VID_A3X				0x0100
#define	DF_VID_A3Y				0x0108
#define	DF_VID_A3C				0x0110
#define	DF_VID_A3T				0x0118
#define	DF_VID_VRR				0x0120
#define	DF_VID_AWT				0x0128

// Flat panel specific
#define	DF_FP_PT1				0x0400
#define	DF_FP_PT2				0x0408
#define	DF_FP_PM				0x0410
#define	DF_FP_DFC				0x0418
#define		DF_DFC_NO_DITHER		0x00000070
//#define	DF_FP_BLFSR				0x0420
//#define	DF_FP_RLFSR				0x0428
//#define	DF_FP_FMI				0x0430
//#define	DF_FP_FMD				0x0438
//#define	DF_FP_RSVD				0x0440
#define	DF_FP_DCA				0x0448
#define	DF_FP_DMD				0x0450
#define	DF_FP_CRC				0x0458
//#define	DF_FP_FBB				0x0460
#define	DF_FP_CRC32				0x0468

// VOP specific
#define	DF_VOP_CFG				0x0800
#define	DF_VOP_SIG				0x0808

#define	DF_VID_VCR				0x1000

#define DF_DCFG_VID_EN				0x00000001

//-----------------------------------//
//	DF_DCFG Bit Definitions			 //
//-----------------------------------//
// DF_DCFG_CLR_MASK turns off everything but CRT sync skew.	 The blank mask turns off
// CRT DACs, the CRT sync enables and resets the display logic.
#define DF_DCFG_CLR_MASK			0x0001C000
#define DF_DCFG_BLANK_MASK			0xFFFFFFF0	// 0xF431FF30
#define DF_DCFG_ENABLE_MASK			0x0000000F
#define DF_DCFG_DPMS_STBY			0x00000005
#define DF_DCFG_DPMS_SUSP			0x00000003

#define DF_DCFG_DIS_EN				0x00000001
#define DF_DCFG_HSYNC_EN			0x00000002
#define DF_DCFG_VSYNC_EN			0x00000004
#define DF_DCFG_DAC_BL_EN			0x00000008
//#define DF_DCFG_DAC_PWDNX			0x00000020
//#define DF_DCFG_FP_PWR_EN			0x00000040
//#define DF_DCFG_FP_DATA_EN		0x00000080
#define DF_DCFG_CRT_HSYNC_POL		0x00000100
#define DF_DCFG_CRT_VSYNC_POL		0x00000200
//#define DF_DCFG_FP_HSYNC_POL		0x00000400
//#define DF_DCFG_FP_VSYNC_POL		0x00000800
//#define DF_DCFG_XGA_FP			0x00001000
//#define DF_DCFG_FP_DITH_EN		0x00002000
#define DF_DCFG_CRT_SYNC_SKW_MASK	0x0001C000
#define DF_DCFG_CRT_SYNC_SKW_POS	14
//#define DF_DCFG_PWR_SEQ_DLY_MASK	0x000E0000
//#define DF_DCFG_PWR_SEQ_DLY_POS	17
//#define DF_DCFG_PWR_SEQ_DLY_VAL	0x00080000
#define DF_DCFG_VG_CK				0x00100000
#define DF_DCFG_GV_PAL_BYP			0x00200000
//#define DF_DCFG_DDC_SCL			0x00400000
//#define DF_DCFG_DDC_SDA			0x00800000
//#define DF_DCFG_DDC_OE			0x01000000
#define DF_DCFG_DAC_VREF			0x04000000
//#define DF_DCFG_FP_PWR_ON			0x08000000


//-----------------------------------//
//	DF_FP_PT1 Bit Definitions		 //
//-----------------------------------//
#define DF_PT1_HPULSE_MASK			0x0000001F
#define DF_PT1_HDELAY_MASK			0x000000E0	// Not used in LXVG
#define DF_PT1_HDELAY_SHIFT							5
#define DF_PT1_O					0x00004000
#define DF_PT1_U					0x00008000
#define DF_PT1_VSIZE_MASK			0x07FF0000
#define DF_PT1_VSIZE_SHIFT							16
#define DF_PT1_HSRC					0x08000000
#define DF_PT1_HSIP					0x20000000
#define DF_PT1_VSIP					0x40000000


//-----------------------------------//
//	DF_FP_PT2 Bit Definitions		 //
//-----------------------------------//
#define DF_PT2_CLP					0x00002000
#define DF_PT2_PIXF_MASK			0x00070000
#define DF_PT2_PIXF_SHIFT							16
#define DF_PT2_PIXF_000				0x00000000
#define DF_PT2_PIXF_001				0x00010000
#define DF_PT2_PIXF_002				0x00020000
#define DF_PT2_PIXF_003				0x00030000
#define DF_PT2_MCS					0x00080000
#define DF_PT2_PSEL_MASK			0x00300000
#define DF_PT2_PSEL_STN				0x00000000
#define DF_PT2_PSEL_TFT				0x00100000
#define DF_PT2_HSP					0x00400000
#define DF_PT2_VSP					0x00800000
#define DF_PT2_VFS					0x01000000
#define DF_PT2_LMS					0x02000000
#define DF_PT2_LHS					0x04000000
#define DF_PT2_SCRC					0x08000000
#define DF_PT2_LPOL					0x20000000
#define DF_PT2_TPASS				0x40000000

//-----------------------------------//
//	DF_FP_PM Bit Definitions		 //
//-----------------------------------//
#define DF_PM_SINV					0x00002000
#define DF_PM_VDEL_MASK				0x0000C000
#define DF_PM_VDEL_SHIFT							14
#define DF_PM_HDEL_MASK				0x00030000
#define DF_PM_HDEL_SHIFT							16
#define DF_PM_PD0					0x00040000
#define DF_PM_PD1					0x00080000
#define DF_PM_PD2					0x00100000
#define DF_PM_PUB0					0x00200000
#define DF_PM_PUB1					0x00400000
#define DF_PM_PUB2					0x00800000
#define DF_PM_P						0x01000000
#define DF_PM_D						0x02000000
#define DF_PM_PWR_SEQ				0x08000000

//-----------------------------------//
//	DF_VOP_CFG Bit Definitions		   //
//-----------------------------------//
#define DF_VOP_ENABLE_MASK			0x00000003
#define DF_VOP_VIP11				0x00000001
#define DF_VOP_VIP2					0x00000002
#define DF_VOP_CCIR656				0x00000003
#define DF_VOP_LVL2					0x00000004
#define DF_VOP_EXTSAV				0x00000008
#define DF_VOP_422CO				0x00000000
#define DF_VOP_422RCO				0x00000010
#define DF_VOP_422ASS				0x00000020
#define DF_VOP_SC120X				0x00000040
#define DF_VOP_SIGE					0x00000080
#define DF_VOP_SIGFR				0x00000100


//
// VIP Memory Mapped Register Set
//
#define	VIP_CTRL1					0x0000
#define	VIP_CTRL2					0x0004
#define	VIP_STATUS					0x0008
#define	VIP_INTS					0x000C
#define	VIP_CURTGT					0x0010
#define	VIP_MAX_ADDR				0x0014

#define	VIP_AVID_EBASE				0x0018
#define	VIP_AVID_OBASE				0x001C
#define	VIP_AVBI_EBASE				0x0020
#define	VIP_AVBI_OBASE				0x0024
#define	VIP_A_PITCH					0x0028
#define	VIP_CTRL3					0x002C
#define	VIP_A_VOBUF					0x0030
#define	VIP_A_UOBUF					0x0034

#define	VIP_BVID_EBASE				0x0038
#define	VIP_BVID_OBASE				0x003C
#define	VIP_BVBI_EBASE				0x0040
#define	VIP_BVBI_OBASE				0x0044
#define	VIP_B_PITCH					0x0048
#define	VIP_B_VBUF					0x0050
#define	VIP_B_UBUF					0x0054

#define	VIP_AMSG_1BASE				0x0058
#define	VIP_AMSG_2BASE				0x005C
#define	VIP_AMSG_SIZE				0x0060

#define	VIP_PAGE_OFFS				0x0068
#define	VIP_VERT_ST					0x006C
#define	VIP_FIFO_ADDR				0x0070
#define	VIP_FIFO_RW					0x0074
#define	VIP_FRM_DCNT				0x0078
#define	VIP_A_VEBUF					0x007C
#define	VIP_A_UEBUF					0x0080



// CHIPSET IDS
// Keep as bitwise flags to make "either" comparisons easier.

#define CHIPSET_CX5530				0x01
#define CHIPSET_REDC				0x02
#define CHIPSET_DHRUVA				0x10
#define CHIPSET_BHARGAVA			0x20
#define CX5530_DISPLAY_CONFIG		0x00000004

// TRAPPED PCI DEVICES

#define PCI_CONFIG_MASK				0xFFFFFF00		// bits for comparison
#define PCI_NSM_FLAGS_REG0			0x44			// register for flags
#define PCI_NSM_FLAGS_REG1			0x45			// register for flags
#define PCI_NSM_FLAGS_REG2			0x46			// register for flags
#define PCI_NSM_FLAGS_REG3			0x47			// register for flags
#define PCI_NSM_FLAG_DISABLE		0x01			// flag to disable LXVG

// SYSTEM FLAGS (stored in VGState)

#define SF_DISABLED				0x00000001		// LXVG is disabled
#define SF_SECONDARY			0x00000002		// Init to secondary controller capability
#define SF_PRIMARY				0x00000004		// Init to primary controller capability
// The next flag indicates a mode switch has occurred.	Generally only the first mode
// switch is interesting.
#define SF_MODE_SET				0x00000008
#define SF_END_POST				0x00000010		// POST is complete
#define SF_DRIVER_ENABLED		0x00000020		// Graphics driver is controlling system
#define SF_DIAG_SMI				0x00000040		// Use diagnostic SMI settings

#define SF_MONOCHROME			0x00000100		// monochrome mode
#define SF_3D0_RANGE			0x00000200		// remembers state of ioaddr bit in misc output
#define SF_SCALE_DISABLED		0x00000400		// Disable graphics scaling during fixed timings
#define SF_FORCE_VALIDATION		0x00000800		// force all HW validation


// SMM HEADER FLAGS

#define SMM_IO_WRITE			0x0002
#define SMM_VR_WRITE			0x0001

// FLAGS FOR PCI TRAP EVENTS

#define PCI_TRAP_WRITE			0x80

#include "VGdata.h"

#define TRUE 1
#define FALSE 0


extern VGDATA VGdata;
extern unsigned long VGState;
extern unsigned long lockNest;			// Nested SMI recognition scheme
extern unsigned long saveLock;			// Nested SMI recognition scheme
extern unsigned short vReg[];
extern unsigned long vga_config_addr;
extern unsigned long GPregister_base;
extern unsigned long VGregister_base;
extern unsigned long DFregister_base;
extern unsigned long VIPregister_base;
extern unsigned long framebuffer_base;
extern unsigned long VG_SMI_Mask;
extern unsigned char crc_time;


//------------//
//	ROUTINES  //
//------------//

// ROUTINES IN MSR.C

unsigned char msrInit(void);
unsigned short msrFindDevice(struct tagMSR *);
unsigned short msrIdDevice(unsigned long);
unsigned short msrRead(unsigned short msrIdx, unsigned short msrReg, struct mValue *);
unsigned short msrWrite(unsigned short msrIdx, unsigned short msrReg, unsigned long outHi, unsigned long outLo);
void msrModify(unsigned short msrIdx, unsigned short msrReg,
				unsigned long andHi, unsigned long andLo, unsigned long orHi, unsigned long orLo);
void msrSave(unsigned short msrIdx, unsigned short *, struct mValue *);
void msrRestore(unsigned short msrIdx, unsigned short *, struct mValue *);
void msrDump(unsigned short msrIdx, unsigned short msrReg);


// ROUTINES IN VSA2.C

void decode_vsa2_event(void);
void vsa2_io_read(unsigned short size, unsigned long data);


// ROUTINES IN DECODE.C

void virtual_register_event(unsigned char reg, unsigned long rwFlag, unsigned long vrData);
void pci_trap_event(unsigned long address, unsigned long size,	unsigned long data);


// ROUTINES IN GXHWCTL.C

void hw_initialize(unsigned short config);
void hw_gp_msr_init(void);
void hw_vip_msr_init(void);
void hw_vg_msr_init(void);
void hw_df_msr_init(void);
void hw_mcp_msr_init(void);
void hw_fb_map_init(unsigned long fbLoc);

// ROUTINES IN INIT.C

void lxvg_initialize(unsigned short init_parms);


// ROUTINES IN UTILS.ASM

unsigned long read_fb_32(unsigned long offset);
void write_fb_32(unsigned long offset, unsigned long data);
unsigned long read_gp_32(unsigned long offset);
void write_gp_32(unsigned long offset, unsigned long data);
unsigned char read_vg_8(unsigned long offset);
void write_vg_8(unsigned long offset, unsigned char data);
unsigned long read_vg_32(unsigned long offset);
void write_vg_32(unsigned long offset, unsigned long data);
unsigned long read_df_32(unsigned long offset);
void write_df_32(unsigned long offset, unsigned long data);
unsigned long read_vip_32(unsigned long offset);
void write_vip_32(unsigned long offset, unsigned long data);
void asmRead(unsigned short msrReg, unsigned long msrAddr, unsigned long *ptrHigh, unsigned long *ptrLow);
void asmWrite(unsigned short msrReg, unsigned long msrAddr, unsigned long *ptrHigh, unsigned long *ptrLow);

