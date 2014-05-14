/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Misc functions which need access to vb2_context but are not public APIs
 */

#ifndef VBOOT_REFERENCE_VBOOT_2MISC_H_
#define VBOOT_REFERENCE_VBOOT_2MISC_H_

#include "2api.h"

/**
 * Get the shared data pointer from the vboot context
 *
 * @param ctx		Vboot context
 * @return The shared data pointer.
 */
static __inline struct vb2_shared_data *vb2_get_sd(struct vb2_context *ctx) {
	return (struct vb2_shared_data *)ctx->workbuf;
}

/**
 * Set up the verified boot context data, if not already set up.
 *
 * This uses ctx->workbuf_used=0 as a flag to indicate that the data has not
 * yet been set up.  Caller must set that before calling any voot functions;
 * see 2api.h.
 *
 * @param ctx		Vboot context to initialize
 * @return VB2_SUCCESS, or error code on error.
 */
int vb2_init_context(struct vb2_context *ctx);

#endif  /* VBOOT_REFERENCE_VBOOT_2MISC_H_ */
