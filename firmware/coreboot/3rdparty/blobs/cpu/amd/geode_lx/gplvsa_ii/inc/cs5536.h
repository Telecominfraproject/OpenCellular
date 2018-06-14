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

//**************************  CS5536 related defines  ***************************



// Southbridge MPCI CTRL:
#define ME	(1 << 0)	// Enable in-bound memory accesses
#define IE	(1 << 1)	// Enable in-bound I/O accesses
#define CIS_MASK  (3 << 3)	// CIS Mode 
  #define CIS_A (1 << 3)
  #define CIS_B (2 << 3)
  #define CIS_C (3 << 3)



#define MPCI_SOUTH	0x51000000	 // 2.4.2.0

#define MDD_PORT    4  // Port that MDD is at



#define REGION_R0   0x20
#define REGION_R15  (REGION_R0 + 15)




/****** 5536 GPIO definitions ******/

// offsets from GPIO base
#define GPIO5536_BASE			(0x6100)

#define NUM_5536_GPIO			(28)

#define GPIOH_OFFSET			(0x80)
 
#define GPIO_OUT_VAL 			(0x00)
#define GPIO_OUT_EN 			(0x04)
#define GPIO_OUT_OD_EN 			(0x08)
#define GPIO_OUT_INV_EN 		(0x0C)
#define GPIO_OUT_AUX1_SEL 		(0x10)
#define GPIO_OUT_AUX2_SEL 		(0x14)
#define GPIO_PU_EN 				(0x18)
#define GPIO_PD_EN				(0x1C)
#define GPIO_IN_EN				(0x20)
#define GPIO_IN_INV_EN			(0x24)
#define GPIO_IN_FIL_EN			(0x28)
#define GPIO_IN_EVC_EN			(0x2C)
#define GPIO_READBACK			(0x30)
#define GPIO_IN_AUX_SEL			(0x34)
#define GPIO_EVENT_EN			(0x38)
#define GPIO_LOCK_EN			(0x3C)
#define GPIO_IN_PEDG_EN			(0x40)
#define GPIO_IN_NEDG_EN			(0x44)
#define GPIO_IN_PEDG_STS		(0x48)
#define GPIO_IN_NEDG_STS		(0x4C)

#define	GPIO00_FILA				(0x50)
#define GPIO00_FILC				(0x52)
#define GPIO00_EVCNT			(0x54)
#define GPIO00_EVCMP			(0x56)
#define	GPIO01_FILA				(0x58)
#define GPIO01_FILC				(0x5A)
#define GPIO01_EVCNT			(0x5C)
#define GPIO01_EVCMP			(0x5E)
#define	GPIO02_FILA				(0x60)
#define GPIO02_FILC				(0x62)
#define GPIO02_EVCNT			(0x64)
#define GPIO02_EVCMP			(0x66)
#define	GPIO03_FILA				(0x68)
#define GPIO03_FILC				(0x6A)
#define GPIO03_EVCNT			(0x6C)
#define GPIO03_EVCMP			(0x6E)
#define	GPIO04_FILA				(0x70)
#define GPIO04_FILC				(0x72)
#define GPIO04_EVCNT			(0x74)
#define GPIO04_EVCMP			(0x76)
#define	GPIO05_FILA				(0x78)
#define GPIO05_FILC				(0x7A)
#define GPIO05_EVCNT			(0x7C)
#define GPIO05_EVCMP			(0x7E)
#define	GPIO06_FILA				(0xD0)
#define GPIO06_FILC				(0xD2)
#define GPIO06_EVCNT			(0xD4)
#define GPIO06_EVCMP			(0xD6)
#define	GPIO07_FILA				(0xD8)
#define GPIO07_FILC				(0xDA)
#define GPIO07_EVCNT			(0xDC)
#define GPIO07_EVCMP			(0xDE)
  
#define GPIO_MAPX				(0xE0)
#define GPIO_MAPY				(0xE4)
#define GPIO_MAPZ				(0xE8)
#define GPIO_MAPW				(0xEC)

#define GPIO_FE0				(0xF0)
#define GPIO_FE1				(0xF1)
#define GPIO_FE2				(0xF2)
#define GPIO_FE3				(0xF3)
#define GPIO_FE4				(0xF4)
#define GPIO_FE5				(0xF5)
#define GPIO_FE6				(0xF6)
#define GPIO_FE7				(0xF7)

#define GPIOL_IN_EVENT_DECR		(0xF8)
#define GPIOH_IN_EVENT_DECR		(0xFC)

