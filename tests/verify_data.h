/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_VERIFY_DATA_H_
#define VBOOT_REFERENCE_VERIFY_DATA_H_

/* Reads a pre-processed key of a [len] bytes from [input_file] and
 * returns it in a RSAPublicKey structure.
 * Caller owns the returned key and must free it.
 */
RSAPublicKey* read_RSAkey(char *input_file, int len);

/* Returns the SHA-1 digest of [input_file].
 * Caller owns the returned digest and must free it. 
 */
uint8_t* SHA1_file(char *input_file);

/* Returns the SHA-256 digest of [input_file].
 * Caller owns the returned digest and must free it. 
 */
uint8_t* SHA256_file(char *input_file);

/* Returns the SHA-512 digest of [input_file].
 * Caller owns the returned digest and must free it. 
 */
uint8_t* SHA512_file(char *input_file);

/* Returns the appropriate digest for the [input_file] based on the
 * signature [algorithm].
 * Caller owns the returned digest and must free it.
 */
uint8_t* calculate_digest(char *input_file, int algorithm);

/* Return a signature of [len] bytes read from [input_file].
 * Caller owns the returned signature and must free it.
 */
uint8_t* read_signature(char *input_file, int len);

#endif  /* VBOOT_REFERENCE_VERIFY_DATA_H_ */
