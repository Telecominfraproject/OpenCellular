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

static VbError_t VbGbbReadKey(VbCommonParams *cparams, uint32_t offset,
			      VbPublicKey **keyp)
{
	VbPublicKey hdr, *key;
	VbError_t ret;
	uint32_t size;

	ret = VbRegionReadData(cparams, VB_REGION_GBB, offset,
			       sizeof(VbPublicKey), &hdr);
	if (ret)
		return ret;

	/* Deal with a zero-size key (used in testing) */
	size = hdr.key_offset + hdr.key_size;
	if (size < sizeof(hdr))
		size = sizeof(hdr);
	key = VbExMalloc(size);
	ret = VbRegionReadData(cparams, VB_REGION_GBB, offset, size, key);
	if (ret) {
		VbExFree(key);
		return ret;
	}

	*keyp = key;
	return VBERROR_SUCCESS;
}

VbError_t VbGbbReadRootKey(VbCommonParams *cparams, VbPublicKey **keyp)
{
	return VbGbbReadKey(cparams, cparams->gbb->rootkey_offset, keyp);
}

VbError_t VbGbbReadRecoveryKey(VbCommonParams *cparams, VbPublicKey **keyp)
{
	return VbGbbReadKey(cparams, cparams->gbb->recovery_key_offset, keyp);
}
