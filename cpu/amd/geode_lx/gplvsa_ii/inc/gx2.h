/*
* Copyright (c) 2006-2008 Advanced Micro Devices,Inc. ("AMD").
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


// Vail MSRs
#define MSR_SMM_CTRL               0x1301
  #define SMM_NMI_EN     (1L << 0)	// Enables NMIs during SMM
  #define SMM_SUSP_EN    (1L << 1)	// Enables SUSP# pin during SMM
  #define NEST_SMI_EN    (1L << 2)	// Enables SSMIs during SMM
  #define SMM_INST_EN    (1L << 3)	// Enables SMM instructions
  #define INTL_SMI_EN    (1L << 4)	// Enables SSMIs
  #define EXTL_SMI_EN    (1L << 5)	// Enables SMI# pin

#define MSR_SMM_HDR                0x132B
#define MSR_SMM_LOC                0x133B
#define MSR_EFLAGS                 0x1418
#define MSR_CR0                    0x1420
#define MSR_DR7                    0x1343


//
// Region Control Registers (see page 10-188 of Vail spec)
//
#define REGION_CD        (1L << 0)	// Cache disabled
#define REGION_WA        (1L << 1)	// Write-allocate
#define REGION_WP        (1L << 2)	// Write-protect
#define REGION_WT        (1L << 3)	// Write-through
#define REGION_WC        (1L << 4)	// Write-combine
#define REGION_WS        (1L << 5)	// Write-serialize
#define REGION_EN        (1L << 8) // Region enable

#define MSR_RCONF_DEFAULT          0x1808
#define MSR_RCONF_BYPASS           0x180A
#define MSR_RCONF_A0_BF            0x180B
#define MSR_RCONF_C0_DF            0x180C
#define MSR_RCONF_E0_FF            0x180D
#define MSR_RCONF_SMM              0x180E
#define MSR_RCONF_DMM              0x180F


// Bit(s)           Field 
// ------      -----------------------------------------------
//  7:0        Region Properties
//   8         Enable
//  11:9       reserved
//  31:12      Start of region (4 KB granularity; inclusive)
//  43:32      reserved
//  63:44      Top of region (4 KB granularity; inclusive)
#define MSR_RCONF0                 0x1810
#define MSR_RCONF1                 0x1811
#define MSR_RCONF2                 0x1812
#define MSR_RCONF3                 0x1813
#define MSR_RCONF4                 0x1814
#define MSR_RCONF5                 0x1815
#define MSR_RCONF6                 0x1816
#define MSR_RCONF7                 0x1817


//========================================================================================
#define BIZARRO          (1L << 28)
#define ROUTING          0xFFFFC000	// Mask for routing field



// Ports under Redcloud MBIU0
#define PORT_MBIU0  0x10000000L	// By convention


// Standard MBus Device MSRs:
#define MBD_MSR_CAP                0x2000
#define MBD_MSR_CONFIG             0x2001		// 3 LSBs = subtractive port
  // MBIU0: 0x00000002
  // MBIU1: 0x00000004
  // MBIU2: 0x00000004

#define MBD_MSR_SMI                0x2002
#define MBD_MSR_ERROR              0x2003
#define MBD_MSR_PM                 0x2004
#define MBD_MSR_DIAG               0x2005

// Northbridge MPCI
#define MPCI_CTRL                  0x2010
  #define LDE            (1 << 9)    // Enable latency disconnect timer
#define MPCI_ARB                   0x2011
#define MPCI_PBUS                  0x2012
#define MPCI_REN                   0x2014		// Fixed Region Enables
#define MPCI_A0_BF                 0x2015		// Fixed Regions Properties A0000-BFFFF
#define MPCI_C0_DF                 0x2016		// Fixed Regions Properties C0000-DFFFF
#define MPCI_E0_FF                 0x2017		// Fixed Regions Properties E0000-FFFFF
#define MPCI_R0                    0x2018		// Base memory
#define MPCI_R1                    0x2019		// Extended memory
#define MPCI_R2                    0x201A		// SMM memory
#define MPCI_R3                    0x201B
#define MPCI_R4                    0x201C
#define MPCI_R5                    0x201D
  // MPCI Region Control Registers (see page 89 of MPCI spec)
  #define REGION_CD      (1L << 0)	// Cache disabled
  #define REGION_DD      (1L << 1)	// Discard data
  #define REGION_WP      (1L << 2)	// Write protect
  #define REGION_WT      (1L << 3)	// Write through
  #define REGION_WC      (1L << 4)	// Write combine
  #define REGION_PF      (1L << 5)	// Prefetchable

#define MPCI_ExtMSR                0x201E



// Revision IDs
#define CPU_REV_1_0				0x11
#define CPU_REV_1_1				0x12
#define CPU_REV_2_0				0x20


// MBus Device IDs:
#define ID_SHIFT                   12
#define ID_MBIU                    0x01
#define ID_MC                      0x20
#define ID_VAIL                    0x86
#define ID_AES                     0x30
#define ID_VIP                     0x3C
#define ID_GP                      0x3D
#define ID_VG                      0x3E
#define ID_DF                      0x3F
#define ID_MCP                     0x02
#define ID_MPCI                    0x05
#define ID_FG                      0xF0
#define ID_OHCI                    0x42
#define ID_USB_20                  0x43
#define ID_ATA                     0x47
#define ID_ATA100                  0x48
#define ID_MDD                     0xDF
#define ID_AC97                    0x33

		

/////////////////////////////////////////////////////////////
//                     Northbridge
/////////////////////////////////////////////////////////////
// MBIU0
//
// Capabilities: 22711830 010C1086
// P2D_BM  = 6;    0x20-0x25
// P2D_BMO = 2;    0x26-0x27
// P2D_R   = 1;    0x28
// P2D_RO  = 3;    0x29-0x2B
// P2D_SC  = 1;    0x2C
// P2D_SCO = 0;
// IOD_BM  = 3;    0xE0-0xE2
// IOD_SC  = 6;    0xE3-0xE8
// NPORTS  = 5;
// STATS   = 2;
//
//  Port   Dev_ID        Routing          FS/2     Device Description
// -----  -------- -------------------  --------   ------------------
//   0      01h    10000000  0.1.0.0.0  20000000   MBIU0
//   1      20h    20000000  1.0.0.0.0  24000000   Memory Controller
//   2      01h    40000000  2.0.0.0.0             MBIU1 (subtractive)
//   3      86h    60000000  3.0.0.0.0  2C000000   Vail (self-reference)
//   4      3Eh    80000000  4.0.0.0.0  30000000   Video Generator
//   5      3Dh    A0000000  5.0.0.0.0  34000000   Graphics Processor
//   6      3Fh    C0000000  6.0.0.0.0  38000000   Display Filter
//   7                                             <empty>

#define VG_PORT         4L
#define GP_PORT			5L
#define VG_SMI_MSR		(VG_PORT << 29) + MBD_MSR_SMI
#define GP_SMI_MSR		(GP_PORT << 29) + MBD_MSR_SMI


//
// MBIU1
//
// Capabilities: 20281830 01004009
// P2D_BM  = 9;    0x20-0x28
// P2D_BMO = 0; 
// P2D_R   = 4;    0x29-0x2C
// P2D_RO  = 0;
// P2D_SC  = 1;    0x2D
// P2D_SCO = 0;
// IOD_BM  = 3;    0xE0-0xE2
// IOD_SC  = 6;    0xE3-0xE8
// NPORTS  = 5;
// STATS   = 2;
//
//  Port   Dev_ID        Routing         FS/2       Device Description
// -----  -------- -------------------  --------   ------------------
//   0                                             <empty>
//   1      01h    44000000  2.1.0.0.0  01000000   MBIU1 (self-reference)
//   2                                             <empty>
//   3      02h    4C000000  2.3.0.0.0  00000000   MCP
//   4      05h    50000000  2.4.0.0.0  80000000   MPCI Northbridge (subtractive)
//   5      F0h    54000000  2.5.0.0.0  A0000000   FooGlue
//   6                                             <empty>
//   7                                             <empty>

#define MBIU0_PORT		1  // From MBIU1's point of view


/////////////////////////////////////////////////////////////
//                  CS5535
/////////////////////////////////////////////////////////////
//
// Capabilities: 327920A0 80000003	  (simulator: 303820a0)
// P2D_BM  = 3;    0x20-0x22
// P2D_BMK = 2;    0x23-0x24
// IOD_BM  = 10;   0xE0-0xE9
// IOD_SC  = 8;    0xEA-0xF1
// NPORTS  = 7;
// STATS   = 3;
//
//
//  Port   Dev_ID        Routing         FS/2      Device Description
// -----  -------- -------------------  --------   -----------------
//          05h    51000000  2.4.2.0.0  88000000   MPCI Southbridge
//   0      01h    51020000  2.4.2.0.1  88100000   MBIU2
//   1             51100000  2.4.2.1.0             MPCI (self-reference)
//   2      42h    51200000  2.4.2.2.0  89000000   OHCI #2
//   3      47h    51300000  2.4.2.3.0  89800000   ATA-5
//   4      DFh    51400000  2.4.2.4.0  8A000000   MDD (subtractive)
//   5      33h    51500000  2.4.2.5.0  8A800000   AC97 codec
//   6      42h    51600000  2.4.2.6.0  8B000000   OHCI #1
//   7      02h    51700000  2.4.2.7.0  8B800000   MCP


/////////////////////////////////////////////////////////////
//                  CS5536
/////////////////////////////////////////////////////////////
//
// Capabilities: 327920A0 80000003
// P2D_BM  = 3;    0x20-0x22
// P2D_BMK = 2;    0x23-0x24
// IOD_BM  = 10;   0xE0-0xE9
// IOD_SC  = 8;    0xEA-0xF1
// NPORTS  = 7;
// STATS   = 3;
//
//
//  Port   Dev_ID        Routing         FS/2      Device Description
// -----  -------- -------------------  --------   -----------------
//          05h    51000000  2.4.2.0.0  88000000   MPCI Southbridge
//   0      01h    51020000  2.4.2.0.1  88100000   MBIU2
//   1             51100000  2.4.2.1.0             MPCI (self-reference)
//   2      42h    51200000  2.4.2.2.0  89000000   <empty>
//   3      47h    51300000  2.4.2.3.0  89800000   ATA-5
//   4      DFh    51400000  2.4.2.4.0  8A000000   MDD (subtractive)
//   5      33h    51500000  2.4.2.5.0  8A800000   AC97 codec
//   6      43h    51600000  2.4.2.6.0  8B000000   USB 2.0
//   7      02h    51700000  2.4.2.7.0  8B800000   MCP


//
//				Arcturus
//
//	Device     AD PIN    Physical Device
//  ------     ------    ---------------------------------------------------
//	  13         23      MacPhyter
//	  14         24      PCI Slot 1  
//	  15         25      PCI Slot 2
//	  16         26      Chipset Register Space - pin H26 High
//	  17         27      USB Register Space     - pin H26 High
//	  18         28      Chipset Register Space - pin H26 Low
//	  19         29      USB Register Space     - pin H26 Low


//
//                  MPCI
//

// Fields for both MPCI_MSR_SMI and MPCI_MSR_ERROR
  #define MARM         (1L << 0)
  #define TARM         (1L << 1)
  #define BMM          (1L << 2)
  #define SSMM         (1L << 2)   // only in MPCI SB MSR_SMI
  #define VPHM         (1L << 3)	// only in MPCI NB MSR_SMI
  #define SYSM         (1L << 4)
  #define PARM         (1L << 5)
  #define MARE         (1L << 16)
  #define TARE         (1L << 17)
  #define BME          (1L << 18)  // Northbridge only ?
  #define VPHE         (1L << 19)	// only in MPCI NB MSR_SMI
  #define SYSE         (1L << 20)
  #define PARE         (1L << 21)
  #define TASE         (1L << 22)



// FooGlue MSRs:
#define FG_IIOC                    0x0010
  #define MODE_5530    0
  #define MODE_5535A   1
  #define MODE_5535B   2
#define FG_A20M                    0x0011
  #define A20M         (1 << 0)
#define FG_NMI                     0x0012
  #define NMI          (1 << 0)

#define FG_INIT                    0x0013
  #define INIT         (1 << 0)



#define MBIU_COH                   0x0080
#define MBIU_PAE                   0x0081
#define MBIU_ARB                   0x0082
#define MBIU_ASMI                  0x0083
#define MBIU_ERR                   0x0084
#define MBIU_DEBUG                 0x0085
#define MBIU_CAP                   0x0086
#define MBIU_NOUT_RESP             0x0087
#define MBIU_NOUT_WDATA            0x0088
#define MBIU_WHOAMI                0x008B
  // MBIU_WHOAMI tells self-reference:
  // MBIU0: 0x00000003
  // MBIU1: 0x00000001
  // MBIU2: 0x00000001
#define MBIU_SLV                   0x008C





//
// Descriptor MSRs
//
#define MSR_MEM_DESCR              0x0020
#define MSR_IO_DESCR               0x00E0
  // Defines for IOD_SC
  #define REN            (1L << 20)
  #define WEN            (1L << 21)







//
// Descriptor Statistics MSRs
//
#define MSR_STATISTICS_CNT         0x00A0		// and A4, A8, AC
  // High dword is Load Value;  Low dword is Count
#define MSR_STATISTICS_MASK        0x00A1		// and A5, A9, AD
  // High dword is IOD mask;    Low dword is P2D mask 
#define MSR_STATISTICS_ACTION      0x00A2		// and A6, AA, AE
  #define HIT_LDEN       (1L << 0)		// Load CNT on descriptor hit
  #define HIT_DEC        (1L << 1)		// Decrement CNT on descriptor hit
  #define HIT_SMI        (1L << 2)		// Assert ASMI on descriptor hit
  #define HIT_ERR        (1L << 3)		// Assert AERR on descriptor hit
  #define ALWAYS_DEC     (1L << 4)		// Always decrement CNT (unless loading or (CNT = 0 & !RELOAD))
  #define ZERO_SMI       (1L << 5)		// Assert ASMI on CNT = 0
  #define ZERO_ERR       (1L << 6)		// Assert AERR on CNT = 0
  #define WRAP           (1L << 7)		// Reload CNT from LOAD_VAL on CNT = 0

// Vail PM stuff
#define BC_MSR0                    0x1900
  #define SUSP_EN        (0x1000L)

#define XC_CONFIG                  0x1210
  #define XC_CLK_SUSP    (0x01L)

// GX2 Memory Controller PM stuff

// MC PMode1 Up Delay
#define MC_CF1017_DATA             0x001A
  #define PM1_UP_DLY_MASK	(0xFF00L)
	// 240ns delay, DDR spec. state minimum delay is 200ns
  #define PM1_UP_DLY_VAL	(0xF000L)

// MC PMode Sensitivity
// bits[63:32]=PMode1, bits[31:0]=PMode0
#define MC_CF_PMCTR	            0x0020
  // number of MC clocks that MC must be inactive
  // before entering PMode1
  #define PM1_SENS_VAL   (0x020L)


// GX2 GLCP PM stuff

// Clock Disable Delay
#define MCP_CLK_DIS_DLY            0x0008
  #define CLKDIS_MASK    (0x00FFFFFFL)

// PM Clock Disable
#define MCP_PMCLKDISABLE (0x0009)

#define MCP_PMCLKOFF     (0x0010)

// PM CLK4ACK MSR
#define MCP_CLK4ACK                0x0013
  #define S3_CLK4ACK     (0x07BE7FC3L)
  #define S1_CLK4ACK     S3_CLK4ACK

//  Throttling PM I/O regs default location
#define MCP_GLB_PM                 0x000B
  #define MCP_GLB_THEN   (0x01L)

#define MCP_DOTPLL                 0x0015
#define MCP_DBGCLKCTL              0x0016

// default location to map GLCP P_CNT I/O space regs
#define PMGX2_BASE                 (0x9E00)
/* offset from PMGX2_BASE */
#define P_CNT_OFS        (0) 
#define P_LVL2_OFS       (4)
#define P_LVL3_OFS       (8)

// GLCP MSR offsets for clock throttling
#define MCP_CNT                    (0x0018)
  #define CNT_THEN       (0x10L)
  #define CNT_MASK       (0x0FL)
  #define CNT_MAX        (0x01L)
  #define CNT_NONE       (0x0FL)
#define MCP_LVL2                   (0x0019)
#define MCP_TH_SD                  (0x001C)
  #define SD_MASK        (0x0FFFL)
  #define PLVL2_IN       (0x1000L)
#define MCP_TH_SF                  (0x001D)
  #define SF_MASK        (0x0FFL)
#define MCP_TH_OD                  (0x001E)
  #define OD_IRQ         (0x8000L)
  #define OD_SMI         (0x4000L)
  #define OD_MASK        (0x3FFFL)





