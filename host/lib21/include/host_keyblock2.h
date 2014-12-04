/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host-side functions for verified boot key structures
 */

#ifndef VBOOT_REFERENCE_HOST_KEYBLOCK2_H_
#define VBOOT_REFERENCE_HOST_KEYBLOCK2_H_

#include "2struct.h"

struct vb2_private_key;
struct vb2_public_key;

/**
 * Create and sign a keyblock.
 *
 * @param kb_ptr	On success, points to a newly allocated keyblock buffer.
 *			Caller is responsible for calling free() on this.
 * @param data_key	Data key to contain inside keyblock.
 * @param signing_keys	List of keys to sign the keyblock with.
 * @param signing_key_count	Number of keys in signing_keys.
 * @param flags		Flags for keyblock.
 * @param desc		Description for keyblock.  If NULL, description will be
 *			taken from the data key.
 * @return VB2_SUCCESS, or non-zero error code if failure.
 */
int vb2_keyblock_create(struct vb2_keyblock **kb_ptr,
			const struct vb2_public_key *data_key,
			const struct vb2_private_key **signing_keys,
			uint32_t signing_key_count,
			uint32_t flags,
			const char *desc);

#endif  /* VBOOT_REFERENCE_HOST_KEYBLOCK2_H_ */
