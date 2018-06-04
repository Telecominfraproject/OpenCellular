/*
 * This file is part of the superiotool project.
 *
 * Copyright (C) 2016 Derek Waldner
 * Copyright (C) 2016 Sencore Inc <opensource@sencore.com>
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

#include "superiotool.h"

#define DEVICE_ID_BYTE1_REG	0x20
#define DEVICE_ID_BYTE2_REG	0x21

#define VENDOR_ID_BYTE1_REG	0x23
#define VENDOR_ID_BYTE2_REG	0x24

#define EXAR_VENDOR_ID		0xa813

static const struct superio_registers reg_table[] = {
	{0x8403, "XR28V384", {
		/* We assume reserved bits are read as 0. */
		{NOLDN, NULL,
			{0x02,0x07,0x20,0x21,0x23,0x24,0x25,0x26,0x27,EOT},
			{0x00,0x00,0x03,0x84,0x13,0xa8,0x00,0x00,0x00,EOT}},
		{0x0, "COM1",
			{0x30,0x60,0x61,0x70,0xf0,0xf1,0xf4,0xf5,0xf6,EOT},
			{0x01,0x03,0xf8,0x03,0x00,0x44,0x00,0x00,0x00,EOT}},
		{0x1, "COM2",
			{0x30,0x60,0x61,0x70,0xf0,0xf4,0xf5,0xf6,EOT},
			{0x01,0x02,0xf8,0x04,0x00,0x00,0x00,0x00,EOT}},
		{0x2, "COM3",
			{0x30,0x60,0x61,0x70,0xf0,0xf4,0xf5,0xf6,EOT},
			{0x01,0x03,0xe8,0x05,0x00,0x00,0x00,0x00,EOT}},
		{0x3, "COM4",
			{0x30,0x60,0x61,0x70,0xf0,0xf4,0xf5,0xf6,EOT},
			{0x01,0x02,0xe8,0x09,0x00,0x00,0x00,0x00,EOT}},
		{0x8, "WDT",
			{0x30,0x60,0x61,0x70,0xf0,0xf1,EOT},
			{0x01,0x04,0x42,0x00,0x02,0x0a,EOT}},
		{EOT}}},
	{EOT}
};

void enter_conf_mode_exar(uint16_t port)
{
	OUTB(0x67, port);
	OUTB(0x67, port);
}

void exit_conf_mode_exar(uint16_t port)
{
	OUTB(0xaa, port);
}

void probe_idregs_exar(uint16_t port)
{
	uint16_t vid, did;

	probing_for("Exar", "", port);

	enter_conf_mode_exar(port);

	did = regval(port, DEVICE_ID_BYTE1_REG);
	did |= (regval(port, DEVICE_ID_BYTE2_REG) << 8);

	vid = regval(port, VENDOR_ID_BYTE1_REG);
	vid |= (regval(port, VENDOR_ID_BYTE2_REG) << 8);

	if (vid != EXAR_VENDOR_ID || superio_unknown(reg_table, did)) {
		if (verbose)
			printf(NOTFOUND "vid=0x%04x, id=0x%04x\n", vid, did);
		exit_conf_mode_exar(port);
		return;
	}

	printf("Found Exar %s (vid=0x%04x, id=0x%04x) at 0x%x\n",
		get_superio_name(reg_table, did), vid, did, port);
	chip_found = 1;

	dump_superio("Exar", reg_table, port, did, LDN_SEL);

	exit_conf_mode_exar(port);
}

void print_exar_chips(void)
{
	print_vendor_chips("Exar", reg_table);
}
