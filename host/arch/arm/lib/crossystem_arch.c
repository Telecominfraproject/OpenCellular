/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "vboot_common.h"
#include "vboot_nvstorage.h"
#include "host_common.h"
#include "crossystem_arch.h"

#define CONFIG_LENGTH_FMAP 0x400

#define offsetof(struct_name, field) ((int) &(((struct_name*)0)->field))

/* This is used to keep u-boot and kernel in sync */
#define SHARED_MEM_VERSION 1
#define SHARED_MEM_SIGNATURE "CHROMEOS"

typedef struct {
  const char *signal_name;
  unsigned gpio_number;
  uint8_t  needs_inversion;
}  GpioMap;

static const GpioMap vb_gpio_map_kaen[] = {
  {"recoverysw_cur", 56, 1},
  {"devsw_cur", 168},
  {"wpsw_cur", 59, 1},
};

/* This is map is for kaen, function to gpio number mapping */

/*
 * This structure has been copied from u-boot-next
 * files/lib/chromeos/os_storage.c. Keep it in sync until a better interface
 * is implemented.
 */
typedef struct {
  uint32_t total_size;
  uint8_t  signature[10];
  uint16_t version;
  uint64_t nvcxt_lba;
  uint16_t vbnv[2];
  uint8_t  nvcxt_cache[VBNV_BLOCK_SIZE];
  uint8_t  write_protect_sw;
  uint8_t  recovery_sw;
  uint8_t  developer_sw;
  uint8_t  binf[5];
  uint32_t chsw;
  uint8_t  hwid[256];
  uint8_t  fwid[256];
  uint8_t  frid[256];
  uint32_t fmap_base;
  uint8_t  shared_data_body[CONFIG_LENGTH_FMAP];
} __attribute__((packed)) VbSharedMem;

typedef struct {
  const char *cs_field_name;
  const void *cs_value;
} VbVarInfo;

static VbSharedMem shared_memory;
static const char *blob_name = "/sys/kernel/debug/chromeos_arm";
static VbVarInfo vb_cs_map[] = {
  {"hwid", &shared_memory.hwid},
  {"fwid", &shared_memory.fwid},
  {"ro_fwid", &shared_memory.frid},
  {"devsw_boot", &shared_memory.developer_sw},
  {"recoverysw_boot", &shared_memory.recovery_sw},
  {"wpsw_boot", &shared_memory.write_protect_sw},
  {"recovery_reason", &shared_memory.binf[4]},
};

static int VbGetGpioStatus(unsigned gpio_number) {
  char const *gpio_name_format = "/sys/class/gpio/gpio%d/value";
  char gpio_name [80];
  char gpio_value[3]; /* gpio value file contains the value and the CR */
  FILE *gpio_file;
  int rv = -1, size;

  snprintf(gpio_name, sizeof(gpio_name), gpio_name_format, gpio_number);
  gpio_file = fopen(gpio_name, "r");
  if (!gpio_file) {
    fprintf(stderr, "%s: failed to open %s\n", __FUNCTION__, gpio_name);
    return -1;
  }

  size = fread(&gpio_value, 1, sizeof(gpio_value), gpio_file);
  if ((size == 2) && (gpio_value[1] == 0xa)) {
    rv = gpio_value[0] - '0'; /* we expect 0 or 1 only */
  } else {
    fprintf(stderr,  "%s: failed to read %s, got %d\n",
            __FUNCTION__, gpio_name, size);
  }
  fclose(gpio_file);
  return rv;
}

static int VbGetVarGpio(const char* name) {
  int i;
  const GpioMap* pmap;

  for (i = 0, pmap = vb_gpio_map_kaen;
       i < ARRAY_SIZE(vb_gpio_map_kaen);
       i++, pmap++) {
    if (!strcmp(name, pmap->signal_name))
      return VbGetGpioStatus(pmap->gpio_number) ^ pmap->needs_inversion;
  }
  return 2;  /* means not found */
}

