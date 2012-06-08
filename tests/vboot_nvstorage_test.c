/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware NV storage library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_common.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"

/* Single NV storage field to test */
typedef struct VbNvField {
  VbNvParam param;          /* Parameter index */
  uint32_t default_value;   /* Expected default value */
  uint32_t test_value;      /* Value to test writing */
  uint32_t test_value2;     /* Second value to test writing */
  char* desc;               /* Field description */
} VbNvField;

/* Array of fields to test, terminated with a field with desc==NULL. */
static VbNvField nvfields[] = {
  {VBNV_DEBUG_RESET_MODE, 0, 1, 0, "debug reset mode"},
  {VBNV_TRY_B_COUNT, 0, 6, 15, "try B count"},
  {VBNV_RECOVERY_REQUEST, 0, 0x42, 0xED, "recovery request"},
  {VBNV_LOCALIZATION_INDEX, 0, 0x69, 0xB0, "localization index"},
  {VBNV_KERNEL_FIELD, 0, 0x12345678, 0xFEDCBA98, "kernel field"},
  {VBNV_DEV_BOOT_USB, 0, 1, 0, "dev boot usb"},
  {VBNV_DEV_BOOT_SIGNED_ONLY, 0, 1, 0, "dev boot custom"},
  {0, 0, 0, 0, NULL}
};

static void VbNvStorageTest(void) {

  VbNvField* vnf;
  VbNvContext c;
  uint8_t goodcrc;
  uint32_t data;

  memset(&c, 0xA6, sizeof(c));

  /* Open with invalid data should set defaults */
  TEST_EQ(VbNvSetup(&c), 0, "VbNvSetup()");
  TEST_EQ(c.raw[0], 0x70, "VbNvSetup() reset header byte");
  /* Close then regenerates the CRC */
  TEST_EQ(VbNvTeardown(&c), 0, "VbNvTeardown()");
  TEST_NEQ(c.raw[15], 0, "VbNvTeardown() CRC");
  TEST_EQ(c.raw_changed, 1, "VbNvTeardown() changed");
  goodcrc = c.raw[15];
  /* Another open-close pair should not cause further changes */
  VbNvSetup(&c);
  VbNvTeardown(&c);
  TEST_EQ(c.raw_changed, 0, "VbNvTeardown() didn't change");
  TEST_EQ(c.raw[15], goodcrc, "VbNvTeardown() CRC same");

  /* Perturbing the header should force defaults */
  c.raw[0] ^= 0x40;
  VbNvSetup(&c);
  TEST_EQ(c.raw[0], 0x70, "VbNvSetup() reset header byte again");
  /* Close then regenerates the CRC */
  VbNvTeardown(&c);
  TEST_EQ(c.raw_changed, 1, "VbNvTeardown() changed again");
  TEST_EQ(c.raw[15], goodcrc, "VbNvTeardown() CRC same again");

  /* So should perturbing some other byte */
  TEST_EQ(c.raw[11], 0, "Kernel byte starts at 0");
  c.raw[11] = 12;
  VbNvSetup(&c);
  TEST_EQ(c.raw[11], 0, "VbNvSetup() reset kernel byte");
  /* Close then regenerates the CRC */
  VbNvTeardown(&c);
  TEST_EQ(c.raw_changed, 1, "VbNvTeardown() changed again");
  TEST_EQ(c.raw[15], goodcrc, "VbNvTeardown() CRC same again");

  /* Clear the kernel and firmware flags */
  VbNvSetup(&c);
  TEST_EQ(VbNvGet(&c, VBNV_FIRMWARE_SETTINGS_RESET, &data), 0,
          "Get firmware settings reset");
  TEST_EQ(data, 1, "Firmware settings are reset");
  TEST_EQ(VbNvSet(&c, VBNV_FIRMWARE_SETTINGS_RESET, 0), 0,
          "Clear firmware settings reset");
  VbNvGet(&c, VBNV_FIRMWARE_SETTINGS_RESET, &data);
  TEST_EQ(data, 0, "Firmware settings are clear");

  TEST_EQ(VbNvGet(&c, VBNV_KERNEL_SETTINGS_RESET, &data), 0,
          "Get kernel settings reset");
  TEST_EQ(data, 1, "Kernel settings are reset");
  TEST_EQ(VbNvSet(&c, VBNV_KERNEL_SETTINGS_RESET, 0), 0,
          "Clear kernel settings reset");
  VbNvGet(&c, VBNV_KERNEL_SETTINGS_RESET, &data);
  TEST_EQ(data, 0, "Kernel settings are clear");
  TEST_EQ(c.raw[0], 0x40, "Header byte now just has the header bit");
  VbNvTeardown(&c);
  /* That should have changed the CRC */
  TEST_NEQ(c.raw[15], goodcrc, "VbNvTeardown() CRC changed due to flags clear");

  /* Test other fields */
  VbNvSetup(&c);
  for (vnf = nvfields; vnf->desc; vnf++) {
    TEST_EQ(VbNvGet(&c, vnf->param, &data), 0, vnf->desc);
    TEST_EQ(data, vnf->default_value, vnf->desc);

    TEST_EQ(VbNvSet(&c, vnf->param, vnf->test_value), 0, vnf->desc);
    TEST_EQ(VbNvGet(&c, vnf->param, &data), 0, vnf->desc);
    TEST_EQ(data, vnf->test_value, vnf->desc);

    TEST_EQ(VbNvSet(&c, vnf->param, vnf->test_value2), 0, vnf->desc);
    TEST_EQ(VbNvGet(&c, vnf->param, &data), 0, vnf->desc);
    TEST_EQ(data, vnf->test_value2, vnf->desc);
  }
  VbNvTeardown(&c);

  /* None of those changes should have caused a reset to defaults */
  VbNvSetup(&c);
  VbNvGet(&c, VBNV_FIRMWARE_SETTINGS_RESET, &data);
  TEST_EQ(data, 0, "Firmware settings are still clear");
  VbNvGet(&c, VBNV_KERNEL_SETTINGS_RESET, &data);
  TEST_EQ(data, 0, "Kernel settings are still clear");
  VbNvTeardown(&c);

  /* Verify writing identical settings doesn't cause the CRC to regenerate */
  VbNvSetup(&c);
  TEST_EQ(c.regenerate_crc, 0, "No regen CRC on open");
  for (vnf = nvfields; vnf->desc; vnf++)
    TEST_EQ(VbNvSet(&c, vnf->param, vnf->test_value2), 0, vnf->desc);
  TEST_EQ(c.regenerate_crc, 0, "No regen CRC if data not changed");
  VbNvTeardown(&c);
  TEST_EQ(c.raw_changed, 0, "No raw change if data not changed");
}


/* disable MSVC warnings on unused arguments */
__pragma(warning (disable: 4100))

int main(int argc, char* argv[]) {
  int error_code = 0;

  VbNvStorageTest();

  if (!gTestSuccess)
    error_code = 255;

  return error_code;
}
