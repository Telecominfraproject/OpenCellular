/* Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Boot descriptor block firmware RSA
 */

#include <string.h>
#include "bdb.h"

/* Public key structure in RAM */
struct public_key {
	uint32_t arrsize;    /* Size of n[] and rr[] arrays in elements */
	uint32_t n0inv;      /* -1 / n[0] mod 2^32 */
	const uint32_t *n;   /* Modulus as little endian array */
	const uint32_t *rr;  /* R^2 as little endian array */
};

/**
 * a[] -= mod
 */
static void subM(const struct public_key *key, uint32_t *a)
{
	int64_t A = 0;
	uint32_t i;
	for (i = 0; i < key->arrsize; ++i) {
		A += (uint64_t)a[i] - key->n[i];
		a[i] = (uint32_t)A;
		A >>= 32;
	}
}

/**
 * Return a[] >= mod
 */
int vb2_mont_ge(const struct public_key *key, uint32_t *a)
{
	uint32_t i;
	for (i = key->arrsize; i;) {
		--i;
		if (a[i] < key->n[i])
			return 0;
		if (a[i] > key->n[i])
			return 1;
	}
	return 1;  /* equal */
}

/**
 * Montgomery c[] += a * b[] / R % mod
 */
static void montMulAdd(const struct public_key *key,
                       uint32_t *c,
                       const uint32_t a,
                       const uint32_t *b)
{
	uint64_t A = (uint64_t)a * b[0] + c[0];
	uint32_t d0 = (uint32_t)A * key->n0inv;
	uint64_t B = (uint64_t)d0 * key->n[0] + (uint32_t)A;
	uint32_t i;

	for (i = 1; i < key->arrsize; ++i) {
		A = (A >> 32) + (uint64_t)a * b[i] + c[i];
		B = (B >> 32) + (uint64_t)d0 * key->n[i] + (uint32_t)A;
		c[i - 1] = (uint32_t)B;
	}

	A = (A >> 32) + (B >> 32);

	c[i - 1] = (uint32_t)A;

	if (A >> 32) {
		subM(key, c);
	}
}

/**
 * Montgomery c[] = a[] * b[] / R % mod
 */
static void montMul(const struct public_key *key,
                    uint32_t *c,
                    const uint32_t *a,
                    const uint32_t *b)
{
	uint32_t i;
	for (i = 0; i < key->arrsize; ++i) {
		c[i] = 0;
	}
	for (i = 0; i < key->arrsize; ++i) {
		montMulAdd(key, c, a[i], b);
	}
}

int vb2_safe_memcmp(const void *s1, const void *s2, size_t size)
{
	const unsigned char *us1 = s1;
	const unsigned char *us2 = s2;
	int result = 0;

	if (0 == size)
		return 0;

	/*
	 * Code snippet without data-dependent branch due to Nate Lawson
	 * (nate@root.org) of Root Labs.
	 */
	while (size--)
		result |= *us1++ ^ *us2++;

	return result != 0;
}

/*
 * PKCS 1.5 padding (from the RSA PKCS#1 v2.1 standard)
 *
 * Depending on the RSA key size and hash function, the padding is calculated
 * as follows:
 *
 * 0x00 || 0x01 || PS || 0x00 || T
 *
 * T: DER Encoded DigestInfo value which depends on the hash function used.
 *
 * SHA-256: (0x)30 31 30 0d 06 09 60 86 48 01 65 03 04 02 01 05 00 04 20 || H.
 *
 * Length(T) = 51 octets for SHA-256
 *
 * PS: octet string consisting of {Length(RSA Key) - Length(T) - 3} 0xFF
 */
static const uint8_t sha256_tail[] = {
	0x00,0x30,0x31,0x30,0x0d,0x06,0x09,0x60,
	0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x01,
	0x05,0x00,0x04,0x20
};

