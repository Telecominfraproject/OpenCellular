/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Utility functions for message digest functions. */

#ifndef VBOOT_REFERENCE_DIGEST_UTILITY_H_
#define VBOOT_REFERENCE_DIGEST_UTILITY_H_

#include <inttypes.h>

/* Returns the SHA-1 digest of data in [input_file].
 * Caller owns the returned digest and must free it.
 */
uint8_t* SHA1_file(char *input_file);

/* Returns the SHA-256 digest of data in [input_file].
 * Caller owns the returned digest and must free it.
 */
uint8_t* SHA256_file(char *input_file);

/* Returns the SHA-512 digest of data in [input_file].
 * Caller owns the returned digest and must free it.
 */
uint8_t* SHA512_file(char *input_file);

/* Returns the appropriate digest for the data in [input_file]
 * based on the signature [algorithm].
 * Caller owns the returned digest and must free it.
 */
uint8_t* calculate_digest(char *input_file, int algorithm);

#endif  /* VBOOT_REFERENCE_DIGEST_UTILITY_H_ */