static int VbReadSharedMemory(void) {
  FILE *data_file = NULL;
  int rv = -1;
  int size;

  do {
    data_file = fopen(blob_name, "rb");
    if (!data_file) {
      fprintf(stderr, "%s: failed to open %s\n", __FUNCTION__, blob_name);
      break;
    }
    size = fread(&shared_memory, 1, sizeof(shared_memory), data_file);
    if ((size != sizeof(shared_memory)) || (size != shared_memory.total_size)) {
      fprintf(stderr,  "%s: failed to read shared memory: got %d bytes, "
              "expected %d, should have been %d\n",
              __FUNCTION__,
              size,
              sizeof(shared_memory),
              shared_memory.total_size);
      break;
    }

    if (strcmp((const char*)shared_memory.signature, SHARED_MEM_SIGNATURE)) {
      fprintf(stderr, "%s: signature verification failed\n", __FUNCTION__);
      break;
    }

    if (shared_memory.version != SHARED_MEM_VERSION) {
      fprintf(stderr, "%s: version mismatch: %d != %d\n",
              __FUNCTION__, shared_memory.version, SHARED_MEM_VERSION);
      break;
    }
    rv = 0;
  } while (0);

  if (data_file)
    fclose(data_file);

  return rv;
}

/* Retrieve the address of an entity in the shared memory based on the entity
 * name, as described in vb_cs_map table.
 *
 * Return NULL if the entity name is not found in the table.
 */
static const void* VbGetVarAuto(const char* name) {
  int i;
  VbVarInfo *pi;

  for (i = 0, pi = vb_cs_map; i < ARRAY_SIZE(vb_cs_map); i++, pi++) {
    if (strcmp(pi->cs_field_name, name)) continue;
    return pi->cs_value;
  }
  return NULL;
}

int VbReadNvStorage(VbNvContext* vnc) {
  Memcpy(vnc->raw, shared_memory.nvcxt_cache, sizeof(vnc->raw));
  return 0;
}

int VbWriteNvStorage(VbNvContext* vnc) {
  FILE *data_file = NULL;
  int rv = -1;
  int size;

  do {
    data_file = fopen(blob_name, "w");
    if (!data_file) {
      fprintf(stderr, "%s: failed to open %s\n", __FUNCTION__, blob_name);
      break;
    }
    size = fwrite(vnc->raw, 1, sizeof(vnc->raw), data_file);
    if (size != sizeof(vnc->raw)) {
      fprintf(stderr,  "%s: failed to write shared memory (%d)\n",
              __FUNCTION__, size);
      break;
    }
    rv = 0;
  } while (0);

  if (data_file)
    fclose(data_file);

  return rv;
}

VbSharedDataHeader *VbSharedDataRead(void) {
  /* don't need this malloc/copy, but have to do it to comply with the
   * wrapper.
   */
  VbSharedDataHeader *p = Malloc(sizeof(*p));
  Memcpy(p, shared_memory.shared_data_body, sizeof(*p));
  return p;
}

int VbGetArchPropertyInt(const char* name) {

  const uint8_t *value;
  int rv;

  value = VbGetVarAuto(name);

  if (value) return (int) *value;

  rv = VbGetVarGpio(name);
  if (rv <= 1) return rv;

  if (!strcasecmp(name,"fmap_base")) {
    return shared_memory.fmap_base;
  }

  return -1;
}

const char* VbGetArchPropertyString(const char* name, char* dest, int size) {
  const char* value = VbGetVarAuto(name);
  if (value) return StrCopy(dest, value, size);

  if (!strcasecmp(name,"arch")) {
    return StrCopy(dest, "arm", size);
  } else if (!strcasecmp(name,"mainfw_act")) {
    switch(shared_memory.binf[2]) {
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
    switch(shared_memory.binf[3]) {
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
    switch(shared_memory.binf[2]) {
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
  return VbReadSharedMemory();
}
