/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Common functions between firmware and kernel verified boot.
 */

#ifndef VBOOT_REFERENCE_VB2_COMMON_H_
#define VBOOT_REFERENCE_VB2_COMMON_H_

#include "2api.h"
#include "2common.h"
#include "2return_codes.h"
#include "2sha.h"
#include "2struct.h"
#include "vb2_struct.h"

/**
 * Verify the integrity of a signature struct
 * @param sig		Signature struct
 * @param size		Size of buffer containing signature struct
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_verify_signature2(const struct vb2_signature2 *sig,
			  uint32_t size);

/**
 * Verify a signature against an expected hash digest.
 *
 * @param key		Key to use in signature verification
 * @param sig		Signature to verify (may be destroyed in process)
 * @param digest	Digest of signed data
 * @param wb		Work buffer
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_verify_digest2(const struct vb2_public_key *key,
		       struct vb2_signature2 *sig,
		       const uint8_t *digest,
		       const struct vb2_workbuf *wb);

/**
 * Verify data matches signature.
 *
 * @param data		Data to verify
 * @param size		Size of data buffer.  Note that amount of data to
 *			actually validate is contained in sig->data_size.
 * @param sig		Signature of data (destroyed in process)
 * @param key		Key to use to validate signature
 * @param wb		Work buffer
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_verify_data2(const void *data,
		     uint32_t size,
		     struct vb2_signature2 *sig,
		     const struct vb2_public_key *key,
		     const struct vb2_workbuf *wb);

/**
 * Check the sanity of a key block using a public key.
 *
 * Header fields are also checked for sanity.  Does not verify key index or key
 * block flags.  Signature inside block is destroyed during check.
 *
 * @param block		Key block to verify
 * @param size		Size of key block buffer
 * @param key		Key to use to verify block
 * @param wb		Work buffer
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_verify_keyblock2(struct vb2_keyblock2 *block,
			 uint32_t size,
			 const struct vb2_public_key *key,
			 const struct vb2_workbuf *wb);

/**
 * Check the sanity of a firmware preamble using a public key.
 *
 * The signature in the preamble is destroyed during the check.
 *
 * @param preamble     	Preamble to verify
 * @param size		Size of preamble buffer
 * @param key		Key to use to verify preamble
 * @param wb		Work buffer
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_verify_fw_preamble2(struct vb2_fw_preamble2 *preamble,
			    uint32_t size,
			    const struct vb2_public_key *key,
			    const struct vb2_workbuf *wb);

#endif  /* VBOOT_REFERENCE_VB2_COMMON_H_ */
