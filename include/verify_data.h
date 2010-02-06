/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_VERIFY_DATA_H_
#define VBOOT_REFERENCE_VERIFY_DATA_H_

/* Reads a pre-processed key from [input_file] and
 * returns it in a RSAPublicKey structure.
 * Caller owns the returned key and must free it.
 */
RSAPublicKey* read_RSAkey(char *input_file);

/* Return a signature of [len] bytes read from [input_file].
 * Caller owns the returned signature and must free it.
 */
uint8_t* read_signature(char *input_file, int len);

#endif  /* VBOOT_REFERENCE_VERIFY_DATA_H_ */
