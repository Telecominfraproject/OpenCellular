/*
 * This file is part of the coreboot project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _SOC_APOLLOLAKE_PCI_DEVS_H_
#define _SOC_APOLLOLAKE_PCI_DEVS_H_

#include <device/pci_def.h>
#include <rules.h>

#define _SA_DEVFN(slot)		PCI_DEVFN(SA_DEV_SLOT_ ## slot, 0)
#define _PCH_DEVFN(slot, func)	PCI_DEVFN(PCH_DEV_SLOT_ ## slot, func)

#if !defined(__SIMPLE_DEVICE__)
#include <device/device.h>
#include <device/pci_def.h>
#define _SA_DEV(slot)		dev_find_slot(0, _SA_DEVFN(slot))
#define _PCH_DEV(slot, func)	dev_find_slot(0, _PCH_DEVFN(slot, func))
#else
#include <arch/io.h>
#define _SA_DEV(slot)		PCI_DEV(0, SA_DEV_SLOT_ ## slot, 0)
#define _PCH_DEV(slot, func)	PCI_DEV(0, PCH_DEV_SLOT_ ## slot, func)
#endif

/* System Agent Devices */

#define SA_DEV_SLOT_ROOT	0x00
#define  SA_DEVFN_ROOT		_SA_DEVFN(ROOT)
#define  SA_DEV_ROOT		_SA_DEV(ROOT)

#define SA_DEV_SLOT_PUNIT	0x01
#define  SA_DEVFN_PUNIT		_SA_DEVFN(PUNIT)
#define  SA_DEV_PUNIT		_SA_DEV(PUNIT)

#define SA_DEV_SLOT_IGD		0x02
#define  SA_DEVFN_IGD		_SA_DEVFN(IGD)
#define  SA_DEV_IGD		_SA_DEV(IGD)

/* PCH Devices */

#define PCH_DEV_SLOT_NPK	0x00
#define  PCH_DEVFN_NPK		_PCH_DEVFN(NPK, 2)
#define  PCH_DEV_NPK		_PCH_DEV(NPK,2)

#define PCH_DEV_SLOT_P2SB	0x0d
#define  PCH_DEVFN_P2SB		_PCH_DEVFN(P2SB, 0)
#define  PCH_DEVFN_PMC		_PCH_DEVFN(P2SB, 1)
#define  PCH_DEVFN_SPI		_PCH_DEVFN(P2SB, 2)
#define  PCH_DEV_P2SB		_PCH_DEV(P2SB, 0)
#define  PCH_DEV_PMC		_PCH_DEV(P2SB, 1)
#define  PCH_DEV_SPI		_PCH_DEV(P2SB, 2)

#define PCH_DEV_SLOT_HDA	0x0e
#define  PCH_DEVFN_HDA		_PCH_DEVFN(HDA, 0)
#define  PCH_DEV_HDA		_PCH_DEV(HDA, 0)

#define PCH_DEV_SLOT_CSE	0x0f
#define  PCH_DEVFN_CSE		_PCH_DEVFN(CSE, 0)
#define  PCH_DEV_CSE		_PCH_DEV(CSE, 0)

#define PCH_DEV_SLOT_ISH	0x11
#define  PCH_DEVFN_ISH		_PCH_DEVFN(ISH, 0)
#define  PCH_DEV_ISH		_PCH_DEV(ISH, 0)

#define PCH_DEV_SLOT_SATA	0x12
#define  PCH_DEVFN_SATA		_PCH_DEVFN(SATA, 0)
#define  PCH_DEV_SATA		_PCH_DEV(SATA, 0)

#define PCH_DEV_SLOT_PCIE	0x13
#define  PCH_DEVFN_PCIE1	_PCH_DEVFN(PCIE, 0)
#define  PCH_DEVFN_PCIE2	_PCH_DEVFN(PCIE, 1)
#define  PCH_DEVFN_PCIE3	_PCH_DEVFN(PCIE, 2)
#define  PCH_DEVFN_PCIE4	_PCH_DEVFN(PCIE, 3)
#define  PCH_DEV_PCIE1		_PCH_DEV(PCIE, 0)
#define  PCH_DEV_PCIE2		_PCH_DEV(PCIE, 1)
#define  PCH_DEV_PCIE3		_PCH_DEV(PCIE, 2)
#define  PCH_DEV_PCIE4		_PCH_DEV(PCIE, 3)

#define PCH_DEV_SLOT_PCIE_1	0x14
#define  PCH_DEVFN_PCIE5	_PCH_DEVFN(PCIE_1, 0)
#define  PCH_DEVFN_PCIE6	_PCH_DEVFN(PCIE_1, 1)
#define  PCH_DEV_PCIE5		_PCH_DEV(PCIE_1, 0)
#define  PCH_DEV_PCIE6		_PCH_DEV(PCIE_1, 1)

