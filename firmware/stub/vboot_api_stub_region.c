/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Stub implementations of region API function.
 */

#include <stdint.h>

#define _STUB_IMPLEMENTATION_

#include <stdlib.h>

#include "vboot_api.h"

VbError_t VbExRegionRead(VbCommonParams *cparams,
			 enum vb_firmware_region region, uint32_t offset,
			 uint32_t size, void *buf)
{
	return VBERROR_SUCCESS;
}
