/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stddef.h>
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
  VDAT_STRING_LOAD_KERNEL_DEBUG,    /* LoadKernel() debug information */
  VDAT_STRING_MAINFW_ACT            /* Active main firmware */
} VdatStringField;


/* Fields that GetVdatInt() can get */
typedef enum VdatIntField {
  VDAT_INT_FLAGS = 0,                /* Flags */
  VDAT_INT_HEADER_VERSION,           /* Header version for VbSharedData */
  VDAT_INT_DEVSW_BOOT,               /* Dev switch position at boot */
  VDAT_INT_DEVSW_VIRTUAL,            /* Dev switch is virtual */
  VDAT_INT_RECSW_BOOT,               /* Recovery switch position at boot */
  VDAT_INT_HW_WPSW_BOOT,             /* Hardware WP switch position at boot */
  VDAT_INT_SW_WPSW_BOOT,             /* Flash chip's WP setting at boot */

  VDAT_INT_FW_VERSION_TPM,           /* Current firmware version in TPM */
  VDAT_INT_KERNEL_VERSION_TPM,       /* Current kernel version in TPM */
  VDAT_INT_TRIED_FIRMWARE_B,         /* Tried firmware B due to fwb_tries */
  VDAT_INT_KERNEL_KEY_VERIFIED,      /* Kernel key verified using
                                      * signature, not just hash */
  VDAT_INT_RECOVERY_REASON,          /* Recovery reason for current boot */
  VDAT_INT_FW_BOOT2                  /* Firmware selection by vboot2 */
} VdatIntField;


/* Description of build options that may be specified on the
 * kernel command line. */
typedef enum VbBuildOption {
  VB_BUILD_OPTION_UNKNOWN,
  VB_BUILD_OPTION_DEBUG,
  VB_BUILD_OPTION_NODEBUG
} VbBuildOption;

static const char *fw_results[] = {"unknown", "trying", "success", "failure"};

/* Masks for kern_nv usage by kernel. */
#define KERN_NV_FWUPDATE_TRIES_MASK 0x0000000F
#define KERN_NV_BLOCK_DEVMODE_FLAG  0x00000010
/* If you want to use the remaining currently-unused bits in kern_nv
 * for something kernel-y, define a new field (the way we did for
 * fwupdate_tries).  Don't just modify kern_nv directly, because that
 * makes it too easy to accidentally corrupt other sub-fields. */
#define KERN_NV_CURRENTLY_UNUSED    0xFFFFFFE0

/* Return true if the FWID starts with the specified string. */
int FwidStartsWith(const char *start) {
  char fwid[VB_MAX_STRING_PROPERTY];
  if (!VbGetSystemPropertyString("fwid", fwid, sizeof(fwid)))
    return 0;

  return 0 == strncmp(fwid, start, strlen(start));
}

static int vnc_read;

int VbGetNvStorage(VbNvParam param) {
  uint32_t value;
  int retval;
  static VbNvContext cached_vnc;

  /* TODO: locking around NV access */
  if (!vnc_read) {
    if (0 != VbReadNvStorage(&cached_vnc))
      return -1;
    vnc_read = 1;
  }

  if (0 != VbNvSetup(&cached_vnc))
    return -1;
  retval = VbNvGet(&cached_vnc, param, &value);
  if (0 != VbNvTeardown(&cached_vnc))
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
    vnc_read = 0;
    if (0 != VbWriteNvStorage(&vnc))
      goto VbSetNvCleanup;
  }

  /* Success */
  retval = 0;

VbSetNvCleanup:
  /* TODO: release lock */
  return retval;
}

/*
 * Set a param value, and try to flag it for persistent backup.
 * It's okay if backup isn't supported. It's best-effort only.
 */
static int VbSetNvStorage_WithBackup(VbNvParam param, int value)
{
  int retval;
  retval = VbSetNvStorage(param, value);
  if (!retval)
    VbSetNvStorage(VBNV_BACKUP_NVRAM_REQUEST, 1);
  return retval;
}

/* Find what build/debug status is specified on the kernel command
 * line, if any. */
static VbBuildOption VbScanBuildOption(void) {
  FILE* f = NULL;
  char buf[4096] = "";
  char *t, *saveptr;
  const char *delimiters = " \r\n";

  f = fopen(KERNEL_CMDLINE_PATH, "r");
  if (NULL != f) {
    if (NULL == fgets(buf, sizeof(buf), f))
      buf[0] = 0;
    fclose(f);
  }
  for (t = strtok_r(buf, delimiters, &saveptr); t;
       t = strtok_r(NULL, delimiters, &saveptr)) {
    if (0 == strcmp(t, "cros_debug"))
      return VB_BUILD_OPTION_DEBUG;
    else if (0 == strcmp(t, "cros_nodebug"))
      return VB_BUILD_OPTION_NODEBUG;
  }

  return VB_BUILD_OPTION_UNKNOWN;
}


