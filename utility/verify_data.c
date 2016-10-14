/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Routines for verifying a file's signature. Useful in testing the core
 * RSA verification implementation.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define _STUB_IMPLEMENTATION_  /* For malloc()/free() */

#include "2sysincludes.h"

#include "2common.h"
#include "2sha.h"
#include "2rsa.h"
#include "cryptolib.h"
#include "file_keys.h"
#include "host_key.h"
#include "host_misc.h"
#include "vb2_common.h"

/* ANSI Color coding sequences. */
#define COL_GREEN "\e[1;32m"
#define COL_RED "\e[0;31m"
#define COL_STOP "\e[m"

uint8_t* read_signature(char* input_file, int len)
{
	int i, sigfd;
	uint8_t* signature = NULL;
	if ((sigfd = open(input_file, O_RDONLY)) == -1) {
		fprintf(stderr, "Couldn't open signature file\n");
		return NULL;
	}

	/* Read the signature into a buffer*/
	signature = (uint8_t*) malloc(len);
	if (!signature) {
		close(sigfd);
		return NULL;
	}

	if( (i = read(sigfd, signature, len)) != len ) {
		fprintf(stderr, "Expected signature length %d, Received %d\n",
			len, i);
		close(sigfd);
		free(signature);
		return NULL;
	}

	close(sigfd);
	return signature;
}

int main(int argc, char* argv[])
{
	uint8_t workbuf[VB2_VERIFY_DIGEST_WORKBUF_BYTES]
		 __attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
	struct vb2_workbuf wb;
	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	int return_code = 1;  /* Default to error. */
	uint8_t digest[VB2_MAX_DIGEST_SIZE];
	struct vb2_packed_key *pk = NULL;
	uint8_t *signature = NULL;
	uint32_t sig_len = 0;

	if (argc != 5) {
		int i;
		fprintf(stderr,
			"Usage: %s <algorithm> <key file> <signature file>"
			" <input file>\n\n", argv[0]);
		fprintf(stderr,
			"where <algorithm> depends on the signature algorithm"
			" used:\n");
		for(i = 0; i < VB2_ALG_COUNT; i++)
			fprintf(stderr, "\t%d for %s\n", i, algo_strings[i]);
		return -1;
	}

	int algorithm = atoi(argv[1]);
	if (algorithm >= kNumAlgorithms) {
		fprintf(stderr, "Invalid algorithm %d\n", algorithm);
		goto error;
	}

	pk = vb2_read_packed_keyb(argv[2], algorithm, 0);
	if (!pk) {
		fprintf(stderr, "Can't read RSA public key.\n");
		goto error;
	}

	struct vb2_public_key k2;
	if (VB2_SUCCESS != vb2_unpack_key(&k2, (const uint8_t *)pk,
					  pk->key_offset + pk->key_size)) {
		fprintf(stderr, "Can't unpack RSA public key.\n");
		goto error;
	}

	if (VB2_SUCCESS != vb2_read_file(argv[3], &signature, &sig_len)) {
		fprintf(stderr, "Can't read signature.\n");
		goto error;
	}

	uint32_t expect_sig_size =
			vb2_rsa_sig_size(vb2_crypto_to_signature(algorithm));
	if (sig_len != expect_sig_size) {
		fprintf(stderr, "Expected signature size %u, got %u\n",
			expect_sig_size, sig_len);
		goto error;
	}

	if (VB2_SUCCESS != DigestFile(argv[4], vb2_crypto_to_hash(algorithm),
				       digest, sizeof(digest))) {
		fprintf(stderr, "Error calculating digest.\n");
		goto error;
	}

	if (VB2_SUCCESS == vb2_rsa_verify_digest(&k2, signature, digest, &wb)) {
		return_code = 0;
		fprintf(stderr, "Signature Verification "
			COL_GREEN "SUCCEEDED" COL_STOP "\n");
	} else {
		fprintf(stderr, "Signature Verification "
			COL_RED "FAILED" COL_STOP "\n");
	}

error:
	if (pk)
		free(pk);
	if (signature)
		free(signature);

	return return_code;
}
