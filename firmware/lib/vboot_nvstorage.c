/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Non-volatile storage routines.
 */
#include "sysincludes.h"

#include "crc8.h"
#include "utility.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"

/*
 * Constants for NV storage.  We use this rather than structs and bitfields so
 * the data format is consistent across platforms and compilers.
 *
 * These constants must match the equivalent constants in 2lib/2nvstorage.c.
 * (We currently don't share a common header file because we're tring to keep
 * the two libs independent, and we hope to deprecate this one.)
 */
#define HEADER_OFFSET                0
#define HEADER_MASK                     0xC0
#define HEADER_SIGNATURE                0x40
#define HEADER_FIRMWARE_SETTINGS_RESET  0x20
#define HEADER_KERNEL_SETTINGS_RESET    0x10

#define BOOT_OFFSET                  1
#define BOOT_DEBUG_RESET_MODE           0x80
#define BOOT_DISABLE_DEV_REQUEST        0x40
#define BOOT_OPROM_NEEDED               0x20
#define BOOT_BACKUP_NVRAM               0x10
#define BOOT_TRY_B_COUNT_MASK           0x0F

#define RECOVERY_OFFSET              2
#define LOCALIZATION_OFFSET          3

#define DEV_FLAGS_OFFSET             4
#define DEV_BOOT_USB_MASK               0x01
#define DEV_BOOT_SIGNED_ONLY_MASK       0x02
#define DEV_BOOT_LEGACY_MASK            0x04

#define TPM_FLAGS_OFFSET             5
#define TPM_CLEAR_OWNER_REQUEST         0x01
#define TPM_CLEAR_OWNER_DONE            0x02

#define RECOVERY_SUBCODE_OFFSET      6

#define BOOT2_OFFSET                 7
#define BOOT2_RESULT_MASK               0x03
#define BOOT2_TRIED                     0x04
#define BOOT2_TRY_NEXT                  0x08
#define BOOT2_PREV_RESULT_MASK          0x30
#define BOOT2_PREV_RESULT_SHIFT 4  /* Number of bits to shift result */
#define BOOT2_PREV_TRIED                0x40

#define KERNEL_FIELD_OFFSET         11
#define CRC_OFFSET                  15

int VbNvSetup(VbNvContext *context)
{
	uint8_t *raw = context->raw;

	/* Nothing has changed yet. */
	context->raw_changed = 0;
	context->regenerate_crc = 0;

	/* Check data for consistency */
	if ((HEADER_SIGNATURE != (raw[HEADER_OFFSET] & HEADER_MASK))
	    || (Crc8(raw, CRC_OFFSET) != raw[CRC_OFFSET])) {
		/* Data is inconsistent (bad CRC or header); reset defaults */
		Memset(raw, 0, VBNV_BLOCK_SIZE);
		raw[HEADER_OFFSET] = (HEADER_SIGNATURE |
				      HEADER_FIRMWARE_SETTINGS_RESET |
				      HEADER_KERNEL_SETTINGS_RESET);

		/* Regenerate CRC on exit */
		context->regenerate_crc = 1;
	}

	return 0;
}

int VbNvTeardown(VbNvContext *context)
{
	if (context->regenerate_crc) {
		context->raw[CRC_OFFSET] = Crc8(context->raw, CRC_OFFSET);
		context->regenerate_crc = 0;
		context->raw_changed = 1;
	}

	return 0;
}

