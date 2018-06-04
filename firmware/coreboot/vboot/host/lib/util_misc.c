/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Miscellaneous functions for userspace vboot utilities.
 */

#include <openssl/bn.h>
#include <openssl/rsa.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "2sysincludes.h"

#include "2common.h"
#include "2sha.h"
#include "host_common.h"
#include "openssl_compat.h"
#include "util_misc.h"
#include "vb2_common.h"
#include "host_key2.h"
#include "vboot_common.h"

const char *packed_key_sha1_string(const struct vb2_packed_key *key)
{
	uint8_t *buf = ((uint8_t *)key) + key->key_offset;
	uint32_t buflen = key->key_size;
	uint8_t digest[VB2_SHA1_DIGEST_SIZE];
	static char dest[VB2_SHA1_DIGEST_SIZE * 2 + 1];

	vb2_digest_buffer(buf, buflen, VB2_HASH_SHA1, digest, sizeof(digest));

	char *dnext = dest;
	int i;
	for (i = 0; i < sizeof(digest); i++)
		dnext += sprintf(dnext, "%02x", digest[i]);

	return dest;
}

const char *private_key_sha1_string(const struct vb2_private_key *key)
{
	uint8_t *buf;
	uint32_t buflen;
	uint8_t digest[VB2_SHA1_DIGEST_SIZE];
	static char dest[VB2_SHA1_DIGEST_SIZE * 2 + 1];

	if (!key->rsa_private_key ||
	    vb_keyb_from_rsa(key->rsa_private_key, &buf, &buflen)) {
		return "<error>";
	}

	vb2_digest_buffer(buf, buflen, VB2_HASH_SHA1, digest, sizeof(digest));

	char *dnext = dest;
	int i;
	for (i = 0; i < sizeof(digest); i++)
		dnext += sprintf(dnext, "%02x", digest[i]);

	free(buf);
	return dest;
}

int vb_keyb_from_rsa(struct rsa_st *rsa_private_key,
		     uint8_t **keyb_data, uint32_t *keyb_size)
{
	uint32_t i, nwords;
	BIGNUM *N = NULL;
	BIGNUM *Big1 = NULL, *Big2 = NULL, *Big32 = NULL, *BigMinus1 = NULL;
	BIGNUM *B = NULL;
	BIGNUM *N0inv = NULL, *R = NULL, *RR = NULL;
	BIGNUM *RRTemp = NULL, *NnumBits = NULL;
	BIGNUM *n = NULL, *rr = NULL;
	const BIGNUM *rsa_private_key_n;
	BN_CTX *bn_ctx = BN_CTX_new();
	uint32_t n0invout;
	uint32_t bufsize;
	uint32_t *outbuf;
	int retval = 1;

	/* Size of RSA key in 32-bit words */
	nwords = RSA_size(rsa_private_key) / 4;

	bufsize = (2 + nwords + nwords) * sizeof(uint32_t);
	outbuf = malloc(bufsize);
	if (!outbuf)
		goto done;

	*keyb_data = (uint8_t *)outbuf;
	*keyb_size = bufsize;

	*outbuf++ = nwords;

	/* Initialize BIGNUMs */
#define NEW_BIGNUM(x) do { x = BN_new(); if (!x) goto done; } while (0)
	NEW_BIGNUM(N);
	NEW_BIGNUM(Big1);
	NEW_BIGNUM(Big2);
	NEW_BIGNUM(Big32);
	NEW_BIGNUM(BigMinus1);
	NEW_BIGNUM(N0inv);
	NEW_BIGNUM(R);
	NEW_BIGNUM(RR);
	NEW_BIGNUM(RRTemp);
	NEW_BIGNUM(NnumBits);
	NEW_BIGNUM(n);
	NEW_BIGNUM(rr);
	NEW_BIGNUM(B);
#undef NEW_BIGNUM

	RSA_get0_key(rsa_private_key, &rsa_private_key_n, NULL, NULL);
	BN_copy(N, rsa_private_key_n);
	BN_set_word(Big1, 1L);
	BN_set_word(Big2, 2L);
	BN_set_word(Big32, 32L);
	BN_sub(BigMinus1, Big1, Big2);

	BN_exp(B, Big2, Big32, bn_ctx); /* B = 2^32 */

	/* Calculate and output N0inv = -1 / N[0] mod 2^32 */
	BN_mod_inverse(N0inv, N, B, bn_ctx);
	BN_sub(N0inv, B, N0inv);
	n0invout = BN_get_word(N0inv);

	*outbuf++ = n0invout;

	/* Calculate R = 2^(# of key bits) */
	BN_set_word(NnumBits, BN_num_bits(N));
	BN_exp(R, Big2, NnumBits, bn_ctx);

	/* Calculate RR = R^2 mod N */
	BN_copy(RR, R);
	BN_mul(RRTemp, RR, R, bn_ctx);
	BN_mod(RR, RRTemp, N, bn_ctx);


	/* Write out modulus as little endian array of integers. */
	for (i = 0; i < nwords; ++i) {
		uint32_t nout;

		BN_mod(n, N, B, bn_ctx); /* n = N mod B */
		nout = BN_get_word(n);
		*outbuf++ = nout;

		BN_rshift(N, N, 32); /*  N = N/B */
	}

	/* Write R^2 as little endian array of integers. */
	for (i = 0; i < nwords; ++i) {
		uint32_t rrout;

		BN_mod(rr, RR, B, bn_ctx); /* rr = RR mod B */
		rrout = BN_get_word(rr);
		*outbuf++ = rrout;

		BN_rshift(RR, RR, 32); /* RR = RR/B */
	}

	outbuf = NULL;
	retval = 0;

done:
	free(outbuf);
	/* Free BIGNUMs. */
	BN_free(Big1);
	BN_free(Big2);
	BN_free(Big32);
	BN_free(BigMinus1);
	BN_free(N0inv);
	BN_free(R);
	BN_free(RRTemp);
	BN_free(NnumBits);
	BN_free(n);
	BN_free(rr);

	return retval;
}
