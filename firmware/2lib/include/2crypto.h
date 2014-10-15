/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Crypto constants for verified boot
 */

#ifndef VBOOT_REFERENCE_VBOOT_2CRYPTO_H_
#define VBOOT_REFERENCE_VBOOT_2CRYPTO_H_
#include <stdint.h>

/* Verified boot crypto algorithms */
enum vb2_crypto_algorithm {
	VB2_ALG_RSA1024_SHA1   = 0,
	VB2_ALG_RSA1024_SHA256 = 1,
	VB2_ALG_RSA1024_SHA512 = 2,
	VB2_ALG_RSA2048_SHA1   = 3,
	VB2_ALG_RSA2048_SHA256 = 4,
	VB2_ALG_RSA2048_SHA512 = 5,
	VB2_ALG_RSA4096_SHA1   = 6,
	VB2_ALG_RSA4096_SHA256 = 7,
	VB2_ALG_RSA4096_SHA512 = 8,
	VB2_ALG_RSA8192_SHA1   = 9,
	VB2_ALG_RSA8192_SHA256 = 10,
	VB2_ALG_RSA8192_SHA512 = 11,

	/* Number of algorithms */
	VB2_ALG_COUNT
};

#endif /* VBOOT_REFERENCE_VBOOT_2CRYPTO_H_ */
