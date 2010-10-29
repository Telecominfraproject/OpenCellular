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


/* Create a key block header containing [data_key] and [flags], signed
 * by private key the file [signing_key_pem_file] and algorithm [algorithm]
 * using the external signer program [external_signer] for all private key
 * operations.
 * Caller owns the returned pointer, and must free
 * it with Free(). */
VbKeyBlockHeader* KeyBlockCreate_external(const VbPublicKey* data_key,
                                          const char* signing_key_pem_file,
                                          uint64_t algorithm,
                                          uint64_t flags,
                                          const char* external_signer);

/* Create a key block header containing [data_key] and [flags], signed
 * by [signing_key].  Caller owns the returned pointer, and must free
 * it with Free(). */
VbKeyBlockHeader* KeyBlockCreate(const VbPublicKey* data_key,
                                 const VbPrivateKey* signing_key,
                                 uint64_t flags);


/* Read a key block from a .keyblock file.  Caller owns the returned
 * pointer, and must free it with Free().
 *
 * Returns NULL if error. */
VbKeyBlockHeader* KeyBlockRead(const char* filename);


/* Write a key block to a file in .keyblock format. */
int KeyBlockWrite(const char* filename, const VbKeyBlockHeader* key_block);

#endif  /* VBOOT_REFERENCE_HOST_KEYBLOCK_H_ */
