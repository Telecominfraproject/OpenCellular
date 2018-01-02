/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware API for loading and verifying rewritable firmware.
 * (Firmware portion)
 */

#include "2sysincludes.h"
#include "2common.h"

#include "sysincludes.h"
#include "bmpblk_header.h"
#include "region.h"
#include "gbb_access.h"
#include "gbb_header.h"
#include "load_kernel_fw.h"
#include "utility.h"
#include "vboot_api.h"
#include "vboot_struct.h"

static VbError_t VbRegionReadGbb(VbCommonParams *cparams, uint32_t offset,
				  uint32_t size, void *buf)
{
	return VbRegionReadData(cparams, VB_REGION_GBB, offset, size, buf);
}

VbError_t VbRegionReadHWID(VbCommonParams *cparams, char *hwid,
			   uint32_t max_size)
{
	GoogleBinaryBlockHeader *gbb;
	VbError_t ret;

	if (!max_size)
		return VBERROR_INVALID_PARAMETER;
	*hwid = '\0';
	StrnAppend(hwid, "{INVALID}", max_size);
	if (!cparams)
		return VBERROR_INVALID_GBB;

	gbb = cparams->gbb;

	if (0 == gbb->hwid_size) {
		VB2_DEBUG("VbHWID(): invalid hwid size\n");
		return VBERROR_SUCCESS; /* oddly enough! */
	}

	if (gbb->hwid_size > max_size) {
		VB2_DEBUG("VbDisplayDebugInfo(): invalid hwid offset/size\n");
		return VBERROR_INVALID_PARAMETER;
	}
	ret = VbRegionReadGbb(cparams, gbb->hwid_offset, gbb->hwid_size, hwid);
	if (ret)
		return ret;

	return VBERROR_SUCCESS;
}

#define OUTBUF_LEN 128

void VbRegionCheckVersion(VbCommonParams *cparams)
{
	GoogleBinaryBlockHeader *gbb;

	if (!cparams)
		return;

	gbb = cparams->gbb;

	/*
	 * If GBB flags is nonzero, complain because that's something that the
	 * factory MUST fix before shipping. We only have to do this here,
	 * because it's obvious that something is wrong if we're not displaying
	 * screens from the GBB.
	 */
	if (gbb->major_version == GBB_MAJOR_VER && gbb->minor_version >= 1 &&
	    (gbb->flags != 0)) {
		uint32_t used = 0;
		char outbuf[OUTBUF_LEN];

		*outbuf = '\0';
		used += StrnAppend(outbuf + used, "gbb.flags is nonzero: 0x",
				OUTBUF_LEN - used);
		used += Uint64ToString(outbuf + used, OUTBUF_LEN - used,
				       gbb->flags, 16, 8);
		used += StrnAppend(outbuf + used, "\n", OUTBUF_LEN - used);
		(void)VbExDisplayDebugInfo(outbuf);
	}
}
