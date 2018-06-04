/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2005 Li-Ta Lo <ollie@lanl.gov>
 * Copyright (C) 2005 Tyan
 * (Written by Yinghai Lu <yhlu@tyan.com> for Tyan)
 * Copyright (C) 2005 Ronald G. Minnich <rminnich@gmail.com>
 * Copyright (C) 2005-2007 Stefan Reinauer <stepan@openbios.org>
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

#include <console/console.h>
#include <commonlib/endian.h>
#include <compiler.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <string.h>
#include <cbfs.h>
#include <cbmem.h>
#include <arch/acpigen.h>

/* Rmodules don't like weak symbols. */
u32 __weak map_oprom_vendev(u32 vendev) { return vendev; }

struct rom_header *pci_rom_probe(struct device *dev)
{
	struct rom_header *rom_header;
	struct pci_data *rom_data;

	/* If it's in FLASH, then don't check device for ROM. */
	rom_header = cbfs_boot_map_optionrom(dev->vendor, dev->device);

	u32 vendev = (dev->vendor << 16) | dev->device;
	u32 mapped_vendev = vendev;

	mapped_vendev = map_oprom_vendev(vendev);

	if (!rom_header) {
		if (vendev != mapped_vendev) {
			rom_header = cbfs_boot_map_optionrom(
					mapped_vendev >> 16,
					mapped_vendev & 0xffff);
		}
	}

	if (rom_header) {
		printk(BIOS_DEBUG, "In CBFS, ROM address for %s = %p\n",
		       dev_path(dev), rom_header);
	} else if (!IS_ENABLED(CONFIG_ON_DEVICE_ROM_LOAD)) {
			printk(BIOS_DEBUG, "PCI Option ROM loading disabled "
				"for %s\n", dev_path(dev));
			return NULL;
	} else {
		uintptr_t rom_address;

		rom_address = pci_read_config32(dev, PCI_ROM_ADDRESS);

		if (rom_address == 0x00000000 || rom_address == 0xffffffff) {
#if IS_ENABLED(CONFIG_BOARD_EMULATION_QEMU_X86)
			if ((dev->class >> 8) == PCI_CLASS_DISPLAY_VGA)
				rom_address = 0xc0000;
			else
#endif
				return NULL;
		} else {
			/* Enable expansion ROM address decoding. */
			pci_write_config32(dev, PCI_ROM_ADDRESS,
					   rom_address|PCI_ROM_ADDRESS_ENABLE);
		}

		printk(BIOS_DEBUG, "Option ROM address for %s = %lx\n",
		       dev_path(dev), (unsigned long)rom_address);
		rom_header = (struct rom_header *)rom_address;
	}

	printk(BIOS_SPEW, "PCI expansion ROM, signature 0x%04x, "
	       "INIT size 0x%04x, data ptr 0x%04x\n",
	       le32_to_cpu(rom_header->signature),
	       rom_header->size * 512, le32_to_cpu(rom_header->data));

	if (le32_to_cpu(rom_header->signature) != PCI_ROM_HDR) {
		printk(BIOS_ERR, "Incorrect expansion ROM header "
		       "signature %04x\n", le32_to_cpu(rom_header->signature));
		return NULL;
	}

	rom_data = (((void *)rom_header) + le32_to_cpu(rom_header->data));

	printk(BIOS_SPEW, "PCI ROM image, vendor ID %04x, device ID %04x,\n",
	       rom_data->vendor, rom_data->device);
	/* If the device id is mapped, a mismatch is expected */
	if ((dev->vendor != rom_data->vendor
	    || dev->device != rom_data->device)
	    && (vendev == mapped_vendev)) {
		printk(BIOS_ERR, "ID mismatch: vendor ID %04x, "
		       "device ID %04x\n", rom_data->vendor, rom_data->device);
		return NULL;
	}

	printk(BIOS_SPEW, "PCI ROM image, Class Code %04x%02x, "
	       "Code Type %02x\n", rom_data->class_hi, rom_data->class_lo,
	       rom_data->type);

	if (dev->class != ((rom_data->class_hi << 8) | rom_data->class_lo)) {
		printk(BIOS_DEBUG, "Class Code mismatch ROM %08x, dev %08x\n",
		       (rom_data->class_hi << 8) | rom_data->class_lo,
		       dev->class);
		// return NULL;
	}

	return rom_header;
}

static void *pci_ram_image_start = (void *)PCI_RAM_IMAGE_START;

struct rom_header *pci_rom_load(struct device *dev,
				struct rom_header *rom_header)
{
	struct pci_data * rom_data;
	unsigned int rom_size;
	unsigned int image_size=0;

	do {
		/* Get next image. */
		rom_header = (struct rom_header *)((void *) rom_header
							    + image_size);

		rom_data = (struct pci_data *)((void *) rom_header
				+ le32_to_cpu(rom_header->data));

		image_size = le32_to_cpu(rom_data->ilen) * 512;
	} while ((rom_data->type != 0) && (rom_data->indicator != 0)); // make sure we got x86 version

	if (rom_data->type != 0)
		return NULL;

	rom_size = rom_header->size * 512;

	/*
	 * We check to see if the device thinks it is a VGA device not
	 * whether the ROM image is for a VGA device because some
	 * devices have a mismatch between the hardware and the ROM.
	 */
	if (PCI_CLASS_DISPLAY_VGA == (dev->class >> 8)) {
#if !IS_ENABLED(CONFIG_MULTIPLE_VGA_ADAPTERS)
		extern struct device *vga_pri; /* Primary VGA device (device.c). */
		if (dev != vga_pri) return NULL; /* Only one VGA supported. */
#endif
		if ((void *)PCI_VGA_RAM_IMAGE_START != rom_header) {
			printk(BIOS_DEBUG, "Copying VGA ROM Image from %p to "
			       "0x%x, 0x%x bytes\n", rom_header,
			       PCI_VGA_RAM_IMAGE_START, rom_size);
			memcpy((void *)PCI_VGA_RAM_IMAGE_START, rom_header,
			       rom_size);
		}
		return (struct rom_header *) (PCI_VGA_RAM_IMAGE_START);
	}

