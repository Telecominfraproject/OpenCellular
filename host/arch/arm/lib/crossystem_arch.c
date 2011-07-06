/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>

#include "vboot_common.h"
#include "vboot_nvstorage.h"
#include "host_common.h"
#include "crossystem_arch.h"

/* Base name for FDT files */
#define FDT_BASE_PATH "/proc/device-tree/firmware/chromeos"
/* Device for NVCTX write */
#define NVCTX_PATH "/dev/mmcblk%d"
/* BINF */
#define BINF_MAINFW_ACT       1
#define BINF_ECFW_ACT         2
#define BINF_MAINFW_TYPE      3
#define BINF_RECOVERY_REASON  4
/* Errors */
#define E_FAIL      -1
#define E_FILEOP    -2
#define E_MEM       -3
/* Common constants */
#define FNAME_SIZE  80
#define SECTOR_SIZE 512
#define MAX_NMMCBLK 9

static int FindEmmcDev(void) {
  int mmcblk;
  char filename[FNAME_SIZE];
  for (mmcblk = 0; mmcblk < MAX_NMMCBLK; mmcblk++) {
    /* Get first non-removable mmc block device */
    snprintf(filename, sizeof(filename), "/sys/block/mmcblk%d/removable",
              mmcblk);
    if (ReadFileInt(filename) == 0)
      return mmcblk;
  }
  /* eMMC not found */
  return E_FAIL;
}

static int ReadFdtValue(const char *property, int *value) {
  char filename[FNAME_SIZE];
  FILE *file;
  int data = 0;

  snprintf(filename, sizeof(filename), FDT_BASE_PATH "/%s", property);
  file = fopen(filename, "rb");
  if (!file) {
    fprintf(stderr, "Unable to open FDT property %s\n", property);
    return E_FILEOP;
  }

  fread(&data, 1, sizeof(data), file);
  fclose(file);

  if (value)
    *value = ntohl(data); /* FDT is network byte order */

  return 0;
}

static int ReadFdtInt(const char *property) {
  int value;
  if (ReadFdtValue(property, &value))
    return E_FAIL;
  return value;
}

