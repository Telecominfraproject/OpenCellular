/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility that outputs the cryptographic digest of a contents of a
 * file in a format that can be directly used to generate PKCS#1 v1.5
 * signatures via the "openssl" command line utility.
 */


#include <stdio.h>
#include <stdlib.h>

#include "2sysincludes.h"
#include "2common.h"
#include "host_common.h"
#include "host_signature2.h"
#include "signature_digest.h"

int main(int argc, char* argv[])
{
	int error_code = -1;
	uint8_t *buf = NULL;
	uint8_t *signature_digest = NULL;
	uint32_t len;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <alg_id> <file>", argv[0]);
		goto cleanup;
	}

	int algorithm = atoi(argv[1]);
	if (algorithm < 0 || algorithm >= VB2_ALG_COUNT) {
		fprintf(stderr, "Invalid Algorithm!\n");
		goto cleanup;
	}

	if (VB2_SUCCESS != vb2_read_file(argv[2], &buf, &len)) {
		fprintf(stderr, "Could not read file: %s\n", argv[2]);
		goto cleanup;
	}

	enum vb2_hash_algorithm hash_alg = vb2_crypto_to_hash(algorithm);
	uint32_t digest_size = vb2_digest_size(hash_alg);
	uint32_t digestinfo_size = 0;
	const uint8_t *digestinfo = NULL;
	if (VB2_SUCCESS != vb2_digest_info(hash_alg, &digestinfo,
					   &digestinfo_size))
		goto cleanup;

	uint32_t signature_digest_len = digest_size + digestinfo_size;
	signature_digest = SignatureDigest(buf, len, algorithm);
	if(signature_digest &&
	   fwrite(signature_digest, signature_digest_len, 1, stdout) == 1)
		error_code = 0;

cleanup:
	free(signature_digest);
	free(buf);
	return error_code;
}
