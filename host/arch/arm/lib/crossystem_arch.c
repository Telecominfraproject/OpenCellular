/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#ifndef HAVE_MACOS
#include <linux/fs.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>

#include "vboot_common.h"
#include "host_common.h"
#include "crossystem.h"
#include "crossystem_arch.h"
#include "crossystem_vbnv.h"

/* Base name for firmware FDT files */
#define FDT_BASE_PATH "/proc/device-tree/firmware/chromeos"
/* Path to compatible FDT entry */
#define FDT_COMPATIBLE_PATH "/proc/device-tree/compatible"
/* Path to the chromeos_arm platform device */
#define PLATFORM_DEV_PATH "/sys/devices/platform/chromeos_arm"
/* Device for NVCTX write */
#define NVCTX_PATH "/dev/mmcblk%d"
/* Base name for GPIO files */
#define GPIO_BASE_PATH "/sys/class/gpio"
#define GPIO_EXPORT_PATH GPIO_BASE_PATH "/export"
/* Name of NvStorage type property */
#define FDT_NVSTORAGE_TYPE_PROP "nonvolatile-context-storage"
/* Errors */
#define E_FAIL      -1
#define E_FILEOP    -2
#define E_MEM       -3
/* Common constants */
#define FNAME_SIZE  80
#define SECTOR_SIZE 512
#define MAX_NMMCBLK 9

static int FindEmmcDev(void)
{
	int mmcblk;
	unsigned value;
	char filename[FNAME_SIZE];
	for (mmcblk = 0; mmcblk < MAX_NMMCBLK; mmcblk++) {
		/* Get first non-removable mmc block device */
		snprintf(filename, sizeof(filename),
			 "/sys/block/mmcblk%d/removable", mmcblk);
		if (ReadFileInt(filename, &value) < 0)
			continue;
		if (value == 0)
			return mmcblk;
	}
	/* eMMC not found */
	return E_FAIL;
}

static int ReadFdtValue(const char *property, int *value)
{
	char filename[FNAME_SIZE];
	FILE *file;
	int data = 0;

	snprintf(filename, sizeof(filename), FDT_BASE_PATH "/%s", property);
	file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Unable to open FDT property %s\n", property);
		return E_FILEOP;
	}

	if (fread(&data, 1, sizeof(data), file) != sizeof(data)) {
		fprintf(stderr, "Unable to read FDT property %s\n", property);
		return E_FILEOP;
	}
	fclose(file);

	if (value)
		*value = ntohl(data); /* FDT is network byte order */

	return 0;
}

static int ReadFdtInt(const char *property)
{
	int value = 0;
	if (ReadFdtValue(property, &value))
		return E_FAIL;
	return value;
}

static void GetFdtPropertyPath(const char *property, char *path, size_t size)
{
	if (property[0] == '/')
		StrCopy(path, property, size);
	else
		snprintf(path, size, FDT_BASE_PATH "/%s", property);
}

static int FdtPropertyExist(const char *property)
{
	char filename[FNAME_SIZE];
	struct stat file_status;

	GetFdtPropertyPath(property, filename, sizeof(filename));
	if (!stat(filename, &file_status))
		return 1; // It exists!
	else
		return 0; // It does not exist or some error happened.
}

static int ReadFdtBlock(const char *property, void **block, size_t *size)
{
	char filename[FNAME_SIZE];
	FILE *file;
	size_t property_size;
	char *data;

	if (!block)
		return E_FAIL;

	GetFdtPropertyPath(property, filename, sizeof(filename));
	file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Unable to open FDT property %s\n", property);
		return E_FILEOP;
	}

	fseek(file, 0, SEEK_END);
	property_size = ftell(file);
	rewind(file);

	data = malloc(property_size +1);
	if (!data) {
		fclose(file);
		return E_MEM;
	}
	data[property_size] = 0;

	if (1 != fread(data, property_size, 1, file)) {
		fprintf(stderr, "Unable to read from property %s\n", property);
		fclose(file);
		free(data);
		return E_FILEOP;
	}

	fclose(file);
	*block = data;
	if (size)
		*size = property_size;

	return 0;
}

static char * ReadFdtString(const char *property)
{
	void *str = NULL;
	/* Do not need property size */
	ReadFdtBlock(property, &str, 0);
	return (char *)str;
}

static int VbGetPlatformGpioStatus(const char* name)
{
	char gpio_name[FNAME_SIZE];
	unsigned value;

	snprintf(gpio_name, sizeof(gpio_name), "%s/%s/value",
		 PLATFORM_DEV_PATH, name);
	if (ReadFileInt(gpio_name, &value) < 0)
		return -1;

	return (int)value;
}