/* Determine whether the running OS image was built for debugging.
 * Returns 1 if yes, 0 if no or indeterminate. */
int VbGetDebugBuild(void) {
  return VB_BUILD_OPTION_DEBUG == VbScanBuildOption();
}


/* Determine whether OS-level debugging should be allowed.
 * Returns 1 if yes, 0 if no or indeterminate. */
int VbGetCrosDebug(void) {
  /* If the currently running system specifies its debug status, use
   * that in preference to other indicators. */
  VbBuildOption option = VbScanBuildOption();
  if (VB_BUILD_OPTION_DEBUG == option) {
      return 1;
  } else if (VB_BUILD_OPTION_NODEBUG == option) {
      return 0;
  }

  /* Command line is silent; allow debug if the dev switch is on. */
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
               sh->timer_vb_init_enter,
               sh->timer_vb_init_exit,
               sh->timer_vb_select_firmware_enter,
               sh->timer_vb_select_firmware_exit,
               sh->timer_vb_select_and_load_kernel_enter,
               sh->timer_vb_select_and_load_kernel_exit);
      break;

    case VDAT_STRING_LOAD_FIRMWARE_DEBUG:
      value = GetVdatLoadFirmwareDebug(dest, size, sh);
      break;

    case VDAT_STRING_LOAD_KERNEL_DEBUG:
      value = GetVdatLoadKernelDebug(dest, size, sh);
      break;

    case VDAT_STRING_MAINFW_ACT:
      switch(sh->firmware_index) {
        case 0:
          StrCopy(dest, "A", size);
          break;
        case 1:
          StrCopy(dest, "B", size);
          break;
        case 0xFF:
          StrCopy(dest, "recovery", size);
          break;
        default:
          value = NULL;
      }
      break;

    default:
      value = NULL;
      break;
  }

  free(sh);
  return value;
}


int GetVdatInt(VdatIntField field) {
  VbSharedDataHeader* sh = VbSharedDataRead();
  int value = -1;

  if (!sh)
    return -1;

  /* Fields supported in version 1 */
  switch (field) {
    case VDAT_INT_FLAGS:
      value = (int)sh->flags;
      break;
    case VDAT_INT_HEADER_VERSION:
      value = sh->struct_version;
      break;
    case VDAT_INT_TRIED_FIRMWARE_B:
      value = (sh->flags & VBSD_FWB_TRIED ? 1 : 0);
      break;
    case VDAT_INT_KERNEL_KEY_VERIFIED:
      value = (sh->flags & VBSD_KERNEL_KEY_VERIFIED ? 1 : 0);
      break;
    case VDAT_INT_FW_VERSION_TPM:
      value = (int)sh->fw_version_tpm;
      break;
    case VDAT_INT_KERNEL_VERSION_TPM:
      value = (int)sh->kernel_version_tpm;
      break;
    case VDAT_INT_FW_BOOT2:
      value = (sh->flags & VBSD_BOOT_FIRMWARE_VBOOT2 ? 1 : 0);
    default:
      break;
  }

  /* Fields added in struct version 2 */
  if (sh->struct_version >= 2) {
    switch(field) {
      case VDAT_INT_DEVSW_BOOT:
        value = (sh->flags & VBSD_BOOT_DEV_SWITCH_ON ? 1 : 0);
        break;
      case VDAT_INT_DEVSW_VIRTUAL:
        value = (sh->flags & VBSD_HONOR_VIRT_DEV_SWITCH ? 1 : 0);
        break;
      case VDAT_INT_RECSW_BOOT:
        value = (sh->flags & VBSD_BOOT_REC_SWITCH_ON ? 1 : 0);
        break;
      case VDAT_INT_HW_WPSW_BOOT:
        value = (sh->flags & VBSD_BOOT_FIRMWARE_WP_ENABLED ? 1 : 0);
        break;
      case VDAT_INT_SW_WPSW_BOOT:
        value = (sh->flags & VBSD_BOOT_FIRMWARE_SW_WP_ENABLED ? 1 : 0);
        break;
      case VDAT_INT_RECOVERY_REASON:
        value = sh->recovery_reason;
        break;
      default:
        break;
    }
  }

  free(sh);
  return value;
}

