/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>

#include "host_common.h"

#include "crossystem.h"
#include "utility.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"

/* ACPI constants from Chrome OS Main Processor Firmware Spec */
/* GPIO signal types */
#define GPIO_SIGNAL_TYPE_RECOVERY 1
#define GPIO_SIGNAL_TYPE_DEV 2
#define GPIO_SIGNAL_TYPE_WP 3
/* CHSW bitflags */
#define CHSW_RECOVERY_BOOT     0x00000002
#define CHSW_RECOVERY_EC_BOOT  0x00000004
#define CHSW_DEV_BOOT          0x00000020
#define CHSW_WP_BOOT           0x00000200
/* CMOS reboot field bitflags */
#define CMOSRF_RECOVERY        0x80
#define CMOSRF_DEBUG_RESET     0x40
#define CMOSRF_TRY_B           0x20
/* Boot reasons from BINF.0, from early H2C firmware */
/* Unknown */
#define BINF0_UNKNOWN                  0
/* Normal boot to Chrome OS */
#define BINF0_NORMAL                   1
/* Developer mode boot (developer mode warning displayed) */
#define BINF0_DEVELOPER                2
/* Recovery initiated by user, using recovery button */
#define BINF0_RECOVERY_BUTTON          3
/* Recovery initiated by user pressing a key at developer mode warning
 * screen */
#define BINF0_RECOVERY_DEV_SCREEN_KEY  4
/* Recovery caused by BIOS failed signature check (neither rewritable
 * firmware was valid) */
#define BINF0_RECOVERY_RW_FW_BAD       5
/* Recovery caused by no OS kernel detected */
#define BINF0_RECOVERY_NO_OS           6
/* Recovery caused by OS kernel failed signature check */
#define BINF0_RECOVERY_BAD_OS          7
/* Recovery initiated by OS */
#define BINF0_RECOVERY_OS_INITIATED    8
/* OS-initiated S3 diagnostic path (debug mode boot) */
#define BINF0_S3_DIAGNOSTIC_PATH       9
/* S3 resume failed */
#define BINF0_S3_RESUME_FAILED        10
/* Recovery caused by TPM error */
#define BINF0_RECOVERY_TPM_ERROR      11
/* Firmware types from BINF.3 */
#define BINF3_RECOVERY   0
#define BINF3_NORMAL     1
#define BINF3_DEVELOPER  2

/* Base name for ACPI files */
#define ACPI_BASE_PATH "/sys/devices/platform/chromeos_acpi"
/* Paths for frequently used ACPI files */
#define ACPI_BINF_PATH ACPI_BASE_PATH "/BINF"
#define ACPI_CHNV_PATH ACPI_BASE_PATH "/CHNV"
#define ACPI_CHSW_PATH ACPI_BASE_PATH "/CHSW"
#define ACPI_FMAP_PATH ACPI_BASE_PATH "/FMAP"
#define ACPI_GPIO_PATH ACPI_BASE_PATH "/GPIO"
#define ACPI_VBNV_PATH ACPI_BASE_PATH "/VBNV"

/* Base name for GPIO files */
#define GPIO_BASE_PATH "/sys/class/gpio"
#define GPIO_EXPORT_PATH GPIO_BASE_PATH "/export"

/* Filename for NVRAM file */
#define NVRAM_PATH "/dev/nvram"

/* Filename for kernel command line */
#define KERNEL_CMDLINE_PATH "/proc/cmdline"


/* Copy up to dest_size-1 characters from src to dest, ensuring null
   termination (which strncpy() doesn't do).  Returns the destination
   string. */
char* StrCopy(char* dest, const char* src, int dest_size) {
  strncpy(dest, src, dest_size);
  dest[dest_size - 1] = '\0';
  return dest;
}


/* Read a string from a file.  Passed the destination, dest size, and
 * filename to read.
 *
 * Returns the destination, or NULL if error. */