	printk(BIOS_DEBUG, "Copying non-VGA ROM image from %p to %p, 0x%x "
	       "bytes\n", rom_header, pci_ram_image_start, rom_size);

	memcpy(pci_ram_image_start, rom_header, rom_size);
	pci_ram_image_start += rom_size;
	return (struct rom_header *) (pci_ram_image_start-rom_size);
}

/* ACPI */
#if IS_ENABLED(CONFIG_HAVE_ACPI_TABLES)

/* VBIOS may be modified after oprom init so use the copy if present. */
static struct rom_header *check_initialized(struct device *dev)
{
	struct rom_header *run_rom;
	struct pci_data *rom_data;

	if (!IS_ENABLED(CONFIG_VGA_ROM_RUN))
		return NULL;

	run_rom = (struct rom_header *)(uintptr_t)PCI_VGA_RAM_IMAGE_START;
	if (read_le16(&run_rom->signature) != PCI_ROM_HDR)
		return NULL;

	rom_data = (struct pci_data *)((u8 *)run_rom
			+ read_le32(&run_rom->data));

	if (read_le32(&rom_data->signature) == PCI_DATA_HDR
			&& read_le16(&rom_data->device) == dev->device
			&& read_le16(&rom_data->vendor) == dev->vendor)
		return run_rom;
	else
		return NULL;
}

static unsigned long
pci_rom_acpi_fill_vfct(struct device *device, struct acpi_vfct *vfct_struct,
		       unsigned long current)
{
	struct acpi_vfct_image_hdr *header = &vfct_struct->image_hdr;
	struct rom_header *rom;

	vfct_struct->VBIOSImageOffset = (size_t)header - (size_t)vfct_struct;

	rom = check_initialized(device);
	if (!rom)
		rom = pci_rom_probe(device);
	if (!rom) {
		printk(BIOS_ERR, "pci_rom_acpi_fill_vfct failed\n");
		return current;
	}

	printk(BIOS_DEBUG, "           Copying %sVBIOS image from %p\n",
			rom == (struct rom_header *)
					(uintptr_t)PCI_VGA_RAM_IMAGE_START ?
			"initialized " : "",
			rom);

	header->DeviceID = device->device;
	header->VendorID = device->vendor;
	header->PCIBus = device->bus->secondary;
	header->PCIFunction = PCI_FUNC(device->path.pci.devfn);
	header->PCIDevice = PCI_SLOT(device->path.pci.devfn);
	header->ImageLength = rom->size * 512;
	memcpy((void *)&header->VbiosContent, rom, header->ImageLength);

	current += header->ImageLength;
	return current;
}

unsigned long
pci_rom_write_acpi_tables(struct device *device, unsigned long current,
			  struct acpi_rsdp *rsdp)
{
	struct acpi_vfct *vfct;
	struct rom_header *rom;

	/* Only handle VGA devices */
	if ((device->class >> 8) != PCI_CLASS_DISPLAY_VGA)
		return current;

	/* Only handle enabled devices */
	if (!device->enabled)
		return current;

	/* Probe for option rom */
	rom = pci_rom_probe(device);
	if (!rom)
		return current;

	/* AMD/ATI uses VFCT */
	if (device->vendor == PCI_VENDOR_ID_ATI) {
		current = ALIGN(current, 8);
		printk(BIOS_DEBUG, "ACPI:    * VFCT at %lx\n", current);
		vfct = (struct acpi_vfct *)current;
		acpi_create_vfct(device, vfct, pci_rom_acpi_fill_vfct);
		current += vfct->header.length;
		acpi_add_table(rsdp, vfct);
	}

	return current;
}

void pci_rom_ssdt(struct device *device)
{
	static size_t ngfx;

	/* Only handle VGA devices */
	if ((device->class >> 8) != PCI_CLASS_DISPLAY_VGA)
		return;

	/* Only handle enabled devices */
	if (!device->enabled)
		return;

	/* Probe for option rom */
	const struct rom_header *rom = pci_rom_probe(device);
	if (!rom || !rom->size) {
		printk(BIOS_WARNING, "%s: Missing PCI Option ROM\n",
		       dev_path(device));
		return;
	}

	const char *scope = acpi_device_path(device);
	if (!scope) {
		printk(BIOS_ERR, "%s: Missing ACPI scope\n", dev_path(device));
		return;
	}

	/* Supports up to four devices. */
	if ((CBMEM_ID_ROM0 + ngfx) > CBMEM_ID_ROM3) {
		printk(BIOS_ERR, "%s: Out of CBMEM IDs.\n", dev_path(device));
		return;
	}

	/* Prepare memory */
	const size_t cbrom_length = rom->size * 512;
	if (!cbrom_length) {
		printk(BIOS_ERR, "%s: ROM has zero length!\n",
		       dev_path(device));
		return;
	}

	void *cbrom = cbmem_add(CBMEM_ID_ROM0 + ngfx, cbrom_length);
	if (!cbrom) {
		printk(BIOS_ERR, "%s: Failed to allocate CBMEM.\n",
		       dev_path(device));
		return;
	}
	/* Increment CBMEM id for next device */
	ngfx++;

	memcpy(cbrom, rom, cbrom_length);

	/* write _ROM method */
	acpigen_write_scope(scope);
	acpigen_write_rom(cbrom, cbrom_length);
	acpigen_pop_len(); /* pop scope */
}
#endif
