/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Access to portions of the GBB using the region API.
 */

#ifndef VBOOT_REFERENCE_GBB_ACCESS_H_
#define VBOOT_REFERENCE_GBB_ACCESS_H_

#include "vboot_api.h"

struct vb2_context;
struct VbPublicKey;

/**
 * Read the root key from the GBB
 *
 * @param ctx		Vboot context
 * @param keyp		Returns a pointer to the key. The caller must call
 *			free() on the key when finished with it.
 * @return VBERROR_... error, VBERROR_SUCCESS on success,
 */
VbError_t VbGbbReadRootKey(struct vb2_context *ctx,
			   struct VbPublicKey **keyp);

/**
 * Read the recovery key from the GBB
 *
 * @param ctx		Vboot context
 * @param keyp		Returns a pointer to the key. The caller must call
 *			free() on the key when finished with it.
 * @return VBERROR_... error, VBERROR_SUCCESS on success,
 */
VbError_t VbGbbReadRecoveryKey(struct vb2_context *ctx,
			       struct VbPublicKey **keyp);

/**
 * Read the hardware ID from the GBB
 *
 * @param ctx		Vboot context
 * @param hwid		Place to put HWID, which will be null-terminated
 * @param max_size	Maximum size of HWID including terminated null
 *			character (suggest 256). If this size is too small
 *			then VBERROR_INVALID_PARAMETER is returned.
 * @return VBERROR_... error, VBERROR_SUCCESS on success,
 */
VbError_t VbGbbReadHWID(struct vb2_context *ctx, char *hwid, uint32_t max_size);

#endif
