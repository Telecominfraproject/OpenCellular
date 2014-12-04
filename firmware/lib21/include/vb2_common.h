/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Common functions between firmware and kernel verified boot.
 */

#ifndef VBOOT_REFERENCE_VB2_COMMON_H_
#define VBOOT_REFERENCE_VB2_COMMON_H_

#include "2common.h"
#include "2return_codes.h"
#include "2struct.h"
#include "vb2_struct.h"

/**
 * Return the description of an object starting with a vb2_struct_common header.
 *
 * Does not sanity-check the buffer; merely returns the pointer.
 *
 * @param buf		Pointer to common object
 * @return A pointer to description or an empty string if none.
 */
const char *vb2_common_desc(const void *buf);

/**
 * Verify the common struct header is fully contained in its parent data
 *
 * Also verifies the description is either zero-length or null-terminated.
 *
 * @param parent	Parent data
 * @param parent_size	Parent size in bytes
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_verify_common_header(const void *parent, uint32_t parent_size);

/**
 * Verify a member is within the data for a parent object
 *
 * @param parent	Parent data (starts with struct vb2_struct_common)
 * @param min_offset	Pointer to minimum offset where member can be located.
 *			If this offset is 0 on input, uses the size of the
 *			fixed header (and description, if any).  This will be
 *			updated on return to the end of the passed member.  On
 *			error, the value of min_offset is undefined.
 * @param member_offset Offset of member data from start of parent, in bytes
 * @param member_size	Size of member data, in bytes
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_verify_common_member(const void *parent,
			     uint32_t *min_offset,
			     uint32_t member_offset,
			     uint32_t member_size);

/**
 * Verify a member which starts with a common header is within the parent
 *
 * This does not verify the contents of the member or its header, only that the
 * member's claimed total size fits within the parent's claimed total size at
 * the specified offset.
 *
 * @param parent	Parent data (starts with struct vb2_struct_common)
 * @param min_offset	Pointer to minimum offset where member can be located.
 *			If this offset is 0 on input, uses the size of the
 *			fixed header (and description, if any).  This will be
 *			updated on return to the end of the passed member.  On
 *			error, the value of min_offset is undefined.
 * @param member_offset Offset of member data from start of parent, in bytes.
 *                      This should be the start of the common header of the
 *                      member.
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_verify_common_subobject(const void *parent,
				uint32_t *min_offset,
				uint32_t member_offset);

/**
 * Unpack a key for use in verification
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
 * Unpack the RSA data fields for a public key
 *
 * This is called by vb2_unpack_key() to extract the arrays from a packed key.
 * These elements of *key will point inside the key_data buffer.
 *
 * @param key		Destination key for RSA data fields
 * @param key_data	Packed key data (from inside a packed key buffer)
 * @param key_size	Size of packed key data in bytes
 */
int vb2_unpack_key_data(struct vb2_public_key *key,
			const uint8_t *key_data,
			uint32_t key_size);

/**
 * Verify the integrity of a signature struct
 * @param sig		Signature struct
 * @param size		Size of buffer containing signature struct
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_verify_signature(const struct vb2_signature *sig,
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
int vb2_verify_data(const void *data,
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
