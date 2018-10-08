/* Copyright (C) 2015 by Yves Godin <support@nuranwireless.com>
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>

#include <osmocom/core/utils.h>

#include "oc2gbts_temp.h"

static const char *temp_devs[_NUM_TEMP_SENSORS] = {
	[OC2GBTS_TEMP_SUPPLY]	 	= "/var/oc2g/temp/main-supply/temp",
	[OC2GBTS_TEMP_SOC]	 		= "/var/oc2g/temp/cpu/temp",
	[OC2GBTS_TEMP_FPGA]	 		= "/var/oc2g/temp/fpga/temp",
	[OC2GBTS_TEMP_RMSDET] 		= "/var/oc2g/temp/rmsdet/temp",
	[OC2GBTS_TEMP_OCXO]			= "/var/oc2g/temp/ocxo/temp",
	[OC2GBTS_TEMP_TX]			= "/var/oc2g/temp/tx0/temp",
	[OC2GBTS_TEMP_PA]			= "/var/oc2g/temp/pa0/temp",
};

int oc2gbts_temp_get(enum oc2gbts_temp_sensor sensor, int *temp)
{
	char buf[PATH_MAX];
	char tempstr[8];
	int fd, rc;

	if (sensor < 0 || sensor >= _NUM_TEMP_SENSORS)
		return -EINVAL;

	snprintf(buf, sizeof(buf)-1, "%s", temp_devs[sensor]);
	buf[sizeof(buf)-1] = '\0';

	fd = open(buf, O_RDONLY);
	if (fd < 0)
		return fd;

	rc = read(fd, tempstr, sizeof(tempstr));
	tempstr[sizeof(tempstr)-1] = '\0';
	if (rc < 0) {
		close(fd);
		return rc;
	}
	if (rc == 0) {
		close(fd);
		return -EIO;
	}
	close(fd);
	*temp = atoi(tempstr);
	return 0;
}
