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

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>

#include "oc2gbts_power.h"

static const char *power_enable_devs[_NUM_POWER_SOURCES] = {
	[OC2GBTS_POWER_PA]     = "/var/oc2g/pa-state/pa0/state",
};

static const char *power_sensor_devs[_NUM_POWER_SOURCES] = {
	[OC2GBTS_POWER_SUPPLY]	= "/var/oc2g/pwr-sense/main-supply/",
	[OC2GBTS_POWER_PA]		= "/var/oc2g/pwr-sense/pa0/",
};

static const char *power_sensor_type_str[_NUM_POWER_TYPES] = {
	[OC2GBTS_POWER_POWER]	= "power",
	[OC2GBTS_POWER_VOLTAGE]	= "voltage",
	[OC2GBTS_POWER_CURRENT]	= "current",
};

int oc2gbts_power_sensor_get(
	enum oc2gbts_power_source source,
	enum oc2gbts_power_type type,
	int *power)
{
	char buf[PATH_MAX];
	char pwrstr[10];
	int fd, rc;

	if (source >= _NUM_POWER_SOURCES)
		return -EINVAL;

	if (type >= _NUM_POWER_TYPES)
		return -EINVAL;

	snprintf(buf, sizeof(buf)-1, "%s%s", power_sensor_devs[source], power_sensor_type_str[type]);
	buf[sizeof(buf)-1] = '\0';

	fd = open(buf, O_RDONLY);
	if (fd < 0)
		return fd;

	rc = read(fd, pwrstr, sizeof(pwrstr));
	pwrstr[sizeof(pwrstr)-1] = '\0';
	if (rc < 0) {
		close(fd);
		return rc;
	}
	if (rc == 0) {
		close(fd);
		return -EIO;
	}
	close(fd);
	*power = atoi(pwrstr);
	return 0;
}


int oc2gbts_power_set(
        enum oc2gbts_power_source source,
        int en)
{
	int fd;
	int rc;

	if (source != OC2GBTS_POWER_PA) {
		return -EINVAL;
	}

	fd = open(power_enable_devs[source], O_WRONLY);
	if (fd < 0) {
		return fd;
	}
	rc = write(fd, en?"1":"0", 2);
	close( fd );
	
	if (rc != 2) {
		return -1;
	}
	
	if (en) usleep(50*1000);

	return 0;
}

int oc2gbts_power_get(
        enum oc2gbts_power_source source)
{
	int fd;
	int rc;
	int retVal = 0;
	char enstr[10];

	fd = open(power_enable_devs[source], O_RDONLY);
	if (fd < 0) {
		return fd;
	}

	rc = read(fd, enstr, sizeof(enstr));
        enstr[rc-1] = '\0';
        
	close(fd);

        if (rc < 0) {
                return rc;
        }
        if (rc == 0) {
                return -EIO;
        }

	rc = strcmp(enstr, "enabled");
	if(rc == 0) {
		retVal = 1;
	}
	
        return retVal;
}

static const char *vswr_devs[_NUM_VSWR_SENSORS] = {
	[OC2GBTS_VSWR]		= "/var/oc2g/vswr/tx0/vswr",
};

int oc2gbts_vswr_get(enum oc2gbts_vswr_sensor sensor, int *vswr)
{
	char buf[PATH_MAX];
	char vswrstr[8];
	int fd, rc;

	if (sensor < 0 || sensor >= _NUM_VSWR_SENSORS)
		return -EINVAL;

	snprintf(buf, sizeof(buf)-1, "%s", vswr_devs[sensor]);
	buf[sizeof(buf)-1] = '\0';

	fd = open(buf, O_RDONLY);
	if (fd < 0)
		return fd;

	rc = read(fd, vswrstr, sizeof(vswrstr));
	vswrstr[sizeof(vswrstr)-1] = '\0';
	if (rc < 0) {
		close(fd);
		return rc;
	}
	if (rc == 0) {
		close(fd);
		return -EIO;
	}
	close(fd);
	*vswr = atoi(vswrstr);
	return 0;
}
