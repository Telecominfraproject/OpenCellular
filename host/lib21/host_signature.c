/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for signatures.
 */

#include <openssl/rsa.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "2sha.h"
#include "vb2_common.h"
#include "host_common.h"
#include "host_key2.h"
#include "host_signature2.h"
#include "host_misc.h"

/**
 * Get the digest info for a hash algorithm
 *
 * @param hash_alg	Hash algorithm
 * @param buf_ptr	On success, points to the digest info
 * @param size_ptr	On success, contains the info size in bytes
 * @return VB2_SUCCESS, or non-zero error code on failure.
 */
static int vb2_digest_info(enum vb2_hash_algorithm hash_alg,
			   const uint8_t **buf_ptr,
			   uint32_t *size_ptr)
{
	*buf_ptr = NULL;
	*size_ptr = 0;

	switch (hash_alg) {
#if VB2_SUPPORT_SHA1
	case VB2_HASH_SHA1:
		{
			static const uint8_t info[] = {
				0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e,
				0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14
			};
			*buf_ptr = info;
			*size_ptr = sizeof(info);
			return VB2_SUCCESS;
		}
#endif
#if VB2_SUPPORT_SHA256
	case VB2_HASH_SHA256:
		{
			static const uint8_t info[] = {
				0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
				0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
				0x00, 0x04, 0x20
			};
			*buf_ptr = info;
			*size_ptr = sizeof(info);
			return VB2_SUCCESS;
		}
#endif
#if VB2_SUPPORT_SHA512
	case VB2_HASH_SHA512:
		{
			static const uint8_t info[] = {
				0x30, 0x51, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
				0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03, 0x05,
				0x00, 0x04, 0x40
			};
			*buf_ptr = info;
			*size_ptr = sizeof(info);
			return VB2_SUCCESS;
		}
#endif
	default:
		return VB2_ERROR_DIGEST_INFO;
	}
}

int vb2_sign_data(struct vb2_signature **sig_ptr,
		  const uint8_t *data,
		  uint32_t size,
		  const struct vb2_private_key *key,
		  const char *desc)
{
	struct vb2_signature s = {
		.c.magic = VB2_MAGIC_SIGNATURE,
		.c.struct_version_major = VB2_SIGNATURE_VERSION_MAJOR,
		.c.struct_version_minor = VB2_SIGNATURE_VERSION_MINOR,
		.c.fixed_size = sizeof(s),
		.sig_alg = key->sig_alg,
		.hash_alg = key->hash_alg,
		.data_size = size,
		.id = key->id,
	};

	struct vb2_digest_context dc;
	uint32_t digest_size;
	const uint8_t *info = NULL;
	uint32_t info_size = 0;
	uint32_t sig_digest_size;
	uint8_t *sig_digest;
	uint8_t *buf;

	*sig_ptr = NULL;

	/* Use key description if no description supplied */
	if (!desc)
		desc = key->desc;

	s.c.desc_size = vb2_desc_size(desc);

	s.sig_offset = s.c.fixed_size + s.c.desc_size;
	s.sig_size = vb2_sig_size(key->sig_alg, key->hash_alg);
	if (!s.sig_size)
		return VB2_SIGN_DATA_SIG_SIZE;

	s.c.total_size = s.sig_offset + s.sig_size;

	/* Determine digest size and allocate buffer */
	if (s.sig_alg != VB2_SIG_NONE) {
		if (vb2_digest_info(s.hash_alg, &info, &info_size))
			return VB2_SIGN_DATA_DIGEST_INFO;
	}

	digest_size = vb2_digest_size(key->hash_alg);
	if (!digest_size)
		return VB2_SIGN_DATA_DIGEST_SIZE;

	sig_digest_size = info_size + digest_size;
	sig_digest = malloc(sig_digest_size);
	if (!sig_digest)
		return VB2_SIGN_DATA_DIGEST_ALLOC;

	/* Prepend digest info, if any */
	if (info_size)
		memcpy(sig_digest, info, info_size);

	/* Calculate hash digest */
	if (vb2_digest_init(&dc, s.hash_alg)) {
		free(sig_digest);
		return VB2_SIGN_DATA_DIGEST_INIT;
	}

	if (vb2_digest_extend(&dc, data, size)) {
		free(sig_digest);
		return VB2_SIGN_DATA_DIGEST_EXTEND;
	}

	if (vb2_digest_finalize(&dc, sig_digest + info_size, digest_size)) {
		free(sig_digest);
		return VB2_SIGN_DATA_DIGEST_FINALIZE;
	}

	/* Allocate signature buffer and copy header */
	buf = calloc(1, s.c.total_size);
	memcpy(buf, &s, sizeof(s));

	/* strcpy() is ok because we allocated buffer based on desc length */
	if (desc)
		strcpy((char *)buf + s.c.fixed_size, desc);

	if (s.sig_alg == VB2_SIG_NONE) {
		/* Bare hash signature is just the digest */
		memcpy(buf + s.sig_offset, sig_digest, sig_digest_size);
	} else {
		/* RSA-encrypt the signature */
		if (RSA_private_encrypt(sig_digest_size,
					sig_digest,
					buf + s.sig_offset,
					key->rsa_private_key,
					RSA_PKCS1_PADDING) == -1) {
			free(sig_digest);
			free(buf);
			return VB2_SIGN_DATA_RSA_ENCRYPT;
		}
	}

	free(sig_digest);
	*sig_ptr = (struct vb2_signature *)buf;
	return VB2_SUCCESS;
}

