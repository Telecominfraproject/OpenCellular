/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Non-volatile storage routines.
 */

#include "utility.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"

/* Constants for NV storage.  We use this rather than structs and
 * bitfields so the data format is consistent across platforms and
 * compilers. */
#define HEADER_OFFSET                0
#define HEADER_MASK                     0xC0
#define HEADER_SIGNATURE                0x40
#define HEADER_FIRMWARE_SETTINGS_RESET  0x20
#define HEADER_KERNEL_SETTINGS_RESET    0x10

#define BOOT_OFFSET                  1
#define BOOT_DEBUG_RESET_MODE           0x80
#define BOOT_TRY_B_COUNT_MASK           0x0F

#define RECOVERY_OFFSET              2
#define LOCALIZATION_OFFSET          3

#define FIRMWARE_FLAGS_OFFSET        5
#define FIRMWARE_TRIED_FIRMWARE_B       0x80
#define FIRMWARE_FW_VERIFIED_KERNEL_KEY 0x40
#define FIRMWARE_TEST_ERR_FUNC_MASK     0x38
#define FIRMWARE_TEST_ERR_FUNC_SHIFT    3
#define FIRMWARE_TEST_ERR_NUM_MASK      0x07

#define KERNEL_FIELD_OFFSET         11
#define CRC_OFFSET                  15


/* Return CRC-8 of the data, using x^8 + x^2 + x + 1 polynomial.  A
 * table-based algorithm would be faster, but for only 15 bytes isn't
 * worth the code size. */
static uint8_t Crc8(const uint8_t* data, int len) {
  unsigned crc = 0;
  int i, j;

  for (j = len; j; j--, data++) {
    crc ^= (*data << 8);
    for(i = 8; i; i--) {
      if (crc & 0x8000)
        crc ^= (0x1070 << 3);
      crc <<= 1;
    }
  }

  return (uint8_t)(crc >> 8);
}


int VbNvSetup(VbNvContext* context) {
  uint8_t* raw = context->raw;

  /* Nothing has changed yet. */
  context->raw_changed = 0;
  context->regenerate_crc = 0;

  /* Check data for consistency */
  if ((HEADER_SIGNATURE != (raw[HEADER_OFFSET] & HEADER_MASK))
      || (Crc8(raw, CRC_OFFSET) != raw[CRC_OFFSET])) {

    /* Data is inconsistent (bad CRC or header), so reset defaults */
    Memset(raw, 0, VBNV_BLOCK_SIZE);
    raw[HEADER_OFFSET] = (HEADER_SIGNATURE | HEADER_FIRMWARE_SETTINGS_RESET |
                          HEADER_KERNEL_SETTINGS_RESET);

    /* Regenerate CRC on exit */
    context->regenerate_crc = 1;
  }

  return 0;
}


int VbNvTeardown(VbNvContext* context) {

  if (context->regenerate_crc) {
    context->raw[CRC_OFFSET] = Crc8(context->raw, CRC_OFFSET);
    context->regenerate_crc = 0;
    context->raw_changed = 1;
  }

  return 0;
}


int VbNvGet(VbNvContext* context, VbNvParam param, uint32_t* dest) {
  const uint8_t* raw = context->raw;

  switch (param) {
    case VBNV_FIRMWARE_SETTINGS_RESET:
      *dest = (raw[HEADER_OFFSET] & HEADER_FIRMWARE_SETTINGS_RESET ? 1 : 0);
      return 0;

    case VBNV_KERNEL_SETTINGS_RESET:
      *dest = (raw[HEADER_OFFSET] & HEADER_KERNEL_SETTINGS_RESET ? 1 : 0);
      return 0;

    case VBNV_DEBUG_RESET_MODE:
      *dest = (raw[BOOT_OFFSET] & BOOT_DEBUG_RESET_MODE ? 1 : 0);
      return 0;

    case VBNV_TRY_B_COUNT:
      *dest = raw[BOOT_OFFSET] & BOOT_TRY_B_COUNT_MASK;
      return 0;

    case VBNV_RECOVERY_REQUEST:
      *dest = raw[RECOVERY_OFFSET];
      return 0;

    case VBNV_LOCALIZATION_INDEX:
      *dest = raw[LOCALIZATION_OFFSET];
      return 0;

    case VBNV_KERNEL_FIELD:
      *dest = (raw[KERNEL_FIELD_OFFSET]
               | (raw[KERNEL_FIELD_OFFSET + 1] << 8)
               | (raw[KERNEL_FIELD_OFFSET + 2] << 16)
               | (raw[KERNEL_FIELD_OFFSET + 3] << 24));
      return 0;

    case VBNV_TRIED_FIRMWARE_B:
      *dest = (raw[FIRMWARE_FLAGS_OFFSET] & FIRMWARE_TRIED_FIRMWARE_B ? 1 : 0);
      return 0;

    case VBNV_FW_VERIFIED_KERNEL_KEY:
      *dest = (raw[FIRMWARE_FLAGS_OFFSET] & FIRMWARE_FW_VERIFIED_KERNEL_KEY ?
               1 : 0);
      return 0;

    case VBNV_TEST_ERROR_FUNC:
      *dest = (raw[FIRMWARE_FLAGS_OFFSET] & FIRMWARE_TEST_ERR_FUNC_MASK)
          >> FIRMWARE_TEST_ERR_FUNC_SHIFT;
      return 0;

    case VBNV_TEST_ERROR_NUM:
      *dest = raw[FIRMWARE_FLAGS_OFFSET] & FIRMWARE_TEST_ERR_NUM_MASK;
      return 0;

    default:
      return 1;
  }
}


