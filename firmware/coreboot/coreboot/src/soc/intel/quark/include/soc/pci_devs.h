/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
 * Copyright (C) 2013 Sage Electronic Engineering, LLC.
 * Copyright (C) 2015-2017 Intel Corporation.
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

#ifndef _QUARK_PCI_DEVS_H_
#define _QUARK_PCI_DEVS_H_

#include <device/pci.h>
#include <device/pci_def.h>

/* DEVICE 0 (Memory Controller Hub) */
#define MC_BDF			PCI_DEV(PCI_BUS_NUMBER_QNC, MC_DEV, MC_FUN)

/* Device IDs */
#define I2CGPIO_DEVID		0x0934
#define HSUART_DEVID		0x0936
#define EHCI_DEVID		0x0939
#define LPC_DEVID		0X095E
#define PCIE_PORT0_DEVID	0x11c3
#define PCIE_PORT1_DEVID	0x11c4

/* IO Fabric 1 */
#define SIO1_DEV		0x14
#define SD_MMC_DEV		SIO1_DEV
#define HSUART0_DEV		SIO1_DEV
#define HSUART1_DEV		SIO1_DEV
#define SD_MMC_FUNC		0
#define HSUART0_FUNC		1
#define USB_DEV_PORT_FUNC	2
#define EHCI_FUNC		3
#define OHCI_FUNC		4
#define HSUART1_FUNC		5
#define HSUART0_BDF	PCI_DEV(PCI_BUS_NUMBER_QNC, HSUART0_DEV, HSUART0_FUNC)
#define HSUART1_BDF	PCI_DEV(PCI_BUS_NUMBER_QNC, HSUART1_DEV, HSUART1_FUNC)

/* IO Fabric 2 */
#define SIO2_DEV		0x15
#define I2CGPIO_DEV		SIO2_DEV
#define I2CGPIO_FUNC		2
#define I2CGPIO_DEV_FUNC	PCI_DEVFN(I2CGPIO_DEV, I2CGPIO_FUNC)
#define I2CGPIO_BDF	PCI_DEV(PCI_BUS_NUMBER_QNC, I2CGPIO_DEV, I2CGPIO_FUNC)

/* PCIe Ports */
#define PCIE_DEV		0x17
#define PCIE_PORT0_DEV		PCIE_DEV
#define PCIE_PORT0_FUNC		0
#define PCIE_PORT0_DEV_FUNC	PCI_DEVFN(PCIE_DEV, PCIE_PORT0_FUNC)
#define PCIE_PORT0_BDF	PCI_DEV(PCI_BUS_NUMBER_QNC, PCIE_DEV, PCIE_PORT0_FUNC)

#define PCIE_PORT1_DEV		PCIE_DEV
#define PCIE_PORT1_FUNC		1
#define PCIE_PORT1_DEV_FUNC	PCI_DEVFN(PCIE_DEV, PCIE_PORT1_FUNC)
#define PCIE_PORT1_BDF	PCI_DEV(PCI_BUS_NUMBER_QNC, PCIE_DEV, PCIE_PORT1_FUNC)

/* Platform Controller Unit */
#define LPC_DEV			PCI_DEVICE_NUMBER_QNC_LPC
#define LPC_FUNC		PCI_FUNCTION_NUMBER_QNC_LPC
#define LPC_DEV_FUNC		PCI_DEVFN(LPC_DEV, LPC_FUNC)
#define LPC_BDF			PCI_DEV(PCI_BUS_NUMBER_QNC, LPC_DEV, LPC_FUNC)

#endif /* _QUARK_PCI_DEVS_H_ */
