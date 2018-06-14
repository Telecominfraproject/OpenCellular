/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "2sha.h"
#include "bdb.h"

int bdb_sha256(void *digest, const void *buf, size_t size)
{
	struct vb2_sha256_context ctx;

	vb2_sha256_init(&ctx);
	vb2_sha256_update(&ctx, buf, size);
	vb2_sha256_finalize(&ctx, digest);

	return BDB_SUCCESS;
}
