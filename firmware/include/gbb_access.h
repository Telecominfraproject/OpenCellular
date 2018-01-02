/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Access to portions of the GBB using the region API.
 */

#ifndef VBOOT_REFERENCE_GBB_ACCESS_H_
#define VBOOT_REFERENCE_GBB_ACCESS_H_

#include "vboot_api.h"

struct BmpBlockHeader;
struct ImageInfo;
struct GoogleBinaryBlockHeader;
struct ScreenLayout;
struct VbPublicKey;

/**
 * Read the GBB header
 *
 * This accesses the GBB and reads its header.
 *
 * @param cparams	Vboot common parameters
 * @param gbb		Place to put GBB header
 */
VbError_t VbGbbReadHeader_static(VbCommonParams *cparams,
				 struct GoogleBinaryBlockHeader *gbb);

/**
 * Read the root key from the GBB
 *
 * @param cparams	Vboot common parameters
 * @param keyp		Returns a pointer to the key. The caller must call
 *			free() on the key when finished with it.
 * @return VBERROR_... error, VBERROR_SUCCESS on success,
 */
VbError_t VbGbbReadRootKey(VbCommonParams *cparams,
			   struct VbPublicKey **keyp);

/**
 * Read the recovery key from the GBB
 *
 * @param cparams	Vboot common parameters
 * @param keyp		Returns a pointer to the key. The caller must call
 *			free() on the key when finished with it.
 * @return VBERROR_... error, VBERROR_SUCCESS on success,
 */
VbError_t VbGbbReadRecoveryKey(VbCommonParams *cparams,
			       struct VbPublicKey **keyp);

#endif
