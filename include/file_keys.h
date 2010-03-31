/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility functions for file and key handling.
 */

#ifndef VBOOT_REFERENCE_FILE_KEYS_H_
#define VBOOT_REFERENCE_FILE_KEYS_H_

#include "cryptolib.h"

/* Read file named [input_file] into a buffer and stores the length into
 * [len].
 *
 * Returns a pointer to the buffer. Caller owns the returned pointer and
 * must free it.
 */
uint8_t* BufferFromFile(const char* input_file, uint64_t* len);

/* Read a pre-processed RSA Public Key from file [input_file].
 *
 * Returns a pointer to the read key. Caller owns the returned pointer and
 * must free it.
 */
RSAPublicKey* RSAPublicKeyFromFile(const char* input_file);

/* Returns the appropriate digest for the data in [input_file]
 * based on the signature [algorithm].
 * Caller owns the returned digest and must free it.
 */
uint8_t* DigestFile(char* input_file, int sig_algorithm);

/* Helper function to invoke external program to calculate signature on
 * [input_file] using private key [key_file] and signature algorithm
 * [algorithm].
 *
 * Returns the signature. Caller owns the buffer and must Free() it.
 */
uint8_t* SignatureFile(const char* input_fie, const char* key_file,
                       int algorithm);

#endif  /* VBOOT_REFERENCE_FILE_KEYS_H_ */