// GPIO atomic register values
#define	GPIO00_SET		(0x00000001L)
#define GPIO00_CLR		(0x00010000L)
#define GPIO01_SET		(0x00000002L)
#define GPIO01_CLR		(0x00020000L)
#define	GPIO02_SET		(0x00000004L)
#define GPIO02_CLR		(0x00040000L)
#define GPIO03_SET		(0x00000008L)
#define GPIO03_CLR		(0x00080000L)
#define	GPIO04_SET		(0x00000010L)
#define GPIO04_CLR		(0x00100000L)
#define GPIO05_SET		(0x00000020L)
#define GPIO05_CLR		(0x00200000L)
#define	GPIO06_SET		(0x00000040L)
#define GPIO06_CLR		(0x00400000L)
#define GPIO07_SET		(0x00000080L)
#define GPIO07_CLR		(0x00800000L)
#define	GPIO08_SET		(0x00000100L)
#define GPIO08_CLR		(0x01000000L)
#define GPIO09_SET		(0x00000200L)
#define GPIO09_CLR		(0x02000000L)
#define	GPIO10_SET		(0x00000400L)
#define GPIO10_CLR		(0x04000000L)
#define GPIO11_SET		(0x00000800L)
#define GPIO11_CLR		(0x08000000L)
#define	GPIO12_SET		(0x00001000L)
#define GPIO12_CLR		(0x10000000L)
#define GPIO13_SET		(0x00002000L)
#define GPIO13_CLR		(0x20000000L)
#define	GPIO14_SET		(0x00004000L)
#define GPIO14_CLR		(0x40000000L)
#define GPIO15_SET		(0x00008000L)
#define GPIO15_CLR		(0x80000000L)

#define	GPIO16_SET		(0x00000001L)
#define GPIO16_CLR		(0x00010000L)
#define GPIO17_SET		(0x00000002L)
#define GPIO17_CLR		(0x00020000L)
#define	GPIO18_SET		(0x00000004L)
#define GPIO18_CLR		(0x00040000L)
#define GPIO19_SET		(0x00000008L)
#define GPIO19_CLR		(0x00080000L)
#define	GPIO20_SET		(0x00000010L)
#define GPIO20_CLR		(0x00100000L)
#define GPIO21_SET		(0x00000020L)
#define GPIO21_CLR		(0x00200000L)
#define	GPIO22_SET		(0x00000040L)
#define GPIO22_CLR		(0x00400000L)
#define GPIO23_SET		(0x00000080L)
#define GPIO23_CLR		(0x00800000L)
#define	GPIO24_SET		(0x00000100L)
#define GPIO24_CLR		(0x01000000L)
#define GPIO25_SET		(0x00000200L)
#define GPIO25_CLR		(0x02000000L)
#define	GPIO26_SET		(0x00000400L)
#define GPIO26_CLR		(0x04000000L)
#define GPIO27_SET		(0x00000800L)
#define GPIO27_CLR		(0x08000000L)
#define	GPIO28_SET		(0x00001000L)
#define GPIO28_CLR		(0x10000000L)
#define GPIO29_SET		(0x00002000L)
#define GPIO29_CLR		(0x20000000L)
#define	GPIO30_SET		(0x00004000L)
#define GPIO30_CLR		(0x40000000L)
#define GPIO31_SET		(0x00008000L)
#define GPIO31_CLR		(0x80000000L)

// Hawk platform has Sleep Button connected to GPIO25
#define DEFAULT_SLPB_GPIO	(25)

// GPIO13 AUX_IN is dedicated 5536 Sleep Button but it is only connected
// to Working Power Domain and can't wake the system from Standby.
// #define DEFAULT_SLPB_GPIO	(13)


// 5536 Power Management Controller

// offsets from base PMC I/O address, all are 32-bit regs
#define PM_SSD		(0x00)	// Sleep start delay
#define PM_SCXA		(0x04)	// Sleep control X Assert Delay and Enable
#define PM_SCYA		(0x08)	// Sleep control Y Assert Delay and Enable
#define PM_SODA		(0x0C)	// Sleep Output Disable Assert Delay and Enable
#define PM_SCLK		(0x10)	// Sleep Clock Delay and Enable
#define PM_SED		(0x14)	// Sleep End Delay
#define PM_SCXD		(0x18)	// Sleep Control X De-assert Delay and Enable
#define PM_SCYD		(0x1C)	// Sleep Control Y De-assert Delay and Enable
#define PM_SIDD		(0x20)	// Sleep Input Disable De-assert Delay and Enable
#define PM_WKD		(0x30)	// Working De-assert Delay and Enable
#define PM_WKXD		(0x34)	// Work_AUX De-assert Delay and Enable
#define PM_RD		(0x38)	// De-assert Delay from Standby
#define PM_WKXA		(0x3C)	// Work_AUX Assert Delay from Standby Wakeup
#define PM_FSD		(0x40)	// Fail-Safe Delay and Enable
#define PM_TSD		(0x44)	// Thermal-Safe Delay and Enable
#define PM_PSD		(0x48)	// Power-Safe Delay and Enable
#define PM_NWKD		(0x4C)	// Normal Work Delay and Enable
#define PM_AWKD		(0x50)	// Abnormal Work Delay and Enable
#define PM_SSC		(0x54)	// Standby Status and Control



#define USBMSROHCB			0x0008
#define USBMSREHCB			0x0009
#define USBMSRUDCB			0x000A
#define USBMSRUOCB			0x000B
#define PMEEN               (1L << 3)
