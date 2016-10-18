/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2sha.h"
#include "host_common.h"
#include "host_signature2.h"
#include "signature_digest.h"

static void usage(char* argv[]) {
	fprintf(stderr,
		"Usage: %s <alg_id> <digest_file>\n"
		"\n"
		"Generate a padded hash suitable for generating PKCS#1.5 "
		"signatures.\n",
		basename(argv[0]));
}

int main(int argc, char* argv[])
{
	int algorithm = -1;
	int error_code = -1;
	uint8_t* digest = NULL;
	uint8_t* padded_digest = NULL;
	uint32_t len;

	if (argc != 3) {
		usage(argv);
		goto cleanup;
	}
	algorithm = atoi(argv[1]);
	if (algorithm < 0 || algorithm >= VB2_ALG_COUNT) {
		fprintf(stderr, "Invalid Algorithm!\n");
		goto cleanup;
	}

	enum vb2_hash_algorithm hash_alg = vb2_crypto_to_hash(algorithm);
	uint32_t digest_size = vb2_digest_size(hash_alg);
	uint32_t digestinfo_size = 0;
	const uint8_t* digestinfo = NULL;
	if (VB2_SUCCESS != vb2_digest_info(hash_alg, &digestinfo,
					   &digestinfo_size)) {
		fprintf(stderr, "SignatureBuf(): Couldn't get digest info\n");
		goto cleanup;
	}
	uint32_t padded_digest_len = digest_size + digestinfo_size;

	if (VB2_SUCCESS != vb2_read_file(argv[2], &digest, &len)) {
		fprintf(stderr, "Could not read file: %s\n", argv[2]);
		goto cleanup;
	}

	padded_digest = PrependDigestInfo(hash_alg, digest);
	if(padded_digest &&
	   fwrite(padded_digest, padded_digest_len, 1, stdout) == 1)
		error_code = 0;

cleanup:
	free(padded_digest);
	free(digest);
	return error_code;
}
