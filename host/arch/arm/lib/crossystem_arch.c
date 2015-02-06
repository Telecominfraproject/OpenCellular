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
#include "vboot_nvstorage.h"
#include "host_common.h"
#include "crossystem.h"
#include "crossystem_arch.h"

#define MOSYS_PATH "/usr/sbin/mosys"

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

typedef struct PlatformFamily {
  const char* compatible_string; /* Last string in FDT compatible entry */
  const char* platform_string;   /* String to return */
} PlatformFamily;

/* Array of platform family names, terminated with a NULL entry */
const PlatformFamily platform_family_array[] = {
  {"nvidia,tegra124", "Tegra5"},
  {"nvidia,tegra250", "Tegra2"},
  {"nvidia,tegra20", "Tegra2"},
  {"ti,omap4", "OMAP4"},
  {"ti,omap3", "OMAP3"},
  {"samsung,exynos4210", "EXYNOS4"},
  {"samsung,exynos5250", "EXYNOS5"},
  {"samsung,exynos5420", "EXYNOS5"},
  {"qcom,ipq8064", "IPQ8064"},
  /* Terminate with NULL entry */
  {NULL, NULL}
};

static int FindEmmcDev(void) {
  int mmcblk;
  unsigned value;
  char filename[FNAME_SIZE];
  for (mmcblk = 0; mmcblk < MAX_NMMCBLK; mmcblk++) {
    /* Get first non-removable mmc block device */
    snprintf(filename, sizeof(filename), "/sys/block/mmcblk%d/removable",
              mmcblk);
    if (ReadFileInt(filename, &value) < 0)
      continue;
    if (value == 0)
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

  if (fread(&data, 1, sizeof(data), file) != sizeof(data)) {
    fprintf(stderr, "Unable to read FDT property %s\n", property);
    return E_FILEOP;
  }
  fclose(file);

  if (value)
    *value = ntohl(data); /* FDT is network byte order */

  return 0;
}

static int ReadFdtInt(const char *property) {
  int value = 0;
  if (ReadFdtValue(property, &value))
    return E_FAIL;
  return value;
}

static void GetFdtPropertyPath(const char *property, char *path, size_t size) {
  if (property[0] == '/')
    StrCopy(path, property, size);
  else
    snprintf(path, size, FDT_BASE_PATH "/%s", property);
}

static int FdtPropertyExist(const char *property) {
  char filename[FNAME_SIZE];
  struct stat file_status;

  GetFdtPropertyPath(property, filename, sizeof(filename));
  if (!stat(filename, &file_status))
    return 1; // It exists!
  else
    return 0; // It does not exist or some error happened.
}

static int ReadFdtBlock(const char *property, void **block, size_t *size) {
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

static char * ReadFdtString(const char *property) {
  void *str = NULL;
  /* Do not need property size */
  ReadFdtBlock(property, &str, 0);
  return (char *)str;
}

static char * ReadFdtPlatformFamily(void) {
  char *compat = NULL;
  char *s;
  const PlatformFamily* p;
  size_t size = 0;
  int slen;

  if(ReadFdtBlock(FDT_COMPATIBLE_PATH, (void **)&compat, &size))
    return NULL;

  if (size > 0)
    compat[size-1] = 0;

  /* Check each null separated string in compatible against the family array */
  s = compat;
  while ((s-compat) < size) {
    slen = strlen(s);
    for (p = platform_family_array; p->compatible_string; p++) {
      if (!strcmp(s, p->compatible_string)) {
        free(compat);
        return strdup(p->platform_string);
      }
    }
    s += slen + 1;
  }

  /* No recognized 'compatible' entry found */
  free(compat);
  return NULL;
}

static int VbGetPlatformGpioStatus(const char* name) {
  char gpio_name[FNAME_SIZE];
  unsigned value;

  snprintf(gpio_name, sizeof(gpio_name), "%s/%s/value",
           PLATFORM_DEV_PATH, name);
  if (ReadFileInt(gpio_name, &value) < 0)
    return -1;

  return (int)value;
}

static int VbGetGpioStatus(unsigned gpio_number) {
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

static int VbGetVarGpio(const char* name) {
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
   * GPIO for now, at the risk that one day we might actually want to read
   * from a GPIO port 0.  We should figure out how to represent "non-exist"
   * properly.
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

static int ExecuteMosys(char * const argv[], char *buf, size_t bufsize) {
  int status, mosys_to_crossystem[2];
  pid_t pid;
  ssize_t n;

  if (pipe(mosys_to_crossystem) < 0) {
    VBDEBUG(("pipe() error\n"));
    return -1;
  }

  if ((pid = fork()) < 0) {
    VBDEBUG(("fork() error\n"));
    close(mosys_to_crossystem[0]);
    close(mosys_to_crossystem[1]);
    return -1;
  } else if (!pid) {  /* Child */
    close(mosys_to_crossystem[0]);
    /* Redirect pipe's write-end to mosys' stdout */
    if (STDOUT_FILENO != mosys_to_crossystem[1]) {
      if (dup2(mosys_to_crossystem[1], STDOUT_FILENO) != STDOUT_FILENO) {
        VBDEBUG(("stdout dup2() failed (mosys)\n"));
        close(mosys_to_crossystem[1]);
        exit(1);
      }
    }
    /* Execute mosys */
    execv(MOSYS_PATH, argv);
    /* We shouldn't be here; exit now! */
    VBDEBUG(("execv() of mosys failed\n"));
    close(mosys_to_crossystem[1]);
    exit(1);
  } else {  /* Parent */
    close(mosys_to_crossystem[1]);
    if (bufsize) {
      bufsize--;  /* Reserve 1 byte for '\0' */
      while ((n = read(mosys_to_crossystem[0], buf, bufsize)) > 0) {
        buf += n;
        bufsize -= n;
      }
      *buf = '\0';
    } else {
      n = 0;
    }
    close(mosys_to_crossystem[0]);
    if (n < 0)
      VBDEBUG(("read() error while reading output from mosys\n"));
    if (waitpid(pid, &status, 0) < 0 || status) {
      VBDEBUG(("waitpid() or mosys error\n"));
      fprintf(stderr, "waitpid() or mosys error\n");
      return -1;
    }
    if (n < 0)
      return -1;
  }
  return 0;
}

static int VbReadNvStorage_mosys(VbNvContext* vnc) {
  char hexstring[VBNV_BLOCK_SIZE * 2 + 32];  /* Reserve extra 32 bytes */
  char * const argv[] = {
    MOSYS_PATH, "nvram", "vboot", "read", NULL
  };
  char hexdigit[3];
  int i;

  if (ExecuteMosys(argv, hexstring, sizeof(hexstring)))
    return -1;
  hexdigit[2] = '\0';
  for (i = 0; i < VBNV_BLOCK_SIZE; i++) {
    hexdigit[0] = hexstring[i * 2];
    hexdigit[1] = hexstring[i * 2 + 1];
    vnc->raw[i] = strtol(hexdigit, NULL, 16);
  }
  return 0;
}

static int VbWriteNvStorage_mosys(VbNvContext* vnc) {
  char hexstring[VBNV_BLOCK_SIZE * 2 + 1];
  char * const argv[] = {
    MOSYS_PATH, "nvram", "vboot", "write", hexstring, NULL
  };
  int i;

  for (i = 0; i < VBNV_BLOCK_SIZE; i++)
    snprintf(hexstring + i * 2, 3, "%02x", vnc->raw[i]);
  hexstring[sizeof(hexstring) - 1] = '\0';
  if (ExecuteMosys(argv, NULL, 0))
    return -1;
  return 0;
}

static int VbReadNvStorage_disk(VbNvContext* vnc) {
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

  if (size != sizeof(vnc->raw) || (size + offset > SECTOR_SIZE))
    return E_FAIL;

  nvctx_fd = open(nvctx_path, O_RDONLY);
  if (nvctx_fd == -1) {
    fprintf(stderr, "%s: failed to open %s\n", __FUNCTION__, nvctx_path);
    goto out;
  }
  lseek(nvctx_fd, lba * SECTOR_SIZE, SEEK_SET);

  rv = read(nvctx_fd, sector, SECTOR_SIZE);
  if (size <= 0) {
    fprintf(stderr, "%s: failed to read nvctx from device %s\n",
            __FUNCTION__, nvctx_path);
    goto out;
  }
  Memcpy(vnc->raw, sector+offset, size);
  rv = 0;

out:
  if (nvctx_fd > 0)
    close(nvctx_fd);

  return rv;
}

static int VbWriteNvStorage_disk(VbNvContext* vnc) {
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

  if (size != sizeof(vnc->raw) || (size + offset > SECTOR_SIZE))
    return E_FAIL;

  do {
    nvctx_fd = open(nvctx_path, O_RDWR);
    if (nvctx_fd == -1) {
      fprintf(stderr, "%s: failed to open %s\n", __FUNCTION__, nvctx_path);
      break;
    }
    lseek(nvctx_fd, lba * SECTOR_SIZE, SEEK_SET);
    rv = read(nvctx_fd, sector, SECTOR_SIZE);
    if (rv <= 0) {
      fprintf(stderr, "%s: failed to read nvctx from device %s\n",
              __FUNCTION__, nvctx_path);
      break;
    }
    Memcpy(sector+offset, vnc->raw, size);
    lseek(nvctx_fd, lba * SECTOR_SIZE, SEEK_SET);
    rv = write(nvctx_fd, sector, SECTOR_SIZE);
    if (rv <= 0) {
      fprintf(stderr,  "%s: failed to write nvctx to device %s\n",
              __FUNCTION__, nvctx_path);
      break;
    }
#ifndef HAVE_MACOS
    /* Must flush buffer cache here to make sure it goes to disk */
    rv = ioctl(nvctx_fd, BLKFLSBUF, 0);
    if (rv < 0) {
      fprintf(stderr,  "%s: failed to flush nvctx to device %s\n",
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

int VbReadNvStorage(VbNvContext* vnc) {
  /* Default to disk for older firmware which does not provide storage type */
  char *media;
  if (!FdtPropertyExist(FDT_NVSTORAGE_TYPE_PROP))
    return VbReadNvStorage_disk(vnc);
  media = ReadFdtString(FDT_NVSTORAGE_TYPE_PROP);
  if (!strcmp(media, "disk"))
    return VbReadNvStorage_disk(vnc);
  if (!strcmp(media, "mkbp") || !strcmp(media, "flash"))
    return VbReadNvStorage_mosys(vnc);
  return -1;
}

int VbWriteNvStorage(VbNvContext* vnc) {
  /* Default to disk for older firmware which does not provide storage type */
  char *media;
  if (!FdtPropertyExist(FDT_NVSTORAGE_TYPE_PROP))
    return VbWriteNvStorage_disk(vnc);
  media = ReadFdtString(FDT_NVSTORAGE_TYPE_PROP);
  if (!strcmp(media, "disk"))
    return VbWriteNvStorage_disk(vnc);
  if (!strcmp(media, "mkbp") || !strcmp(media, "flash"))
    return VbWriteNvStorage_mosys(vnc);
  return -1;
}

VbSharedDataHeader *VbSharedDataRead(void) {
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

int VbGetArchPropertyInt(const char* name) {
  if (!strcasecmp(name, "fmap_base")) {
    return ReadFdtInt("fmap-offset");
  } else if (!strcasecmp(name, "devsw_cur")) {
    /* Systems with virtual developer switches return at-boot value */
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
    /* Try finding the GPIO through the chromeos_arm platform device first. */
    value = VbGetPlatformGpioStatus("write-protect");
    if (value != -1)
      return value;
    return VbGetVarGpio("write-protect-switch");
  } else if (!strcasecmp(name, "recoverysw_ec_boot"))
    /* TODO: read correct value using ectool */
    return 0;
  else
    return -1;
}

const char* VbGetArchPropertyString(const char* name, char* dest,
                                    size_t size) {
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
  else if (!strcasecmp(name, "ddr_type"))
    prop = "ddr-type";

  if (prop)
    str = ReadFdtString(prop);

  if (!strcasecmp(name, "platform_family"))
    str = ReadFdtPlatformFamily();

  if (str) {
      rv = StrCopy(dest, str, size);
      free(str);
      return rv;
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
