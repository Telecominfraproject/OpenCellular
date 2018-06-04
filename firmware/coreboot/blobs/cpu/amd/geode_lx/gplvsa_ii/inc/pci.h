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

// PCI related definitions
#define PCI_CONFIG_ADDRESS	0xCF8
#define PCI_CONFIG_DATA		0xCFC

#define VENDOR_ID			0x00
#define COMMAND				0x04
  #define IO_SPACE				(1 << 0)
  #define MEM_SPACE				(1 << 1)
  #define BUS_MASTER			(1 << 2)
  #define SPECIAL_CYCLES		(1 << 3)
  #define MEM_WR_INVALIDATE		(1 << 4)
  #define VGA_PALETTE_SNOOP		(1 << 5)
  #define PARITY_RESPONSE		(1 << 6)
  #define WAIT_CYCLE_CONTROL	(1 << 7)
  #define SERR_ENABLE			(1 << 8)
  #define FAST_BACK_TO_BACK		(1 << 9)

#define STATUS              0x06
  #define CAPABILITIES_LIST		(1L << 20)
  #define PCI_66MHZ_CAPABLE		(1L << 21)
  #define BACK2BACK_CAPABLE		(1L << 23)
  #define MASTER_PARITY_ERROR	(1L << 24)
  #define DEVSEL_FAST			(0L << 25)
  #define DEVSEL_MEDIUM 		(1L << 25)
  #define DEVSEL_SLOW			(2L << 25)
  #define SIGNALED_TARGET_ABORT	(1L << 27)
  #define RECEIVED_TARGET_ABORT	(1L << 28)
  #define RECEIVED_MASTER_ABORT	(1L << 29)
  #define SIGNALED_SYSTEM_ERROR	(1L << 30)
  #define DETECTED_PARITY_ERROR	(1L << 31)



#define REVISION_ID			0x08
#define CACHE_LINE			0x0C
#define LATENCY_TIMER		0x0D
#define HEADER_TYPE			0x0E
#define BIST				0x0F


// Capability List IDs
#define CAP_ID_PM			0x01
#define CAP_ID_AGP			0x02
#define CAP_ID_VPD			0x03
#define CAP_ID_SLOT			0x04
#define CAP_ID_MSI			0x05
#define CAP_ID_HOT_SWAP		0x06


#define SUBSYSTEM_VENDOR_ID 0x2C

#define INTERRUPT_LINE		0x3C
#define INTERRUPT_PIN		0x3D
#define MIN_GNT				0x3E
#define MAX_LAT				0x3F


#define BAR0				0x10
#define BAR1				0x14
#define BAR2				0x18
#define BAR3				0x1C
#define BAR4				0x20
#define BAR5				0x24

// PCI Power Management:
#define PCI_PM_REG			0x40

// Graphics-specific registers:
#define OEM_BAR0			0x50
#define OEM_BAR1			0x54
#define OEM_BAR2			0x58
#define OEM_BAR3			0x5C

// EHCI-specific registers
#define EECP				0x50
#define USBLEGSUP			(EECP)
  #define OS_OWNED_SEMAPHORE    0x01000000
  #define BIOS_OWNED_SEMAPHORE  0x00010000
#define USBLEGCTLSTS		(EECP+4)
  #define SMI_ON_BAR			0x80000000
  #define SMI_ON_COMMAND		0x40000000
  #define SMI_ON_OC				0x20000000

#define SRBN_REG			0x60

// 5536 B0 ATA-specific registers:
#define IDE_CFG				0x40
#define IDE_DTC				0x48
#define IDE_CAST			0x4C
#define IDE_ETC				0x50
#define IDE_PM				0x54

