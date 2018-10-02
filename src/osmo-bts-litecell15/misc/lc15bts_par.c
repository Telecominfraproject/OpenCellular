/* lc15bts - access to hardware related parameters */

/* Copyright (C) 2015 by Yves Godin <support@nuranwireless.com>
 * 
 * Based on sysmoBTS:
 *     sysmobts_par.c
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <osmocom/core/utils.h>
#include <osmocom/core/talloc.h>

#include "lc15bts_par.h"

const struct value_string lc15bts_par_names[_NUM_LC15BTS_PAR+1] = {
	{ LC15BTS_PAR_TEMP_SUPPLY_MAX,	"temp-supply-max" },
	{ LC15BTS_PAR_TEMP_SOC_MAX,	"temp-soc-max" },
	{ LC15BTS_PAR_TEMP_FPGA_MAX,	"temp-fpga-max" },
	{ LC15BTS_PAR_TEMP_RMSDET_MAX,	"temp-rmsdet-max" },
	{ LC15BTS_PAR_TEMP_OCXO_MAX,	"temp-ocxo-max" },
	{ LC15BTS_PAR_TEMP_TX0_MAX,	"temp-tx0-max" },
	{ LC15BTS_PAR_TEMP_TX1_MAX,	"temp-tx1-max" },
	{ LC15BTS_PAR_TEMP_PA0_MAX,	"temp-pa0-max" },
	{ LC15BTS_PAR_TEMP_PA1_MAX,	"temp-pa1-max" },
	{ LC15BTS_PAR_VOLT_SUPPLY_MAX,	"volt-supply-max" },
	{ LC15BTS_PAR_PWR_SUPPLY_MAX,	"pwr-supply-max" },
	{ LC15BTS_PAR_PWR_PA0_MAX,	"pwr-pa0-max" },
	{ LC15BTS_PAR_PWR_PA1_MAX,	"pwr-pa1-max" },
	{ LC15BTS_PAR_VSWR_TX0_MAX,	"vswr-tx0-max" },
	{ LC15BTS_PAR_VSWR_TX1_MAX,	"vswr-tx1-max" },
	{ LC15BTS_PAR_GPS_FIX, 		"gps-fix" },
	{ LC15BTS_PAR_SERNR,		"serial-nr" },
	{ LC15BTS_PAR_HOURS, 		"hours-running" },
	{ LC15BTS_PAR_BOOTS, 		"boot-count" },
	{ LC15BTS_PAR_KEY, 		"key" },
	{ 0, NULL }
};

int lc15bts_par_is_int(enum lc15bts_par par)
{
	switch (par) {
	case LC15BTS_PAR_TEMP_SUPPLY_MAX:
	case LC15BTS_PAR_TEMP_SOC_MAX:
	case LC15BTS_PAR_TEMP_FPGA_MAX:
	case LC15BTS_PAR_TEMP_RMSDET_MAX:
	case LC15BTS_PAR_TEMP_OCXO_MAX:
	case LC15BTS_PAR_TEMP_TX0_MAX:
	case LC15BTS_PAR_TEMP_TX1_MAX:
	case LC15BTS_PAR_TEMP_PA0_MAX:
	case LC15BTS_PAR_TEMP_PA1_MAX:
	case LC15BTS_PAR_VOLT_SUPPLY_MAX:
	case LC15BTS_PAR_VSWR_TX0_MAX:
	case LC15BTS_PAR_VSWR_TX1_MAX:
	case LC15BTS_PAR_SERNR:
	case LC15BTS_PAR_HOURS:
	case LC15BTS_PAR_BOOTS:
	case LC15BTS_PAR_PWR_SUPPLY_MAX:
	case LC15BTS_PAR_PWR_PA0_MAX:
	case LC15BTS_PAR_PWR_PA1_MAX:
		return 1;
	default:
		return 0;
	}
}

FILE *lc15bts_par_get_path(void *ctx, enum lc15bts_par par, const char* mode)
{
	char *fpath;
	FILE *fp;

	if (par >= _NUM_LC15BTS_PAR)
		return NULL;

	fpath = talloc_asprintf(ctx, "%s/%s", USER_ROM_PATH, get_value_string(lc15bts_par_names, par));
	if (!fpath)
		return NULL;

	fp = fopen(fpath, mode);
	if (!fp)
		fprintf(stderr, "Failed to open %s due to '%s' error\n", fpath, strerror(errno));

	talloc_free(fpath);

	return fp;
}

int lc15bts_par_get_int(void *ctx, enum lc15bts_par par, int *ret)
{
	FILE *fp = lc15bts_par_get_path(ctx, par, "r");
	int rc;

	if (fp == NULL) {
		return -errno;
	}

	rc = fscanf(fp, "%d", ret);
	if (rc != 1) {
		fclose(fp);
		return -EIO;
	}
	fclose(fp);
	return 0;
}

int lc15bts_par_set_int(void *ctx, enum lc15bts_par par, int val)
{
	FILE *fp = lc15bts_par_get_path(ctx, par, "w");
	int rc;

	if (fp == NULL) {
		return -errno;
	}

	rc = fprintf(fp, "%d", val);
	if (rc < 0) {
		fclose(fp);
		return -EIO;
	}

	fsync(fp);
	fclose(fp);
	return 0;
}

int lc15bts_par_get_buf(void *ctx, enum lc15bts_par par, uint8_t *buf, unsigned int size)
{	
	FILE *fp = lc15bts_par_get_path(ctx, par, "rb");
	int rc;

	if (fp == NULL) {
		return -errno;
	}

	rc = fread(buf, 1, size, fp);
	
	fclose(fp);
	
	return rc;
}

int lc15bts_par_set_buf(void *ctx, enum lc15bts_par par, const uint8_t *buf, unsigned int size)
{
        FILE *fp = lc15bts_par_get_path(ctx, par, "wb");
        int rc;

        if (fp == NULL) {
                return -errno;
	}

        rc = fwrite(buf, 1, size, fp);

        fsync(fp);
        fclose(fp);

        return rc;
}

int lc15bts_par_get_gps_fix(time_t *ret)
{
	char fpath[PATH_MAX];
	FILE *fp;
	int rc;

	snprintf(fpath, sizeof(fpath)-1, "%s/%s", USER_ROM_PATH, get_value_string(lc15bts_par_names, LC15BTS_PAR_GPS_FIX));
	fpath[sizeof(fpath)-1] = '\0';

	fp = fopen(fpath, "r");
	if (fp == NULL) {
		return -errno;
	}

	rc = fscanf(fp, "%lld", (long long *)ret);
	if (rc != 1) {
		fclose(fp);
		return -EIO;
	}
	fclose(fp);

	return 0;
}

int lc15bts_par_set_gps_fix(time_t val)
{
	char fpath[PATH_MAX];
	FILE *fp;
	int rc;

	snprintf(fpath, sizeof(fpath)-1, "%s/%s", USER_ROM_PATH, get_value_string(lc15bts_par_names, LC15BTS_PAR_GPS_FIX));
	fpath[sizeof(fpath)-1] = '\0';

	fp = fopen(fpath, "w");
	if (fp == NULL) {
		return -errno;
	}

	rc = fprintf(fp, "%lld", (long long)val);
	if (rc < 0) {
		fclose(fp);
		return -EIO;
	}
	fsync(fp);
	fclose(fp);

	return 0;
}