int vb2_sig_size_for_key(uint32_t *size_ptr,
			 const struct vb2_private_key *key,
			 const char *desc)
{
	uint32_t size = vb2_sig_size(key->sig_alg, key->hash_alg);

	if (!size)
		return VB2_ERROR_SIG_SIZE_FOR_KEY;

	size += sizeof(struct vb2_signature);
	size += vb2_desc_size(desc ? desc : key->desc);

	*size_ptr = size;
	return VB2_SUCCESS;
}

int vb2_sig_size_for_keys(uint32_t *size_ptr,
			  const struct vb2_private_key **key_list,
			  uint32_t key_count)
{
	uint32_t total = 0, size = 0;
	int rv, i;

	*size_ptr = 0;

	for (i = 0; i < key_count; i++) {
		rv = vb2_sig_size_for_key(&size, key_list[i], NULL);
		if (rv)
			return rv;
		total += size;
	}

	*size_ptr = total;
	return VB2_SUCCESS;
}

int vb2_sign_object(uint8_t *buf,
		    uint32_t sig_offset,
		    const struct vb2_private_key *key,
		    const char *desc)
{
	struct vb2_struct_common *c = (struct vb2_struct_common *)buf;
	struct vb2_signature *sig = NULL;
	int rv;

	rv = vb2_sign_data(&sig, buf, sig_offset, key, desc);
	if (rv)
		return rv;

	if (sig_offset + sig->c.total_size > c->total_size) {
		free(sig);
		return VB2_SIGN_OBJECT_OVERFLOW;
	}

	memcpy(buf + sig_offset, sig, sig->c.total_size);
	free(sig);

	return VB2_SUCCESS;
}

int vb2_sign_object_multiple(uint8_t *buf,
			     uint32_t sig_offset,
			     const struct vb2_private_key **key_list,
			     uint32_t key_count)
{
	struct vb2_struct_common *c = (struct vb2_struct_common *)buf;
	uint32_t sig_next = sig_offset;
	int rv, i;

	for (i = 0; i < key_count; i++)	{
		struct vb2_signature *sig = NULL;

		rv = vb2_sign_data(&sig, buf, sig_offset, key_list[i], NULL);
		if (rv)
			return rv;

		if (sig_next + sig->c.total_size > c->total_size) {
			free(sig);
			return VB2_SIGN_OBJECT_OVERFLOW;
		}

		memcpy(buf + sig_next, sig, sig->c.total_size);
		sig_next += sig->c.total_size;
		free(sig);
	}

	return VB2_SUCCESS;
}