/* Return version of VbSharedData struct or -1 if not found. */
int VbSharedDataVersion(void) {
  return GetVdatInt(VDAT_INT_HEADER_VERSION);
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
  } else if (!strcasecmp(name,"recovery_request")) {
    value = VbGetNvStorage(VBNV_RECOVERY_REQUEST);
  } else if (!strcasecmp(name,"dbg_reset")) {
    value = VbGetNvStorage(VBNV_DEBUG_RESET_MODE);
  } else if (!strcasecmp(name,"disable_dev_request")) {
    value = VbGetNvStorage(VBNV_DISABLE_DEV_REQUEST);
  } else if (!strcasecmp(name,"clear_tpm_owner_request")) {
    value = VbGetNvStorage(VBNV_CLEAR_TPM_OWNER_REQUEST);
  } else if (!strcasecmp(name,"clear_tpm_owner_done")) {
    value = VbGetNvStorage(VBNV_CLEAR_TPM_OWNER_DONE);
  } else if (!strcasecmp(name,"fwb_tries")) {
    value = VbGetNvStorage(VBNV_TRY_B_COUNT);
  } else if (!strcasecmp(name,"fw_vboot2")) {
    value = GetVdatInt(VDAT_INT_FW_BOOT2);
  } else if (!strcasecmp(name,"fw_try_count")) {
    value = VbGetNvStorage(VBNV_FW_TRY_COUNT);
  } else if (!strcasecmp(name,"fwupdate_tries")) {
    value = VbGetNvStorage(VBNV_KERNEL_FIELD);
    if (value != -1)
      value &= KERN_NV_FWUPDATE_TRIES_MASK;
  } else if (!strcasecmp(name,"block_devmode")) {
    value = VbGetNvStorage(VBNV_KERNEL_FIELD);
    if (value != -1) {
      value &= KERN_NV_BLOCK_DEVMODE_FLAG;
      value = !!value;
    }
  } else if (!strcasecmp(name,"loc_idx")) {
    value = VbGetNvStorage(VBNV_LOCALIZATION_INDEX);
  } else if (!strcasecmp(name,"backup_nvram_request")) {
    value = VbGetNvStorage(VBNV_BACKUP_NVRAM_REQUEST);
  } else if (!strcasecmp(name,"dev_boot_usb")) {
    value = VbGetNvStorage(VBNV_DEV_BOOT_USB);
  } else if (!strcasecmp(name,"dev_boot_legacy")) {
    value = VbGetNvStorage(VBNV_DEV_BOOT_LEGACY);
  } else if (!strcasecmp(name,"dev_boot_signed_only")) {
    value = VbGetNvStorage(VBNV_DEV_BOOT_SIGNED_ONLY);
  } else if (!strcasecmp(name,"oprom_needed")) {
    value = VbGetNvStorage(VBNV_OPROM_NEEDED);
  } else if (!strcasecmp(name,"recovery_subcode")) {
    value = VbGetNvStorage(VBNV_RECOVERY_SUBCODE);
  }
  /* Other parameters */
  else if (!strcasecmp(name,"cros_debug")) {
    value = VbGetCrosDebug();
  } else if (!strcasecmp(name,"debug_build")) {
    value = VbGetDebugBuild();
  } else if (!strcasecmp(name,"devsw_boot")) {
    value = GetVdatInt(VDAT_INT_DEVSW_BOOT);
  } else if (!strcasecmp(name,"devsw_virtual")) {
    value = GetVdatInt(VDAT_INT_DEVSW_VIRTUAL);
  } else if (!strcasecmp(name, "recoverysw_boot")) {
    value = GetVdatInt(VDAT_INT_RECSW_BOOT);
  } else if (!strcasecmp(name, "wpsw_boot")) {
    value = GetVdatInt(VDAT_INT_HW_WPSW_BOOT);
  } else if (!strcasecmp(name, "sw_wpsw_boot")) {
    value = GetVdatInt(VDAT_INT_SW_WPSW_BOOT);
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


const char* VbGetSystemPropertyString(const char* name, char* dest,
                                      size_t size) {
  static const char unknown_string[] = "unknown";

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
  } else if (!strcasecmp(name, "mainfw_act")) {
    return GetVdatString(dest, size, VDAT_STRING_MAINFW_ACT);
  } else if (!strcasecmp(name, "vdat_timers")) {
    return GetVdatString(dest, size, VDAT_STRING_TIMERS);
  } else if (!strcasecmp(name, "vdat_lfdebug")) {
    return GetVdatString(dest, size, VDAT_STRING_LOAD_FIRMWARE_DEBUG);
  } else if (!strcasecmp(name, "vdat_lkdebug")) {
    return GetVdatString(dest, size, VDAT_STRING_LOAD_KERNEL_DEBUG);
  } else if (!strcasecmp(name, "ddr_type")) {
    return unknown_string;
  } else if (!strcasecmp(name, "fw_try_next")) {
    return VbGetNvStorage(VBNV_FW_TRY_NEXT) ? "B" : "A";
  } else if (!strcasecmp(name, "fw_tried")) {
    return VbGetNvStorage(VBNV_FW_TRIED) ? "B" : "A";
  } else if (!strcasecmp(name, "fw_result")) {
    int v = VbGetNvStorage(VBNV_FW_RESULT);
    if (v < ARRAY_SIZE(fw_results))
      return fw_results[v];
    else
      return "unknown";
  } else if (!strcasecmp(name, "fw_prev_tried")) {
    return VbGetNvStorage(VBNV_FW_PREV_TRIED) ? "B" : "A";
  } else if (!strcasecmp(name, "fw_prev_result")) {
    int v = VbGetNvStorage(VBNV_FW_PREV_RESULT);
    if (v < ARRAY_SIZE(fw_results))
      return fw_results[v];
    else
      return "unknown";
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
  } else if (!strcasecmp(name,"recovery_request")) {
    return VbSetNvStorage(VBNV_RECOVERY_REQUEST, value);
  } else if (!strcasecmp(name,"recovery_subcode")) {
    return VbSetNvStorage(VBNV_RECOVERY_SUBCODE, value);
  } else if (!strcasecmp(name,"dbg_reset")) {
    return VbSetNvStorage(VBNV_DEBUG_RESET_MODE, value);
  } else if (!strcasecmp(name,"disable_dev_request")) {
    return VbSetNvStorage(VBNV_DISABLE_DEV_REQUEST, value);
  } else if (!strcasecmp(name,"clear_tpm_owner_request")) {
    return VbSetNvStorage(VBNV_CLEAR_TPM_OWNER_REQUEST, value);
  } else if (!strcasecmp(name,"clear_tpm_owner_done")) {
    /* Can only clear this flag; it's set by firmware. */
    return VbSetNvStorage(VBNV_CLEAR_TPM_OWNER_DONE, 0);
  } else if (!strcasecmp(name,"fwb_tries")) {
    return VbSetNvStorage(VBNV_TRY_B_COUNT, value);
  } else if (!strcasecmp(name,"fw_try_count")) {
    return VbSetNvStorage(VBNV_FW_TRY_COUNT, value);
  } else if (!strcasecmp(name,"oprom_needed")) {
    return VbSetNvStorage(VBNV_OPROM_NEEDED, value);
  } else if (!strcasecmp(name,"backup_nvram_request")) {
      /* Best-effort only, since it requires firmware and TPM support. */
    return VbSetNvStorage(VBNV_BACKUP_NVRAM_REQUEST, value);
  } else if (!strcasecmp(name,"fwupdate_tries")) {
    int kern_nv = VbGetNvStorage(VBNV_KERNEL_FIELD);
    if (kern_nv == -1)
      return -1;
    kern_nv &= ~KERN_NV_FWUPDATE_TRIES_MASK;
    kern_nv |= (value & KERN_NV_FWUPDATE_TRIES_MASK);
    return VbSetNvStorage_WithBackup(VBNV_KERNEL_FIELD, kern_nv);
  } else if (!strcasecmp(name,"block_devmode")) {
    int kern_nv = VbGetNvStorage(VBNV_KERNEL_FIELD);
    if (kern_nv == -1)
      return -1;
    kern_nv &= ~KERN_NV_BLOCK_DEVMODE_FLAG;
    if (value)
	kern_nv |= KERN_NV_BLOCK_DEVMODE_FLAG;
    return VbSetNvStorage_WithBackup(VBNV_KERNEL_FIELD, kern_nv);
  } else if (!strcasecmp(name,"loc_idx")) {
    return VbSetNvStorage_WithBackup(VBNV_LOCALIZATION_INDEX, value);
  } else if (!strcasecmp(name,"dev_boot_usb")) {
    return VbSetNvStorage_WithBackup(VBNV_DEV_BOOT_USB, value);
  } else if (!strcasecmp(name,"dev_boot_legacy")) {
    return VbSetNvStorage_WithBackup(VBNV_DEV_BOOT_LEGACY, value);
  } else if (!strcasecmp(name,"dev_boot_signed_only")) {
    return VbSetNvStorage_WithBackup(VBNV_DEV_BOOT_SIGNED_ONLY, value);
  }

  return -1;
}


int VbSetSystemPropertyString(const char* name, const char* value) {
  /* Chain to architecture-dependent properties */
  if (0 == VbSetArchPropertyString(name, value))
    return 0;

  if (!strcasecmp(name, "fw_try_next")) {
    if (!strcasecmp(value, "A"))
      return VbSetNvStorage(VBNV_FW_TRY_NEXT, 0);
    else if (!strcasecmp(value, "B"))
      return VbSetNvStorage(VBNV_FW_TRY_NEXT, 1);
    else
      return -1;

  } else if (!strcasecmp(name, "fw_result")) {
    int i;

    for (i = 0; i < ARRAY_SIZE(fw_results); i++) {
      if (!strcasecmp(value, fw_results[i]))
	return VbSetNvStorage(VBNV_FW_RESULT, i);
    }
    return -1;
  }

  return -1;
}
