/* Copyright (C) 2015 by Yves Godin <support@nuranwireless.com>
 * 
 * Based on sysmoBTS:
 *     sysmobts_misc.c
 *     (C) 2012 by Harald Welte <laforge@gnumonks.org>
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

#include "oc2gbts_mgr.h"
#include "btsconfig.h"
#include "oc2gbts_misc.h"
#include "oc2gbts_par.h"
#include "oc2gbts_temp.h"
#include "oc2gbts_power.h"
#include "oc2gbts_bid.h"

/*********************************************************************
 * Temperature handling
 *********************************************************************/

static const struct {
	const char *name;
	int has_max;
	enum oc2gbts_temp_sensor sensor;
	enum oc2gbts_par ee_par;
} temp_data[] = {
	{
		.name = "supply_temp",
		.has_max = 1,
		.sensor = OC2GBTS_TEMP_SUPPLY,
		.ee_par = OC2GBTS_PAR_TEMP_SUPPLY_MAX,
	}, {
		.name = "soc_temp",
		.has_max = 0,
		.sensor = OC2GBTS_TEMP_SOC,
		.ee_par = OC2GBTS_PAR_TEMP_SOC_MAX,
	}, {
		.name = "fpga_temp",
		.has_max = 0,
		.sensor = OC2GBTS_TEMP_FPGA,
		.ee_par = OC2GBTS_PAR_TEMP_FPGA_MAX,

	}, {
		.name = "rmsdet_temp",
		.has_max = 1,
		.sensor = OC2GBTS_TEMP_RMSDET,
		.ee_par = OC2GBTS_PAR_TEMP_RMSDET_MAX,
	}, {
		.name = "ocxo_temp",
		.has_max = 1,
		.sensor = OC2GBTS_TEMP_OCXO,
		.ee_par = OC2GBTS_PAR_TEMP_OCXO_MAX,
	}, {
		.name = "tx_temp",
		.has_max = 0,
		.sensor = OC2GBTS_TEMP_TX,
		.ee_par = OC2GBTS_PAR_TEMP_TX_MAX,
	}, {
		.name = "pa_temp",
		.has_max = 1,
		.sensor = OC2GBTS_TEMP_PA,
		.ee_par = OC2GBTS_PAR_TEMP_PA_MAX,
	}
};

static const struct {
	const char *name;
	int has_max;
	enum oc2gbts_power_source sensor_source;
	enum oc2gbts_power_type sensor_type;
	enum oc2gbts_par ee_par;
} power_data[] = {
	{
		.name = "supply_volt",
		.has_max = 1,
		.sensor_source = OC2GBTS_POWER_SUPPLY,
		.sensor_type = OC2GBTS_POWER_VOLTAGE,
		.ee_par = OC2GBTS_PAR_VOLT_SUPPLY_MAX,
	}, {
		.name = "supply_pwr",
		.has_max = 1,
		.sensor_source = OC2GBTS_POWER_SUPPLY,
		.sensor_type = OC2GBTS_POWER_POWER,
		.ee_par = OC2GBTS_PAR_PWR_SUPPLY_MAX,
	}, {
		.name = "pa_pwr",
		.has_max = 1,
		.sensor_source = OC2GBTS_POWER_PA,
		.sensor_type = OC2GBTS_POWER_POWER,
		.ee_par = OC2GBTS_PAR_PWR_PA_MAX,
	}
};

static const struct {
	const char *name;
	int has_max;
	enum oc2gbts_vswr_sensor sensor;
	enum oc2gbts_par ee_par;
} vswr_data[] = {
	{
		.name = "vswr",
		.has_max = 0,
		.sensor = OC2GBTS_VSWR,
		.ee_par = OC2GBTS_PAR_VSWR_MAX,
	}
};

static const struct value_string power_unit_strs[] = {
	{ OC2GBTS_POWER_POWER, "W" },
	{ OC2GBTS_POWER_VOLTAGE, "V" },
	{ 0, NULL }
};

