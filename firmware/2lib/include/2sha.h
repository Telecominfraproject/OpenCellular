/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_2SHA_H_
#define VBOOT_REFERENCE_2SHA_H_

#include "2crypto.h"
#include "2struct.h"

/* Hash algorithms may be disabled individually to save code space */

#ifndef VB2_SUPPORT_SHA1
#define VB2_SUPPORT_SHA1 1
#endif

#ifndef VB2_SUPPORT_SHA256
#define VB2_SUPPORT_SHA256 1
#endif

#ifndef VB2_SUPPORT_SHA512
#define VB2_SUPPORT_SHA512 1
#endif

#define VB2_SHA1_DIGEST_SIZE 20
#define VB2_SHA1_BLOCK_SIZE 64

/* Context structs for hash algorithms */

struct vb2_sha1_context {
	uint32_t count;
	uint32_t state[5];
#if defined(HAVE_ENDIAN_H) && defined(HAVE_LITTLE_ENDIAN)
	union {
		uint8_t b[VB2_SHA1_BLOCK_SIZE];
		uint32_t w[VB2_SHA1_BLOCK_SIZE / sizeof(uint32_t)];
	} buf;
#else
	uint8_t buf[VB2_SHA1_BLOCK_SIZE];
#endif
};

#define VB2_SHA256_DIGEST_SIZE 32
#define VB2_SHA256_BLOCK_SIZE 64

struct vb2_sha256_context {
	uint32_t h[8];
	uint32_t total_size;
	uint32_t size;
	uint8_t block[2 * VB2_SHA256_BLOCK_SIZE];
};

#define VB2_SHA512_DIGEST_SIZE 64
#define VB2_SHA512_BLOCK_SIZE 128

struct vb2_sha512_context {
	uint64_t h[8];
	uint32_t total_size;
	uint32_t size;
	uint8_t block[2 * VB2_SHA512_BLOCK_SIZE];
};

/* Hash algorithm independent digest context; includes all of the above. */
struct vb2_digest_context {
	/* Context union for all algorithms */
	union {
#if VB2_SUPPORT_SHA1
		struct vb2_sha1_context sha1;
#endif
#if VB2_SUPPORT_SHA256
		struct vb2_sha256_context sha256;
#endif
#if VB2_SUPPORT_SHA512
		struct vb2_sha512_context sha512;
#endif
	};

	/* Current hash algorithm */
	enum vb2_hash_algorithm hash_alg;

	/* 1 if digest is computed with vb2ex_hwcrypto routines, else 0 */
	int using_hwcrypto;
};

/**
 * Initialize a hash context.
 *
 * @param ctx		Hash context
 */
void vb2_sha1_init(struct vb2_sha1_context *ctx);
void vb2_sha256_init(struct vb2_sha256_context *ctx);
void vb2_sha512_init(struct vb2_sha512_context *ctx);

/**
 * Update (extend) a hash.
 *
 * @param ctx		Hash context
 * @param data		Data to hash
 * @param size		Length of data in bytes
 */
void vb2_sha1_update(struct vb2_sha1_context *ctx,
		     const uint8_t *data,
		     uint32_t size);
void vb2_sha256_update(struct vb2_sha256_context *ctx,
		       const uint8_t *data,
		       uint32_t size);
void vb2_sha512_update(struct vb2_sha512_context *ctx,
		       const uint8_t *data,
		       uint32_t size);

/**
 * Finalize a hash digest.
 *
 * @param ctx		Hash context
 * @param digest	Destination for hash; must be VB_SHA*_DIGEST_SIZE bytes
 */
void vb2_sha1_finalize(struct vb2_sha1_context *ctx, uint8_t *digest);
void vb2_sha256_finalize(struct vb2_sha256_context *ctx, uint8_t *digest);
void vb2_sha512_finalize(struct vb2_sha512_context *ctx, uint8_t *digest);

/**
 * Convert vb2_crypto_algorithm to vb2_hash_algorithm.
 *
 * @param algorithm	Crypto algorithm (vb2_crypto_algorithm)
 *
 * @return The hash algorithm for that crypto algorithm, or VB2_HASH_INVALID if
 * the crypto algorithm or its corresponding hash algorithm is invalid or not
 * supported.
 */
enum vb2_hash_algorithm vb2_crypto_to_hash(uint32_t algorithm);

/**
 * Return the size of the digest for a hash algorithm.
 *
 * @param hash_alg	Hash algorithm
 * @return The size of the digest, or 0 if error.
 */
int vb2_digest_size(enum vb2_hash_algorithm hash_alg);

/**
 * Initialize a digest context for doing block-style digesting.
 *
 * @param dc		Digest context
 * @param hash_alg	Hash algorithm
 * @return VB2_SUCCESS, or non-zero on error.
 */
int vb2_digest_init(struct vb2_digest_context *dc,
		    enum vb2_hash_algorithm hash_alg);

/**
 * Extend a digest's hash with another block of data.
 *
 * @param dc		Digest context
 * @param buf		Data to hash
 * @param size		Length of data in bytes
 * @return VB2_SUCCESS, or non-zero on error.
 */
int vb2_digest_extend(struct vb2_digest_context *dc,
		      const uint8_t *buf,
		      uint32_t size);

/**
 * Finalize a digest and store the result.
 *
 * The destination digest should be at least vb2_digest_size(algorithm).
 *
 * @param dc		Digest context
 * @param digest	Destination for digest
 * @param digest_size	Length of digest buffer in bytes.
 * @return VB2_SUCCESS, or non-zero on error.
 */
int vb2_digest_finalize(struct vb2_digest_context *dc,
			uint8_t *digest,
			uint32_t digest_size);

#endif  /* VBOOT_REFERENCE_2SHA_H_ */
