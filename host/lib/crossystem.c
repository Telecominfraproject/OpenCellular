/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

#include "host_common.h"

#include "crossystem.h"
#include "crossystem_arch.h"
#include "utility.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"
#include "vboot_struct.h"

/* Filename for kernel command line */
#define KERNEL_CMDLINE_PATH "/proc/cmdline"

/* Fields that GetVdatString() can get */
typedef enum VdatStringField {
  VDAT_STRING_TIMERS = 0,           /* Timer values */
  VDAT_STRING_LOAD_FIRMWARE_DEBUG,  /* LoadFirmware() debug information */
  VDAT_STRING_LOAD_KERNEL_DEBUG     /* LoadKernel() debug information */
} VdatStringField;


/* Fields that GetVdatInt() can get */
typedef enum VdatIntField {
  VDAT_INT_FLAGS = 0,                /* Flags */
  VDAT_INT_FW_VERSION_TPM,           /* Current firmware version in TPM */
  VDAT_INT_KERNEL_VERSION_TPM,       /* Current kernel version in TPM */
  VDAT_INT_TRIED_FIRMWARE_B,         /* Tried firmware B due to fwb_tries */
  VDAT_INT_KERNEL_KEY_VERIFIED,      /* Kernel key verified using
                                      * signature, not just hash */
  VDAT_INT_RECOVERY_REASON           /* Recovery reason for current boot */
} VdatIntField;


/* Masks for kern_nv usage by kernel */
#define KERN_NV_FWUPDATE_TRIES_MASK 0x0000000F


/* Return true if the FWID starts with the specified string. */
int FwidStartsWith(const char *start) {
  char fwid[128];
  if (!VbGetSystemPropertyString("fwid", fwid, sizeof(fwid)))
    return 0;

  return 0 == strncmp(fwid, start, strlen(start));
}


int VbGetNvStorage(VbNvParam param) {
  VbNvContext vnc;
  uint32_t value;
  int retval;

  /* TODO: locking around NV access */

  if (0 != VbReadNvStorage(&vnc))
    return -1;
  if (0 != VbNvSetup(&vnc))
    return -1;
  retval = VbNvGet(&vnc, param, &value);
  if (0 != VbNvTeardown(&vnc))
    return -1;
  if (0 != retval)
    return -1;

  /* TODO: If vnc.raw_changed, attempt to reopen NVRAM for write and
   * save the new defaults.  If we're able to, log. */
  /* TODO: release lock */

  return (int)value;
}


int VbSetNvStorage(VbNvParam param, int value) {
  VbNvContext vnc;
  int retval = -1;
  int i;

  if (0 != VbReadNvStorage(&vnc))
    return -1;

  if (0 != VbNvSetup(&vnc))
    goto VbSetNvCleanup;
  i = VbNvSet(&vnc, param, (uint32_t)value);
  if (0 != VbNvTeardown(&vnc))
    goto VbSetNvCleanup;
  if (0 != i)
    goto VbSetNvCleanup;

  if (vnc.raw_changed) {
    if (0 != VbWriteNvStorage(&vnc))
      goto VbSetNvCleanup;
  }

  /* Success */
  retval = 0;

VbSetNvCleanup:
  /* TODO: release lock */
  return retval;
}


/* Determine whether OS-level debugging should be allowed.  Passed the
 * destination and its size.  Returns 1 if yes, 0 if no, -1 if error. */
int VbGetCrosDebug(void) {
  FILE* f = NULL;
  char buf[4096] = "";
  char *t, *saveptr;

  /* Try reading firmware type. */
  if (VbGetArchPropertyString("mainfw_type", buf, sizeof(buf))) {
    if (0 == strcmp(buf, "recovery"))
      return 0;  /* Recovery mode never allows debug. */
    else if (0 == strcmp(buf, "developer"))
      return 1;  /* Developer firmware always allows debug. */
  }

  /* Normal new firmware, older ChromeOS firmware, or non-Chrome firmware.
   * For all these cases, check /proc/cmdline for cros_[no]debug. */
  f = fopen(KERNEL_CMDLINE_PATH, "rt");
  if (f) {
    if (NULL == fgets(buf, sizeof(buf), f))
      *buf = 0;
    fclose(f);
  }
  for (t = strtok_r(buf, " ", &saveptr); t; t=strtok_r(NULL, " ", &saveptr)) {
    if (0 == strcmp(t, "cros_debug"))
      return 1;
    else if (0 == strcmp(t, "cros_nodebug"))
      return 0;
  }

  /* Normal new firmware or older Chrome OS firmware allows debug if the
   * dev switch is on. */
  if (1 == VbGetSystemPropertyInt("devsw_boot"))
    return 1;

  /* All other cases disallow debug. */
  return 0;
}


