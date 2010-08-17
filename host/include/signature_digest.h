/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_SIGNATURE_DIGEST_H_
#define VBOOT_REFERENCE_SIGNATURE_DIGEST_H_

#include <stdint.h>

/* Returns a buffer with DigestInfo (which depends on [algorithm])
 * prepended to [digest].
 */
uint8_t* PrependDigestInfo(unsigned int algorithm, uint8_t* digest);

/* Function that outputs the message digest of the contents of a buffer in a
 * format that can be used as input to OpenSSL for an RSA signature.
 * Needed until the stable OpenSSL release supports SHA-256/512 digests for
 * RSA signatures.
 *
 * Returns DigestInfo || Digest where DigestInfo is the OID depending on the
 * choice of the hash algorithm (see padding.c). Caller owns the returned
 * pointer and must Free() it.
 */
uint8_t* SignatureDigest(const uint8_t* buf, uint64_t len,
                         unsigned int algorithm);

/* Calculates the signature on a buffer [buf] of length [len] using
 * the private RSA key file from [key_file] and signature algorithm
 * [algorithm].
 *
 * Returns the signature. Caller owns the buffer and must Free() it.
 */
uint8_t* SignatureBuf(const uint8_t* buf, uint64_t len, const char* key_file,
                      unsigned int algorithm);
#endif  /* VBOOT_REFERENCE_SIGNATURE_DIGEST_H_ */
