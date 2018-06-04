/*
 * Copyright 2017 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __COMMONLIB_STORAGE_SD_MMC_H__
#define __COMMONLIB_STORAGE_SD_MMC_H__

#include <commonlib/sd_mmc_ctrlr.h>
#include <commonlib/storage.h>
#include <stddef.h>
#include <console/console.h>

#define SD_MMC_IO_RETRIES	1000

#define IS_SD(x)		(x->version & SD_VERSION_SD)

#define SET_BUS_WIDTH(ctrlr, width)		\
	do {					\
		ctrlr->bus_width = width;	\
		ctrlr->set_ios(ctrlr);		\
	} while (0)

#define SET_CLOCK(ctrlr, clock_hz)		\
	do {					\
		ctrlr->request_hz = clock_hz;	\
		ctrlr->set_ios(ctrlr);		\
	} while (0)

#define SET_TIMING(ctrlr, timing_value)		\
	do {					\
		ctrlr->timing = timing_value;	\
		ctrlr->set_ios(ctrlr);		\
	} while (0)

/* Common support routines */
int sd_mmc_enter_standby(struct storage_media *media);
uint64_t sd_mmc_extract_uint32_bits(const uint32_t *array, int start,
	int count);
int sd_mmc_send_status(struct storage_media *media, ssize_t tries);
int sd_mmc_set_blocklen(struct sd_mmc_ctrlr *ctrlr, int len);

/* MMC support routines */
int mmc_change_freq(struct storage_media *media);
int mmc_complete_op_cond(struct storage_media *media);
const char *mmc_partition_name(struct storage_media *media,
	unsigned int partition_number);
int mmc_send_ext_csd(struct sd_mmc_ctrlr *ctrlr, unsigned char *ext_csd);
int mmc_send_op_cond(struct storage_media *media);
int mmc_set_bus_width(struct storage_media *media);
int mmc_set_partition(struct storage_media *media,
	unsigned int partition_number);
int mmc_update_capacity(struct storage_media *media);

/* SD card support routines */
int sd_change_freq(struct storage_media *media);
const char *sd_partition_name(struct storage_media *media,
	unsigned int partition_number);
int sd_send_if_cond(struct storage_media *media);
int sd_send_op_cond(struct storage_media *media);
int sd_set_bus_width(struct storage_media *media);
int sd_set_partition(struct storage_media *media,
	unsigned int partition_number);

/* Controller debug functions */
#define sdhc_debug(format...) \
	do {						\
		if (IS_ENABLED(CONFIG_SDHC_DEBUG))	\
			printk(BIOS_DEBUG, format);	\
	} while (0)
#define sdhc_trace(format...) \
	do {						\
		if (IS_ENABLED(CONFIG_SDHC_TRACE))	\
			printk(BIOS_DEBUG, format);	\
	} while (0)
#define sdhc_error(format...) printk(BIOS_ERR, "ERROR: " format)

/* Card/device debug functions */
#define sd_mmc_debug(format...) \
	do {						\
		if (IS_ENABLED(CONFIG_SD_MMC_DEBUG))	\
			printk(BIOS_DEBUG, format);	\
	} while (0)
#define sd_mmc_trace(format...) \
	do {						\
		if (IS_ENABLED(CONFIG_SD_MMC_TRACE))	\
			printk(BIOS_DEBUG, format);	\
	} while (0)
#define sd_mmc_error(format...) printk(BIOS_ERR, "ERROR: " format)

#endif /* __COMMONLIB_STORAGE_SD_MMC_H__ */
