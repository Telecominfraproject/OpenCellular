/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <stdint.h>
#include <stdio.h>

#include "cryptolib.h"
#include "file_keys.h"
#include "rsa_padding_test.h"
#include "test_common.h"
#include "utility.h"

#include "2sysincludes.h"
#include "2rsa.h"
#include "vb2_common.h"

/**
 * Convert an old-style RSA public key struct to a new one.
 *
 * The new one does not allocate memory, so you must keep the old one around
 * until you're done with the new one.
 *
 * @param k2		Destination new key
 * @param key		Source old key
 */
void vb2_public_key_to_vb2(struct vb2_public_key *k2,
			   const struct RSAPublicKey *key)
{
	k2->arrsize = key->len;
	k2->n0inv = key->n0inv;
	k2->n = key->n;
	k2->rr = key->rr;
	k2->sig_alg = vb2_crypto_to_signature(key->algorithm);
	k2->hash_alg = vb2_crypto_to_hash(key->algorithm);
}

/**
 * Test valid and invalid signatures.
 */
static void test_signatures(const struct vb2_public_key *key)
{
	uint8_t workbuf[VB2_VERIFY_DIGEST_WORKBUF_BYTES]
		 __attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
	uint8_t sig[RSA1024NUMBYTES];
	struct vb2_workbuf wb;
	int unexpected_success;
	int i;

	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	/* The first test signature is valid. */
	Memcpy(sig, signatures[0], sizeof(sig));
	TEST_SUCC(vb2_rsa_verify_digest(key, sig, test_message_sha1_hash, &wb),
		  "RSA Padding Test valid sig");

	/* All other signatures should fail verification. */
	unexpected_success = 0;
	for (i = 1; i < sizeof(signatures) / sizeof(signatures[0]); i++) {
		Memcpy(sig, signatures[i], sizeof(sig));
		if (!vb2_rsa_verify_digest(key, sig,
					   test_message_sha1_hash, &wb)) {
			fprintf(stderr,
				"RSA Padding Test vector %d FAILED!\n", i);
			unexpected_success++;
		}
	}
	TEST_EQ(unexpected_success, 0, "RSA Padding Test invalid sigs");
}


/**
 * Test other error conditions in vb2_rsa_verify_digest().
 */
static void test_verify_digest(struct vb2_public_key *key) {
	uint8_t workbuf[VB2_VERIFY_DIGEST_WORKBUF_BYTES]
		 __attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
	uint8_t sig[RSA1024NUMBYTES];
	struct vb2_workbuf wb;
	enum vb2_signature_algorithm orig_key_alg = key->sig_alg;

	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	Memcpy(sig, signatures[0], sizeof(sig));
	TEST_SUCC(vb2_rsa_verify_digest(key, sig, test_message_sha1_hash, &wb),
		  "vb2_rsa_verify_digest() good");

	Memcpy(sig, signatures[0], sizeof(sig));
	vb2_workbuf_init(&wb, workbuf, sizeof(sig) * 3 - 1);
	TEST_EQ(vb2_rsa_verify_digest(key, sig, test_message_sha1_hash, &wb),
		VB2_ERROR_RSA_VERIFY_WORKBUF,
		"vb2_rsa_verify_digest() small workbuf");
	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	key->sig_alg = VB2_SIG_INVALID;
	Memcpy(sig, signatures[0], sizeof(sig));
	TEST_EQ(vb2_rsa_verify_digest(key, sig, test_message_sha1_hash, &wb),
		VB2_ERROR_RSA_VERIFY_ALGORITHM,
		"vb2_rsa_verify_digest() bad key alg");
	key->sig_alg = orig_key_alg;

	key->arrsize *= 2;
	Memcpy(sig, signatures[0], sizeof(sig));
	TEST_EQ(vb2_rsa_verify_digest(key, sig, test_message_sha1_hash, &wb),
		VB2_ERROR_RSA_VERIFY_SIG_LEN,
		"vb2_rsa_verify_digest() bad sig len");
	key->arrsize /= 2;

	/* Corrupt the signature near start and end */
	Memcpy(sig, signatures[0], sizeof(sig));
	sig[3] ^= 0x42;
	TEST_EQ(vb2_rsa_verify_digest(key, sig, test_message_sha1_hash, &wb),
		VB2_ERROR_RSA_PADDING, "vb2_rsa_verify_digest() bad sig");

	Memcpy(sig, signatures[0], sizeof(sig));
	sig[RSA1024NUMBYTES - 3] ^= 0x56;
	TEST_EQ(vb2_rsa_verify_digest(key, sig, test_message_sha1_hash, &wb),
		VB2_ERROR_RSA_PADDING, "vb2_rsa_verify_digest() bad sig end");
}

int main(int argc, char *argv[])
{
	int error = 0;
	RSAPublicKey *key;
	struct vb2_public_key k2;

	/* Read test key */
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <test public key>\n", argv[0]);
		return 1;
	}
	key = RSAPublicKeyFromFile(argv[1]);

	if (!key) {
		fprintf(stderr, "Couldn't read RSA public key for the test.\n");
		return 1;
	}

	// TODO: why is test key algorithm wrong?
	key->algorithm = 0;

	/* Convert test key to Vb2 format */
	vb2_public_key_to_vb2(&k2, key);

	/* Run tests */
	test_signatures(&k2);
	test_verify_digest(&k2);

	/* Clean up and exit */
	RSAPublicKeyFree(key);

	if (!gTestSuccess)
		error = 255;

	return error;
}
