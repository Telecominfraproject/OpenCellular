/* Misc HW routines for Sysmocom BTS */

/* (C) 2012 by Harald Welte <laforge@gnumonks.org>
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

static const struct value_string sysmobts_led_names[] = {
	{ LED_RF_ACTIVE,	"activity_led" },
	{ LED_ONLINE,		"online_led" },
	{ 0, NULL }
};

int sysmobts_led_set(enum sysmobts_led nr, int on)
{
	char tmp[PATH_MAX+1];
	const char *filename;
	int fd;
	uint8_t byte;

	if (on)
		byte = '1';
	else
		byte = '0';

	filename = get_value_string(sysmobts_led_names, nr);
	if (!filename)
		return -EINVAL;

	snprintf(tmp, sizeof(tmp)-1, "/sys/class/leds/%s/brightness", filename);
	tmp[sizeof(tmp)-1] = '\0';

	fd = open(tmp, O_WRONLY);
	if (fd < 0)
		return -ENODEV;

	write(fd, &byte, 1);

	close(fd);

	return 0;
}

#if 0
#define HWMON_PREFIX "/sys/class/hwmon/hwmon0/device"

static FILE *temperature_f[NUM_TEMP];

int sysmobts_temp_init()
{
	char tmp[PATH_MAX+1];
	FILE *in;
	int rc = 0;

	for (i = 0; i < NUM_TEMP; i++) {
		snprintf(tmp, sizeof(tmp)-1, HWMON_PREFIX "/temp%u_input", i+1),
		tmp[sizeof(tmp)-1] = '\0';

		temperature_f[i] = fopen(tmp, "r");
		if (!temperature_f[i])
			rc = -ENODEV;
	}

	return 0;
}

int sysmobts_temp_get(uint8_t num)
{
	if (num >= NUM_TEMP)
		return -EINVAL;

	if (!temperature_f[num])
		return -ENODEV;


	in = fopen(tmp, "r");
	if (!in)
		return -ENODEV;

	fclose(tmp);

	return 0;
}
#endif