int vb2_check_padding(const uint8_t *sig, const struct public_key *key,
		      uint32_t pad_size)
{
	/* Determine padding to use depending on the signature type */
	const uint32_t tail_size = sizeof(sha256_tail);
	int result = 0;
	int i;

	/* First 2 bytes are always 0x00 0x01 */
	result |= *sig++ ^ 0x00;
	result |= *sig++ ^ 0x01;

	/* Then 0xff bytes until the tail */
	for (i = 0; i < pad_size - tail_size - 2; i++)
		result |= *sig++ ^ 0xff;

	/*
	 * Then the tail.  Even though there are probably no timing issues
	 * here, we use vb2_safe_memcmp() just to be on the safe side.
	 */
	result |= vb2_safe_memcmp(sig, sha256_tail, tail_size);

	return result ? BDB_ERROR_DIGEST : BDB_SUCCESS;
}

/* Array size for RSA4096 */
#define ARRSIZE4096 (4096 / 32)

/**
 * In-place public exponentiation. (exponent 65537, key size 4096 bits)
 *
 * @param key		Key to use in signing
 * @param inout		Input and output big-endian byte array
 */
static void modpowF4(const struct public_key *key, uint8_t *inout)
{
	uint32_t a[ARRSIZE4096];
	uint32_t aR[ARRSIZE4096];
	uint32_t aaR[ARRSIZE4096];
	uint32_t *aaa = aaR;  /* Re-use location. */
	int i;

	/* Convert from big endian byte array to little endian word array. */
	for (i = 0; i < ARRSIZE4096; ++i) {
		uint32_t tmp =
			(inout[((ARRSIZE4096 - 1 - i) * 4) + 0] << 24) |
			(inout[((ARRSIZE4096 - 1 - i) * 4) + 1] << 16) |
			(inout[((ARRSIZE4096 - 1 - i) * 4) + 2] << 8) |
			(inout[((ARRSIZE4096 - 1 - i) * 4) + 3] << 0);
		a[i] = tmp;
	}

	montMul(key, aR, a, key->rr);  /* aR = a * RR / R mod M   */
	for (i = 0; i < 16; i+=2) {
		montMul(key, aaR, aR, aR);  /* aaR = aR * aR / R mod M */
		montMul(key, aR, aaR, aaR);  /* aR = aaR * aaR / R mod M */
	}
	montMul(key, aaa, aR, a);  /* aaa = aR * a / R mod M */

	/* Make sure aaa < mod; aaa is at most 1x mod too large. */
	if (vb2_mont_ge(key, aaa)) {
		subM(key, aaa);
	}

	/* Convert to bigendian byte array */
	for (i = ARRSIZE4096 - 1; i >= 0; --i) {
		uint32_t tmp = aaa[i];
		*inout++ = (uint8_t)(tmp >> 24);
		*inout++ = (uint8_t)(tmp >> 16);
		*inout++ = (uint8_t)(tmp >>  8);
		*inout++ = (uint8_t)(tmp >>  0);
	}
}

int bdb_rsa4096_verify(const uint8_t *key_data,
		       const uint8_t *sig,
		       const uint8_t *digest)
{
	const uint32_t *kdata32 = (const uint32_t *)key_data;
	struct public_key key;
	uint8_t sig_work[BDB_RSA4096_SIG_SIZE];
	uint32_t pad_size;
	int rv;

	/* Unpack key */
	if (kdata32[0] != ARRSIZE4096)
		return BDB_ERROR_DIGEST;  /* Wrong key size */

	key.arrsize = kdata32[0];
	key.n0inv = kdata32[1];
	key.n = kdata32 + 2;
	key.rr = kdata32 + 2 + key.arrsize;

	/* Copy signature to work buffer */
	memcpy(sig_work, sig, sizeof(sig_work));

	modpowF4(&key, sig_work);

	/*
	 * Check padding.  Continue on to check the digest even if error to
	 * reduce the risk of timing based attacks.
	 */
	pad_size = key.arrsize * sizeof(uint32_t) - BDB_SHA256_DIGEST_SIZE;
	rv = vb2_check_padding(sig_work, &key, pad_size);

	/*
	 * Check digest.  Even though there are probably no timing issues here,
	 * use vb2_safe_memcmp() just to be on the safe side.  (That's also why
	 * we don't return before this check if the padding check failed.)
	 */
	if (vb2_safe_memcmp(sig_work + pad_size, digest,
			    BDB_SHA256_DIGEST_SIZE))
		rv = BDB_ERROR_DIGEST;

	return rv;
}

