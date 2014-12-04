/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host-side functions for verified boot key structures
 */

#ifndef VBOOT_REFERENCE_HOST_SIGNATURE2_H_
#define VBOOT_REFERENCE_HOST_SIGNATURE2_H_

#include "2struct.h"

struct vb2_private_key;

/**
 * Sign data buffer
 *
 * @param sig_ptr	On success, points to a newly allocated signature.
 *			Caller is responsible for calling free() on this.
 * @param data		Pointer to data to sign
 * @param size		Size of data to sign in bytes
 * @param key		Private key to use to sign data
 * @param desc		Optional description for signature.  If NULL, the
 *			key description will be used.
 * @return VB2_SUCCESS, or non-zero error code on failure.
 */
int vb2_sign_data(struct vb2_signature **sig_ptr,
		  const uint8_t *data,
		  uint32_t size,
		  const struct vb2_private_key *key,
		  const char *desc);

/**
 * Calculate the signature size for a private key.
 *
 * @param size_ptr	On success, contains the signature size in bytes.
 * @param key		Key to calculate signature length from.
 * @param desc		Optional description for signature.  If NULL, the
 *			key description will be used.
 * @return VB2_SUCCESS, or non-zero error code on failure.
 */
int vb2_sig_size_for_key(uint32_t *size_ptr,
			 const struct vb2_private_key *key,
			 const char *desc);

/**
 * Calculate the total signature size for a list of keys.
 *
 * @param size_ptr	On success, contains the signature size in bytes.
 * @param key_list	List of keys to calculate signature length from.
 * @param key_count	Number of keys.
 * @return VB2_SUCCESS, or non-zero error code on failure.
 */
int vb2_sig_size_for_keys(uint32_t *size_ptr,
			  const struct vb2_private_key **key_list,
			  uint32_t key_count);

/**
 * Sign object with a key.
 *
 * @param buf		Buffer containing object to sign, starting with
 *			common header
 * @param sig_offset	Offset in buffer at which to store signature.  All
 *			data before this in the buffer will be signed.
 * @param key		Key to sign object with
 * @param desc		If non-null, description to use for signature
 */
int vb2_sign_object(uint8_t *buf,
		    uint32_t sig_offset,
		    const struct vb2_private_key *key,
		    const char *desc);

/**
 * Sign object with list of keys.
 *
 * @param buf		Buffer containing object to sign, starting with
 *			common header
 * @param sig_offset	Offset to start signatures.  All data before this
 *			in the buffer will be signed.
 * @param key_list	List of keys to sign object with
 * @param key_count	Number of keys in list
 */
int vb2_sign_object_multiple(uint8_t *buf,
			     uint32_t sig_offset,
			     const struct vb2_private_key **key_list,
			     uint32_t key_count);

#endif  /* VBOOT_REFERENCE_HOST_SIGNATURE2_H_ */
