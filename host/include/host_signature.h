/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host-side functions for verified boot.
 */

#ifndef VBOOT_REFERENCE_HOST_SIGNATURE_H_
#define VBOOT_REFERENCE_HOST_SIGNATURE_H_

#include "cryptolib.h"
#include "host_key.h"
#include "utility.h"
#include "vboot_struct.h"


/* Initialize a signature struct. */
void SignatureInit(VbSignature* sig, uint8_t* sig_data,
                   uint64_t sig_size, uint64_t data_size);


/* Allocate a new signature with space for a [sig_size] byte signature. */
VbSignature* SignatureAlloc(uint64_t sig_size, uint64_t data_size);


/* Copy a signature key from [src] to [dest].
 *
 * Returns 0 if success, non-zero if error. */
int SignatureCopy(VbSignature* dest, const VbSignature* src);


/* Calculates a SHA-512 checksum.
 * Caller owns the returned pointer, and must free it with Free().
 *
 * Returns NULL on error. */
VbSignature* CalculateChecksum(const uint8_t* data, uint64_t size);


/* Calculates a hash of the data using the algorithm from the specified key.
 * Caller owns the returned pointer, and must free it with Free().
 *
 * Returns NULL on error. */
VbSignature* CalculateHash(const uint8_t* data, uint64_t size,
                           const VbPrivateKey* key);

/* Calculates a signature for the data using the specified key.
 * Caller owns the returned pointer, and must free it with Free().
 *
 * Returns NULL on error. */
VbSignature* CalculateSignature(const uint8_t* data, uint64_t size,
                                const VbPrivateKey* key);

/* Calculates a signature for the data using the specified key and
 * an external program.
 * Caller owns the returned pointer, and must free it with Free().
 *
 * Returns NULL on error. */
VbSignature* CalculateSignature_external(const uint8_t* data, uint64_t size,
                                         const char* key_file,
                                         uint64_t key_algorithm,
                                         const char* external_signer);

#endif  /* VBOOT_REFERENCE_HOST_SIGNATURE_H_ */