int VbNvGet(VbNvContext *context, VbNvParam param, uint32_t *dest)
{
	const uint8_t *raw = context->raw;

	switch (param) {
	case VBNV_FIRMWARE_SETTINGS_RESET:
		*dest = (raw[HEADER_OFFSET] & HEADER_FIRMWARE_SETTINGS_RESET ?
			 1 : 0);
		return 0;

	case VBNV_KERNEL_SETTINGS_RESET:
		*dest = (raw[HEADER_OFFSET] & HEADER_KERNEL_SETTINGS_RESET ?
			 1 : 0);
		return 0;

	case VBNV_DEBUG_RESET_MODE:
		*dest = (raw[BOOT_OFFSET] & BOOT_DEBUG_RESET_MODE ? 1 : 0);
		return 0;

	case VBNV_TRY_B_COUNT:
	case VBNV_FW_TRY_COUNT:
		*dest = raw[BOOT_OFFSET] & BOOT_TRY_B_COUNT_MASK;
		return 0;

	case VBNV_RECOVERY_REQUEST:
		*dest = raw[RECOVERY_OFFSET];
		return 0;

	case VBNV_RECOVERY_SUBCODE:
		*dest = raw[RECOVERY_SUBCODE_OFFSET];
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

	case VBNV_DEV_BOOT_USB:
		*dest = (raw[DEV_FLAGS_OFFSET] & DEV_BOOT_USB_MASK ? 1 : 0);
		return 0;

	case VBNV_DEV_BOOT_LEGACY:
		*dest = (raw[DEV_FLAGS_OFFSET] & DEV_BOOT_LEGACY_MASK ? 1 : 0);
		return 0;

	case VBNV_DEV_BOOT_SIGNED_ONLY:
		*dest = (raw[DEV_FLAGS_OFFSET] & DEV_BOOT_SIGNED_ONLY_MASK ?
			 1 : 0);
		return 0;

	case VBNV_DISABLE_DEV_REQUEST:
		*dest = (raw[BOOT_OFFSET] & BOOT_DISABLE_DEV_REQUEST ? 1 : 0);
		return 0;

	case VBNV_OPROM_NEEDED:
		*dest = (raw[BOOT_OFFSET] & BOOT_OPROM_NEEDED ? 1 : 0);
		return 0;

	case VBNV_CLEAR_TPM_OWNER_REQUEST:
		*dest = (raw[TPM_FLAGS_OFFSET] & TPM_CLEAR_OWNER_REQUEST ?
			 1 : 0);
		return 0;

	case VBNV_CLEAR_TPM_OWNER_DONE:
		*dest = (raw[TPM_FLAGS_OFFSET] & TPM_CLEAR_OWNER_DONE ? 1 : 0);
		return 0;

	case VBNV_BACKUP_NVRAM_REQUEST:
		*dest = (raw[BOOT_OFFSET] & BOOT_BACKUP_NVRAM ? 1 : 0);
		return 0;

	case VBNV_FW_TRY_NEXT:
		*dest = (raw[BOOT2_OFFSET] & BOOT2_TRY_NEXT ? 1 : 0);
		return 0;

	case VBNV_FW_TRIED:
		*dest = (raw[BOOT2_OFFSET] & BOOT2_TRIED ? 1 : 0);
		return 0;

	case VBNV_FW_RESULT:
		*dest = raw[BOOT2_OFFSET] & BOOT2_RESULT_MASK;
		return 0;

	case VBNV_FW_PREV_TRIED:
		*dest = (raw[BOOT2_OFFSET] & BOOT2_PREV_TRIED ? 1 : 0);
		return 0;

	case VBNV_FW_PREV_RESULT:
		*dest = (raw[BOOT2_OFFSET] & BOOT2_PREV_RESULT_MASK)
			>> BOOT2_PREV_RESULT_SHIFT;
		return 0;

	default:
		return 1;
	}
}