char* GetVdatLoadFirmwareDebug(char* dest, int size,
                               const VbSharedDataHeader* sh) {
  snprintf(dest, size,
           "Check A result=%d\n"
           "Check B result=%d\n"
           "Firmware index booted=0x%02x\n"
           "TPM combined version at start=0x%08x\n"
           "Lowest combined version from firmware=0x%08x\n",
           sh->check_fw_a_result,
           sh->check_fw_b_result,
           sh->firmware_index,
           sh->fw_version_tpm_start,
           sh->fw_version_lowest);
  return dest;
}


#define TRUNCATED "\n(truncated)\n"

char* GetVdatLoadKernelDebug(char* dest, int size,
                             const VbSharedDataHeader* sh) {
  int used = 0;
  int first_call_tracked = 0;
  int call;

  /* Make sure we have space for truncation warning */
  if (size < strlen(TRUNCATED) + 1)
    return NULL;
  size -= strlen(TRUNCATED) + 1;

  used += snprintf(
      dest + used, size - used,
      "Calls to LoadKernel()=%d\n",
      sh->lk_call_count);
  if (used > size)
    goto LoadKernelDebugExit;

  /* Report on the last calls */
  if (sh->lk_call_count > VBSD_MAX_KERNEL_CALLS)
    first_call_tracked = sh->lk_call_count - VBSD_MAX_KERNEL_CALLS;
  for (call = first_call_tracked; call < sh->lk_call_count; call++) {
    const VbSharedDataKernelCall* shc =
        sh->lk_calls + (call & (VBSD_MAX_KERNEL_CALLS - 1));
    int first_part_tracked = 0;
    int part;

    used += snprintf(
        dest + used, size - used,
        "Call %d:\n"
        "  Boot flags=0x%02x\n"
        "  Boot mode=%d\n"
        "  Test error=%d\n"
        "  Return code=%d\n"
        "  Debug flags=0x%02x\n"
        "  Drive sectors=%" PRIu64 "\n"
        "  Sector size=%d\n"
        "  Check result=%d\n"
        "  Kernel partitions found=%d\n",
        call + 1,
        shc->boot_flags,
        shc->boot_mode,
        shc->test_error_num,
        shc->return_code,
        shc->flags,
        shc->sector_count,
        shc->sector_size,
        shc->check_result,
        shc->kernel_parts_found);
    if (used > size)
      goto LoadKernelDebugExit;

    /* If we found too many partitions, only prints ones where the
     * structure has info. */
    if (shc->kernel_parts_found > VBSD_MAX_KERNEL_PARTS)
      first_part_tracked = shc->kernel_parts_found - VBSD_MAX_KERNEL_PARTS;

    /* Report on the partitions checked */
    for (part = first_part_tracked; part < shc->kernel_parts_found; part++) {
      const VbSharedDataKernelPart* shp =
          shc->parts + (part & (VBSD_MAX_KERNEL_PARTS - 1));

      used += snprintf(
          dest + used, size - used,
          "  Kernel %d:\n"
          "    GPT index=%d\n"
          "    Start sector=%" PRIu64 "\n"
          "    Sector count=%" PRIu64 "\n"
          "    Combined version=0x%08x\n"
          "    Check result=%d\n"
          "    Debug flags=0x%02x\n",
          part + 1,
          shp->gpt_index,
          shp->sector_start,
          shp->sector_count,
          shp->combined_version,
          shp->check_result,
          shp->flags);
      if (used > size)
        goto LoadKernelDebugExit;
    }
  }

LoadKernelDebugExit:

  /* Warn if data was truncated; we left space for this above. */
  if (used > size)
    strcat(dest, TRUNCATED);

  return dest;
}


