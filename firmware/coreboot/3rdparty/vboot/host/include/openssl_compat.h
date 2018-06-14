/* Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_OPENSSL_COMPAT_H_
#define VBOOT_REFERENCE_OPENSSL_COMPAT_H_

#include <openssl/rsa.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L

static inline void RSA_get0_key(const RSA *rsa, const BIGNUM **n,
				const BIGNUM **e, const BIGNUM **d)
{
	if (n != NULL)
		*n = rsa->n;
	if (e != NULL)
		*e = rsa->e;
	if (d != NULL)
		*d = rsa->d;
}

#endif  /* OPENSSL_VERSION_NUMBER < 0x10100000L */

#endif  /* VBOOT_REFERENCE_OPENSSL_COMPAT_H_ */
