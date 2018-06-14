/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host-side functions for verified boot.
 */

#ifndef VBOOT_REFERENCE_HOST_KEYBLOCK_H_
#define VBOOT_REFERENCE_HOST_KEYBLOCK_H_

#include "host_key.h"
#include "vboot_struct.h"

struct vb2_keyblock;

/**
 * Create a keyblock header
 *
 * @param data_key	Data key to store in keyblock
 * @param signing_key	Key to sign keyblock with.  May be NULL if keyblock
 *			only needs a hash digest.
 * @param flags		Keyblock flags
 *
 * @return The keyblock, or NULL if error.  Caller must free() it.
 */
struct vb2_keyblock *vb2_create_keyblock(
		const struct vb2_packed_key *data_key,
		const struct vb2_private_key *signing_key,
		uint32_t flags);

/**
 * Create a keyblock header using an external signer for all private key
 * operations.
 *
 * @param data_key		Data key to store in keyblock
 * @param signing_key_pem_file	Filename of private key
 * @param algorithm		Signing algorithm index
 * @param flags			Keyblock flags
 * @param external_signer	Path to external signer program
 *
 * @return The keyblock, or NULL if error.  Caller must free() it.
 */
struct vb2_keyblock *vb2_create_keyblock_external(
		const struct vb2_packed_key *data_key,
		const char *signing_key_pem_file,
		uint32_t algorithm,
		uint32_t flags,
		const char *external_signer);

/**
 * Read a keyblock from a .keyblock file.
 *
 * @param filename	File to read keyblock from
 *
 * @return The keyblock, or NULL if error.  Caller must free() it.
 */
struct vb2_keyblock *vb2_read_keyblock(const char *filename);

/**
 * Write a keyblock to a file in .keyblock format.
 *
 * @param filename	Filename to write
 * @param keyblock	Keyblock to write
 *
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_write_keyblock(const char *filename,
		       const struct vb2_keyblock *keyblock);

#endif  /* VBOOT_REFERENCE_HOST_KEYBLOCK_H_ */