static int VbGetGpioStatus(unsigned gpio_number)
{
	char gpio_name[FNAME_SIZE];
	unsigned value;

	snprintf(gpio_name, sizeof(gpio_name), "%s/gpio%d/value",
		 GPIO_BASE_PATH, gpio_number);
	if (ReadFileInt(gpio_name, &value) < 0) {
		/* Try exporting the GPIO */
		FILE* f = fopen(GPIO_EXPORT_PATH, "wt");
		if (!f)
			return -1;
		fprintf(f, "%d", gpio_number);
		fclose(f);

		/* Try re-reading the GPIO value */
		if (ReadFileInt(gpio_name, &value) < 0)
			return -1;
	}

	return (int)value;
}

static int VbGetVarGpio(const char* name)
{
	int gpio_num;
	void *pp = NULL;
	int *prop;
	size_t proplen = 0;
	int ret = 0;

	/* TODO: This should at some point in the future use the phandle
	 * to find the gpio chip and thus the base number. Assume 0 now,
	 * which isn't 100% future-proof (i.e. if one of the switches gets
	 * moved to an offchip gpio controller.
	 */

	ret = ReadFdtBlock(name, &pp, &proplen);
	if (ret || !pp || proplen != 12) {
		ret = 2;
		goto out;
	}
	prop = pp;
	gpio_num = ntohl(prop[1]);

	/*
	 * TODO(chrome-os-partner:11296): Use gpio_num == 0 to denote non-exist
	 * GPIO for now, at the risk that one day we might actually want to
	 * read from a GPIO port 0.  We should figure out how to represent
	 * "non-exist" properly.
	 */
	if (gpio_num)
		ret = VbGetGpioStatus(gpio_num);
	else
		ret = -1;
out:
	if (pp)
		free(pp);

	return ret;
}

static int vb2_read_nv_storage_disk(struct vb2_context *ctx)
{
	int nvctx_fd = -1;
	uint8_t sector[SECTOR_SIZE];
	int rv = -1;
	char nvctx_path[FNAME_SIZE];
	int emmc_dev;
	int lba = ReadFdtInt("nonvolatile-context-lba");
	int offset = ReadFdtInt("nonvolatile-context-offset");
	int size = ReadFdtInt("nonvolatile-context-size");

	emmc_dev = FindEmmcDev();
	if (emmc_dev < 0)
		return E_FAIL;
	snprintf(nvctx_path, sizeof(nvctx_path), NVCTX_PATH, emmc_dev);

	if (size != sizeof(ctx->nvdata) || (size + offset > SECTOR_SIZE))
		return E_FAIL;

	nvctx_fd = open(nvctx_path, O_RDONLY);
	if (nvctx_fd == -1) {
		fprintf(stderr, "%s: failed to open %s\n", __FUNCTION__,
			nvctx_path);
		goto out;
	}
	lseek(nvctx_fd, lba * SECTOR_SIZE, SEEK_SET);

	rv = read(nvctx_fd, sector, SECTOR_SIZE);
	if (size <= 0) {
		fprintf(stderr, "%s: failed to read nvctx from device %s\n",
			__FUNCTION__, nvctx_path);
		goto out;
	}
	memcpy(ctx->nvdata, sector+offset, size);
	rv = 0;

out:
	if (nvctx_fd > 0)
		close(nvctx_fd);

	return rv;
}

static int vb2_write_nv_storage_disk(struct vb2_context *ctx)
{
	int nvctx_fd = -1;
	uint8_t sector[SECTOR_SIZE];
	int rv = -1;
	char nvctx_path[FNAME_SIZE];
	int emmc_dev;
	int lba = ReadFdtInt("nonvolatile-context-lba");
	int offset = ReadFdtInt("nonvolatile-context-offset");
	int size = ReadFdtInt("nonvolatile-context-size");

	emmc_dev = FindEmmcDev();
	if (emmc_dev < 0)
		return E_FAIL;
	snprintf(nvctx_path, sizeof(nvctx_path), NVCTX_PATH, emmc_dev);

	if (size != sizeof(ctx->nvdata) || (size + offset > SECTOR_SIZE))
		return E_FAIL;

	do {
		nvctx_fd = open(nvctx_path, O_RDWR);
		if (nvctx_fd == -1) {
			fprintf(stderr, "%s: failed to open %s\n",
				__FUNCTION__, nvctx_path);
			break;
		}
		lseek(nvctx_fd, lba * SECTOR_SIZE, SEEK_SET);
		rv = read(nvctx_fd, sector, SECTOR_SIZE);
		if (rv <= 0) {
			fprintf(stderr,
				"%s: failed to read nvctx from device %s\n",
				__FUNCTION__, nvctx_path);
			break;
		}
		memcpy(sector+offset, ctx->nvdata, size);
		lseek(nvctx_fd, lba * SECTOR_SIZE, SEEK_SET);
		rv = write(nvctx_fd, sector, SECTOR_SIZE);
		if (rv <= 0) {
			fprintf(stderr,
				"%s: failed to write nvctx to device %s\n",
				__FUNCTION__, nvctx_path);
			break;
		}
#ifndef HAVE_MACOS
		/* Must flush buffer cache here to make sure it goes to disk */
		rv = ioctl(nvctx_fd, BLKFLSBUF, 0);
		if (rv < 0) {
			fprintf(stderr,
				"%s: failed to flush nvctx to device %s\n",
				__FUNCTION__, nvctx_path);
			break;
		}
#endif
		rv = 0;
	} while (0);

	if (nvctx_fd > 0)
		close(nvctx_fd);

	return rv;
}

