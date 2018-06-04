/*
 * Copyright (c) 2012, Google Inc.
 *
 * Based on drivers/spi/winbond.c
 *
 * Copyright 2008, Network Appliance Inc.
 * Jason McMullan <mcmullan@netapp.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <console/console.h>
#include <stdlib.h>
#include <spi_flash.h>
#include <spi-generic.h>
#include <string.h>

#include "spi_flash_internal.h"

/* GD25Pxx-specific commands */
#define CMD_GD25_WREN		0x06	/* Write Enable */
#define CMD_GD25_WRDI		0x04	/* Write Disable */
#define CMD_GD25_RDSR		0x05	/* Read Status Register */
#define CMD_GD25_WRSR		0x01	/* Write Status Register */
#define CMD_GD25_READ		0x03	/* Read Data Bytes */
#define CMD_GD25_FAST_READ	0x0b	/* Read Data Bytes at Higher Speed */
#define CMD_GD25_PP		0x02	/* Page Program */
#define CMD_GD25_SE		0x20	/* Sector (4K) Erase */
#define CMD_GD25_BE		0xd8	/* Block (64K) Erase */
#define CMD_GD25_CE		0xc7	/* Chip Erase */
#define CMD_GD25_DP		0xb9	/* Deep Power-down */
#define CMD_GD25_RES		0xab	/* Release from DP, and Read Signature */

struct gigadevice_spi_flash_params {
	uint16_t	id;
	/* Log2 of page size in power-of-two mode */
	uint8_t		l2_page_size;
	uint16_t	pages_per_sector;
	uint16_t	sectors_per_block;
	uint16_t	nr_blocks;
	const char	*name;
};

static const struct gigadevice_spi_flash_params gigadevice_spi_flash_table[] = {
	{
		.id			= 0x4014,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 16,
		.name			= "GD25Q80",
	},
	{
		.id			= 0x4015,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 32,
		.name			= "GD25Q16(B)",
	},
	{
		.id			= 0x4016,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 64,
		.name			= "GD25Q32(B)",
	},
	{
		.id			= 0x6016,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 64,
		.name			= "GD25LQ32",
	},
	{
		.id			= 0x4017,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 128,
		.name			= "GD25Q64B/GD25B64C",
	},
	{
		.id			= 0x6017,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 128,
		.name			= "GD25LQ64C/GD25LB64C",
	},
	{
		.id			= 0x4018,
		.l2_page_size		= 8,
		.pages_per_sector	= 16,
		.sectors_per_block	= 16,
		.nr_blocks		= 256,
		.name			= "GD25Q128(B)",
	},
};

static int gigadevice_write(const struct spi_flash *flash, u32 offset,
			size_t len, const void *buf)
{
	unsigned long byte_addr;
	unsigned long page_size;
	size_t chunk_len;
	size_t actual;
	int ret = 0;
	u8 cmd[4];

	page_size = flash->page_size;

	for (actual = 0; actual < len; actual += chunk_len) {
		byte_addr = offset % page_size;
		chunk_len = min(len - actual, page_size - byte_addr);
		chunk_len = spi_crop_chunk(&flash->spi, sizeof(cmd), chunk_len);

		ret = spi_flash_cmd(&flash->spi, CMD_GD25_WREN, NULL, 0);
		if (ret < 0) {
			printk(BIOS_WARNING,
			       "SF gigadevice.c: Enabling Write failed\n");
			goto out;
		}

		cmd[0] = CMD_GD25_PP;
		cmd[1] = (offset >> 16) & 0xff;
		cmd[2] = (offset >> 8) & 0xff;
		cmd[3] = offset & 0xff;
#if IS_ENABLED(CONFIG_DEBUG_SPI_FLASH)
		printk(BIOS_SPEW,
		       "PP gigadevice.c: 0x%p => cmd = { 0x%02x 0x%02x%02x%02x }"
		       " chunk_len = %zu\n", buf + actual,
		       cmd[0], cmd[1], cmd[2], cmd[3], chunk_len);
#endif

		ret = spi_flash_cmd_write(&flash->spi, cmd, sizeof(cmd),
					  buf + actual, chunk_len);
		if (ret < 0) {
			printk(BIOS_WARNING,
			       "SF gigadevice.c: Page Program failed\n");
			goto out;
		}

		ret = spi_flash_cmd_wait_ready(flash, SPI_FLASH_PROG_TIMEOUT);
		if (ret)
			goto out;

		offset += chunk_len;
	}

#if IS_ENABLED(CONFIG_DEBUG_SPI_FLASH)
	printk(BIOS_SPEW,
	       "SF gigadevice.c: Successfully programmed %zu bytes @ %#x\n",
	       len, (unsigned int)(offset - len));
#endif

	ret = 0;

out:
	return ret;
}

static const struct spi_flash_ops spi_flash_ops = {
	.write = gigadevice_write,
	.erase = spi_flash_cmd_erase,
	.status = spi_flash_cmd_status,
#if IS_ENABLED(CONFIG_SPI_FLASH_NO_FAST_READ)
	.read = spi_flash_cmd_read_slow,
#else
	.read = spi_flash_cmd_read_fast,
#endif
};

int spi_flash_probe_gigadevice(const struct spi_slave *spi, u8 *idcode,
				struct spi_flash *flash)
{
	const struct gigadevice_spi_flash_params *params;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(gigadevice_spi_flash_table); i++) {
		params = &gigadevice_spi_flash_table[i];
		if (params->id == ((idcode[1] << 8) | idcode[2]))
			break;
	}

	if (i == ARRAY_SIZE(gigadevice_spi_flash_table)) {
		printk(BIOS_WARNING,
		       "SF gigadevice.c: Unsupported ID %#02x%02x\n",
		       idcode[1], idcode[2]);
		return -1;
	}

	memcpy(&flash->spi, spi, sizeof(*spi));
	flash->name = params->name;

	/* Assuming power-of-two page size initially. */
	flash->page_size = 1 << params->l2_page_size;
	flash->sector_size = flash->page_size * params->pages_per_sector;
	flash->size = flash->sector_size * params->sectors_per_block *
			params->nr_blocks;
	flash->erase_cmd = CMD_GD25_SE;
	flash->status_cmd = CMD_GD25_RDSR;

	flash->ops = &spi_flash_ops;

	return 0;
}
