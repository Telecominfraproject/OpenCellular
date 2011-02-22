/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware image library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_common.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"


static void VbNvStorageTest(void) {

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

  /* Test debug reset mode field */
  VbNvSetup(&c);
  TEST_EQ(VbNvGet(&c, VBNV_DEBUG_RESET_MODE, &data), 0,
          "Get debug reset mode");
  TEST_EQ(data, 0, "Debug reset mode default");
  TEST_EQ(VbNvSet(&c, VBNV_DEBUG_RESET_MODE, 1), 0,
          "Set debug reset mode");
  VbNvGet(&c, VBNV_DEBUG_RESET_MODE, &data);
  TEST_EQ(data, 1, "Debug reset mode set");
  VbNvTeardown(&c);

  /* Test try B count */
  VbNvSetup(&c);
  TEST_EQ(VbNvGet(&c, VBNV_TRY_B_COUNT, &data), 0, "Get try b count");
  TEST_EQ(data, 0, "Try b count default");
  TEST_EQ(VbNvSet(&c, VBNV_TRY_B_COUNT, 6), 0, "Set try b count");
  VbNvGet(&c, VBNV_TRY_B_COUNT, &data);
  TEST_EQ(data, 6, "Try b count set");
  VbNvSet(&c, VBNV_TRY_B_COUNT, 15);
  VbNvGet(&c, VBNV_TRY_B_COUNT, &data);
  TEST_EQ(data, 15, "Try b count set 2");
  VbNvTeardown(&c);

  /* Test recovery request */
  VbNvSetup(&c);
  TEST_EQ(VbNvGet(&c, VBNV_RECOVERY_REQUEST, &data), 0, "Get recovery request");
  TEST_EQ(data, 0, "Default recovery request");
  TEST_EQ(VbNvSet(&c, VBNV_RECOVERY_REQUEST, 0x42), 0, "Set recovery request");
  VbNvGet(&c, VBNV_RECOVERY_REQUEST, &data);
  TEST_EQ(data, 0x42, "Set recovery request");
  VbNvSet(&c, VBNV_RECOVERY_REQUEST, 0xED);
  VbNvGet(&c, VBNV_RECOVERY_REQUEST, &data);
  TEST_EQ(data, 0xED, "Set recovery request 2");
  VbNvTeardown(&c);

  /* Test localization index */
  VbNvSetup(&c);
  TEST_EQ(VbNvGet(&c, VBNV_LOCALIZATION_INDEX, &data), 0,
          "Get localization index");
  TEST_EQ(data, 0, "Default localization index");
  TEST_EQ(VbNvSet(&c, VBNV_LOCALIZATION_INDEX, 0x69), 0,
          "Set localization index");
  VbNvGet(&c, VBNV_LOCALIZATION_INDEX, &data);
  TEST_EQ(data, 0x69, "Set localization index");
  VbNvSet(&c, VBNV_LOCALIZATION_INDEX, 0xB0);
  VbNvGet(&c, VBNV_LOCALIZATION_INDEX, &data);
  TEST_EQ(data, 0xB0, "Set localization index 2");
  VbNvTeardown(&c);

  /* Test kernel field */
  VbNvSetup(&c);
  TEST_EQ(VbNvGet(&c, VBNV_KERNEL_FIELD, &data), 0, "Get kernel field");
  TEST_EQ(data, 0, "Default kernel field");
  TEST_EQ(VbNvSet(&c, VBNV_KERNEL_FIELD, 0x12345678), 0, "Set kernel field");
  VbNvGet(&c, VBNV_KERNEL_FIELD, &data);
  TEST_EQ(data, 0x12345678, "Set kernel field");
  VbNvSet(&c, VBNV_KERNEL_FIELD, 0xFEDCBA98);
  VbNvGet(&c, VBNV_KERNEL_FIELD, &data);
  TEST_EQ(data, 0xFEDCBA98, "Set kernel field 2");
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
  VbNvSet(&c, VBNV_DEBUG_RESET_MODE, 1);
  VbNvSet(&c, VBNV_RECOVERY_REQUEST, 0xED);
  VbNvSet(&c, VBNV_LOCALIZATION_INDEX, 0xB0);
  VbNvSet(&c, VBNV_KERNEL_FIELD, 0xFEDCBA98);
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
