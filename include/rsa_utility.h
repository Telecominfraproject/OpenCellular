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

/* Create a RSAPublic key structure from binary blob [buf] of length
 * [len]. */
RSAPublicKey* RSAPublicKeyFromBuf(uint8_t* buf, int len);

#endif  /* VBOOT_REFERENCE_RSA_UTILITY_H_ */
