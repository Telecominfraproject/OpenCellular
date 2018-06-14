/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host-side functions for firmware preamble
 */

#ifndef VBOOT_REFERENCE_HOST_FW_PREAMBLE2_H_
#define VBOOT_REFERENCE_HOST_FW_PREAMBLE2_H_

struct vb2_private_key;
struct vb21_fw_preamble;
struct vb21_signature;
/**
 * Create and sign a firmware preamble.
 *
 * @param fp_ptr	On success, points to a newly allocated preamble buffer.
 *			Caller is responsible for calling free() on this.
 * @param signing_key	Key to sign the preamble with
 * @param hash_list	Component hashes to include in the keyblock
 * @param hash_count	Number of component hashes
 * @param fw_version	Firmware version
 * @param flags		Flags for preamble
 * @param desc		Description for preamble, or NULL if none
 * @return VB2_SUCCESS, or non-zero error code if failure.
 */
int vb21_fw_preamble_create(struct vb21_fw_preamble **fp_ptr,
			    const struct vb2_private_key *signing_key,
			    const struct vb21_signature **hash_list,
			    uint32_t hash_count,
			    uint32_t fw_version,
			    uint32_t flags,
			    const char *desc);

#endif  /* VBOOT_REFERENCE_HOST_FW_PREAMBLE2_H_ */
