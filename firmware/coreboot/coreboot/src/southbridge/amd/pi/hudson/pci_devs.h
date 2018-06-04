/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Sage Electronic Engineering, LLC.
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

#ifndef _PI_HUDSON_PCI_DEVS_H_
#define _PI_HUDSON_PCI_DEVS_H_

#define BUS0			0

/* XHCI */
#define XHCI_DEV		0x10
#define XHCI_FUNC		0
#define XHCI_DEVID		0x7814
#define XHCI_DEVFN		PCI_DEVFN(XHCI_DEV,XHCI_FUNC)

#define XHCI2_DEV		0x10
#define XHCI2_FUNC		1
#define XHCI2_DEVID		0x7814
#define XHCI2_DEVFN		PCI_DEVFN(XHCI2_DEV,XHCI2_FUNC)

/* SATA */
#define SATA_DEV		0x11
#define SATA_FUNC		0
#define SATA_IDE_DEVID		0x7800
#define AHCI_DEVID_MS		0x7801
#define AHCI_DEVID_AMD		0x7804
#define SATA_DEVFN		PCI_DEVFN(SATA_DEV,SATA_FUNC)

/* OHCI */
#define OHCI1_DEV		0x12
#define OHCI1_FUNC		0
#define OHCI2_DEV		0x13
#define OHCI2_FUNC		0
#define OHCI3_DEV		0x16
#define OHCI3_FUNC		0
#define OHCI4_DEV		0x14
#define OHCI4_FUNC		5
#define OHCI_DEVID		0x7807
#define OHCI1_DEVFN		PCI_DEVFN(OHCI1_DEV,OHCI1_FUNC)
#define OHCI2_DEVFN		PCI_DEVFN(OHCI2_DEV,OHCI2_FUNC)
#define OHCI3_DEVFN		PCI_DEVFN(OHCI3_DEV,OHCI3_FUNC)
#define OHCI4_DEVFN		PCI_DEVFN(OHCI4_DEV,OHCI4_FUNC)

/* EHCI */
#define EHCI1_DEV		0x12
#define EHCI1_FUNC		2
#define EHCI2_DEV		0x13
#define EHCI2_FUNC		2
#define EHCI3_DEV		0x16
#define EHCI3_FUNC		2
#define EHCI_DEVID		0x7808
#define EHCI1_DEVFN		PCI_DEVFN(EHCI1_DEV,EHCI1_FUNC)
#define EHCI2_DEVFN		PCI_DEVFN(EHCI2_DEV,EHCI2_FUNC)
#define EHCI3_DEVFN		PCI_DEVFN(EHCI3_DEV,EHCI3_FUNC)

/* SMBUS */
#define SMBUS_DEV		0x14
#define SMBUS_FUNC		0
#define SMBUS_DEVID		0x780B
#define SMBUS_DEVFN		PCI_DEVFN(SMBUS_DEV,SMBUS_FUNC)

/* IDE */
#if IS_ENABLED(CONFIG_SOUTHBRIDGE_AMD_PI_BOLTON)
#define IDE_DEV			0x14
#define IDE_FUNC		1
#define IDE_DEVID		0x780C
#define IDE_DEVFN		PCI_DEVFN(IDE_DEV,IDE_FUNC)
#endif

/* HD Audio */
#define HDA_DEV			0x14
#define HDA_FUNC		2
#define HDA_DEVID		0x780D
#define HDA_DEVFN		PCI_DEVFN(HDA_DEV,HDA_FUNC)

/* LPC BUS */
#define PCU_DEV			0x14
#define LPC_DEV			PCU_DEV
#define LPC_FUNC		3
#define LPC_DEVID		0x780E
#define LPC_DEVFN		PCI_DEVFN(LPC_DEV,LPC_FUNC)

/* PCI Ports */
#define SB_PCI_PORT_DEV		0x14
#define SB_PCI_PORT_FUNC	4
#define SB_PCI_PORT_DEVID	0x780F
#define SB_PCI_PORT_DEVFN	PCI_DEVFN(SB_PCI_PORT_DEV,SB_PCI_PORT_FUNC)

/* SD Controller */
#define SD_DEV			0x14
#define SD_FUNC			7
#define SD_DEVID		0x7806
#define SD_DEVFN		PCI_DEVFN(SD_DEV,SD_FUNC)

/* PCIe Ports */
#if IS_ENABLED(CONFIG_SOUTHBRIDGE_AMD_PI_BOLTON)
#define SB_PCIE_DEV		0x15
#define SB_PCIE_PORT1_FUNC	0
#define SB_PCIE_PORT2_FUNC	1
#define SB_PCIE_PORT3_FUNC	2
#define SB_PCIE_PORT4_FUNC	3
#define SB_PCIE_PORT1_DEVID	0x7820
#define SB_PCIE_PORT2_DEVID	0x7821
#define SB_PCIE_PORT3_DEVID	0x7822
#define SB_PCIE_PORT4_DEVID	0x7823
#define SB_PCIE_PORT1_DEVFN	PCI_DEVFN(SB_PCIE_DEV,SB_PCIE_PORT1_FUNC)
#define SB_PCIE_PORT2_DEVFN	PCI_DEVFN(SB_PCIE_DEV,SB_PCIE_PORT2_FUNC)
#define SB_PCIE_PORT3_DEVFN	PCI_DEVFN(SB_PCIE_DEV,SB_PCIE_PORT3_FUNC)
#define SB_PCIE_PORT4_DEVFN	PCI_DEVFN(SB_PCIE_DEV,SB_PCIE_PORT4_FUNC)
#endif

#endif /* _PI_HUDSON_PCI_DEVS_H_ */
