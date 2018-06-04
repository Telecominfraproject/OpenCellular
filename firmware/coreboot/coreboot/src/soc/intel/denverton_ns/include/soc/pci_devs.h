/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
 * Copyright (C) 2014-2017 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _DENVERTON_NS_PCI_DEVS_H_
#define _DENVERTON_NS_PCI_DEVS_H_

/* All these devices live on bus 0 with the associated device and function */

#include <rules.h>

#define _SA_DEVFN(slot) PCI_DEVFN(SA_DEV_SLOT_##slot, 0)
#define _PCH_DEVFN(slot, func) PCI_DEVFN(PCH_DEV_SLOT_##slot, func)

#if ENV_RAMSTAGE
#include <device/device.h>
#include <device/pci_def.h>
#define _SA_DEV(slot) dev_find_slot(0, _SA_DEVFN(slot))
#define _PCH_DEV(slot, func) dev_find_slot(0, _PCH_DEVFN(slot, func))
#else
#include <arch/io.h>
#define _SA_DEV(slot) PCI_DEV(0, SA_DEV_SLOT_##slot, 0)
#define _PCH_DEV(slot, func) PCI_DEV(0, PCH_DEV_SLOT_##slot, func)
#endif

/* SoC transaction router */
#define SA_DEV 0x0
#define SA_FUNC 0
#define SA_DEVID 0x1980
#define SA_DEVID_DNVAD 0x1995
#define SOC_DEV SA_DEV
#define SOC_FUNC SA_FUNC
#define SOC_DEVID SA_DEVID

/* RAS */
#define RAS_DEV 0x4
#define RAS_FUNC 0
#define RAS_DEVID 0x19a1

/* Root Complex Event Collector */
#define RCEC_DEV 0x5
#define RCEC_FUNC 0
#define RCEC_DEVID 0x19a2

/* Virtual Root Port 2 */
#define VRP2_DEV 0x6
#define VRP2_FUNC 0
#define VRP2_DEVID 0x19a3

/* PCIe Root Ports */
#define PCIE_DEV 0x09
#define MAX_PCIE_PORT 0x8
#define PCIE_PORT1_DEV 0x09
#define PCIE_PORT1_FUNC 0
#define PCIE_PORT1_DEVID 0x19a4
#define PCIE_PORT2_DEV 0x0a
#define PCIE_PORT2_FUNC 0
#define PCIE_PORT2_DEVID 0x19a5
#define PCIE_PORT3_DEV 0x0b
#define PCIE_PORT3_FUNC 0
#define PCIE_PORT3_DEVID 0x19a6
#define PCIE_PORT4_DEV 0x0c
#define PCIE_PORT4_FUNC 0
#define PCIE_PORT4_DEVID 0x19a7
#define PCIE_PORT5_DEV 0x0e
#define PCIE_PORT5_FUNC 0
#define PCIE_PORT5_DEVID 0x19a8
#define PCIE_PORT6_DEV 0x0f
#define PCIE_PORT6_FUNC 0
#define PCIE_PORT6_DEVID 0x19a9
#define PCIE_PORT7_DEV 0x10
#define PCIE_PORT7_FUNC 0
#define PCIE_PORT7_DEVID 0x19aa
#define PCIE_PORT8_DEV 0x11
#define PCIE_PORT8_FUNC 0
#define PCIE_PORT8_DEVID 0x19ab

/* SMBUS 2 */
#define SMBUS2_DEV 0x12
#define SMBUS2_FUNC 0
#define SMBUS2_DEVID 0x19ac

/* SATA */
#define SATA_DEV 0x13
#define SATA_FUNC 0
#define AHCI_DEVID 0x19b2
#define SATA2_DEV 0x14
#define SATA2_FUNC 0
#define AHCI2_DEVID 0x19c2

/* xHCI */
#define XHCI_DEV 0x15
#define XHCI_FUNC 0
#define XHCI_DEVID 0x19d0

/* Virtual Root Port 0 */
#define VRP0_DEV 0x16
#define VRP0_FUNC 0
#define VRP0_DEVID 0x19d1