char* ReadFileString(char* dest, int size, const char* filename) {
  char* got;
  FILE* f;

  f = fopen(filename, "rt");
  if (!f)
    return NULL;

  got = fgets(dest, size, f);
  fclose(f);
  return got;
}


/* Read an integer from a file.
 *
 * Returns the parsed integer, or -1 if error. */
int ReadFileInt(const char* filename) {
  char buf[64];
  int value;
  char* e = NULL;

  if (!ReadFileString(buf, sizeof(buf), filename))
    return -1;

  /* Convert to integer.  Allow characters after the int ("123 blah"). */
  value = strtol(buf, &e, 0);
  if (e == buf)
    return -1;  /* No characters consumed, so conversion failed */

  return value;
}


/* Check if a bit is set in a file which contains an integer.
 *
 * Returns 1 if the bit is set, 0 if clear, or -1 if error. */
int ReadFileBit(const char* filename, int bitmask) {
  int value = ReadFileInt(filename);
  if (value == -1)
    return -1;
  else return (value & bitmask ? 1 : 0);
}


/* Return true if the FWID starts with the specified string. */
static int FwidStartsWith(const char *start) {
  char fwid[128];
  if (!VbGetSystemPropertyString("fwid", fwid, sizeof(fwid)))
    return 0;

  return 0 == strncmp(fwid, start, strlen(start));
}


/* Read a GPIO of the specified signal type (see ACPI GPIO SignalType).
 *
 * Returns 1 if the signal is asserted, 0 if not asserted, or -1 if error. */
int ReadGpio(int signal_type) {
  char name[128];
  int index = 0;
  int gpio_type;
  int active_high;
  int controller_offset;
  char controller_name[128];
  int value;

  /* Scan GPIO.* to find a matching signal type */
  for (index = 0; ; index++) {
    snprintf(name, sizeof(name), "%s.%d/GPIO.0", ACPI_GPIO_PATH, index);
    gpio_type = ReadFileInt(name);
    if (gpio_type == signal_type)
      break;
    else if (gpio_type == -1)
      return -1;  /* Ran out of GPIOs before finding a match */
  }

  /* Read attributes and controller info for the GPIO */
  snprintf(name, sizeof(name), "%s.%d/GPIO.1", ACPI_GPIO_PATH, index);
  active_high = ReadFileBit(name, 0x00000001);
  snprintf(name, sizeof(name), "%s.%d/GPIO.2", ACPI_GPIO_PATH, index);
  controller_offset = ReadFileInt(name);
  if (active_high == -1 || controller_offset == -1)
    return -1;  /* Missing needed info */

  /* We only support the NM10 for now */
  snprintf(name, sizeof(name), "%s.%d/GPIO.3", ACPI_GPIO_PATH, index);
  if (!ReadFileString(controller_name, sizeof(controller_name), name))
    return -1;
  if (0 != strcmp(controller_name, "NM10"))
    return -1;

  /* Assume the NM10 has offset 192 */
  /* TODO: should really check gpiochipNNN/label to see if it's the
   * address we expect for the NM10, and then read the offset from
   * gpiochipNNN/base. */
  controller_offset += 192;

  /* Try reading the GPIO value */
  snprintf(name, sizeof(name), "%s/gpio%d/value",
           GPIO_BASE_PATH, controller_offset);
  value = ReadFileInt(name);

  if (value == -1) {
    /* Try exporting the GPIO */
    FILE* f = fopen(GPIO_EXPORT_PATH, "wt");
    if (!f)
      return -1;
    fprintf(f, "%d", controller_offset);
    fclose(f);

    /* Try re-reading the GPIO value */
    value = ReadFileInt(name);
  }

  if (value == -1)
    return -1;

  /* Compare the GPIO value with the active value and return 1 if match. */
  return (value == active_high ? 1 : 0);
}


/* Read the CMOS reboot field in NVRAM.
 *
 * Returns 0 if the mask is clear in the field, 1 if set, or -1 if error. */