int vb2_read_nv_storage(struct vb2_context *ctx)
{
	/* Default to disk for older firmware which does not provide storage
	 * type */
	char *media;
	if (!FdtPropertyExist(FDT_NVSTORAGE_TYPE_PROP))
		return vb2_read_nv_storage_disk(ctx);
	media = ReadFdtString(FDT_NVSTORAGE_TYPE_PROP);
	if (!strcmp(media, "disk"))
		return vb2_read_nv_storage_disk(ctx);
	if (!strcmp(media, "cros-ec") || !strcmp(media, "mkbp") ||
	    !strcmp(media, "flash"))
		return vb2_read_nv_storage_mosys(ctx);
	return -1;
}

int vb2_write_nv_storage(struct vb2_context *ctx)
{
	/* Default to disk for older firmware which does not provide storage
	 * type */
	char *media;
	if (!FdtPropertyExist(FDT_NVSTORAGE_TYPE_PROP))
		return vb2_write_nv_storage_disk(ctx);
	media = ReadFdtString(FDT_NVSTORAGE_TYPE_PROP);
	if (!strcmp(media, "disk"))
		return vb2_write_nv_storage_disk(ctx);
	if (!strcmp(media, "cros-ec") || !strcmp(media, "mkbp") ||
	    !strcmp(media, "flash"))
		return vb2_write_nv_storage_mosys(ctx);
	return -1;
}

VbSharedDataHeader *VbSharedDataRead(void)
{
	void *block = NULL;
	size_t size = 0;
	if (ReadFdtBlock("vboot-shared-data", &block, &size))
		return NULL;
	VbSharedDataHeader *p = (VbSharedDataHeader *)block;
	if (p->magic != VB_SHARED_DATA_MAGIC) {
		fprintf(stderr,  "%s: failed to validate magic in "
			"VbSharedDataHeader (%x != %x)\n",
			__FUNCTION__, p->magic, VB_SHARED_DATA_MAGIC);
		return NULL;
	}
	return (VbSharedDataHeader *)block;
}

int VbGetArchPropertyInt(const char* name)
{
	if (!strcasecmp(name, "fmap_base")) {
		return ReadFdtInt("fmap-offset");
	} else if (!strcasecmp(name, "devsw_cur")) {
		/* Systems with virtual developer switches return at-boot
		 * value */
		int flags = VbGetSystemPropertyInt("vdat_flags");
		if ((flags != -1) && (flags & VBSD_HONOR_VIRT_DEV_SWITCH))
			return VbGetSystemPropertyInt("devsw_boot");

		return VbGetVarGpio("developer-switch");
	} else if (!strcasecmp(name, "recoverysw_cur")) {
		int value;
		value = VbGetPlatformGpioStatus("recovery");
		if (value != -1)
			return value;

		return VbGetVarGpio("recovery-switch");
	} else if (!strcasecmp(name, "wpsw_cur")) {
		int value;
		/* Try finding the GPIO through the chromeos_arm platform
		 * device first. */
		value = VbGetPlatformGpioStatus("write-protect");
		if (value != -1)
			return value;
		return VbGetVarGpio("write-protect-switch");
	} else if (!strcasecmp(name, "recoverysw_ec_boot")) {
		/* TODO: read correct value using ectool */
		return 0;
	} else {
		return -1;
	}
}

const char* VbGetArchPropertyString(const char* name, char* dest,
                                    size_t size)
{
	char *str = NULL;
	char *rv = NULL;
	char *prop = NULL;

	if (!strcasecmp(name,"arch"))
		return StrCopy(dest, "arm", size);

	/* Properties from fdt */
	if (!strcasecmp(name, "ro_fwid"))
		prop = "readonly-firmware-version";
	else if (!strcasecmp(name, "hwid"))
		prop = "hardware-id";
	else if (!strcasecmp(name, "fwid"))
		prop = "firmware-version";
	else if (!strcasecmp(name, "mainfw_type"))
		prop = "firmware-type";
	else if (!strcasecmp(name, "ecfw_act"))
		prop = "active-ec-firmware";

	if (prop)
		str = ReadFdtString(prop);

	if (str) {
		rv = StrCopy(dest, str, size);
		free(str);
		return rv;
	}
	return NULL;
}

int VbSetArchPropertyInt(const char* name, int value)
{
	/* All is handled in arch independent fashion */
	return -1;
}

int VbSetArchPropertyString(const char* name, const char* value)
{
	/* All is handled in arch independent fashion */
	return -1;
}

int VbArchInit(void)
{
	return 0;
}
