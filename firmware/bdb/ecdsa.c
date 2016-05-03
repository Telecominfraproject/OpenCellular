/* Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Boot descriptor block firmware ECDSA stub
 */

#include <string.h>
#include "bdb.h"

int bdb_ecdsa521_verify(const uint8_t *key_data,
			const uint8_t *sig,
			const uint8_t *digest)
{
	/* This is just a stub */
	return BDB_ERROR_DIGEST;
}
