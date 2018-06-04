/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015 Timothy Pearson <tpearson@raptorengineeringinc.com>, Raptor Engineering
 * Copyright (C) 2010 Advanced Micro Devices, Inc.
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

#include <stdint.h>
#include <arch/io.h>
#include <device/pci_ids.h>

#define IO_MEM_PORT_DECODE_ENABLE_5	0x48
#define IO_MEM_PORT_DECODE_ENABLE_6	0x4a
#define SPI_BASE_ADDRESS		0xa0

#define SPI_CONTROL_1			0xc
#define TEMPORARY_SPI_BASE_ADDRESS	0xfec10000

/*
 * Enable 4MB (LPC) ROM access at 0xFFC00000 - 0xFFFFFFFF.
 *
 * Hardware should enable LPC ROM by pin straps. This function does not
 * handle the theoretically possible PCI ROM, FWH, or SPI ROM configurations.
 *
 * The SB700 power-on default is to map 512K ROM space.
 *
 * Details: AMD SB700/710/750 BIOS Developer's Guide (BDG), Rev. 1.00,
 *          PN 43366_sb7xx_bdg_pub_1.00, June 2009, section 3.1, page 14.
 */
static void sb700_enable_rom(void)
{
	u8 reg8;
	u32 dword;
	pci_devfn_t dev;

	dev = PCI_DEV(0, 0x14, 3);

	reg8 = pci_io_read_config8(dev, IO_MEM_PORT_DECODE_ENABLE_5);
	if (IS_ENABLED(CONFIG_SPI_FLASH))
		/* Disable decode of variable LPC ROM address ranges 1 and 2. */
		reg8 &= ~((1 << 3) | (1 << 4));
	else
		/* Decode variable LPC ROM address ranges 1 and 2. */
		reg8 |= (1 << 3) | (1 << 4);
	pci_io_write_config8(dev, IO_MEM_PORT_DECODE_ENABLE_5, reg8);

	/* LPC ROM address range 1: */
	/* Enable LPC ROM range mirroring start at 0x000e(0000). */
	pci_io_write_config16(dev, 0x68, 0x000e);
	/* Enable LPC ROM range mirroring end at 0x000f(ffff). */
	pci_io_write_config16(dev, 0x6a, 0x000f);

	/* LPC ROM address range 2: */
	/*
	 * Enable LPC ROM range start at:
	 * 0xfff8(0000): 512KB
	 * 0xfff0(0000): 1MB
	 * 0xffe0(0000): 2MB
	 * 0xffc0(0000): 4MB
	 * 0xff80(0000): 8MB
	 */
	pci_io_write_config16(dev, 0x6c, 0x10000 - (CONFIG_COREBOOT_ROMSIZE_KB >> 6));
	/* Enable LPC ROM range end at 0xffff(ffff). */
	pci_io_write_config16(dev, 0x6e, 0xffff);

	/* SB700 LPC Bridge 0x48.
	 * Turn on all LPC IO Port decode enables
	 */
	pci_io_write_config32(dev, 0x44, 0xffffffff);

	/* SB700 LPC Bridge 0x48.
	 * BIT0: Port Enable for SuperIO 0x2E-0x2F
	 * BIT1: Port Enable for SuperIO 0x4E-0x4F
	 * BIT6: Port Enable for RTC IO 0x70-0x73
	 */
	reg8 = pci_io_read_config8(dev, IO_MEM_PORT_DECODE_ENABLE_5);
	reg8 |= (1 << 0) | (1 << 1) | (1 << 6);
	pci_io_write_config8(dev, IO_MEM_PORT_DECODE_ENABLE_5, reg8);

	/* SB700 LPC Bridge 0x4a.
	 * BIT5: Port Enable for Port 0x80
	 */
	reg8 = pci_io_read_config8(dev, IO_MEM_PORT_DECODE_ENABLE_6);
	reg8 |= (1 << 5);
	pci_io_write_config8(dev, IO_MEM_PORT_DECODE_ENABLE_6, reg8);
}

static void sb700_configure_rom(void)
{
	pci_devfn_t dev;
	uint32_t dword;

	dev = PCI_DEV(0, 0x14, 3);

	if (IS_ENABLED(CONFIG_SOUTHBRIDGE_AMD_SB700_33MHZ_SPI)) {
		uint32_t prev_spi_cfg;
		volatile uint32_t *spi_mmio;

		/* Temporarily set up SPI access to change SPI speed */
		prev_spi_cfg = dword = pci_io_read_config32(dev, SPI_BASE_ADDRESS);
		dword &= ~(0x7ffffff << 5);		/* SPI_BaseAddr */
		dword |= TEMPORARY_SPI_BASE_ADDRESS & (0x7ffffff << 5);
		dword |= (0x1 << 1);			/* SpiRomEnable = 1 */
		pci_io_write_config32(dev, SPI_BASE_ADDRESS, dword);

		spi_mmio = (void *)(TEMPORARY_SPI_BASE_ADDRESS + SPI_CONTROL_1);
		dword = *spi_mmio;
		dword &= ~(0x3 << 12);	/* NormSpeed = 0x1 */
		dword |= (0x1 << 12);
		*spi_mmio = dword;

		/* Restore previous SPI access */
		pci_io_write_config32(dev, SPI_BASE_ADDRESS, prev_spi_cfg);
	}
}

static void bootblock_southbridge_init(void)
{
	sb700_enable_rom();
	sb700_configure_rom();
}
