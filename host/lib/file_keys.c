/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility functions for file and key handling.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "2sysincludes.h"

#include "2common.h"
#include "2sha.h"
#include "file_keys.h"
#include "host_common.h"
#include "signature_digest.h"

int DigestFile(char *input_file, enum vb2_hash_algorithm alg,
	       uint8_t *digest, uint32_t digest_size)
{
	int input_fd, len;
	uint8_t data[VB2_SHA1_BLOCK_SIZE];
	struct vb2_digest_context ctx;

	if( (input_fd = open(input_file, O_RDONLY)) == -1 ) {
		VBDEBUG(("Couldn't open %s\n", input_file));
		return VB2_ERROR_UNKNOWN;
	}
	vb2_digest_init(&ctx, alg);
	while ((len = read(input_fd, data, sizeof(data))) == sizeof(data))
		vb2_digest_extend(&ctx, data, len);
	if (len != -1)
		vb2_digest_extend(&ctx, data, len);
	close(input_fd);

	return vb2_digest_finalize(&ctx, digest, digest_size);
}