int VbGetCmosRebootField(uint8_t mask) {
  FILE* f;
  int chnv, nvbyte;

  /* Get the byte offset from CHNV */
  chnv = ReadFileInt(ACPI_CHNV_PATH);
  if (chnv == -1)
    return -1;

  f = fopen(NVRAM_PATH, "rb");
  if (!f)
    return -1;

  if (0 != fseek(f, chnv, SEEK_SET) || EOF == (nvbyte = fgetc(f))) {
    fclose(f);
    return -1;
  }

  fclose(f);
  return (nvbyte & mask ? 1 : 0);
}


/* Write the CMOS reboot field in NVRAM.
 *
 * Sets (value=0) or clears (value!=0) the mask in the byte.
 *
 * Returns 0 if success, or -1 if error. */
int VbSetCmosRebootField(uint8_t mask, int value) {
  FILE* f;
  int chnv, nvbyte;

  /* Get the byte offset from CHNV */
  chnv = ReadFileInt(ACPI_CHNV_PATH);
  if (chnv == -1)
    return -1;

  f = fopen(NVRAM_PATH, "w+b");
  if (!f)
    return -1;

  /* Read the current value */
  if (0 != fseek(f, chnv, SEEK_SET) || EOF == (nvbyte = fgetc(f))) {
    fclose(f);
    return -1;
  }

  /* Set/clear the mask */
  if (value)
    nvbyte |= mask;
  else
    nvbyte &= ~mask;

  /* Write the byte back */
  if (0 != fseek(f, chnv, SEEK_SET) || EOF == (fputc(nvbyte, f))) {
    fclose(f);
    return -1;
  }

  /* Success */
  fclose(f);
  return 0;
}


/* Read an integer property from VbNvStorage.
 *
 * Returns the parameter value, or -1 if error. */
