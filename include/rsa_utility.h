/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Some utility functions for use with RSA signature verification.
 */

#ifndef VBOOT_REFERENCE_RSA_UTILITY_H_
#define VBOOT_REFERENCE_RSA_UTILITY_H_

#include "rsa.h"

/* Returns the size of a pre-processed RSA public key in bytes with algorithm
 * [algorithm]. */
int RSAProcessedKeySize(int algorithm);

/* Deep free the contents of [key]. */
void RSAPublicKeyFree(RSAPublicKey* key);

/* Create a RSAPublic key structure from binary blob [buf] of length
 * [len].
 *
 * Caller owns the returned key and must free it.
 */
RSAPublicKey* RSAPublicKeyFromBuf(const uint8_t* buf, int len);

/* Perform RSA signature verification on [buf] of length [len] against expected
 * signature [sig] using signature algorithm [algorithm]. The public key used
 * for verification can either be in the form of a pre-process key blob
 * [key_blob] or RSAPublicKey structure [key]. One of [key_blob] or [key] must
 * be non-NULL, and the other NULL or the function will fail.
 *
 * Returns 1 on verification success, 0 on verification failure or invalid
 * arguments.
 *
 * Note: This function is for use in the firmware and assumes all pointers point
 * to areas in the memory of the right size.
 *
 */
int RSAVerifyBinary_f(const uint8_t* key_blob,
                      const RSAPublicKey* key,
                      const uint8_t* buf,
                      int len,
                      const uint8_t* sig,
                      int algorithm);

#endif  /* VBOOT_REFERENCE_RSA_UTILITY_H_ */
