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

/* Base name for ACPI files */
#define ACPI_BASE_PATH "/sys/devices/platform/chromeos_acpi"
/* Paths for frequently used ACPI files */
#define ACPI_CHNV_PATH ACPI_BASE_PATH "/CHNV"
#define ACPI_CHSW_PATH ACPI_BASE_PATH "/CHSW"
#define ACPI_GPIO_PATH ACPI_BASE_PATH "/GPIO"

/* Base name for GPIO files */
#define GPIO_BASE_PATH "/sys/class/gpio"
#define GPIO_EXPORT_PATH GPIO_BASE_PATH "/export"

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


/* Read a system property integer.
 *
 * Returns the property value, or -1 if error. */
int VbGetSystemPropertyInt(const char* name) {

  if (!strcasecmp(name,"devsw_cur")) {
    return ReadGpio(GPIO_SIGNAL_TYPE_DEV);
  } else if (!strcasecmp(name,"devsw_boot")) {
    return ReadFileBit(ACPI_CHSW_PATH, CHSW_DEV_BOOT);
  } else if (!strcasecmp(name,"recoverysw_cur")) {
    return ReadGpio(GPIO_SIGNAL_TYPE_RECOVERY);
  } else if (!strcasecmp(name,"recoverysw_boot")) {
    return ReadFileBit(ACPI_CHSW_PATH, CHSW_RECOVERY_BOOT);
  } else if (!strcasecmp(name,"recoverysw_ec_boot")) {
    return ReadFileBit(ACPI_CHSW_PATH, CHSW_RECOVERY_EC_BOOT);
  } else if (!strcasecmp(name,"wpsw_cur")) {
    return ReadGpio(GPIO_SIGNAL_TYPE_WP);
  } else if (!strcasecmp(name,"wpsw_boot")) {
    return ReadFileBit(ACPI_CHSW_PATH, CHSW_WP_BOOT);
  } else
    return -1;

  /* TODO: remaining properties from spec */
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
  } else
    return NULL;

  /* TODO: remaining properties from spec */
}


/* Set a system property integer.
 *
 * Returns 0 if success, -1 if error. */
int VbSetSystemPropertyInt(const char* name, int value) {

  /* TODO: support setting */
  return -1;
}


/* Set a system property string.
 *
 * Returns 0 if success, -1 if error. */
int VbSetSystemPropertyString(const char* name, const char* value) {

  /* TODO: support setting */
  return -1;
}
