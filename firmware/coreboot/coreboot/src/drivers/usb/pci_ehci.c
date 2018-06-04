/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2006 Eric Biederman (ebiederm@xmission.com)
 * Copyright (C) 2007 AMD
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

#include <stddef.h>
#include <console/console.h>
#include <device/pci_ehci.h>
#include <arch/io.h>
#include <device/pci.h>
#include <device/pci_def.h>
#include <string.h>

#include "ehci_debug.h"
#include "ehci.h"

#if ENV_RAMSTAGE
static struct device_operations *ehci_drv_ops;
static struct device_operations ehci_dbg_ops;
#endif

int ehci_debug_hw_enable(unsigned int *base, unsigned int *dbg_offset)
{
	pci_devfn_t dbg_dev = pci_ehci_dbg_dev(CONFIG_USBDEBUG_HCD_INDEX);

#ifdef __SIMPLE_DEVICE__
	pci_devfn_t dev = dbg_dev;
#else
	device_t dev = dev_find_slot(PCI_DEV2SEGBUS(dbg_dev), PCI_DEV2DEVFN(dbg_dev));
#endif

	u32 class = pci_read_config32(dev, PCI_CLASS_REVISION) >> 8;
	if (class != PCI_EHCI_CLASSCODE)
		return -1;

	u8 pm_cap = pci_find_capability(dev, PCI_CAP_ID_PM);
	if (pm_cap) {
		u16 pm_ctrl = pci_read_config16(dev, pm_cap + PCI_PM_CTRL);
		/* Set to D0 and disable PM events. */
		pm_ctrl &= ~PCI_PM_CTRL_PME_ENABLE;
		pm_ctrl &= ~PCI_PM_CTRL_STATE_MASK;
		pci_write_config16(dev, pm_cap + PCI_PM_CTRL, pm_ctrl);
	}

	u8 pos = pci_find_capability(dev, PCI_CAP_ID_EHCI_DEBUG);
	if (!pos)
		return -1;

	u32 cap = pci_read_config32(dev, pos);

	/* FIXME: We should remove static EHCI_BAR_INDEX. */
	u8 ehci_bar = 0x10 + 4 * ((cap >> 29) - 1);
	if (ehci_bar != EHCI_BAR_INDEX)
		return -1;

	pci_write_config32(dev, ehci_bar, CONFIG_EHCI_BAR);

	pci_write_config8(dev, PCI_COMMAND, PCI_COMMAND_MEMORY |
		PCI_COMMAND_MASTER);

	*base = CONFIG_EHCI_BAR;
	*dbg_offset = (cap>>16) & 0x1ffc;

	return 0;
}

void ehci_debug_select_port(unsigned int port)
{
	pci_devfn_t dbg_dev = pci_ehci_dbg_dev(CONFIG_USBDEBUG_HCD_INDEX);
	pci_ehci_dbg_set_port(dbg_dev, port);
}

#if ENV_RAMSTAGE
static void pci_ehci_set_resources(struct device *dev)
{
	struct resource *res;

	printk(BIOS_DEBUG, "%s EHCI Debug Port hook triggered\n", dev_path(dev));
	usbdebug_disable();

	if (ehci_drv_ops->set_resources)
		ehci_drv_ops->set_resources(dev);
	res = find_resource(dev, EHCI_BAR_INDEX);
	if (!res)
		return;

	usbdebug_re_enable((u32)res->base);
	report_resource_stored(dev, res, "");
	printk(BIOS_DEBUG, "%s EHCI Debug Port relocated\n", dev_path(dev));
}

void pci_ehci_read_resources(struct device *dev)
{
	pci_devfn_t dbg_dev = pci_ehci_dbg_dev(CONFIG_USBDEBUG_HCD_INDEX);

	if (!ehci_drv_ops && pci_match_simple_dev(dev, dbg_dev)) {
		memcpy(&ehci_dbg_ops, dev->ops, sizeof(ehci_dbg_ops));
		ehci_drv_ops = dev->ops;
		ehci_dbg_ops.set_resources = pci_ehci_set_resources;
		dev->ops = &ehci_dbg_ops;
		printk(BIOS_DEBUG, "%s EHCI BAR hook registered\n", dev_path(dev));
	} else {
		printk(BIOS_DEBUG, "More than one caller of %s from %s\n", __func__, dev_path(dev));
	}

	pci_dev_read_resources(dev);
}
#endif

u8 *pci_ehci_base_regs(pci_devfn_t sdev)
{
#ifdef __SIMPLE_DEVICE__
	u8 *base = (u8 *)(pci_read_config32(sdev, EHCI_BAR_INDEX) & ~0x0f);
#else
	device_t dev = dev_find_slot(PCI_DEV2SEGBUS(sdev), PCI_DEV2DEVFN(sdev));
	u8 *base = (u8 *)(pci_read_config32(dev, EHCI_BAR_INDEX) & ~0x0f);
#endif
	return base + HC_LENGTH(read32(base));
}
