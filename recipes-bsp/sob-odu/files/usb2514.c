/* usb2514 - small utility to set configuration of USB2514 hub chip
 * (C) 2013-2014 by sysmocom - s.f.m.c. GmbH, Author: Harald Welte
 * All Rights Reserved
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
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/stat.h>

/* #include <linux/i2c-dev.h> */
#include "i2c-dev.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

enum compare_op {
	EQUAL,
	NOT_EQUAL,
	LESS_THAN_OR_EQUAL,
	GREATER_THAN_OR_EQUAL,
};

struct usb2514_board {
	const char *name;

	unsigned int i2c_bus;
	uint8_t i2c_addr;

	const char *board_version_file;
	unsigned int board_version;
	enum compare_op board_version_op;

	uint8_t ports_swap;

	const char *reset_gpio_path;
	int reset_low_active;
};

struct board_group {
	/* new /sys/firmware/devicetree/base/model */
	const char *device_tree_name;
	/* old pre-device tree kernels, "Hardware :" in /proc/cpuinfo */
	const char *proc_name;

	const struct usb2514_board *boards;
	unsigned int num_boards;
};

static const struct usb2514_board odu_boards[] = {
	{
		.name = "sob-odu v1",
		.i2c_bus = 0,
		.i2c_addr = 0x2C,
		.board_version_file = "/sys/devices/platform/sob-odu.0/board_version",
		.board_version = 1,
		.board_version_op = EQUAL,
		.ports_swap = 0x00,	/* ports are still swapped in hardware */
		.reset_gpio_path = "/sys/devices/platform/sob-odu.0/gpio_hub_reset/value",
		.reset_low_active = 1,
	}, {
		.name = "sob-odu v2",
		.i2c_bus = 0,
		.i2c_addr = 0x2C,
		.board_version_file = "/sys/devices/platform/sob-odu.0/board_version",
		.board_version = 2,
		.board_version_op = EQUAL,
		.ports_swap = 0x0E,	/* swap DN1, DN2, DN3 */
		.reset_gpio_path = "/sys/devices/platform/sob-odu.0/gpio_hub_reset/value",
		.reset_low_active = 0,
	}, {
		.name = "sob-odu v2",
		.i2c_bus = 0,
		.i2c_addr = 0x2C,
		.board_version_file = "/sys/devices/platform/sob-odu.0/board_version",
		.board_version = 0,	/* EEPROM Empty ?!? */
		.board_version_op = EQUAL,
		.ports_swap = 0x0E,	/* swap DN1, DN2, DN3 */
		.reset_gpio_path = "/sys/devices/platform/sob-odu.0/gpio_hub_reset/value",
		.reset_low_active = 0,
	}, {
		.name = "sob-odu v3+",
		.i2c_bus = 0,
		.i2c_addr = 0x2C,
		.board_version_file = "/sys/devices/platform/sob-odu.0/board_version",
		.board_version = 3,
		.board_version_op = GREATER_THAN_OR_EQUAL,
		.ports_swap = 0x0C,	/* swap only DN2 and DN3 */
		.reset_gpio_path = "/sys/devices/platform/sob-odu.0/gpio_hub_reset/value",
		.reset_low_active = 0,
	},
};

static const struct usb2514_board owhw_boards[] = {
	{
		.name = "OWHW",
		.i2c_bus = 1,
		.i2c_addr = 0x2C,
		.board_version_op = EQUAL,
		.ports_swap = 0x10,	/* swap only DN4 */
		.reset_gpio_path = "/dev/gpio/hub_reset/value",
		.reset_low_active = 0,
	},
};

static const struct board_group boards[] = {
	{
		.proc_name = "sob-odu",
		.device_tree_name = "sysmocom ODU",
		.boards = odu_boards,
		.num_boards = ARRAY_SIZE(odu_boards),
	}, {
		.device_tree_name = "GSMK OWHW",
		.boards = owhw_boards,
		.num_boards = ARRAY_SIZE(owhw_boards),
	},
};

