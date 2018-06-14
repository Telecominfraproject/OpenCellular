/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility functions for file and key handling.
 */

#ifndef VBOOT_REFERENCE_FILE_KEYS_H_
#define VBOOT_REFERENCE_FILE_KEYS_H_

#include "2sha.h"

/* Calculates the appropriate digest for the data in [input_file] based on the
 * hash algorithm [alg] and stores it into [digest], which is of size
 * [digest_size].  Returns VB2_SUCCESS, or non-zero on error.
 */
int DigestFile(char *input_file, enum vb2_hash_algorithm alg,
	       uint8_t *digest, uint32_t digest_size);

#endif  /* VBOOT_REFERENCE_FILE_KEYS_H_ */