void oc2gbts_check_temp(int no_rom_write)
{
	int temp_old[ARRAY_SIZE(temp_data)];
	int temp_cur[ARRAY_SIZE(temp_data)];
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(temp_data); i++) {
		int ret = -99;

		if (temp_data[i].sensor == OC2GBTS_TEMP_PA &&
				!oc2gbts_option_get(OC2GBTS_OPTION_PA_TEMP))
			continue;

		rc = oc2gbts_par_get_int(temp_data[i].ee_par, &ret);
		temp_old[i] = ret * 1000;

		oc2gbts_temp_get(temp_data[i].sensor, &temp_cur[i]);
		if (temp_cur[i] < 0 && temp_cur[i] > -1000) {
			LOGP(DTEMP, LOGL_ERROR, "Error reading temperature (%d)\n", temp_data[i].sensor);
			continue;
		}
	
		LOGP(DTEMP, LOGL_DEBUG, "Current %s temperature: %d.%d C\n",
		     temp_data[i].name, temp_cur[i]/1000, temp_cur[i]%1000);

		if (temp_cur[i] > temp_old[i]) {
			LOGP(DTEMP, LOGL_NOTICE, "New maximum %s "
			     "temperature: %d.%d C\n", temp_data[i].name,
			     temp_cur[i]/1000, temp_old[i]%1000);

			if (!no_rom_write) {
				rc = oc2gbts_par_set_int(temp_data[i].ee_par,
						  temp_cur[i]/1000);
				if (rc < 0)
					LOGP(DTEMP, LOGL_ERROR, "error writing new %s "
					     "max temp %d (%s)\n", temp_data[i].name,
					     rc, strerror(errno));
			}
		}
	}
}

void oc2gbts_check_power(int no_rom_write)
{
	int power_old[ARRAY_SIZE(power_data)];
	int power_cur[ARRAY_SIZE(power_data)];
	int i, rc;
	int div_ratio;

	for (i = 0; i < ARRAY_SIZE(power_data); i++) {
		int ret = 0;

		if (power_data[i].sensor_source == OC2GBTS_POWER_PA &&
				!oc2gbts_option_get(OC2GBTS_OPTION_PA))
			continue;

		rc = oc2gbts_par_get_int(power_data[i].ee_par, &ret);
		switch(power_data[i].sensor_type) {
		case OC2GBTS_POWER_VOLTAGE:
			div_ratio = 1000;
			break;
		case OC2GBTS_POWER_POWER:
			div_ratio = 1000000;
			break;
		default:
			div_ratio = 1000;
		}
		power_old[i] = ret * div_ratio;

		oc2gbts_power_sensor_get(power_data[i].sensor_source, power_data[i].sensor_type, &power_cur[i]);
		if (power_cur[i] < 0 && power_cur[i] > -1000) {
			LOGP(DTEMP, LOGL_ERROR, "Error reading power (%d) (%d)\n", power_data[i].sensor_source, power_data[i].sensor_type);
			continue;
		}
		LOGP(DTEMP, LOGL_DEBUG, "Current %s power: %d.%d %s\n",
				power_data[i].name, power_cur[i]/div_ratio, power_cur[i]%div_ratio,
				get_value_string(power_unit_strs, power_data[i].sensor_type));

		if (power_cur[i] > power_old[i]) {
			LOGP(DTEMP, LOGL_NOTICE, "New maximum %s "
				"power: %d.%d %s\n", power_data[i].name,
				power_cur[i]/div_ratio, power_cur[i]%div_ratio,
				 get_value_string(power_unit_strs, power_data[i].sensor_type));

			if (!no_rom_write) {
				rc = oc2gbts_par_set_int(power_data[i].ee_par,
						  power_cur[i]/div_ratio);
				if (rc < 0)
					LOGP(DTEMP, LOGL_ERROR, "error writing new %s "
						"max power %d (%s)\n", power_data[i].name,
						rc, strerror(errno));
			}
		}
	}
}

