/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host-side misc functions for verified boot.
 */

#ifndef VBOOT_REFERENCE_UTIL_MISC_H_
#define VBOOT_REFERENCE_UTIL_MISC_H_

#include "vboot_struct.h"
struct rsa_st;

/* Prints the sha1sum of the given VbPublicKey to stdout. */
void PrintPubKeySha1Sum(VbPublicKey* key);

/*
 * Our packed RSBPublicKey buffer (historically in files ending with ".keyb",
 * but also the part of VbPublicKey and struct vb2_packed_key that is
 * referenced by .key_offset) has this binary format:
 *
 *   struct {
 *       uint32_t nwords;            // size of RSA key in 32-bit words
 *       uint32_t N0inv;             // -1 / N[0] mod 2^32
 *       uint32_t modulus[nwords];   // modulus as a little endian array
 *       uint32_t R2[nwords];        // R^2  as little endian array
 *   };
 *
 * This function allocates and extracts that binary structure directly
 * from the RSA private key, rather than from a file.
 *
 * @param rsa_private_key     RSA private key (duh)
 * @param keyb_data	      Pointer to newly allocated binary blob
 * @param keyb_size	      Size of newly allocated binary blob
 *
 * @return 0 on success, non-zero if unable to allocate enough memory.
 */
int vb_keyb_from_rsa(struct rsa_st *rsa_private_key,
		     uint8_t **keyb_data, uint32_t *keyb_size);

#endif  /* VBOOT_REFERENCE_UTIL_MISC_H_ */
