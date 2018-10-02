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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/utils.h>
#include <osmocom/core/msgb.h>
#include <osmocom/core/application.h>
#include <osmocom/vty/telnet_interface.h>
#include <osmocom/vty/logging.h>

#include "btsconfig.h"
#include "sysmobts_misc.h"
#include "sysmobts_par.h"
#include "sysmobts_mgr.h"

/*********************************************************************
 * Temperature handling
 *********************************************************************/

#define TEMP_PATH	"/sys/class/hwmon/hwmon0/device/temp%u_%s"

static const char *temp_type_str[_NUM_TEMP_TYPES] = {
	[SYSMOBTS_TEMP_INPUT] = "input",
	[SYSMOBTS_TEMP_LOWEST] = "lowest",
	[SYSMOBTS_TEMP_HIGHEST] = "highest",
};

int sysmobts_temp_get(enum sysmobts_temp_sensor sensor,
		      enum sysmobts_temp_type type)
{
	char buf[PATH_MAX];
	char tempstr[8];
	int fd, rc;

	if (sensor < SYSMOBTS_TEMP_DIGITAL ||
	    sensor > SYSMOBTS_TEMP_RF)
		return -EINVAL;

	if (type >= ARRAY_SIZE(temp_type_str))
		return -EINVAL;

	snprintf(buf, sizeof(buf)-1, TEMP_PATH, sensor, temp_type_str[type]);
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

	return atoi(tempstr);
}

static const struct {
	const char *name;
	enum sysmobts_temp_sensor sensor;
	enum sysmobts_par ee_par;
} temp_data[] = {
	{
		.name = "digital",
		.sensor = SYSMOBTS_TEMP_DIGITAL,
		.ee_par = SYSMOBTS_PAR_TEMP_DIG_MAX,
	}, {
		.name = "rf",
		.sensor = SYSMOBTS_TEMP_RF,
		.ee_par = SYSMOBTS_PAR_TEMP_RF_MAX,
	}
};

void sysmobts_check_temp(int no_eeprom_write)
{
	int temp_old[ARRAY_SIZE(temp_data)];
	int temp_hi[ARRAY_SIZE(temp_data)];
	int temp_cur[ARRAY_SIZE(temp_data)];
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(temp_data); i++) {
		int ret;
		rc = sysmobts_par_get_int(temp_data[i].ee_par, &ret);
		temp_old[i] = ret * 1000;
		temp_hi[i] = sysmobts_temp_get(temp_data[i].sensor,
						SYSMOBTS_TEMP_HIGHEST);
		temp_cur[i] = sysmobts_temp_get(temp_data[i].sensor,
						SYSMOBTS_TEMP_INPUT);

		if ((temp_cur[i] < 0 && temp_cur[i] > -1000) ||
		    (temp_hi[i] < 0 && temp_hi[i] > -1000)) {
			LOGP(DTEMP, LOGL_ERROR, "Error reading temperature\n");
			return;
		}

		LOGP(DTEMP, LOGL_DEBUG, "Current %s temperature: %d.%d C\n",
		     temp_data[i].name, temp_cur[i]/1000, temp_cur[i]%1000);

		if (temp_hi[i] > temp_old[i]) {
			LOGP(DTEMP, LOGL_NOTICE, "New maximum %s "
			     "temperature: %d.%d C\n", temp_data[i].name,
			     temp_hi[i]/1000, temp_hi[i]%1000);

			if (!no_eeprom_write) {
				rc = sysmobts_par_set_int(SYSMOBTS_PAR_TEMP_DIG_MAX,
						  temp_hi[0]/1000);
				if (rc < 0)
					LOGP(DTEMP, LOGL_ERROR, "error writing new %s "
					     "max temp %d (%s)\n", temp_data[i].name,
					     rc, strerror(errno));
			}
		}
	}
}

/*********************************************************************
 * Hours handling
 *********************************************************************/
static time_t last_update;

int sysmobts_update_hours(int no_eeprom_write)
{
	time_t now = time(NULL);
	int rc, op_hrs;

	/* first time after start of manager program */
	if (last_update == 0) {
		last_update = now;

		rc = sysmobts_par_get_int(SYSMOBTS_PAR_HOURS, &op_hrs);
		if (rc < 0) {
			LOGP(DTEMP, LOGL_ERROR, "Unable to read "
			     "operational hours: %d (%s)\n", rc,
			     strerror(errno));
			return rc;
		}

		LOGP(DTEMP, LOGL_INFO, "Total hours of Operation: %u\n",
		     op_hrs);

		return 0;
	}

	if (now >= last_update + 3600) {
		rc = sysmobts_par_get_int(SYSMOBTS_PAR_HOURS, &op_hrs);
		if (rc < 0) {
			LOGP(DTEMP, LOGL_ERROR, "Unable to read "
			     "operational hours: %d (%s)\n", rc,
			     strerror(errno));
			return rc;
		}

		/* number of hours to increase */
		op_hrs += (now-last_update)/3600;

		LOGP(DTEMP, LOGL_INFO, "Total hours of Operation: %u\n",
		     op_hrs);

		if (!no_eeprom_write) {
			rc = sysmobts_par_set_int(SYSMOBTS_PAR_HOURS, op_hrs);
			if (rc < 0)
				return rc;
		}

		last_update = now;
	}

	return 0;
}

/*********************************************************************
 * Firmware reloading
 *********************************************************************/

#define SYSMOBTS_FW_PATH	"/lib/firmware"

static const char *fw_names[_NUM_FW] = {
	[SYSMOBTS_FW_FPGA]	= "sysmobts-v2.bit",
	[SYSMOBTS_FW_DSP]	= "sysmobts-v2.out",
};
static const char *fw_devs[_NUM_FW] = {
	[SYSMOBTS_FW_FPGA]	= "/dev/fpgadl_par0",
	[SYSMOBTS_FW_DSP]	= "/dev/dspdl_dm644x_0",
};

int sysmobts_firmware_reload(enum sysmobts_firmware_type type)
{
	char name[PATH_MAX];
	uint8_t buf[1024];
	int fd_in, fd_out, rc;

	if (type >= _NUM_FW)
		return -EINVAL;

	snprintf(name, sizeof(name)-1, "%s/%s",
		 SYSMOBTS_FW_PATH, fw_names[type]);
	name[sizeof(name)-1] = '\0';

	fd_in = open(name, O_RDONLY);
	if (fd_in < 0) {
		LOGP(DFW, LOGL_ERROR, "unable ot open firmware file %s: %s\n",
		     name, strerror(errno));
		return fd_in;
	}

	fd_out = open(fw_devs[type], O_WRONLY);
	if (fd_out < 0) {
		LOGP(DFW, LOGL_ERROR, "unable ot open firmware device %s: %s\n",
		     fw_devs[type], strerror(errno));
		close(fd_in);
		return fd_out;
	}

	while ((rc = read(fd_in, buf, sizeof(buf)))) {
		int written;

		if (rc < 0) {
			LOGP(DFW, LOGL_ERROR, "error %d during read "
			     "from %s: %s\n", rc, name, strerror(errno));
			close(fd_in);
			close(fd_out);
			return -EIO;
		}

		written	= write(fd_out, buf, rc);
		if (written < rc) {
			LOGP(DFW, LOGL_ERROR, "short write during "
			     "fw write to %s\n", fw_devs[type]);
			close(fd_in);
			close(fd_out);
			return -EIO;
		}
	}

	close(fd_in);
	close(fd_out);

	return 0;
}
