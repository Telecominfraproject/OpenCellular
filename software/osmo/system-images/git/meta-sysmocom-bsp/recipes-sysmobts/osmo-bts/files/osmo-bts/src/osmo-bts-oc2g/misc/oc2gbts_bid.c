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

#include "oc2gbts_bid.h"

#define BOARD_REV_MAJ_SYSFS	"/var/oc2g/platform/rev/major"
#define BOARD_REV_MIN_SYSFS	"/var/oc2g/platform/rev/minor"
#define BOARD_OPT_SYSFS		"/var/oc2g/platform/option"
 
static const int option_type_mask[_NUM_OPTION_TYPES] = {
	[OC2GBTS_OPTION_OCXO]		= 0x07,
	[OC2GBTS_OPTION_FPGA]		= 0x03,
	[OC2GBTS_OPTION_PA]			= 0x01,
	[OC2GBTS_OPTION_BAND]		= 0x03,
	[OC2GBTS_OPTION_TX_ATT]		= 0x01,
	[OC2GBTS_OPTION_TX_FIL]		= 0x01,
	[OC2GBTS_OPTION_RX_ATT]		= 0x01,
	[OC2GBTS_OPTION_RMS_FWD]	= 0x01,
	[OC2GBTS_OPTION_RMS_REFL]	= 0x01,
	[OC2GBTS_OPTION_DDR_32B]	= 0x01,
	[OC2GBTS_OPTION_DDR_ECC]	= 0x01,
	[OC2GBTS_OPTION_PA_TEMP]	= 0x01,
};

static const int option_type_shift[_NUM_OPTION_TYPES] = {
	[OC2GBTS_OPTION_OCXO]		= 0,
	[OC2GBTS_OPTION_FPGA]		= 3,
	[OC2GBTS_OPTION_PA]			= 5,
	[OC2GBTS_OPTION_BAND]		= 6,
	[OC2GBTS_OPTION_TX_ATT]		= 8,
	[OC2GBTS_OPTION_TX_FIL]		= 9,
	[OC2GBTS_OPTION_RX_ATT]		= 10,
	[OC2GBTS_OPTION_RMS_FWD]	= 11,
	[OC2GBTS_OPTION_RMS_REFL]	= 12,
	[OC2GBTS_OPTION_DDR_32B]	= 13,
	[OC2GBTS_OPTION_DDR_ECC]	= 14,
	[OC2GBTS_OPTION_PA_TEMP]	= 15,
};


static char board_rev_maj = -1;
static char board_rev_min = -1;
static int board_option = -1;

void oc2gbts_rev_get(char *rev_maj, char *rev_min)
{
	FILE *fp;
	char rev;

	*rev_maj = 0;
	*rev_min = 0;

	if (board_rev_maj != -1 && board_rev_min != -1) {
		*rev_maj = board_rev_maj;
		*rev_min = board_rev_min;
	}

	fp = fopen(BOARD_REV_MAJ_SYSFS, "r");
	if (fp == NULL) return;

	if (fscanf(fp, "%c", &rev) != 1) {
		fclose(fp);
		return;
	}

	fclose(fp);

	*rev_maj = board_rev_maj = rev;

	fp = fopen(BOARD_REV_MIN_SYSFS, "r");
	if (fp == NULL) return;

	if (fscanf(fp, "%c", &rev) != 1) {
		fclose(fp);
		return;
	}

	fclose(fp);

	*rev_min = board_rev_min = rev;
}

int oc2gbts_model_get(void)
{
	FILE *fp;
	int opt;


	if (board_option == -1) {
		fp = fopen(BOARD_OPT_SYSFS, "r");
		if (fp == NULL) {
			return -1;
		}

		if (fscanf(fp, "%X", &opt) != 1) {
			fclose( fp );
			return -1;
		}
		fclose(fp);
	
		board_option = opt;
	}
	return board_option;
}

int oc2gbts_option_get(enum oc2gbts_option_type type)
{
	int rc;
	int option;

	if (type >= _NUM_OPTION_TYPES) {
		return -EINVAL;
	}

	if (board_option == -1) {
		rc = oc2gbts_model_get();
		if (rc < 0) return rc;
	}

	option = (board_option >> option_type_shift[type])
		& option_type_mask[type];

	return option;	
}