int VbNvSet(VbNvContext *context, VbNvParam param, uint32_t value)
{
	uint8_t *raw = context->raw;
	uint32_t current;

	/* If not changing the value, don't regenerate the CRC. */
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
	case VBNV_FW_TRY_COUNT:
		/* Clip to valid range. */
		if (value > BOOT_TRY_B_COUNT_MASK)
			value = BOOT_TRY_B_COUNT_MASK;

		raw[BOOT_OFFSET] &= ~BOOT_TRY_B_COUNT_MASK;
		raw[BOOT_OFFSET] |= (uint8_t)value;
		break;

	case VBNV_RECOVERY_REQUEST:
		/*
		 * Map values outside the valid range to the legacy reason,
		 * since we can't determine if we're called from kernel or user
		 * mode.
		 */
		if (value > 0xFF)
			value = VBNV_RECOVERY_LEGACY;
		raw[RECOVERY_OFFSET] = (uint8_t)value;
		break;

	case VBNV_RECOVERY_SUBCODE:
		raw[RECOVERY_SUBCODE_OFFSET] = (uint8_t)value;
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

	case VBNV_DEV_BOOT_USB:
		if (value)
			raw[DEV_FLAGS_OFFSET] |= DEV_BOOT_USB_MASK;
		else
			raw[DEV_FLAGS_OFFSET] &= ~DEV_BOOT_USB_MASK;
		break;

	case VBNV_DEV_BOOT_LEGACY:
		if (value)
			raw[DEV_FLAGS_OFFSET] |= DEV_BOOT_LEGACY_MASK;
		else
			raw[DEV_FLAGS_OFFSET] &= ~DEV_BOOT_LEGACY_MASK;
		break;

	case VBNV_DEV_BOOT_SIGNED_ONLY:
		if (value)
			raw[DEV_FLAGS_OFFSET] |= DEV_BOOT_SIGNED_ONLY_MASK;
		else
			raw[DEV_FLAGS_OFFSET] &= ~DEV_BOOT_SIGNED_ONLY_MASK;
		break;

	case VBNV_DISABLE_DEV_REQUEST:
		if (value)
			raw[BOOT_OFFSET] |= BOOT_DISABLE_DEV_REQUEST;
		else
			raw[BOOT_OFFSET] &= ~BOOT_DISABLE_DEV_REQUEST;
		break;

	case VBNV_OPROM_NEEDED:
		if (value)
			raw[BOOT_OFFSET] |= BOOT_OPROM_NEEDED;
		else
			raw[BOOT_OFFSET] &= ~BOOT_OPROM_NEEDED;
		break;

	case VBNV_CLEAR_TPM_OWNER_REQUEST:
		if (value)
			raw[TPM_FLAGS_OFFSET] |= TPM_CLEAR_OWNER_REQUEST;
		else
			raw[TPM_FLAGS_OFFSET] &= ~TPM_CLEAR_OWNER_REQUEST;
		break;

	case VBNV_CLEAR_TPM_OWNER_DONE:
		if (value)
			raw[TPM_FLAGS_OFFSET] |= TPM_CLEAR_OWNER_DONE;
		else
			raw[TPM_FLAGS_OFFSET] &= ~TPM_CLEAR_OWNER_DONE;
		break;

	case VBNV_BACKUP_NVRAM_REQUEST:
		if (value)
			raw[BOOT_OFFSET] |= BOOT_BACKUP_NVRAM;
		else
			raw[BOOT_OFFSET] &= ~BOOT_BACKUP_NVRAM;
		break;

	case VBNV_FW_TRY_NEXT:
		if (value)
			raw[BOOT2_OFFSET] |= BOOT2_TRY_NEXT;
		else
			raw[BOOT2_OFFSET] &= ~BOOT2_TRY_NEXT;
		break;

	case VBNV_FW_TRIED:
		if (value)
			raw[BOOT2_OFFSET] |= BOOT2_TRIED;
		else
			raw[BOOT2_OFFSET] &= ~BOOT2_TRIED;
		break;

	case VBNV_FW_RESULT:
		/* Map out of range values to unknown */
		if (value > BOOT2_RESULT_MASK)
			value = VBNV_FW_RESULT_UNKNOWN;

		raw[BOOT2_OFFSET] &= ~BOOT2_RESULT_MASK;
		raw[BOOT2_OFFSET] |= (uint8_t)value;
		break;

	case VBNV_FW_PREV_TRIED:
		if (value)
			raw[BOOT2_OFFSET] |= BOOT2_PREV_TRIED;
		else
			raw[BOOT2_OFFSET] &= ~BOOT2_PREV_TRIED;
		break;

	case VBNV_FW_PREV_RESULT:
		/* Map out of range values to unknown */
		if (value > BOOT2_RESULT_MASK)
			value = VBNV_FW_RESULT_UNKNOWN;

		raw[BOOT2_OFFSET] &= ~BOOT2_PREV_RESULT_MASK;
		raw[BOOT2_OFFSET] |= (uint8_t)value << BOOT2_PREV_RESULT_SHIFT;
		break;

	default:
		return 1;
	}

	/* Need to regenerate CRC, since the value changed. */
	context->regenerate_crc = 1;
	return 0;
}