char* GetVdatString(char* dest, int size, VdatStringField field)
{
  VbSharedDataHeader* sh = VbSharedDataRead();
  char* value = dest;

  if (!sh)
    return NULL;

  switch (field) {
    case VDAT_STRING_TIMERS:
      snprintf(dest, size,
               "LFS=%" PRIu64 ",%" PRIu64
               " LF=%" PRIu64 ",%" PRIu64
               " LK=%" PRIu64 ",%" PRIu64,
               sh->timer_load_firmware_start_enter,
               sh->timer_load_firmware_start_exit,
               sh->timer_load_firmware_enter,
               sh->timer_load_firmware_exit,
               sh->timer_load_kernel_enter,
               sh->timer_load_kernel_exit);
      break;

    case VDAT_STRING_LOAD_FIRMWARE_DEBUG:
      value = GetVdatLoadFirmwareDebug(dest, size, sh);
      break;

    case VDAT_STRING_LOAD_KERNEL_DEBUG:
      value = GetVdatLoadKernelDebug(dest, size, sh);
      break;

    default:
      value = NULL;
      break;
  }

  Free(sh);
  return value;
}


int GetVdatInt(VdatIntField field) {
  VbSharedDataHeader* sh = VbSharedDataRead();
  int value = -1;

  if (!sh)
    return -1;

  switch (field) {
    case VDAT_INT_FLAGS:
      value = (int)sh->flags;
      break;
    case VDAT_INT_FW_VERSION_TPM:
      value = (int)sh->fw_version_tpm;
      break;
    case VDAT_INT_KERNEL_VERSION_TPM:
      value = (int)sh->kernel_version_tpm;
      break;
    case VDAT_INT_TRIED_FIRMWARE_B:
      value = (sh->flags & VBSD_FWB_TRIED ? 1 : 0);
      break;
    case VDAT_INT_KERNEL_KEY_VERIFIED:
      value = (sh->flags & VBSD_KERNEL_KEY_VERIFIED ? 1 : 0);
      break;
    case VDAT_INT_RECOVERY_REASON:
      /* Field added in struct version 2 */
      if (sh->struct_version >= 2)
        value = sh->recovery_reason;
      break;
  }

  Free(sh);
  return value;
}


int VbGetSystemPropertyInt(const char* name) {
  int value = -1;

  /* Check architecture-dependent properties first */
  value = VbGetArchPropertyInt(name);
  if (-1 != value)
    return value;

  /* NV storage values */
  else if (!strcasecmp(name,"kern_nv")) {
    value = VbGetNvStorage(VBNV_KERNEL_FIELD);
  } else if (!strcasecmp(name,"nvram_cleared")) {
    value = VbGetNvStorage(VBNV_KERNEL_SETTINGS_RESET);
  } else if (!strcasecmp(name,"vbtest_errfunc")) {
    value = VbGetNvStorage(VBNV_TEST_ERROR_FUNC);
  } else if (!strcasecmp(name,"vbtest_errno")) {
    value = VbGetNvStorage(VBNV_TEST_ERROR_NUM);
  } else if (!strcasecmp(name,"recovery_request")) {
    value = VbGetNvStorage(VBNV_RECOVERY_REQUEST);
  } else if (!strcasecmp(name,"dbg_reset")) {
    value = VbGetNvStorage(VBNV_DEBUG_RESET_MODE);
  } else if (!strcasecmp(name,"fwb_tries")) {
    value = VbGetNvStorage(VBNV_TRY_B_COUNT);
  } else if (!strcasecmp(name,"fwupdate_tries")) {
    value = VbGetNvStorage(VBNV_KERNEL_FIELD);
    if (value != -1)
      value &= KERN_NV_FWUPDATE_TRIES_MASK;
  } else if (!strcasecmp(name,"loc_idx")) {
    value = VbGetNvStorage(VBNV_LOCALIZATION_INDEX);
  }
  /* Other parameters */
  else if (!strcasecmp(name,"cros_debug")) {
    value = VbGetCrosDebug();
  } else if (!strcasecmp(name,"vdat_flags")) {
    value = GetVdatInt(VDAT_INT_FLAGS);
  } else if (!strcasecmp(name,"tpm_fwver")) {
    value = GetVdatInt(VDAT_INT_FW_VERSION_TPM);
  } else if (!strcasecmp(name,"tpm_kernver")) {
    value = GetVdatInt(VDAT_INT_KERNEL_VERSION_TPM);
  } else if (!strcasecmp(name,"tried_fwb")) {
    value = GetVdatInt(VDAT_INT_TRIED_FIRMWARE_B);
  } else if (!strcasecmp(name,"recovery_reason")) {
    value = GetVdatInt(VDAT_INT_RECOVERY_REASON);
  }

  return value;
}