int VbNvSet(VbNvContext* context, VbNvParam param, uint32_t value) {
  uint8_t* raw = context->raw;
  uint32_t current;

  /* If we're not changing the value, we don't need to regenerate the CRC. */
  if (0 == VbNvGet(context, param, &current) && current == value)
    return 0;

  switch (param) {
    case VBNV_FIRMWARE_SETTINGS_RESET:
      if (value)
        raw[HEADER_OFFSET] |= HEADER_FIRMWARE_SETTINGS_RESET;
      else
        raw[HEADER_OFFSET] &= ~HEADER_FIRMWARE_SETTINGS_RESET;
      break;

    case VBNV_KERNEL_SETTINGS_RESET:
      if (value)
        raw[HEADER_OFFSET] |= HEADER_KERNEL_SETTINGS_RESET;
      else
        raw[HEADER_OFFSET] &= ~HEADER_KERNEL_SETTINGS_RESET;
      break;

    case VBNV_DEBUG_RESET_MODE:
      if (value)
        raw[BOOT_OFFSET] |= BOOT_DEBUG_RESET_MODE;
      else
        raw[BOOT_OFFSET] &= ~BOOT_DEBUG_RESET_MODE;
      break;

    case VBNV_TRY_B_COUNT:
      /* Clip to valid range. */
      if (value > BOOT_TRY_B_COUNT_MASK)
        value = BOOT_TRY_B_COUNT_MASK;

      raw[BOOT_OFFSET] &= ~BOOT_TRY_B_COUNT_MASK;
      raw[BOOT_OFFSET] |= (uint8_t)value;
      break;

    case VBNV_RECOVERY_REQUEST:
      /* Map values outside the valid range to the legacy reason, since we
       * can't determine if we're called from kernel or user mode. */
      if (value > 0xFF)
        value = VBNV_RECOVERY_LEGACY;
      raw[RECOVERY_OFFSET] = (uint8_t)value;
      break;

    case VBNV_LOCALIZATION_INDEX:
      /* Map values outside the valid range to the default index. */
      if (value > 0xFF)
        value = 0;
      raw[LOCALIZATION_OFFSET] = (uint8_t)value;
      break;

    case VBNV_KERNEL_FIELD:
      raw[KERNEL_FIELD_OFFSET] = (uint8_t)(value);
      raw[KERNEL_FIELD_OFFSET + 1] = (uint8_t)(value >> 8);
      raw[KERNEL_FIELD_OFFSET + 2] = (uint8_t)(value >> 16);
      raw[KERNEL_FIELD_OFFSET + 3] = (uint8_t)(value >> 24);
      break;

    case VBNV_TRIED_FIRMWARE_B:
      if (value)
        raw[FIRMWARE_FLAGS_OFFSET] |= FIRMWARE_TRIED_FIRMWARE_B;
      else
        raw[FIRMWARE_FLAGS_OFFSET] &= ~FIRMWARE_TRIED_FIRMWARE_B;
      break;

    case VBNV_FW_VERIFIED_KERNEL_KEY:
      if (value)
        raw[FIRMWARE_FLAGS_OFFSET] |= FIRMWARE_FW_VERIFIED_KERNEL_KEY;
      else
        raw[FIRMWARE_FLAGS_OFFSET] &= ~FIRMWARE_FW_VERIFIED_KERNEL_KEY;
      break;

    case VBNV_TEST_ERROR_FUNC:
      raw[FIRMWARE_FLAGS_OFFSET] &= ~FIRMWARE_TEST_ERR_FUNC_MASK;
      raw[FIRMWARE_FLAGS_OFFSET] |= (value << FIRMWARE_TEST_ERR_FUNC_SHIFT)
          & FIRMWARE_TEST_ERR_FUNC_MASK;
      break;

    case VBNV_TEST_ERROR_NUM:
      raw[FIRMWARE_FLAGS_OFFSET] &= ~FIRMWARE_TEST_ERR_NUM_MASK;
      raw[FIRMWARE_FLAGS_OFFSET] |= (value & FIRMWARE_TEST_ERR_NUM_MASK);
      break;

    default:
      return 1;
  }

  /* Need to regenerate CRC, since the value changed. */
  context->regenerate_crc = 1;
  return 0;
}
