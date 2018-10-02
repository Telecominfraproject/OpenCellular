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

#include "lc15bts_power.h"

#define LC15BTS_PA_VOLTAGE      24000000

#define PA_SUPPLY_MIN_SYSFS     "/var/lc15/pa-supply/min_microvolts"
#define PA_SUPPLY_MAX_SYSFS     "/var/lc15/pa-supply/max_microvolts"

static const char *power_enable_devs[_NUM_POWER_SOURCES] = {
	[LC15BTS_POWER_PA0]     = "/var/lc15/pa-state/pa0/state",
	[LC15BTS_POWER_PA1]     = "/var/lc15/pa-state/pa1/state",
};

static const char *power_sensor_devs[_NUM_POWER_SOURCES] = {
	[LC15BTS_POWER_SUPPLY]	= "/var/lc15/pwr-sense/main-supply/",
	[LC15BTS_POWER_PA0]	= "/var/lc15/pwr-sense/pa0/",
	[LC15BTS_POWER_PA1]	= "/var/lc15/pwr-sense/pa1/",
};

static const char *power_sensor_type_str[_NUM_POWER_TYPES] = {
	[LC15BTS_POWER_POWER]	= "power",
	[LC15BTS_POWER_VOLTAGE]	= "voltage",
	[LC15BTS_POWER_CURRENT]	= "current",
};

int lc15bts_power_sensor_get(
        enum lc15bts_power_source source,
        enum lc15bts_power_type type,
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


int lc15bts_power_set(
        enum lc15bts_power_source source,
        int en)
{
	int fd;
	int rc;

	if ((source != LC15BTS_POWER_PA0) 
		&& (source != LC15BTS_POWER_PA1) ) {
		return -EINVAL;
	}
            
	fd = open(PA_SUPPLY_MAX_SYSFS, O_WRONLY);
	if (fd < 0) {
		return fd;
	}
	rc = write(fd, "32000000", 9);
	close( fd );

	if (rc != 9) {
		return -1;
	}

	fd = open(PA_SUPPLY_MIN_SYSFS, O_WRONLY);
	if (fd < 0) {
		return fd;
	}

	/* TODO NTQ: Make the voltage configurable */
	rc = write(fd, "24000000", 9);
	close( fd );

	if (rc != 9) {
		return -1;
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

int lc15bts_power_get(
        enum lc15bts_power_source source)
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
	[LC15BTS_VSWR_TX0]		= "/var/lc15/vswr/tx0/vswr",
	[LC15BTS_VSWR_TX1]		= "/var/lc15/vswr/tx1/vswr",
};

int lc15bts_vswr_get(enum lc15bts_vswr_sensor sensor, int *vswr)
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
