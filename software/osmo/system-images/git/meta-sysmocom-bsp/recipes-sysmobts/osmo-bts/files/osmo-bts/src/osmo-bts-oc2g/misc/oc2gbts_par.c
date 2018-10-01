/* oc2gbts - access to hardware related parameters */

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

#include "oc2gbts_par.h"

const struct value_string oc2gbts_par_names[_NUM_OC2GBTS_PAR+1] = {
	{ OC2GBTS_PAR_TEMP_SUPPLY_MAX,	"temp-supply-max" },
	{ OC2GBTS_PAR_TEMP_SOC_MAX,		"temp-soc-max" },
	{ OC2GBTS_PAR_TEMP_FPGA_MAX,	"temp-fpga-max" },
	{ OC2GBTS_PAR_TEMP_RMSDET_MAX,	"temp-rmsdet-max" },
	{ OC2GBTS_PAR_TEMP_OCXO_MAX,	"temp-ocxo-max" },
	{ OC2GBTS_PAR_TEMP_TX_MAX,		"temp-tx-max" },
	{ OC2GBTS_PAR_TEMP_PA_MAX,		"temp-pa-max" },
	{ OC2GBTS_PAR_VOLT_SUPPLY_MAX,	"volt-supply-max" },
	{ OC2GBTS_PAR_PWR_SUPPLY_MAX,	"pwr-supply-max" },
	{ OC2GBTS_PAR_PWR_PA_MAX,		"pwr-pa-max" },
	{ OC2GBTS_PAR_VSWR_MAX,			"vswr-max" },
	{ OC2GBTS_PAR_GPS_FIX, 			"gps-fix" },
	{ OC2GBTS_PAR_SERNR,			"serial-nr" },
	{ OC2GBTS_PAR_HOURS, 			"hours-running" },
	{ OC2GBTS_PAR_BOOTS, 			"boot-count" },
	{ OC2GBTS_PAR_KEY, 				"key" },
	{ 0, NULL }
};

int oc2gbts_par_is_int(enum oc2gbts_par par)
{
	switch (par) {
	case OC2GBTS_PAR_TEMP_SUPPLY_MAX:
	case OC2GBTS_PAR_TEMP_SOC_MAX:
	case OC2GBTS_PAR_TEMP_FPGA_MAX:
	case OC2GBTS_PAR_TEMP_RMSDET_MAX:
	case OC2GBTS_PAR_TEMP_OCXO_MAX:
	case OC2GBTS_PAR_TEMP_TX_MAX:
	case OC2GBTS_PAR_TEMP_PA_MAX:
	case OC2GBTS_PAR_VOLT_SUPPLY_MAX:
	case OC2GBTS_PAR_VSWR_MAX:
	case OC2GBTS_PAR_SERNR:
	case OC2GBTS_PAR_HOURS:
	case OC2GBTS_PAR_BOOTS:
	case OC2GBTS_PAR_PWR_SUPPLY_MAX:
	case OC2GBTS_PAR_PWR_PA_MAX:
		return 1;
	default:
		return 0;
	}
}

FILE *oc2gbts_par_get_path(void *ctx, enum oc2gbts_par par, const char* mode)
{
	char *fpath;
	FILE *fp;

	if (par >= _NUM_OC2GBTS_PAR)
		return NULL;

	fpath = talloc_asprintf(ctx, "%s/%s", USER_ROM_PATH, get_value_string(oc2gbts_par_names, par));
	if (!fpath)
		return NULL;

	fp = fopen(fpath, mode);
	if (!fp)
		fprintf(stderr, "Failed to open %s due to '%s' error\n", fpath, strerror(errno));

	talloc_free(fpath);

	return fp;
}

int oc2gbts_par_get_int(enum oc2gbts_par par, int *ret)
{
	char fpath[PATH_MAX];
	FILE *fp;
	int rc;

	if (par >= _NUM_OC2GBTS_PAR)
		return -ENODEV;

	snprintf(fpath, sizeof(fpath)-1, "%s/%s", USER_ROM_PATH, get_value_string(oc2gbts_par_names, par));
	fpath[sizeof(fpath)-1] = '\0';

	fp = fopen(fpath, "r");
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

int oc2gbts_par_set_int(enum oc2gbts_par par, int val)
{
	char fpath[PATH_MAX];
	FILE *fp;
	int rc;

	if (par >= _NUM_OC2GBTS_PAR)
		return -ENODEV;

	snprintf(fpath, sizeof(fpath)-1, "%s/%s", USER_ROM_PATH, get_value_string(oc2gbts_par_names, par));
	fpath[sizeof(fpath)-1] = '\0';

	fp = fopen(fpath, "w");
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

int oc2gbts_par_get_buf(enum oc2gbts_par par, uint8_t *buf,
			 unsigned int size)
{	
	char fpath[PATH_MAX];
	FILE *fp;
	int rc;

	if (par >= _NUM_OC2GBTS_PAR)
		return -ENODEV;

	snprintf(fpath, sizeof(fpath)-1, "%s/%s", USER_ROM_PATH, get_value_string(oc2gbts_par_names, par));
	fpath[sizeof(fpath)-1] = '\0';

	fp = fopen(fpath, "rb");
	if (fp == NULL) {
		return -errno;
	}

	rc = fread(buf, 1, size, fp);
	
	fclose(fp);
	
	return rc;
}

int oc2gbts_par_set_buf(enum oc2gbts_par par, const uint8_t *buf,
			 unsigned int size)
{
        char fpath[PATH_MAX];
        FILE *fp;
        int rc;

        if (par >= _NUM_OC2GBTS_PAR)
                return -ENODEV;

        snprintf(fpath, sizeof(fpath)-1, "%s/%s", USER_ROM_PATH, get_value_string(oc2gbts_par_names, par));
        fpath[sizeof(fpath)-1] = '\0';

        fp = fopen(fpath, "wb");
        if (fp == NULL) {
                return -errno;
	}

        rc = fwrite(buf, 1, size, fp);

        fsync(fp);
        fclose(fp);

        return rc;
}

int oc2gbts_par_get_gps_fix(void *ctx, time_t *ret)
{
	FILE *fp;
	int rc;

	fp = oc2gbts_par_get_path(ctx, OC2GBTS_PAR_GPS_FIX, "r");
	if (fp == NULL) {
		return -errno;
	}

	rc = fscanf(fp, "%ld", ret);
	if (rc != 1) {
		fclose(fp);
		return -EIO;
	}
	fclose(fp);

	return 0;
}

int oc2gbts_par_set_gps_fix(void *ctx, time_t val)
{
	FILE *fp;
	int rc;

	fp = oc2gbts_par_get_path(ctx, OC2GBTS_PAR_GPS_FIX, "w");
	if (fp == NULL) {
		return -errno;
	}

	rc = fprintf(fp, "%ld", val);
	if (rc < 0) {
		fclose(fp);
		return -EIO;
	}
	fsync(fp);
	fclose(fp);

	return 0;
}
