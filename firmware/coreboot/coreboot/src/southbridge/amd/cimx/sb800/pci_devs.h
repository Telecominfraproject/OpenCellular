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

#ifndef _CIMX_SB800_PCI_DEVS_H_
#define _CIMX_SB800_PCI_DEVS_H_

#include <device/pci_def.h>

#define BUS0 0

/* SATA */
#define SATA_DEV 0x11
#define SATA_FUNC 0
# define SATA_IDE_DEVID 0x4390
# define AHCI_DEVID 0x4391
# define RAID_DEVID 0x4392
# define RAID5_DEVID 0x4393
# define SATA_DEVFN PCI_DEVFN(SATA_DEV,SATA_FUNC)

/* OHCI */
#define OHCI1_DEV 0x12
#define OHCI1_FUNC 0
#define OHCI2_DEV 0x13
#define OHCI2_FUNC 0
#define OHCI3_DEV 0x16
#define OHCI3_FUNC 0
#define OHCI4_DEV 0x14
#define OHCI4_FUNC 5
# define OHCI_DEVID 0x4397
# define OHCI1_DEVFN PCI_DEVFN(OHCI1_DEV,OHCI1_FUNC)
# define OHCI2_DEVFN PCI_DEVFN(OHCI2_DEV,OHCI2_FUNC)
# define OHCI3_DEVFN PCI_DEVFN(OHCI3_DEV,OHCI3_FUNC)
# define OHCI4_DEVFN PCI_DEVFN(OHCI4_DEV,OHCI4_FUNC)

/* EHCI */
#define EHCI1_DEV 0x12
#define EHCI1_FUNC 2
#define EHCI2_DEV 0x13
#define EHCI2_FUNC 2
#define EHCI3_DEV 0x16
#define EHCI3_FUNC 2
# define EHCI_DEVID 0x4396
# define EHCI1_DEVFN PCI_DEVFN(EHCI1_DEV,EHCI1_FUNC)
# define EHCI2_DEVFN PCI_DEVFN(EHCI2_DEV,EHCI2_FUNC)
# define EHCI3_DEVFN PCI_DEVFN(EHCI3_DEV,EHCI3_FUNC)

/* IDE */
#define IDE_DEV 0x14
#define IDE_FUNC 1
# define IDE_DEVID 0x439C
# define IDE_DEVFN PCI_DEVFN(IDE_DEV,IDE_FUNC)

/* HD Audio */
#define HDA_DEV 0x14
#define HDA_FUNC 2
# define HDA_DEVID 0x4383
# define HDA_DEVFN PCI_DEVFN(HDA_DEV,HDA_FUNC)

/* PCI Ports */
#define SB_PCI_PORT_DEV 0x14
#define SB_PCI_PORT_FUNC 4
# define SB_PCI_PORT_DEVID 0x4384
# define SB_PCI_PORT_DEVFN PCI_DEVFN(SB_PCI_PORT_DEV,SB_PCI_PORT_FUNC)

/* PCIe Ports */
#define SB_PCIE_DEV 0x15
#define SB_PCIE_PORT1_FUNC 0
#define SB_PCIE_PORT2_FUNC 1
#define SB_PCIE_PORT3_FUNC 2
#define SB_PCIE_PORT4_FUNC 3
# define SB_PCIE_PORT1_DEVID 0x43A0
# define SB_PCIE_PORT2_DEVID 0x43A1
# define SB_PCIE_PORT3_DEVID 0x43A2
# define SB_PCIE_PORT4_DEVID 0x43A3
# define SB_PCIE_PORT1_DEVFN PCI_DEVFN(SB_PCIE_DEV,SB_PCIE_PORT1_FUNC)
# define SB_PCIE_PORT2_DEVFN PCI_DEVFN(SB_PCIE_DEV,SB_PCIE_PORT2_FUNC)
# define SB_PCIE_PORT3_DEVFN PCI_DEVFN(SB_PCIE_DEV,SB_PCIE_PORT3_FUNC)
# define SB_PCIE_PORT4_DEVFN PCI_DEVFN(SB_PCIE_DEV,SB_PCIE_PORT4_FUNC)

/* Fusion Controller Hub */
#define PCU_DEV 0x14
#define LPC_DEV PCU_DEV
#define LPC_FUNC 3
#define SMBUS_DEV 0x14
#define SMBUS_FUNC 0
# define LPC_DEVID 0x439D
# define SMBUS_DEVID 0x4385
# define LPC_DEVFN PCI_DEVFN(LPC_DEV,LPC_FUNC)
# define SMBUS_DEVFN PCI_DEVFN(SMBUS_DEV,SMBUS_FUNC)

#endif /* _CIMX_SB800_PCI_DEVS_H_ */
