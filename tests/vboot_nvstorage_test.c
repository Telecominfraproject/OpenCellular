/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware NV storage library.
 */

#include <stdint.h>
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
  {VBNV_DEV_BOOT_LEGACY, 0, 1, 0, "dev boot legacy"},
  {VBNV_DEV_BOOT_SIGNED_ONLY, 0, 1, 0, "dev boot custom"},
  {VBNV_DISABLE_DEV_REQUEST, 0, 1, 0, "disable dev request"},
  {VBNV_CLEAR_TPM_OWNER_REQUEST, 0, 1, 0, "clear tpm owner request"},
  {VBNV_CLEAR_TPM_OWNER_DONE, 0, 1, 0, "clear tpm owner done"},
  {VBNV_OPROM_NEEDED, 0, 1, 0, "oprom needed"},
  {VBNV_FW_TRY_COUNT, 0, 8, 15, "try count"},
  {VBNV_FW_TRY_NEXT, 0, 1, 0, "try next"},
  {VBNV_FW_TRIED, 0, 1, 0, "firmware tried"},
  {VBNV_FW_RESULT, VBNV_FW_RESULT_UNKNOWN, 1, 2, "firmware result"},
  {VBNV_FW_PREV_TRIED, 0, 1, 0, "firmware prev tried"},
  {VBNV_FW_PREV_RESULT, VBNV_FW_RESULT_UNKNOWN, 1, 3, "firmware prev result"},
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

  /* Test explicitly setting the reset flags again */
  VbNvSetup(&c);
  VbNvSet(&c, VBNV_FIRMWARE_SETTINGS_RESET, 1);
  VbNvGet(&c, VBNV_FIRMWARE_SETTINGS_RESET, &data);
  TEST_EQ(data, 1, "Firmware settings forced reset");
  VbNvSet(&c, VBNV_FIRMWARE_SETTINGS_RESET, 0);

  VbNvSet(&c, VBNV_KERNEL_SETTINGS_RESET, 1);
  VbNvGet(&c, VBNV_KERNEL_SETTINGS_RESET, &data);
  TEST_EQ(data, 1, "Kernel settings forced reset");
  VbNvSet(&c, VBNV_KERNEL_SETTINGS_RESET, 0);
  VbNvTeardown(&c);

  /* Get/set an invalid field */
  VbNvSetup(&c);
  TEST_EQ(VbNvGet(&c, -1, &data), 1, "Get invalid setting");
  TEST_EQ(VbNvSet(&c, -1, 0), 1, "Set invalid setting");
  VbNvTeardown(&c);

  /* Test other fields */
  VbNvSetup(&c);
  /* Test all defaults first, since some fields alias onto others */
  for (vnf = nvfields; vnf->desc; vnf++) {
    printf("Testing field: %s\n", vnf->desc);
    TEST_EQ(VbNvGet(&c, vnf->param, &data), 0, "  get");
    TEST_EQ(data, vnf->default_value, "  default");
  }
  /* Now test get/set */
  for (vnf = nvfields; vnf->desc; vnf++) {
    printf("Testing field: %s\n", vnf->desc);
    TEST_EQ(VbNvSet(&c, vnf->param, vnf->test_value), 0, "  set 1");
    TEST_EQ(VbNvGet(&c, vnf->param, &data), 0, "  get 1");
    TEST_EQ(data, vnf->test_value, "  value 1");

    TEST_EQ(VbNvSet(&c, vnf->param, vnf->test_value2), 0, "  set 2");
    TEST_EQ(VbNvGet(&c, vnf->param, &data), 0, "  get 2");
    TEST_EQ(data, vnf->test_value2, "  value 2");
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

  /* Test out-of-range fields mapping to defaults */
  VbNvSetup(&c);
  VbNvSet(&c, VBNV_TRY_B_COUNT, 16);
  VbNvGet(&c, VBNV_TRY_B_COUNT, &data);
  TEST_EQ(data, 15, "Try b count out of range");
  VbNvSetup(&c);
  VbNvSet(&c, VBNV_FW_TRY_COUNT, 16);
  VbNvGet(&c, VBNV_FW_TRY_COUNT, &data);
  TEST_EQ(data, 15, "Try count out of range");
  VbNvSet(&c, VBNV_RECOVERY_REQUEST, 0x101);
  VbNvGet(&c, VBNV_RECOVERY_REQUEST, &data);
  TEST_EQ(data, VBNV_RECOVERY_LEGACY, "Recovery request out of range");
  VbNvSet(&c, VBNV_LOCALIZATION_INDEX, 0x102);
  VbNvGet(&c, VBNV_LOCALIZATION_INDEX, &data);
  TEST_EQ(data, 0, "Localization index out of range");
  VbNvSet(&c, VBNV_FW_RESULT, VBNV_FW_RESULT_UNKNOWN + 100);
  VbNvGet(&c, VBNV_FW_RESULT, &data);
  TEST_EQ(data, VBNV_FW_RESULT_UNKNOWN, "Firmware result out of range");
  VbNvTeardown(&c);
}


int main(int argc, char* argv[]) {
  int error_code = 0;

  VbNvStorageTest();

  if (vboot_api_stub_check_memory())
    error_code = 255;
  if (!gTestSuccess)
    error_code = 255;

  return error_code;
}
