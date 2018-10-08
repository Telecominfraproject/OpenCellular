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

#include "oc2gbts_clock.h"

#define CLKERR_ERR_SYSFS	"/var/oc2g/clkerr/clkerr1_average"
#define CLKERR_ACC_SYSFS	"/var/oc2g/clkerr/clkerr1_average_accuracy"
#define CLKERR_INT_SYSFS	"/var/oc2g/clkerr/clkerr1_average_interval"
#define CLKERR_FLT_SYSFS	"/var/oc2g/clkerr/clkerr1_fault"
#define CLKERR_RFS_SYSFS	"/var/oc2g/clkerr/refresh"
#define CLKERR_RST_SYSFS	"/var/oc2g/clkerr/reset"

#define OCXODAC_VAL_SYSFS	"/var/oc2g/ocxo/voltage"
#define OCXODAC_ROM_SYSFS	"/var/oc2g/ocxo/eeprom"

/* clock error */
static int clkerr_fd_err = -1;
static int clkerr_fd_accuracy = -1;
static int clkerr_fd_interval = -1;
static int clkerr_fd_fault = -1;
static int clkerr_fd_refresh = -1;
static int clkerr_fd_reset = -1;

/* ocxo dac */
static int ocxodac_fd_value = -1;
static int ocxodac_fd_save = -1;


static int sysfs_read_val(int fd, int *val)
{
	int rc;
	char szVal[32] = {0};

	lseek( fd, 0, SEEK_SET );

	rc = read(fd, szVal, sizeof(szVal) - 1);
	if (rc < 0) {
		return -errno;
	}

	rc = sscanf(szVal, "%d", val);
	if (rc != 1) {
		return -1;
	}

	return 0;
}

static int sysfs_write_val(int fd, int val)
{
	int n, rc;
	char szVal[32] = {0};

	n = sprintf(szVal, "%d", val);

	lseek(fd, 0, SEEK_SET);
	rc = write(fd, szVal, n+1);
	if (rc < 0) {
		return -errno;
	}
	return 0;
}

static int sysfs_write_str(int fd, const char *str)
{
	int rc;

	lseek( fd, 0, SEEK_SET );
	rc = write(fd, str, strlen(str)+1);
	if (rc < 0) {
		return -errno;
	}
	return 0;
}


int oc2gbts_clock_err_open(void)
{
	int rc;
	int fault;

	if (clkerr_fd_err < 0) {
		clkerr_fd_err = open(CLKERR_ERR_SYSFS, O_RDONLY);
		if (clkerr_fd_err < 0) {
			oc2gbts_clock_err_close();
			return clkerr_fd_err;	
		}
	}

	if (clkerr_fd_accuracy < 0) {
		clkerr_fd_accuracy = open(CLKERR_ACC_SYSFS, O_RDONLY);
		if (clkerr_fd_accuracy < 0) {
			oc2gbts_clock_err_close();
			return clkerr_fd_accuracy;	
		}
	}

	if (clkerr_fd_interval < 0) {
		clkerr_fd_interval = open(CLKERR_INT_SYSFS, O_RDONLY);
		if (clkerr_fd_interval < 0) {
			oc2gbts_clock_err_close();
			return clkerr_fd_interval;	
		}
	}

	if (clkerr_fd_fault < 0) {
		clkerr_fd_fault = open(CLKERR_FLT_SYSFS, O_RDONLY);
		if (clkerr_fd_fault < 0) {
			oc2gbts_clock_err_close();
			return clkerr_fd_fault;	
		}
	}

	if (clkerr_fd_refresh < 0) {
		clkerr_fd_refresh = open(CLKERR_RFS_SYSFS, O_WRONLY);
		if (clkerr_fd_refresh < 0) {
			oc2gbts_clock_err_close();
			return clkerr_fd_refresh;	
		}
	}

	if (clkerr_fd_reset < 0) {
	        clkerr_fd_reset = open(CLKERR_RST_SYSFS, O_WRONLY);
	        if (clkerr_fd_reset < 0) {
			oc2gbts_clock_err_close();
			return clkerr_fd_reset;	
		}
	}
	return 0;
}

void oc2gbts_clock_err_close(void)
{
	if (clkerr_fd_err >= 0) {
		close(clkerr_fd_err);
		clkerr_fd_err = -1;
	}

	if (clkerr_fd_accuracy >= 0) {
		close(clkerr_fd_accuracy);
		clkerr_fd_accuracy = -1;
	}

	if (clkerr_fd_interval >= 0) {
		close(clkerr_fd_interval);
		clkerr_fd_interval = -1;
	}

	if (clkerr_fd_fault >= 0) {
		close(clkerr_fd_fault);
		clkerr_fd_fault = -1;
	}

	if (clkerr_fd_refresh >= 0) {
		close(clkerr_fd_refresh);
		clkerr_fd_refresh = -1;
	}

	if (clkerr_fd_reset >= 0) {
		close(clkerr_fd_reset);
		clkerr_fd_reset = -1;
	}
}

int oc2gbts_clock_err_reset(void) 
{
	return sysfs_write_val(clkerr_fd_reset, 1);
}

int oc2gbts_clock_err_get(int *fault, int *error_ppt, 
		int *accuracy_ppq, int *interval_sec)
{
	int rc;

	rc = sysfs_write_str(clkerr_fd_refresh, "once");
	if (rc < 0) {
                return -1;
	}

	rc  = sysfs_read_val(clkerr_fd_fault,    fault);
	rc |= sysfs_read_val(clkerr_fd_err,      error_ppt);
	rc |= sysfs_read_val(clkerr_fd_accuracy, accuracy_ppq);
	rc |= sysfs_read_val(clkerr_fd_interval, interval_sec);
	if (rc) {
		return -1;
	}
	return 0;
}


int oc2gbts_clock_dac_open(void)
{
	if (ocxodac_fd_value < 0) {
	        ocxodac_fd_value = open(OCXODAC_VAL_SYSFS, O_RDWR);
	        if (ocxodac_fd_value < 0) {
	                oc2gbts_clock_dac_close();
	                return ocxodac_fd_value;
	        }
	}

	if (ocxodac_fd_save < 0) {
	        ocxodac_fd_save = open(OCXODAC_ROM_SYSFS, O_WRONLY);
	        if (ocxodac_fd_save < 0) {
	                oc2gbts_clock_dac_close();
	                return ocxodac_fd_save;
	        }
	}
	return 0;
}

void oc2gbts_clock_dac_close(void)
{
        if (ocxodac_fd_value >= 0) {
                close(ocxodac_fd_value);
                ocxodac_fd_value = -1;
        }

        if (ocxodac_fd_save >= 0) {
                close(ocxodac_fd_save);
                ocxodac_fd_save = -1;
        }
}

int oc2gbts_clock_dac_get(int *dac_value)
{
	return sysfs_read_val(ocxodac_fd_value, dac_value);
}

int oc2gbts_clock_dac_set(int dac_value)
{
        return sysfs_write_val(ocxodac_fd_value, dac_value);
}

int oc2gbts_clock_dac_save(void)
{
        return sysfs_write_val(ocxodac_fd_save, 1);
}


