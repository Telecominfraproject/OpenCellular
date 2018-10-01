/* Misc HW routines for NuRAN Wireless OC-2G BTS */

/* Copyright (C) 2015 by Yves Godin <support@nuranwireless.com>
 * 
 * Based on sysmoBTS:
 * 	(C) 2012 by Harald Welte <laforge@gnumonks.org>
 *
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdint.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <osmocom/core/utils.h>

#include "hw_misc.h"

int oc2gbts_led_set(enum oc2gbts_led_color c)
{
	int fd, rc;
	uint8_t cmd[2];

	switch (c) {	
	case LED_OFF:
		cmd[0] = 0;
		cmd[1] = 0;
		break;
	case LED_RED:
		cmd[0] = 1;
		cmd[1] = 0;
		break;
	case LED_GREEN:
		cmd[0] = 0;
		cmd[1] = 1;
		break;
	case LED_ORANGE:
		cmd[0] = 1;
		cmd[1] = 1;
		break;
	default:
		return -EINVAL;
	}

	fd = open("/var/oc2g/leds/led0/brightness", O_WRONLY);
	if (fd < 0)
		return -ENODEV;

	rc = write(fd, cmd[0] ? "1" : "0", 2);
	if (rc != 2) {
		close(fd);
		return -1;
	}
	close(fd);

	fd = open("/var/oc2g/leds/led1/brightness", O_WRONLY);
	if (fd < 0)
		return -ENODEV;

	rc = write(fd, cmd[1] ? "1" : "0", 2);
	if (rc != 2) {
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}