#define RESET_PATH_OLD "/sys/class/gpio/gpio62/value"

/* Default configuration as per data sheet */
static const uint8_t usb2514_default[256] = {
	0x24, 0x04, 0x14, 0x25, 0xB3, 0x0B, 0x9B, 0x20, /* 0x00 */
	0x02, 0x00, 0x00, 0x00, 0x01, 0x32, 0x01, 0x32, /* 0x08 */
	0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x18 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x20 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x28 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x30 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x38 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x40 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x48 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x50 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x58 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x60 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x68 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x70 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x78 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x80 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x88 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x90 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x98 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xa0 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xa8 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xb0 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xb8 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xc0 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xc8 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xd0 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xd8 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xe0 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xe8 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xf0 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, /* 0xf8 */
};

/* Default configuration as per data sheet */
static uint8_t usb2514_odu[256] = {
	0x24, 0x04, 0x14, 0x25, 0xB3, 0x0B, 0x9B, 0x20, /* 0x00 */
	0x02, 0x00, 0x00, 0x00, 0x01, 0x32, 0x01, 0x32, /* 0x08 */
	0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x18 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x20 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x28 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x30 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x38 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x40 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x48 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x50 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x58 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x60 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x68 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x70 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x78 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x80 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x88 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x90 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x98 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xa0 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xa8 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xb0 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xb8 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xc0 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xc8 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xd0 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xd8 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xe0 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xe8 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xf0 */
#if 1	/* swap modem + AIS ports (1,2,3) */
	0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x01, /* 0xf8 */
#else
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, /* 0xf8 */
#endif
};


static int g_fd;

static unsigned long get_support(void)
{
	int rc;
	unsigned long funcs = 0;

	rc = ioctl(g_fd, I2C_FUNCS, funcs);

	printf("supported functions: 0x%lx\n", funcs);

	return funcs;
}

static int write_regs(const uint8_t *regs)
{
	unsigned int i;
	int rc;

	for (i = 0; i < 256; i+= 32) {
		printf("Writing block of %u bytes at index %u\n", 32, i);
		rc = i2c_smbus_write_block_data(g_fd, i, 32, regs+i);
		if (rc < 0)
			fprintf(stderr, "Error writing registers at "
				"offset %u: %d\n", i, rc);
       	}
	return rc;
}

/* attempt to obtain the board version from sysfs */
static int get_board_version(const char *ver_file)
{
	FILE *f;
	unsigned int ver;

	f = fopen(ver_file, "r");
	if (!f)
		return -1;

	if (fscanf(f, "%u", &ver) != 1) {
		fclose(f);
		return -2;
	}

	fclose(f);

	return ver;
}

static int board_ver_matches(const struct usb2514_board *board,
			     unsigned int version)
{
	switch (board->board_version_op) {
	case EQUAL:
		return (version == board->board_version);
	case NOT_EQUAL:
		return (version != board->board_version);
	case LESS_THAN_OR_EQUAL:
		return (version <= board->board_version);
	case GREATER_THAN_OR_EQUAL:
		return (version >= board->board_version);
	default:
		return 0;
	}
}

static char *get_proc_name(void)
{
	FILE *f = fopen("/proc/cpuinfo", "r");
	char linebuf[256];

	while (fgets(linebuf, sizeof(linebuf), f)) {
		/* strip LF at the end of line */
		char *lf = strrchr(linebuf, '\n');
		if (lf)
			*lf = '\0';

		if (strncmp(linebuf, "Hardware", 8) &&
		    strncmp(linebuf, "machine", 7))
			continue;

		/* search for the colon */
		char *colon = strchr(linebuf, ':');
		if (!colon)
			continue;
		colon++;

		/* strip any leading whitespace */
		while (*colon == ' ' || *colon == '\t')
			colon++;

		fclose(f);
		return strdup(colon);
	}

	fclose(f);
	return NULL;
}

