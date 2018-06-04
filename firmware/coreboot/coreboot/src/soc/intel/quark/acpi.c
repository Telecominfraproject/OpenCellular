/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2009 coresystems GmbH
 * Copyright (C) 2014 Google Inc.
 * Copyright (C) 2015-2016 Intel Corporation.
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

#include <soc/acpi.h>
#include <soc/ramstage.h>

unsigned long acpi_fill_madt(unsigned long current)
{
	return current;
}


unsigned long acpi_fill_mcfg(unsigned long current)
{
	return current;
}

void acpi_fill_in_fadt(acpi_fadt_t *fadt)
{
	struct device *dev = dev_find_slot(0,
		PCI_DEVFN(PCI_DEVICE_NUMBER_QNC_LPC,
		PCI_FUNCTION_NUMBER_QNC_LPC));
	uint32_t gpe0_base = pci_read_config32(dev, R_QNC_LPC_GPE0BLK)
		& B_QNC_LPC_GPE0BLK_MASK;
	uint32_t pmbase = pci_read_config32(dev, R_QNC_LPC_PM1BLK)
		& B_QNC_LPC_PM1BLK_MASK;

	fadt->flags = ACPI_FADT_RESET_REGISTER | ACPI_FADT_PLATFORM_CLOCK;

	/* PM1 Status: ACPI 4.8.3.1.1 */
	fadt->pm1a_evt_blk = pmbase + R_QNC_PM1BLK_PM1S;
	fadt->pm1_evt_len = 2;

	fadt->x_pm1a_evt_blk.space_id = ACPI_ADDRESS_SPACE_IO;
	fadt->x_pm1a_evt_blk.bit_width = fadt->pm1_evt_len * 8;
	fadt->x_pm1a_evt_blk.bit_offset = 0;
	fadt->reset_reg.access_size = ACPI_ACCESS_SIZE_WORD_ACCESS;
	fadt->x_pm1a_evt_blk.addrl = pmbase + R_QNC_PM1BLK_PM1S;
	fadt->x_pm1a_evt_blk.addrh = 0x0;

	/* PM1 Control: ACPI 4.8.3.2.1 */
	fadt->pm1a_cnt_blk = pmbase + R_QNC_PM1BLK_PM1C;
	fadt->pm1_cnt_len = 2;

	fadt->x_pm1a_cnt_blk.space_id = ACPI_ADDRESS_SPACE_IO;
	fadt->x_pm1a_cnt_blk.bit_width = fadt->pm1_cnt_len * 8;
	fadt->x_pm1a_cnt_blk.bit_offset = 0;
	fadt->reset_reg.access_size = ACPI_ACCESS_SIZE_WORD_ACCESS;
	fadt->x_pm1a_cnt_blk.addrl = fadt->pm1a_cnt_blk;
	fadt->x_pm1a_cnt_blk.addrh = 0x0;

	/* PM Timer: ACPI 4.8.3.3 */
	fadt->pm_tmr_blk = pmbase + R_QNC_PM1BLK_PM1T;
	fadt->pm_tmr_len = 4;

	fadt->x_pm_tmr_blk.space_id = ACPI_ADDRESS_SPACE_IO;
	fadt->x_pm_tmr_blk.bit_width = fadt->pm_tmr_len * 8;
	fadt->x_pm_tmr_blk.bit_offset = 0;
	fadt->reset_reg.access_size = ACPI_ACCESS_SIZE_DWORD_ACCESS;
	fadt->x_pm_tmr_blk.addrl = fadt->pm_tmr_blk;
	fadt->x_pm_tmr_blk.addrh = 0x0;

	/* Reset Register: ACPI 4.8.3.6, 5.2.3.2 */
	fadt->reset_reg.space_id = ACPI_ADDRESS_SPACE_IO;
	fadt->reset_reg.bit_width = 8;
	fadt->reset_reg.bit_offset = 0;
	fadt->reset_reg.access_size = ACPI_ACCESS_SIZE_BYTE_ACCESS;
	fadt->reset_reg.addrl = 0xcf9;
	fadt->reset_reg.addrh = 0;

	/* Soft/Warm Reset */
	fadt->reset_value = 6;

	/* General-Purpose Event 0 Registers: ACPI 4.8.4.1 */
	fadt->gpe0_blk = gpe0_base;
	fadt->gpe0_blk_len = 4 * 2;

	fadt->x_gpe0_blk.space_id = ACPI_ADDRESS_SPACE_IO;
	fadt->x_gpe0_blk.bit_width = fadt->gpe0_blk_len * 8;
	fadt->x_gpe0_blk.bit_offset = 0;
	fadt->reset_reg.access_size = ACPI_ACCESS_SIZE_DWORD_ACCESS;
	fadt->x_gpe0_blk.addrl = fadt->gpe0_blk;
	fadt->x_gpe0_blk.addrh = 0;

	/* Display the base registers */
	printk(BIOS_SPEW, "FADT:\n");
	printk(BIOS_SPEW, "  0x%08x: GPE0_BASE\n", gpe0_base);
	printk(BIOS_SPEW, "  0x%08x: PMBASE\n", pmbase);
	printk(BIOS_SPEW, "  0x%08x: RESET\n", fadt->reset_reg.addrl);

}