/* Virtual Root Port 1 */
#define VRP1_DEV 0x17
#define VRP1_FUNC 0
#define VRP1_DEVID 0x19d2

/* CSME */
#define ME_HECI_DEV 0x18
#define ME_HECI1_DEV ME_HECI_DEV
#define ME_HECI1_FUNC 0
#define ME_HECI1_DEVID 0x19d3
#define ME_HECI2_DEV ME_HECI_DEV
#define ME_HECI2_FUNC 1
#define ME_HECI2_DEVID 0x19d4
#define ME_IEDR_DEV ME_HECI_DEV
#define ME_IEDR_FUNC 2
#define ME_IEDR_DEVID 0x19ea
#define ME_MEKT_DEV ME_HECI_DEV
#define ME_MEKT_FUNC 3
#define ME_MEKT_DEVID 0x19d5
#define ME_HECI3_DEV ME_HECI_DEV
#define ME_HECI3_FUNC 4
#define ME_HECI3_DEVID 0x19d6

/* HSUART */
#define HSUART_DEV 0x1a
#define HSUART_DEVID 0x19d8
#define HSUART1_DEV HSUART_DEV
#define HSUART1_FUNC 0
#define HSUART1_DEVID HSUART_DEVID
#define HSUART2_DEV HSUART_DEV
#define HSUART2_FUNC 1
#define HSUART2_DEVID HSUART_DEVID
#define HSUART3_DEV HSUART_DEV
#define HSUART3_FUNC 2
#define HSUART3_DEVID HSUART_DEVID

/* IE */
#define IE_HECI_DEV 0x1b
#define IE_HECI1_DEV IE_HECI_DEV
#define IE_HECI1_FUNC 0
#define IE_HECI1_DEVID 0x19e5
#define IE_HECI2_DEV IE_HECI_DEV
#define IE_HECI2_FUNC 1
#define IE_HECI2_DEVID 0x19e6
#define IE_IEDR_DEV IE_HECI_DEV
#define IE_IEDR_FUNC 2
#define IE_IEDR_DEVID 0x19e7
#define IE_MEKT_DEV IE_HECI_DEV
#define IE_MEKT_FUNC 3
#define IE_MEKT_DEVID 0x19e8
#define IE_HECI3_DEV IE_HECI_DEV
#define IE_HECI3_FUNC 4
#define IE_HECI3_DEVID 0x19e9

/* MMC Port */
#define MMC_DEV 0x1c
#define MMC_FUNC 0
#define MMC_DEVID 0x19db

/* Platform Controller Unit */
#define PCU_DEV 0x1f
#define LPC_DEV PCU_DEV
#define LPC_FUNC 0
#define LPC_DEVID 0x19dc
#define P2SB_DEV PCU_DEV
#define P2SB_FUNC 1
#define P2SB_DEVID 0x19dd
#define PMC_DEV PCU_DEV
#define PMC_FUNC 2
#define PMC_DEVID 0x19de
#define SMBUS_DEV PCU_DEV
#define SMBUS_FUNC 4
#define SMBUS_DEVID 0x19df
#define SPI_DEV PCU_DEV
#define SPI_FUNC 5
#define SPI_DEVID 0x19e0
#define NPK_DEV PCU_DEV
#define NPK_FUNC 7
#define NPK_DEVID 0x19e1

/* TODO - New added */
#define SA_DEV_SLOT_ROOT 0x00
#define SA_DEVFN_ROOT _SA_DEVFN(ROOT)
#define SA_DEV_ROOT _SA_DEV(ROOT)

#define PCH_DEV_SLOT_LPC 0x1f
#define PCH_DEVFN_LPC _PCH_DEVFN(LPC, 0)
#define PCH_DEVFN_SPI _PCH_DEVFN(LPC, 5)
#define PCH_DEV_LPC _PCH_DEV(LPC, 0)
#define PCH_DEV_SPI _PCH_DEV(LPC, 5)

#endif /* _DENVERTON_NS_PCI_DEVS_H_ */