static char *get_dt_name(void)
{
	FILE *f;
	char *name = NULL;
	char linebuf[256];

	f = fopen("/sys/firmware/devicetree/base/model", "r");
	if (!f)
		return NULL;

	if (!fgets(linebuf, sizeof(linebuf), f)) {
		fclose(f);
		return NULL;
	}

	fclose(f);

	return strdup(linebuf);
}


static const struct board_group *find_matching_board_group()
{
	int i;
	char *proc_name, *dt_name;

	proc_name = get_proc_name();
	dt_name = get_dt_name();

	for (i = 0; i < ARRAY_SIZE(boards); i++) {
		const struct board_group *bgrp = &boards[i];

		if (dt_name && bgrp->device_tree_name &&
		    !strcmp(dt_name, bgrp->device_tree_name)) {
			free(proc_name);
			free(dt_name);
			return bgrp;
		}

		if (proc_name && bgrp->proc_name &&
		    !strcmp(proc_name, bgrp->proc_name)) {
			free(proc_name);
			free(dt_name);
			return bgrp;
		}
	}

	free(proc_name);
	free(dt_name);

	return NULL;
}


static const struct usb2514_board *
find_matching_board(const struct board_group *bgrp)
{
	int i;

	for (i = 0; i < bgrp->num_boards; i++) {
		const struct usb2514_board *board = &bgrp->boards[i];
		int ver;

		if (board->board_version_file) {
			/* get board version and compare */
			ver = get_board_version(board->board_version_file);
			if (ver < 0)
				continue;
			if (!board_ver_matches(board, ver))
				continue;
		}

		return board;
	}

	return NULL;
}

/* attempt to reset the hub via sysfs */
static int reset_hub(const char *reset_path, int invert_logic)
{
	FILE *f;

	f = fopen(reset_path, "w");
	if (!f)
		return -1;

	if (invert_logic)
		fputs("0", f);
	else
		fputs("1", f);

	usleep(10000);
	rewind(f);

	if (invert_logic)
		fputs("1", f);
	else
		fputs("0", f);

	fclose(f);
	return 0;
}

int main(int argc, char **argv)
{
	int rc;
	char filename[PATH_MAX];
	const struct board_group *bgrp;
	const struct usb2514_board *board;

	bgrp = find_matching_board_group();
	if (!bgrp) {
		fprintf(stderr, "Cannot find matching board group for this system\n");
		exit(1);
	}
	printf("Found matching board group %s(%s)\n", bgrp->proc_name, bgrp->device_tree_name);

	board = find_matching_board(bgrp);
	if (!board) {
		fprintf(stderr, "Cannot find matching config for this system\n");
		exit(1);
	}
	printf("Found matching board %s\n", board->name);

	/* open the I2C bus device */

	snprintf(filename, sizeof(filename)-1, "/dev/i2c-%d", board->i2c_bus);
	rc = open(filename, O_RDWR);
	if (rc < 0) {
		fprintf(stderr, "Error opening the device %s: %d\n", filename, rc);
		exit(1);
	}
	g_fd = rc;
	get_support();

	/* set the slave address */

	rc = ioctl(g_fd, I2C_SLAVE, board->i2c_addr);
	if (rc < 0) {
		fprintf(stderr, "Error setting slave addr: %d\n", rc);
		exit(1);
	}

	if (board->reset_gpio_path) {
		/* First reset the USB hub before loading data into it */
		if (reset_hub(board->reset_gpio_path, board->reset_low_active) < 0) {
			fprintf(stderr, "Couldn't reset the USB hub!\n");
		}
	} else
		fprintf(stderr, "board config doesn't indicate USB hub reset GPIO\n");

	/* patch the port inversion byte into the array */
	usb2514_odu[0xFA] = board->ports_swap;

	rc = write_regs(usb2514_odu);
	if (rc < 0) {
		fprintf(stderr, "Error writing default regs: %d\n", rc);
		exit(1);
	}

	exit(0);
}