/* Array size for RSA3072B */
#define ARRSIZE3072B (3072 / 32)

/**
 * In-place public exponentiation. (exponent 3, key size 3072 bits)
 *
 * @param key		Key to use in signing
 * @param inout		Input and output big-endian byte array
 */
static void modpow3(const struct public_key *key, uint8_t *inout)
{
	uint32_t a[ARRSIZE3072B];
	uint32_t aR[ARRSIZE3072B];
	uint32_t aaR[ARRSIZE3072B];
	uint32_t *aaa = aR; /* Re-use location */
	int i;

	/* Convert from big endian byte array to little endian word array. */
	for (i = 0; i < ARRSIZE3072B; ++i) {
		uint32_t tmp =
			(inout[((ARRSIZE3072B - 1 - i) * 4) + 0] << 24) |
			(inout[((ARRSIZE3072B - 1 - i) * 4) + 1] << 16) |
			(inout[((ARRSIZE3072B - 1 - i) * 4) + 2] << 8) |
			(inout[((ARRSIZE3072B - 1 - i) * 4) + 3] << 0);
		a[i] = tmp;
	}

	montMul(key, aR, a, key->rr);  /* aR = a * RR / R mod M   */
	montMul(key, aaR, aR, aR);  /* aaR = aR * aR / R mod M */
	montMul(key, aaa, aaR, a);  /* aaa = aaR * a / R mod M */

	/* Make sure aaa < mod; aaa is at most 1x mod too large. */
	if (vb2_mont_ge(key, aaa)) {
		subM(key, aaa);
	}

	/* Convert to bigendian byte array */
	for (i = ARRSIZE3072B - 1; i >= 0; --i) {
		uint32_t tmp = aaa[i];
		*inout++ = (uint8_t)(tmp >> 24);
		*inout++ = (uint8_t)(tmp >> 16);
		*inout++ = (uint8_t)(tmp >>  8);
		*inout++ = (uint8_t)(tmp >>  0);
	}
}

int bdb_rsa3072b_verify(const uint8_t *key_data,
			const uint8_t *sig,
			const uint8_t *digest)
{
	const uint32_t *kdata32 = (const uint32_t *)key_data;
	struct public_key key;
	uint8_t sig_work[BDB_RSA3072B_SIG_SIZE];
	uint32_t pad_size;
	int rv;

	/* Unpack key */
	if (kdata32[0] != ARRSIZE3072B)
		return BDB_ERROR_DIGEST;  /* Wrong key size */

	key.arrsize = kdata32[0];
	key.n0inv = kdata32[1];
	key.n = kdata32 + 2;
	key.rr = kdata32 + 2 + key.arrsize;

	/* Copy signature to work buffer */
	memcpy(sig_work, sig, sizeof(sig_work));

	modpow3(&key, sig_work);

	/*
	 * Check padding.  Continue on to check the digest even if error to
	 * reduce the risk of timing based attacks.
	 */
	pad_size = key.arrsize * sizeof(uint32_t) - BDB_SHA256_DIGEST_SIZE;
	rv = vb2_check_padding(sig_work, &key, pad_size);

	/*
	 * Check digest.  Even though there are probably no timing issues here,
	 * use vb2_safe_memcmp() just to be on the safe side.  (That's also why
	 * we don't return before this check if the padding check failed.)
	 */
	if (vb2_safe_memcmp(sig_work + pad_size, digest,
			    BDB_SHA256_DIGEST_SIZE))
		rv = BDB_ERROR_DIGEST;

	return rv;
}
