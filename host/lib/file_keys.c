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
#include "cryptolib.h"
#include "file_keys.h"
#include "host_common.h"
#include "signature_digest.h"

uint8_t *BufferFromFile(const char* input_file, uint64_t* len)
{
	int fd;
	struct stat stat_fd;
	uint8_t* buf = NULL;

	if ((fd = open(input_file, O_RDONLY)) == -1) {
		VBDEBUG(("Couldn't open file %s\n", input_file));
		return NULL;
	}

	if (-1 == fstat(fd, &stat_fd)) {
		VBDEBUG(("Couldn't stat file %s\n", input_file));
		close(fd);
		return NULL;
	}
	*len = stat_fd.st_size;

	buf = (uint8_t *)malloc(*len);
	if (!buf) {
		VbExError("Couldn't allocate %ld bytes for file %s\n",
			  *len, input_file);
		close(fd);
		return NULL;
	}

	if (*len != read(fd, buf, *len)) {
		VBDEBUG(("Couldn't read file %s into a buffer\n", input_file));
		free(buf);
		close(fd);
		return NULL;
	}

	close(fd);
	return buf;
}

RSAPublicKey *RSAPublicKeyFromFile(const char *input_file)
{
	uint64_t len;
	RSAPublicKey* key = NULL;

	uint8_t *buf = BufferFromFile(input_file, &len);
	if (buf)
		key = RSAPublicKeyFromBuf(buf, len);
	free(buf);
	return key;
}

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