void oc2gbts_check_vswr(int no_rom_write)
{
	int vswr_old[ARRAY_SIZE(vswr_data)];
	int vswr_cur[ARRAY_SIZE(vswr_data)];
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(vswr_data); i++) {
		int ret = 0;

		if (vswr_data[i].sensor == OC2GBTS_VSWR &&
				(!oc2gbts_option_get(OC2GBTS_OPTION_RMS_FWD) ||
				!oc2gbts_option_get(OC2GBTS_OPTION_RMS_REFL)))
			continue;

		rc = oc2gbts_par_get_int(vswr_data[i].ee_par, &ret);
		vswr_old[i] = ret * 1000;

		oc2gbts_vswr_get(vswr_data[i].sensor, &vswr_cur[i]);
		if (vswr_cur[i] < 0 && vswr_cur[i] > -1000) {
			LOGP(DTEMP, LOGL_ERROR, "Error reading vswr (%d)\n", vswr_data[i].sensor);
			continue;
		}

		LOGP(DTEMP, LOGL_DEBUG, "Current %s vswr: %d.%d\n",
		     vswr_data[i].name, vswr_cur[i]/1000, vswr_cur[i]%1000);

		if (vswr_cur[i] > vswr_old[i]) {
			LOGP(DTEMP, LOGL_NOTICE, "New maximum %s "
			     "vswr: %d.%d C\n", vswr_data[i].name,
			     vswr_cur[i]/1000, vswr_old[i]%1000);

			if (!no_rom_write) {
				rc = oc2gbts_par_set_int(vswr_data[i].ee_par,
						  vswr_cur[i]/1000);
				if (rc < 0)
					LOGP(DTEMP, LOGL_ERROR, "error writing new %s "
					     "max vswr %d (%s)\n", vswr_data[i].name,
					     rc, strerror(errno));
			}
		}
	}
}

/*********************************************************************
 * Hours handling
 *********************************************************************/
static time_t last_update;

int oc2gbts_update_hours(int no_rom_write)
{
	time_t now = time(NULL);
	int rc, op_hrs = 0;

	/* first time after start of manager program */
	if (last_update == 0) {
		last_update = now;

		rc = oc2gbts_par_get_int(OC2GBTS_PAR_HOURS, &op_hrs);
		if (rc < 0) {
			LOGP(DTEMP, LOGL_ERROR, "Unable to read "
			     "operational hours: %d (%s)\n", rc,
			     strerror(errno));
			/* create a new file anyway */
			if (!no_rom_write)
				rc = oc2gbts_par_set_int(OC2GBTS_PAR_HOURS, op_hrs);

			return rc;
		}

		LOGP(DTEMP, LOGL_INFO, "Total hours of Operation: %u\n",
		     op_hrs);

		return 0;
	}

	if (now >= last_update + 3600) {
		rc = oc2gbts_par_get_int(OC2GBTS_PAR_HOURS, &op_hrs);
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

		if (!no_rom_write) {
			rc = oc2gbts_par_set_int(OC2GBTS_PAR_HOURS, op_hrs);
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

static const char *fw_sysfs[_NUM_FW] = {
	[OC2GBTS_FW_DSP]	= "/sys/kernel/debug/remoteproc/remoteproc0/recovery",
};

int oc2gbts_firmware_reload(enum oc2gbts_firmware_type type)
{
	int fd;
	int rc;

	switch (type) {
	case OC2GBTS_FW_DSP:
		fd = open(fw_sysfs[type], O_WRONLY);
	        if (fd < 0) {
	                LOGP(DFW, LOGL_ERROR, "unable ot open firmware device %s: %s\n",
                    	fw_sysfs[type], strerror(errno));
	                close(fd);
        	        return fd;
		}
		rc = write(fd, "restart", 8); 
		if (rc < 8) {
                        LOGP(DFW, LOGL_ERROR, "short write during "
                             "fw write to %s\n", fw_sysfs[type]);
                        close(fd);
                        return -EIO;
                }
		close(fd);
	default: 
		return -EINVAL;
	}
	return 0;
}
