/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * vboot nv storage related functions exported for use by userspace programs
 */

#ifndef VBOOT_REFERENCE_CROSSYSTEM_VBNV_H_
#define VBOOT_REFERENCE_CROSSYSTEM_VBNV_H_

#ifdef __cplusplus
extern "C" {
#endif

struct vb2_context;

/**
 * Attempt to read non-volatile storage using mosys.
 *
 * Returns 0 if success, non-zero if error.
 */
int vb2_read_nv_storage_mosys(struct vb2_context *ctx);

/**
 * Attempt to write non-volatile storage using mosys.
 *
 * Returns 0 if success, non-zero if error.
 */
int vb2_write_nv_storage_mosys(struct vb2_context* ctx);

#ifdef __cplusplus
}
#endif

#endif  /* VBOOT_REFERENCE_CROSSYSTEM_VBNV_H_ */
