/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Common functions between firmware and kernel verified boot.
 * (Firmware portion)
 */

#include "sysincludes.h"

#include "vboot_api.h"
#include "vboot_common.h"
#include "utility.h"

int VbSharedDataInit(VbSharedDataHeader *header, uint64_t size)
{
	VBDEBUG(("VbSharedDataInit, %d bytes, header %d bytes\n", (int)size,
		 (int)sizeof(VbSharedDataHeader)));

	if (size < sizeof(VbSharedDataHeader)) {
		VBDEBUG(("Not enough data for header.\n"));
		return VBOOT_SHARED_DATA_INVALID;
	}
	if (size < VB_SHARED_DATA_MIN_SIZE) {
		VBDEBUG(("Shared data buffer too small.\n"));
		return VBOOT_SHARED_DATA_INVALID;
	}

	if (!header)
		return VBOOT_SHARED_DATA_INVALID;

	/* Zero the header */
	Memset(header, 0, sizeof(VbSharedDataHeader));

	/* Initialize fields */
	header->magic = VB_SHARED_DATA_MAGIC;
	header->struct_version = VB_SHARED_DATA_VERSION;
	header->struct_size = sizeof(VbSharedDataHeader);
	header->data_size = size;
	header->data_used = sizeof(VbSharedDataHeader);
	header->firmware_index = 0xFF;

	/* Success */
	return VBOOT_SUCCESS;
}
