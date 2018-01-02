/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * EC software sync for verified boot
 */

#ifndef VBOOT_REFERENCE_EC_SYNC_H_
#define VBOOT_REFERENCE_EC_SYNC_H_

#include "vboot_api.h"

struct vb2_context;
struct VbCommonParams;

/**
 * EC sync, phase 1
 *
 * This checks whether the EC is running the correct image to do EC sync, and
 * whether any updates are necessary.
 *
 * @param ctx		Vboot2 context
 * @return VBERROR_SUCCESS, VBERROR_EC_REBOOT_TO_RO_REQUIRED if the EC must
 * reboot back to its RO code to continue EC sync, or other non-zero error
 * code.
 */
VbError_t ec_sync_phase1(struct vb2_context *ctx);

/**
 * Returns non-zero if the EC will perform a slow update during phase 2.
 *
 * This is only valid after calling ec_sync_phase1(), before calling
 * ec_sync_phase2().
 *
 * @param ctx		Vboot2 context
 * @return non-zero if a slow update will be done; zero if no update or a
 * fast update.
 */
int ec_will_update_slowly(struct vb2_context *ctx);

/**
 * Check if auxiliary firmware blobs need to be updated.
 *
 * @param ctx		Vboot2 context
 * @param severity	VB_AUX_FW_{NO,FAST,SLOW}_UPDATE
 * @return VBERROR_SUCCESS or non-zero error code.
 */
VbError_t ec_sync_check_aux_fw(struct vb2_context *ctx,
			       VbAuxFwUpdateSeverity_t *severity);

/**
 * EC sync, phase 2
 *
 * This updates the EC if necessary, makes sure it has protected its image(s),
 * and makes sure it has jumped to the correct image.
 *
 * If ec_will_update_slowly(), it is suggested that the caller display a
 * warning screen before calling phase 2.
 *
 * @param ctx		Vboot2 context
 * @return VBERROR_SUCCESS, VBERROR_EC_REBOOT_TO_RO_REQUIRED if the EC must
 * reboot back to its RO code to continue EC sync, or other non-zero error
 * code.
 */
VbError_t ec_sync_phase2(struct vb2_context *ctx);

/**
 * EC sync, phase 3
 *
 * This completes EC sync and handles battery cutoff if needed.
 *
 * @param ctx		Vboot2 context
 * @return VBERROR_SUCCESS or non-zero error code.
 */
VbError_t ec_sync_phase3(struct vb2_context *ctx);

/**
 * Sync all EC devices to expected versions.
 *
 * This is a high-level function which calls the functions above.
 *
 * @param ctx		Vboot context
 * @param cparams	Vboot common params
 * @return VBERROR_SUCCESS, or non-zero if error.
 */
VbError_t ec_sync_all(struct vb2_context *ctx, struct VbCommonParams *cparams);

#endif  /* VBOOT_REFERENCE_EC_SYNC_H_ */
