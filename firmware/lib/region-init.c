/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware API for loading and verifying rewritable firmware.
 * (Firmware portion)
 */

#include "sysincludes.h"

#include "bmpblk_header.h"
#include "region.h"
#include "gbb_access.h"
#include "gbb_header.h"
#include "load_kernel_fw.h"
#include "utility.h"
#include "vboot_api.h"
#include "vboot_struct.h"

VbError_t VbRegionReadData(VbCommonParams *cparams,
			   enum vb_firmware_region region, uint32_t offset,
			   uint32_t size, void *buf)
{
	/* This is the old API, for backwards compatibility */
	if (region == VB_REGION_GBB && cparams->gbb_data) {
		if (offset + size > cparams->gbb_size)
			return VBERROR_INVALID_GBB;
		Memcpy(buf, cparams->gbb_data + offset, size);
	} else
#ifdef REGION_READ
	{
		VbError_t ret;

		ret = VbExRegionRead(cparams, region, offset, size, buf);
		if (ret)
			return ret;
	}
#else
	return VBERROR_INVALID_GBB;
#endif

	return VBERROR_SUCCESS;
}

VbError_t VbGbbReadHeader_static(VbCommonParams *cparams,
				 GoogleBinaryBlockHeader *gbb)
{
	return VbRegionReadData(cparams, VB_REGION_GBB, 0,
				sizeof(GoogleBinaryBlockHeader), gbb);
}
