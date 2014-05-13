/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_2RSA_H_
#define VBOOT_REFERENCE_2RSA_H_

struct vb2_workbuf;

/* Algorithms for crypto lib */
enum vb2_crypto_algorithm {
	VB2_ALG_RSA1024_SHA1 = 0,
	VB2_ALG_RSA1024_SHA256,
	VB2_ALG_RSA1024_SHA512,
	VB2_ALG_RSA2048_SHA1,
	VB2_ALG_RSA2048_SHA256,
	VB2_ALG_RSA2048_SHA512,
	VB2_ALG_RSA4096_SHA1,
	VB2_ALG_RSA4096_SHA256,
	VB2_ALG_RSA4096_SHA512,
	VB2_ALG_RSA8192_SHA1,
	VB2_ALG_RSA8192_SHA256,
	VB2_ALG_RSA8192_SHA512,
	// TODO: add algorithms for bare SHA with no RSA?

	/* Number of algorithms */
	VB2_ALG_COUNT
};

/* Public key structure in RAM */
struct vb2_public_key {
	uint32_t arrsize;    /* Length of n[] and rr[] in number of uint32_t */
	uint32_t n0inv;      /* -1 / n[0] mod 2^32 */
	const uint32_t *n;   /* Modulus as little endian array */
	const uint32_t *rr;  /* R^2 as little endian array */
	uint32_t algorithm;  /* Algorithm to use when verifying with the key */
};

/**
 * Return the size of a RSA signature
 *
 * @param algorithm	Key algorithm
 * @return The size of the signature, or 0 if error.
 */
uint32_t vb2_rsa_sig_size(uint32_t algorithm);

/**
 * Return the size of a pre-processed RSA public key.
 *
 * @param algorithm	Key algorithm
 * @return The size of the preprocessed key, or 0 if error.
 */
uint32_t vb2_packed_key_size(uint32_t algorithm);

/**
 * Check pkcs 1.5 padding bytes
 *
 * @param sig		Signature to verify
 * @param algorithm	Key algorithm
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_check_padding(uint8_t *sig, int algorithm);

/* Size of work buffer sufficient for vb2_verify_digest() worst case */
#define VB2_VERIFY_DIGEST_WORKBUF_BYTES (3 * 1024)

/**
 * Verify a RSA PKCS1.5 signature against an expected hash digest.
 *
 * @param key		Key to use in signature verification
 * @param sig		Signature to verify (destroyed in process)
 * @param digest	Digest of signed data
 * @param wb		Work buffer
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_verify_digest(const struct vb2_public_key *key,
		      uint8_t *sig,
		      const uint8_t *digest,
		      struct vb2_workbuf *wb);

#endif  /* VBOOT_REFERENCE_2RSA_H_ */