int VbGetNvStorage(VbNvParam param) {
  FILE* f;
  VbNvContext vnc;
  int offs;
  uint32_t value;
  int retval;

  /* Get the byte offset from VBNV */
  offs = ReadFileInt(ACPI_VBNV_PATH ".0");
  if (offs == -1)
    return -1;
  if (VBNV_BLOCK_SIZE > ReadFileInt(ACPI_VBNV_PATH ".1"))
    return -1;  /* NV storage block is too small */

  /* TODO: locking around NV access */
  f = fopen(NVRAM_PATH, "rb");
  if (!f)
    return -1;

  if (0 != fseek(f, offs, SEEK_SET) ||
      1 != fread(vnc.raw, VBNV_BLOCK_SIZE, 1, f)) {
    fclose(f);
    return -1;
  }

  fclose(f);

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


/* Write an integer property to VbNvStorage.
 *
 * Returns 0 if success, -1 if error. */
int VbSetNvStorage(VbNvParam param, int value) {
  FILE* f;
  VbNvContext vnc;
  int offs;
  int retval = -1;
  int i;

  /* Get the byte offset from VBNV */
  offs = ReadFileInt(ACPI_VBNV_PATH ".0");
  if (offs == -1)
    return -1;
  if (VBNV_BLOCK_SIZE > ReadFileInt(ACPI_VBNV_PATH ".1"))
    return -1;  /* NV storage block is too small */

  /* TODO: locking around NV access */
  f = fopen(NVRAM_PATH, "w+b");
  if (!f)
    return -1;

  if (0 != fseek(f, offs, SEEK_SET) ||
      1 != fread(vnc.raw, VBNV_BLOCK_SIZE, 1, f)) {
    goto VbSetNvCleanup;
  }

  if (0 != VbNvSetup(&vnc))
    goto VbSetNvCleanup;
  i = VbNvSet(&vnc, param, (uint32_t)value);
  if (0 != VbNvTeardown(&vnc))
    goto VbSetNvCleanup;
  if (0 != i)
    goto VbSetNvCleanup;

  if (vnc.raw_changed) {
    if (0 != fseek(f, offs, SEEK_SET) ||
        1 != fwrite(vnc.raw, VBNV_BLOCK_SIZE, 1, f))
      goto VbSetNvCleanup;
  }

  /* Success */
  retval = 0;

VbSetNvCleanup:
  fclose(f);
  /* TODO: release lock */
  return retval;
}


/* Read the recovery reason.  Returns the reason code or -1 if error. */
int VbGetRecoveryReason(void) {
  int value;

  /* Try reading type from BINF.4 */
  value = ReadFileInt(ACPI_BINF_PATH ".4");
  if (-1 != value)
    return value;

  /* Fall back to BINF.0 for legacy systems like Mario. */
  switch(ReadFileInt(ACPI_BINF_PATH ".0")) {
    case BINF0_NORMAL:
    case BINF0_DEVELOPER:
      return VBNV_RECOVERY_NOT_REQUESTED;
    case BINF0_RECOVERY_BUTTON:
      return VBNV_RECOVERY_RO_MANUAL;
    case BINF0_RECOVERY_DEV_SCREEN_KEY:
      return VBNV_RECOVERY_RW_DEV_SCREEN;
    case BINF0_RECOVERY_RW_FW_BAD:
    case BINF0_RECOVERY_NO_OS:
      return VBNV_RECOVERY_RW_NO_OS;
    case BINF0_RECOVERY_BAD_OS:
      return VBNV_RECOVERY_RW_INVALID_OS;
    case BINF0_RECOVERY_OS_INITIATED:
      return VBNV_RECOVERY_LEGACY;
    default:
      /* Other values don't map cleanly to firmware type. */
      return -1;
  }
}


/* Read the active main firmware type into the destination buffer.
 * Passed the destination and its size.  Returns the destination, or
 * NULL if error. */
const char* VbReadMainFwType(char* dest, int size) {

  /* Try reading type from BINF.3 */
  switch(ReadFileInt(ACPI_BINF_PATH ".3")) {
    case BINF3_RECOVERY:
      return StrCopy(dest, "recovery", size);
    case BINF3_NORMAL:
      return StrCopy(dest, "normal", size);
    case BINF3_DEVELOPER:
      return StrCopy(dest, "developer", size);
    default:
      break;  /* Fall through to legacy handling */
  }

  /* Fall back to BINF.0 for legacy systems like Mario. */
  switch(ReadFileInt(ACPI_BINF_PATH ".0")) {
    case -1:
      /* Both BINF.0 and BINF.3 are missing, so this isn't Chrome OS
       * firmware. */
      return StrCopy(dest, "nonchrome", size);
    case BINF0_NORMAL:
      return StrCopy(dest, "normal", size);
    case BINF0_DEVELOPER:
      return StrCopy(dest, "developer", size);
    case BINF0_RECOVERY_BUTTON:
    case BINF0_RECOVERY_DEV_SCREEN_KEY:
    case BINF0_RECOVERY_RW_FW_BAD:
    case BINF0_RECOVERY_NO_OS:
    case BINF0_RECOVERY_BAD_OS:
    case BINF0_RECOVERY_OS_INITIATED:
    case BINF0_RECOVERY_TPM_ERROR:
      /* Assorted flavors of recovery boot reason. */
      return StrCopy(dest, "recovery", size);
    default:
      /* Other values don't map cleanly to firmware type. */
      return NULL;
  }
}


/* Determine whether OS-level debugging should be allowed.  Passed the
 * destination and its size.  Returns 1 if yes, 0 if no, -1 if error. */
int VbGetCrosDebug(void) {
  FILE* f = NULL;
  char buf[4096] = "";
  int binf3;
  char *t, *saveptr;

  /* Try reading firmware type from BINF.3. */
  binf3 = ReadFileInt(ACPI_BINF_PATH ".3");
  if (BINF3_RECOVERY == binf3)
    return 0;  /* Recovery mode never allows debug. */
  else if (BINF3_DEVELOPER == binf3)
    return 1;  /* Developer firmware always allows debug. */

  /* Normal new firmware, older ChromeOS firmware, or non-Chrome firmware.
   * For all these cases, check /proc/cmdline for cros_debug. */
  f = fopen(KERNEL_CMDLINE_PATH, "rt");
  if (f) {
    if (NULL == fgets(buf, sizeof(buf), f))
      *buf = 0;
    fclose(f);
  }
  for (t = strtok_r(buf, " ", &saveptr); t; t=strtok_r(NULL, " ", &saveptr)) {
    if (0 == strcmp(t, "cros_debug"))
      return 1;
  }

  /* Normal new firmware or older Chrome OS firmware allows debug if the
   * dev switch is on. */
  if (1 == ReadFileBit(ACPI_CHSW_PATH, CHSW_DEV_BOOT))
    return 1;

  /* All other cases disallow debug. */
  return 0;
}


/* Read a system property integer.
 *
 * Returns the property value, or -1 if error. */
int VbGetSystemPropertyInt(const char* name) {
  int value = -1;

  /* Switch positions */
  if (!strcasecmp(name,"devsw_cur")) {
    value = ReadGpio(GPIO_SIGNAL_TYPE_DEV);
  } else if (!strcasecmp(name,"devsw_boot")) {
    value = ReadFileBit(ACPI_CHSW_PATH, CHSW_DEV_BOOT);
  } else if (!strcasecmp(name,"recoverysw_cur")) {
    value = ReadGpio(GPIO_SIGNAL_TYPE_RECOVERY);
  } else if (!strcasecmp(name,"recoverysw_boot")) {
    value = ReadFileBit(ACPI_CHSW_PATH, CHSW_RECOVERY_BOOT);
  } else if (!strcasecmp(name,"recoverysw_ec_boot")) {
    value = ReadFileBit(ACPI_CHSW_PATH, CHSW_RECOVERY_EC_BOOT);
  } else if (!strcasecmp(name,"wpsw_cur")) {
    value = ReadGpio(GPIO_SIGNAL_TYPE_WP);
    if (-1 != value && FwidStartsWith("Mario."))
      value = 1 - value;  /* Mario reports this backwards */
  } else if (!strcasecmp(name,"wpsw_boot")) {
    value = ReadFileBit(ACPI_CHSW_PATH, CHSW_WP_BOOT);
    if (-1 != value && FwidStartsWith("Mario."))
      value = 1 - value;  /* Mario reports this backwards */
  }
  /* Saved memory is at a fixed location for all H2C BIOS.  If the CHSW
   * path exists in sysfs, it's a H2C BIOS. */
  else if (!strcasecmp(name,"savedmem_base")) {
    return (-1 == ReadFileInt(ACPI_CHSW_PATH) ? -1 : 0x00F00000);
  } else if (!strcasecmp(name,"savedmem_size")) {
    return (-1 == ReadFileInt(ACPI_CHSW_PATH) ? -1 : 0x00100000);
  }
  /* NV storage values with no defaults for older BIOS. */
  else if (!strcasecmp(name,"tried_fwb")) {
    value = VbGetNvStorage(VBNV_TRIED_FIRMWARE_B);
  } else if (!strcasecmp(name,"kern_nv")) {
    value = VbGetNvStorage(VBNV_KERNEL_FIELD);
  } else if (!strcasecmp(name,"nvram_cleared")) {
    value = VbGetNvStorage(VBNV_KERNEL_SETTINGS_RESET);
  }
  /* NV storage values.  If unable to get from NV storage, fall back to the
   * CMOS reboot field used by older BIOS. */
  else if (!strcasecmp(name,"recovery_request")) {
    value = VbGetNvStorage(VBNV_RECOVERY_REQUEST);
    if (-1 == value)
      value = VbGetCmosRebootField(CMOSRF_RECOVERY);
  } else if (!strcasecmp(name,"dbg_reset")) {
    value = VbGetNvStorage(VBNV_DEBUG_RESET_MODE);
    if (-1 == value)
      value = VbGetCmosRebootField(CMOSRF_DEBUG_RESET);
  } else if (!strcasecmp(name,"fwb_tries")) {
    value = VbGetNvStorage(VBNV_TRY_B_COUNT);
    if (-1 == value)
      value = VbGetCmosRebootField(CMOSRF_TRY_B);
  }
  /* Other parameters */
  else if (!strcasecmp(name,"recovery_reason")) {
    return VbGetRecoveryReason();
  } else if (!strcasecmp(name,"fmap_base")) {
    value = ReadFileInt(ACPI_FMAP_PATH);
  } else if (!strcasecmp(name,"cros_debug")) {
    value = VbGetCrosDebug();
  }

  return value;
}


/* Read a system property string into a destination buffer of the specified
 * size.
 *
 * Returns the passed buffer, or NULL if error. */
const char* VbGetSystemPropertyString(const char* name, char* dest, int size) {

  if (!strcasecmp(name,"hwid")) {
    return ReadFileString(dest, size, ACPI_BASE_PATH "/HWID");
  } else if (!strcasecmp(name,"fwid")) {
    return ReadFileString(dest, size, ACPI_BASE_PATH "/FWID");
  } else if (!strcasecmp(name,"ro_fwid")) {
    return ReadFileString(dest, size, ACPI_BASE_PATH "/FRID");
  } else if (!strcasecmp(name,"mainfw_act")) {
    switch(ReadFileInt(ACPI_BINF_PATH ".1")) {
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
    return VbReadMainFwType(dest, size);
  } else if (!strcasecmp(name,"ecfw_act")) {
    switch(ReadFileInt(ACPI_BINF_PATH ".2")) {
      case 0:
        return StrCopy(dest, "RO", size);
      case 1:
        return StrCopy(dest, "RW", size);
      default:
        return NULL;
    }
  } else if (!strcasecmp(name,"kernkey_vfy")) {
    switch(VbGetNvStorage(VBNV_FW_VERIFIED_KERNEL_KEY)) {
      case 0:
        return "hash";
      case 1:
        return "sig";
      default:
        return NULL;
    }
  } else
    return NULL;
}


/* Set a system property integer.
 *
 * Returns 0 if success, -1 if error. */
int VbSetSystemPropertyInt(const char* name, int value) {

  /* NV storage values with no defaults for older BIOS. */
  if (!strcasecmp(name,"nvram_cleared")) {
    /* Can only clear this flag; it's set inside the NV storage library. */
    return VbSetNvStorage(VBNV_KERNEL_SETTINGS_RESET, 0);
  } else if (!strcasecmp(name,"kern_nv")) {
    return VbSetNvStorage(VBNV_KERNEL_FIELD, value);
  }
  /* NV storage values.  If unable to get from NV storage, fall back to the
   * CMOS reboot field used by older BIOS. */
  else if (!strcasecmp(name,"recovery_request")) {
    if (0 == VbSetNvStorage(VBNV_RECOVERY_REQUEST, value))
      return 0;
    return VbSetCmosRebootField(CMOSRF_RECOVERY, value);
  } else if (!strcasecmp(name,"dbg_reset")) {
    if (0 == VbSetNvStorage(VBNV_DEBUG_RESET_MODE, value))
      return 0;
    return  VbSetCmosRebootField(CMOSRF_DEBUG_RESET, value);
  } else if (!strcasecmp(name,"fwb_tries")) {
    if (0 == VbSetNvStorage(VBNV_TRY_B_COUNT, value))
      return 0;
    return VbSetCmosRebootField(CMOSRF_TRY_B, value);
  }

  return -1;
}


/* Set a system property string.
 *
 * Returns 0 if success, -1 if error. */
int VbSetSystemPropertyString(const char* name, const char* value) {

  /* TODO: support setting */
  return -1;
}
