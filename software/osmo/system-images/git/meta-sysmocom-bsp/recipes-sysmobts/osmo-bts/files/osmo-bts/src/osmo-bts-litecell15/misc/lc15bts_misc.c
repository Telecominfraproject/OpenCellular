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

#include "lc15bts_mgr.h"
#include "btsconfig.h"
#include "lc15bts_misc.h"
#include "lc15bts_par.h"
#include "lc15bts_mgr.h"
#include "lc15bts_temp.h"
#include "lc15bts_power.h"

/*********************************************************************
 * Temperature handling
 *********************************************************************/

static const struct {
	const char *name;
	int has_max;
	enum lc15bts_temp_sensor sensor;
	enum lc15bts_par ee_par;
} temp_data[] = {
	{
		.name = "supply_temp",
		.has_max = 1,
		.sensor = LC15BTS_TEMP_SUPPLY,
		.ee_par = LC15BTS_PAR_TEMP_SUPPLY_MAX,
	}, {
		.name = "soc_temp",
		.has_max = 0,
		.sensor = LC15BTS_TEMP_SOC,
		.ee_par = LC15BTS_PAR_TEMP_SOC_MAX,
	}, {
		.name = "fpga_temp",
		.has_max = 0,
		.sensor = LC15BTS_TEMP_FPGA,
		.ee_par = LC15BTS_PAR_TEMP_FPGA_MAX,

	}, {
		.name = "rmsdet_temp",
		.has_max = 1,
		.sensor = LC15BTS_TEMP_RMSDET,
		.ee_par = LC15BTS_PAR_TEMP_RMSDET_MAX,
	}, {
		.name = "ocxo_temp",
		.has_max = 1,
		.sensor = LC15BTS_TEMP_OCXO,
		.ee_par = LC15BTS_PAR_TEMP_OCXO_MAX,
	}, {
		.name = "tx0_temp",
		.has_max = 0,
		.sensor = LC15BTS_TEMP_TX0,
		.ee_par = LC15BTS_PAR_TEMP_TX0_MAX,
	}, {
		.name = "tx1_temp",
		.has_max = 0,
		.sensor = LC15BTS_TEMP_TX1,
		.ee_par = LC15BTS_PAR_TEMP_TX1_MAX,
	}, {
		.name = "pa0_temp",
		.has_max = 1,
		.sensor = LC15BTS_TEMP_PA0,
		.ee_par = LC15BTS_PAR_TEMP_PA0_MAX,
	}, {
		.name = "pa1_temp",
		.has_max = 1,
		.sensor = LC15BTS_TEMP_PA1,
		.ee_par = LC15BTS_PAR_TEMP_PA1_MAX,
	}
};

static const struct {
	const char *name;
	int has_max;
	enum lc15bts_power_source sensor_source;
	enum lc15bts_power_type sensor_type;
	enum lc15bts_par ee_par;
} power_data[] = {
	{
		.name = "supply_volt",
		.has_max = 1,
		.sensor_source = LC15BTS_POWER_SUPPLY,
		.sensor_type = LC15BTS_POWER_VOLTAGE,
		.ee_par = LC15BTS_PAR_VOLT_SUPPLY_MAX,
	}, {
		.name = "supply_pwr",
		.has_max = 1,
		.sensor_source = LC15BTS_POWER_SUPPLY,
		.sensor_type = LC15BTS_POWER_POWER,
		.ee_par = LC15BTS_PAR_PWR_SUPPLY_MAX,
	}, {
		.name = "pa0_pwr",
		.has_max = 1,
		.sensor_source = LC15BTS_POWER_PA0,
		.sensor_type = LC15BTS_POWER_POWER,
		.ee_par = LC15BTS_PAR_PWR_PA0_MAX,
	}, {
		.name = "pa1_pwr",
		.has_max = 1,
		.sensor_source = LC15BTS_POWER_PA1,
		.sensor_type = LC15BTS_POWER_POWER,
		.ee_par = LC15BTS_PAR_PWR_PA1_MAX,
	}
};

static const struct {
	const char *name;
	int has_max;
	enum lc15bts_vswr_sensor sensor;
	enum lc15bts_par ee_par;
} vswr_data[] = {
	{
		.name = "tx0_vswr",
		.has_max = 0,
		.sensor = LC15BTS_VSWR_TX0,
		.ee_par = LC15BTS_PAR_VSWR_TX0_MAX,
	}, {
		.name = "tx1_vswr",
		.has_max = 0,
		.sensor = LC15BTS_VSWR_TX1,
		.ee_par = LC15BTS_PAR_VSWR_TX1_MAX,
	}
};

static const struct value_string power_unit_strs[] = {
	{ LC15BTS_POWER_POWER, "W" },
	{ LC15BTS_POWER_VOLTAGE, "V" },
	{ 0, NULL }
};