#define PCH_DEV_SLOT_XHCI	0x15
#define  PCH_DEVFN_XHCI		_PCH_DEVFN(XHCI, 0)
#define  PCH_DEVFN_XDCI		_PCH_DEVFN(XHCI, 1)
#define  PCH_DEV_XHCI		_PCH_DEV(XHCI, 0)
#define  PCH_DEV_XDCI		_PCH_DEV(XHCI, 1)

/* LPSS I2C, 2 devices cover 8 controllers */
#define PCH_DEV_SLOT_SIO1	0x16
#define  PCH_DEVFN_I2C0		_PCH_DEVFN(SIO1, 0)
#define  PCH_DEVFN_I2C1		_PCH_DEVFN(SIO1, 1)
#define  PCH_DEVFN_I2C2		_PCH_DEVFN(SIO1, 2)
#define  PCH_DEVFN_I2C3		_PCH_DEVFN(SIO1, 3)
#define  PCH_DEV_I2C0		_PCH_DEV(SIO1, 0)
#define  PCH_DEV_I2C1		_PCH_DEV(SIO1, 1)
#define  PCH_DEV_I2C2		_PCH_DEV(SIO1, 2)
#define  PCH_DEV_I2C3		_PCH_DEV(SIO1, 3)

#define PCH_DEV_SLOT_SIO2	0x17
#define  PCH_DEVFN_I2C4		_PCH_DEVFN(SIO2, 0)
#define  PCH_DEVFN_I2C5		_PCH_DEVFN(SIO2, 1)
#define  PCH_DEVFN_I2C6		_PCH_DEVFN(SIO2, 2)
#define  PCH_DEVFN_I2C7		_PCH_DEVFN(SIO2, 3)
#define  PCH_DEV_I2C4		_PCH_DEV(SIO2, 0)
#define  PCH_DEV_I2C5		_PCH_DEV(SIO2, 1)
#define  PCH_DEV_I2C6		_PCH_DEV(SIO2, 2)
#define  PCH_DEV_I2C7		_PCH_DEV(SIO2, 3)

/* LPSS UART */
#define PCH_DEV_SLOT_UART	0x18
#define  PCH_DEVFN_UART0	_PCH_DEVFN(UART, 0)
#define  PCH_DEVFN_UART1	_PCH_DEVFN(UART, 1)
#define  PCH_DEVFN_UART2	_PCH_DEVFN(UART, 2)
#define  PCH_DEVFN_UART3	_PCH_DEVFN(UART, 3)
#define  PCH_DEV_UART0		_PCH_DEV(UART, 0)
#define  PCH_DEV_UART1		_PCH_DEV(UART, 1)
#define  PCH_DEV_UART2		_PCH_DEV(UART, 2)
#define  PCH_DEV_UART3		_PCH_DEV(UART, 3)

/* LPSS SPI */
#define PCH_DEV_SLOT_SPI	0x19
#define  PCH_DEVFN_SPI0		_PCH_DEVFN(SPI, 0)
#define  PCH_DEVFN_SPI1		_PCH_DEVFN(SPI, 1)
#define  PCH_DEVFN_SPI2		_PCH_DEVFN(SPI, 2)
#define  PCH_DEV_SPI0		_PCH_DEV(SPI, 0)
#define  PCH_DEV_SPI1		_PCH_DEV(SPI, 1)
#define  PCH_DEV_SPI2		_PCH_DEV(SPI, 2)

/* LPSS PWM */
#define PCH_DEV_SLOT_PWM	0x1a
#define  PCH_DEVFN_PWM		_PCH_DEVFN(PWM, 0)
#define  PCH_DEV_PWM		_PCH_DEV(PWM, 0)

#define PCH_DEV_SLOT_SDCARD	0x1b
#define  PCH_DEVFN_SDCARD	_PCH_DEVFN(SDCARD, 0)
#define  PCH_DEV_SDCARD		_PCH_DEV(SDCARD, 0)

#define PCH_DEV_SLOT_EMMC	0x1c
#define  PCH_DEVFN_EMMC		_PCH_DEVFN(EMMC, 0)
#define  PCH_DEV_EMMC		_PCH_DEV(EMMC, 0)

#define PCH_DEV_SLOT_SDIO	0x1e
#define  PCH_DEVFN_SDIO		_PCH_DEVFN(SDIO, 0)
#define  PCH_DEV_SDIO		_PCH_DEV(SDIO, 0)

#define PCH_DEV_SLOT_LPC	0x1f
#define  PCH_DEVFN_LPC		_PCH_DEVFN(LPC, 0)
#define  PCH_DEVFN_SMBUS	_PCH_DEVFN(LPC, 1)
#define  PCH_DEV_LPC		_PCH_DEV(LPC, 0)
#define  PCH_DEV_SMBUS		_PCH_DEV(LPC, 1)

#endif
