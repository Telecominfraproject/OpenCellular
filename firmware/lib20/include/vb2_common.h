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

/*
 * Helper functions to get data pointed to by a public key or signature.
 */

const uint8_t *vb2_packed_key_data(const struct vb2_packed_key *key);
uint8_t *vb2_signature_data(struct vb2_signature *sig);

/**
 * Verify the data pointed to by a subfield is inside the parent data.
 *
 * The subfield has a header pointed to by member, and a separate data
 * field at an offset relative to the header.  That is:
 *
 *   struct parent {
 *     (possibly other parent fields)
 *     struct member {
 *        (member header fields)
 *     };
 *     (possibly other parent fields)
 *   };
 *   (possibly some other parent data)
 *   (member data)
 *   (possibly some other parent data)
 *
 * @param parent		Parent data
 * @param parent_size		Parent size in bytes
 * @param member		Subfield header
 * @param member_size		Size of subfield header in bytes
 * @param member_data_offset	Offset of member data from start of member
 * @param member_data_size	Size of member data in bytes
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_verify_member_inside(const void *parent, size_t parent_size,
			     const void *member, size_t member_size,
			     ptrdiff_t member_data_offset,
			     size_t member_data_size);

/**
 * Verify a signature is fully contained in its parent data
 *
 * @param parent	Parent data
 * @param parent_size	Parent size in bytes
 * @param sig		Signature pointer
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_verify_signature_inside(const void *parent,
				uint32_t parent_size,
				const struct vb2_signature *sig);


/**
 * Verify a packed key is fully contained in its parent data
 *
 * @param parent	Parent data
 * @param parent_size	Parent size in bytes
 * @param key		Packed key pointer
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_verify_packed_key_inside(const void *parent,
				 uint32_t parent_size,
				 const struct vb2_packed_key *key);

/**
 * Unpack a vboot1-format key for use in verification
 *
 * The elements of the unpacked key will point into the source buffer, so don't
 * free the source buffer until you're done with the key.
 *
 * @param key		Destintion for unpacked key
 * @param buf		Source buffer containing packed key
 * @param size		Size of buffer in bytes
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_unpack_key(struct vb2_public_key *key,
		   const uint8_t *buf,
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
int vb2_verify_digest(const struct vb2_public_key *key,
		      struct vb2_signature *sig,
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
int vb2_verify_data(const uint8_t *data,
		    uint32_t size,
		    struct vb2_signature *sig,
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
int vb2_verify_keyblock(struct vb2_keyblock *block,
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
int vb2_verify_fw_preamble(struct vb2_fw_preamble *preamble,
			   uint32_t size,
			   const struct vb2_public_key *key,
			   const struct vb2_workbuf *wb);

#endif  /* VBOOT_REFERENCE_VB2_COMMON_H_ */