void lc15bts_check_temp(int no_rom_write)
{
	int temp_old[ARRAY_SIZE(temp_data)];
	int temp_cur[ARRAY_SIZE(temp_data)];
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(temp_data); i++) {
		int ret;
		rc = lc15bts_par_get_int(tall_mgr_ctx, temp_data[i].ee_par, &ret);
		temp_old[i] = ret * 1000;

		lc15bts_temp_get(temp_data[i].sensor, &temp_cur[i]);
		if (temp_cur[i] < 0 && temp_cur[i] > -1000) {
			LOGP(DTEMP, LOGL_ERROR, "Error reading temperature (%d): unexpected value %d\n",
			     temp_data[i].sensor, temp_cur[i]);
			continue;
		}
	
		LOGP(DTEMP, LOGL_DEBUG, "Current %s temperature: %d.%d C\n",
		     temp_data[i].name, temp_cur[i]/1000, temp_cur[i]%1000);

		if (temp_cur[i] > temp_old[i]) {
			LOGP(DTEMP, LOGL_NOTICE, "New maximum %s "
			     "temperature: %d.%d C\n", temp_data[i].name,
			     temp_cur[i]/1000, temp_old[i]%1000);

			if (!no_rom_write) {
				rc = lc15bts_par_set_int(tall_mgr_ctx, temp_data[i].ee_par, temp_cur[i]/1000);
				if (rc < 0)
					LOGP(DTEMP, LOGL_ERROR, "error writing new %s "
					     "max temp %d (%s)\n", temp_data[i].name,
					     rc, strerror(errno));
			}
		}
	}
}

void lc15bts_check_power(int no_rom_write)
{
	int power_old[ARRAY_SIZE(power_data)];
	int power_cur[ARRAY_SIZE(power_data)];
	int i, rc;
	int div_ratio;

	for (i = 0; i < ARRAY_SIZE(power_data); i++) {
		int ret;
		rc = lc15bts_par_get_int(tall_mgr_ctx, power_data[i].ee_par, &ret);
		switch(power_data[i].sensor_type) {
		case LC15BTS_POWER_VOLTAGE:
			div_ratio = 1000;
			break;
		case LC15BTS_POWER_POWER:
			div_ratio = 1000000;
			break;
		default:
			div_ratio = 1000;
		}
		power_old[i] = ret * div_ratio;

		lc15bts_power_sensor_get(power_data[i].sensor_source, power_data[i].sensor_type, &power_cur[i]);
		if (power_cur[i] < 0 && power_cur[i] > -1000) {
			LOGP(DTEMP, LOGL_ERROR, "Error reading power (%d) (%d)\n", power_data[i].sensor_source,
			     power_data[i].sensor_type);
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
				rc = lc15bts_par_set_int(tall_mgr_ctx, power_data[i].ee_par, power_cur[i]/div_ratio);
				if (rc < 0)
					LOGP(DTEMP, LOGL_ERROR, "error writing new %s "
						"max power %d (%s)\n", power_data[i].name,
						rc, strerror(errno));
			}
		}
	}
}

void lc15bts_check_vswr(int no_rom_write)
{
	int vswr_old[ARRAY_SIZE(vswr_data)];
	int vswr_cur[ARRAY_SIZE(vswr_data)];
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(vswr_data); i++) {
		int ret;
		rc = lc15bts_par_get_int(tall_mgr_ctx, vswr_data[i].ee_par, &ret);
		vswr_old[i] = ret * 1000;

		lc15bts_vswr_get(vswr_data[i].sensor, &vswr_cur[i]);
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
				rc = lc15bts_par_set_int(tall_mgr_ctx, vswr_data[i].ee_par, vswr_cur[i]/1000);
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

int lc15bts_update_hours(int no_rom_write)
{
	time_t now = time(NULL);
	int rc, op_hrs;

	/* first time after start of manager program */
	if (last_update == 0) {
		last_update = now;

		rc = lc15bts_par_get_int(tall_mgr_ctx, LC15BTS_PAR_HOURS, &op_hrs);
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
		rc = lc15bts_par_get_int(tall_mgr_ctx, LC15BTS_PAR_HOURS, &op_hrs);
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
			rc = lc15bts_par_set_int(tall_mgr_ctx, LC15BTS_PAR_HOURS, op_hrs);
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
	[LC15BTS_FW_DSP0]	= "/sys/kernel/debug/remoteproc/remoteproc0/recovery",
	[LC15BTS_FW_DSP1]	= "/sys/kernel/debug/remoteproc/remoteproc0/recovery",
};

int lc15bts_firmware_reload(enum lc15bts_firmware_type type)
{
	int fd;
	int rc;

	switch (type) {
	case LC15BTS_FW_DSP0:
	case LC15BTS_FW_DSP1: 
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