const char* VbGetSystemPropertyString(const char* name, char* dest, int size) {
  /* Check architecture-dependent properties first */
  if (VbGetArchPropertyString(name, dest, size))
    return dest;

  if (!strcasecmp(name,"kernkey_vfy")) {
    switch(GetVdatInt(VDAT_INT_KERNEL_KEY_VERIFIED)) {
      case 0:
        return "hash";
      case 1:
        return "sig";
      default:
        return NULL;
    }
  } else if (!strcasecmp(name, "vdat_timers")) {
    return GetVdatString(dest, size, VDAT_STRING_TIMERS);
  } else if (!strcasecmp(name, "vdat_lfdebug")) {
    return GetVdatString(dest, size, VDAT_STRING_LOAD_FIRMWARE_DEBUG);
  } else if (!strcasecmp(name, "vdat_lkdebug")) {
    return GetVdatString(dest, size, VDAT_STRING_LOAD_KERNEL_DEBUG);
  }

  return NULL;
}


int VbSetSystemPropertyInt(const char* name, int value) {
  /* Check architecture-dependent properties first */

  if (0 == VbSetArchPropertyInt(name, value))
    return 0;

  /* NV storage values */
  if (!strcasecmp(name,"nvram_cleared")) {
    /* Can only clear this flag; it's set inside the NV storage library. */
    return VbSetNvStorage(VBNV_KERNEL_SETTINGS_RESET, 0);
  } else if (!strcasecmp(name,"kern_nv")) {
    return VbSetNvStorage(VBNV_KERNEL_FIELD, value);
  } else if (!strcasecmp(name,"vbtest_errfunc")) {
    return VbSetNvStorage(VBNV_TEST_ERROR_FUNC, value);
  } else if (!strcasecmp(name,"vbtest_errno")) {
    return VbSetNvStorage(VBNV_TEST_ERROR_NUM, value);
  } else if (!strcasecmp(name,"recovery_request")) {
    return VbSetNvStorage(VBNV_RECOVERY_REQUEST, value);
  } else if (!strcasecmp(name,"dbg_reset")) {
    return VbSetNvStorage(VBNV_DEBUG_RESET_MODE, value);
  } else if (!strcasecmp(name,"fwb_tries")) {
    return VbSetNvStorage(VBNV_TRY_B_COUNT, value);
  } else if (!strcasecmp(name,"fwupdate_tries")) {
    int kern_nv = VbGetNvStorage(VBNV_KERNEL_FIELD);
    if (kern_nv == -1)
      return -1;
    kern_nv &= ~KERN_NV_FWUPDATE_TRIES_MASK;
    kern_nv |= (value & KERN_NV_FWUPDATE_TRIES_MASK);
    return VbSetNvStorage(VBNV_KERNEL_FIELD, kern_nv);
  } else if (!strcasecmp(name,"loc_idx")) {
    return VbSetNvStorage(VBNV_LOCALIZATION_INDEX, value);
  }

  return -1;
}


int VbSetSystemPropertyString(const char* name, const char* value) {
  /* Chain to architecture-dependent properties */
  return VbSetArchPropertyString(name, value);
}