static int ReadFdtBlock(const char *property, void **block, size_t *size) {
  char filename[FNAME_SIZE];
  FILE *file;
  size_t property_size;
  char *data;

  if (!block)
    return E_FAIL;

  snprintf(filename, sizeof(filename), FDT_BASE_PATH "/%s", property);
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

static char * ReadFdtString(const char *property) {
  void *str = NULL;
  /* Do not need property size */
  ReadFdtBlock(property, &str, 0);
  return (char *)str;
}

static int VbGetGpioStatus(unsigned gpio_number) {
  char const *gpio_name_format = "/sys/class/gpio/gpio%d/value";
  char gpio_name[FNAME_SIZE];

  snprintf(gpio_name, sizeof(gpio_name), gpio_name_format, gpio_number);
  return ReadFileInt(gpio_name);
}

static int VbGetVarGpio(const char* name) {
  int polarity, gpio_num;
  char prop_polarity[FNAME_SIZE];
  char prop_gpio_num[FNAME_SIZE];

  snprintf(prop_polarity, sizeof(prop_polarity), "polarity_%s", name);
  snprintf(prop_gpio_num, sizeof(prop_gpio_num), "gpio_port_%s", name);

  polarity = ReadFdtInt(prop_polarity);
  gpio_num = ReadFdtInt(prop_gpio_num);

  if (polarity == -1 || gpio_num == -1)
    return 2;

  return VbGetGpioStatus(gpio_num) ^ polarity ^ 1;
}

int VbReadNvStorage(VbNvContext* vnc) {
  void *nvcxt_cache;
  size_t size;

  if (ReadFdtBlock("nvcxt_cache", &nvcxt_cache, &size))
    return E_FAIL;

  Memcpy(vnc->raw, nvcxt_cache, MIN(sizeof(vnc->raw), size));
  free(nvcxt_cache);
  return 0;
}

int VbWriteNvStorage(VbNvContext* vnc) {
  int nvctx_fd = -1;
  uint8_t sector[SECTOR_SIZE];
  int rv = -1;
  ssize_t size;
  char nvctx_path[FNAME_SIZE];
  int emmc_dev;

  emmc_dev = FindEmmcDev();
  if (emmc_dev < 0)
    return E_FAIL;
  snprintf(nvctx_path, sizeof(nvctx_path), NVCTX_PATH, emmc_dev);

  do {
    nvctx_fd = open(nvctx_path, O_RDWR);
    if (nvctx_fd == -1) {
      fprintf(stderr, "%s: failed to open %s\n", __FUNCTION__, nvctx_path);
      break;
    }
    size = read(nvctx_fd, sector, SECTOR_SIZE);
    if (size <= 0) {
      fprintf(stderr, "%s: failed to read nvctx from device %s\n",
              __FUNCTION__, nvctx_path);
      break;
    }
    size = MIN(sizeof(vnc->raw), SECTOR_SIZE);
    Memcpy(sector, vnc->raw, size);
    lseek(nvctx_fd, 0, SEEK_SET);
    size = write(nvctx_fd, sector, SECTOR_SIZE);
    if (size <= 0) {
      fprintf(stderr,  "%s: failed to write nvctx to device %s\n",
              __FUNCTION__, nvctx_path);
      break;
    }
    rv = 0;
  } while (0);

  if (nvctx_fd > 0)
    close(nvctx_fd);

  return rv;
}

VbSharedDataHeader *VbSharedDataRead(void) {
  void *block = NULL;
  size_t size = 0;
  if (ReadFdtBlock("vbshared_data", &block, &size))
    return NULL;
  return (VbSharedDataHeader *)block;
}

static int VbGetRecoveryReason(void) {
  int value;
  size_t size;
  uint8_t *binf;
  if (ReadFdtBlock("binf", (void **)&binf, &size))
    return -1;
  value = binf[BINF_RECOVERY_REASON];
  free(binf);
  return value;
}

int VbGetArchPropertyInt(const char* name) {
  if (!strcasecmp(name, "recovery_reason")) {
    return VbGetRecoveryReason();
  } else if (!strcasecmp(name, "fmap_base")) {
    return ReadFdtInt("fmap_base");
  } else if (!strcasecmp(name, "devsw_boot")) {
    return ReadFdtInt("developer_sw");
  } else if (!strcasecmp(name, "recoverysw_boot")) {
    return ReadFdtInt("recovery_sw");
  } else if (!strcasecmp(name, "wpsw_boot")) {
    return ReadFdtInt("write_protect_sw");
  } else if (!strcasecmp(name, "devsw_cur")) {
    return VbGetVarGpio("developer_sw");
  } else if (!strcasecmp(name, "recoverysw_cur")) {
    return VbGetVarGpio("recovery_sw");
  } else if (!strcasecmp(name, "wpsw_cur")) {
    return VbGetVarGpio("write_protect_sw");
  } else if (!strcasecmp(name, "recoverysw_ec_boot")) {
    return 0;
  } else
    return -1;
}

const char* VbGetArchPropertyString(const char* name, char* dest, int size) {
  char *str = NULL;
  char *rv = NULL;
  size_t block_size;
  uint8_t *binf, mainfw_act, mainfw_type, ecfw_act;

  /* Properties from fdt */
  if (!strcasecmp(name, "ro_fwid")) {
    str = ReadFdtString("frid");
  } else if (!strcasecmp(name, "hwid")) {
    str = ReadFdtString("hwid");
  } else if (!strcasecmp(name, "fwid")) {
    str = ReadFdtString("fwid");
  }

  if (str) {
    rv = StrCopy(dest, str, size);
    free(str);
    return rv;
  }

  /* Other properties */
  if (ReadFdtBlock("binf", (void**)&binf, &block_size))
    return NULL;
  mainfw_act  = binf[BINF_MAINFW_ACT];
  ecfw_act    = binf[BINF_ECFW_ACT];
  mainfw_type = binf[BINF_MAINFW_TYPE];
  free(binf);

  if (!strcasecmp(name,"arch")) {
    return StrCopy(dest, "arm", size);
  } else if (!strcasecmp(name,"mainfw_act")) {
    switch(mainfw_act) {
      case 0:
        return StrCopy(dest, "recovery", size);
      case 1:
        return StrCopy(dest, "A", size);
      case 2:
        return StrCopy(dest, "B", size);
      default:
        return NULL;
    }
  } else if (!strcasecmp(name,"mainfw_type")) {
    switch(mainfw_type) {
    case BINF3_RECOVERY:
      return StrCopy(dest, "recovery", size);
    case BINF3_NORMAL:
      return StrCopy(dest, "normal", size);
    case BINF3_DEVELOPER:
      return StrCopy(dest, "developer", size);
    default:
      return NULL;
    }
  } else if (!strcasecmp(name,"ecfw_act")) {
    switch(ecfw_act) {
      case 0:
        return StrCopy(dest, "RO", size);
      case 1:
        return StrCopy(dest, "RW", size);
      default:
        return NULL;
    }
  }
  return NULL;
}

int VbSetArchPropertyInt(const char* name, int value) {
  /* All is handled in arch independent fashion */
  return -1;
}

int VbSetArchPropertyString(const char* name, const char* value) {
  /* All is handled in arch independent fashion */
  return -1;
}

int VbArchInit(void)
{
  return 0;
}
