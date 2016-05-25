/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define OPENSSL_NO_SHA
#include <openssl/pem.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2id.h"
#include "2rsa.h"
#include "2sha.h"
#include "util_misc.h"
#include "vb21_common.h"

#include "host_common.h"
#include "host_key2.h"
#include "host_misc2.h"

#include "file_type.h"
#include "futility.h"

int vb2_lookup_hash_alg(const char *str, enum vb2_hash_algorithm *alg)
{
	const struct vb2_text_vs_enum *entry;
	uint32_t val;
	char *e;

	/* try string first */
	entry = vb2_lookup_by_name(vb2_text_vs_hash, str);
	if (entry) {
		*alg = entry->num;
		return 1;
	}

	/* fine, try number */
	val = strtoul(str, &e, 0);
	if (!*str || (e && *e))
		/* that's not a number */
		return 0;

	if (!vb2_lookup_by_num(vb2_text_vs_hash, val))
		/* That's not a valid alg */
		return 0;

	*alg = val;
	return 1;
}

enum futil_file_type ft_recognize_vb21_key(uint8_t *buf, uint32_t len)
{
	struct vb2_public_key pubkey;
	struct vb2_private_key *privkey = 0;

	/* The pubkey points into buf, so nothing to free */
	if (VB2_SUCCESS == vb21_unpack_key(&pubkey, buf, len))
		return FILE_TYPE_VB2_PUBKEY;

	/* The private key unpacks into new structs */
	if (VB2_SUCCESS == vb21_private_key_unpack(&privkey, buf, len)) {
		vb2_private_key_free(privkey);
		return FILE_TYPE_VB2_PRIVKEY;
	}

	return FILE_TYPE_UNKNOWN;
}

static inline void vb2_print_bytes(const void *ptr, uint32_t len)
{
	const uint8_t *buf = (const uint8_t *)ptr;
	int i;
	for (i = 0; i < len; i++)
		printf("%02x", *buf++);
}

static int vb2_public_key_sha1sum(struct vb2_public_key *key, uint8_t *digest)
{
	struct vb21_packed_key *pkey;

	if (vb21_public_key_pack(&pkey, key))
		return 0;

	vb2_digest_buffer((uint8_t *)pkey + pkey->key_offset, pkey->key_size,
			  VB2_HASH_SHA1, digest, VB2_SHA1_DIGEST_SIZE);

	free(pkey);
	return 1;
}

int ft_show_vb21_pubkey(const char *name, uint8_t *buf, uint32_t len,
			void *data)
{
	struct vb2_public_key key;
	const struct vb2_text_vs_enum *entry;
	uint8_t sha1sum[VB2_SHA1_DIGEST_SIZE];

	/* The key's members will point into the state buffer after this. Don't
	 * free anything. */
	if (VB2_SUCCESS != vb21_unpack_key(&key, buf, len))
		return 1;

	printf("Public Key file:       %s\n", name);
	printf("  Vboot API:           2.1\n");
	printf("  Desc:                \"%s\"\n", key.desc);
	entry = vb2_lookup_by_num(vb2_text_vs_sig, key.sig_alg);
	printf("  Signature Algorithm: %d %s\n", key.sig_alg,
	       entry ? entry->name : "(invalid)");
	entry = vb2_lookup_by_num(vb2_text_vs_hash, key.hash_alg);
	printf("  Hash Algorithm:      %d %s\n", key.hash_alg,
	       entry ? entry->name : "(invalid)");
	printf("  Version:             0x%08x\n", key.version);
	printf("  ID:                  ");
	vb2_print_bytes(key.id, sizeof(*key.id));
	printf("\n");
	if (vb2_public_key_sha1sum(&key, sha1sum) &&
	    memcmp(key.id, sha1sum, sizeof(*key.id))) {
		printf("  Key sha1sum:         ");
		vb2_print_bytes(sha1sum, sizeof(sha1sum));
		printf("\n");
	}
	return 0;
}

static int vb2_private_key_sha1sum(struct vb2_private_key *key, uint8_t *digest)
{
	uint8_t *buf;
	uint32_t buflen;

	if (vb_keyb_from_rsa(key->rsa_private_key, &buf, &buflen))
		return 0;

	vb2_digest_buffer(buf, buflen, VB2_HASH_SHA1, digest,
			  VB2_SHA1_DIGEST_SIZE);

	free(buf);
	return 1;
}

int ft_show_vb21_privkey(const char *name, uint8_t *buf, uint32_t len,
			 void *data)
{
	struct vb2_private_key *key = 0;
	const struct vb2_text_vs_enum *entry;
	uint8_t sha1sum[VB2_SHA1_DIGEST_SIZE];

	if (VB2_SUCCESS != vb21_private_key_unpack(&key, buf, len))
		return 1;

	printf("Private key file:      %s\n", name);
	printf("  Vboot API:           2.1\n");
	printf("  Desc:                \"%s\"\n", key->desc ? key->desc : "");
	entry = vb2_lookup_by_num(vb2_text_vs_sig, key->sig_alg);
	printf("  Signature Algorithm: %d %s\n", key->sig_alg,
	       entry ? entry->name : "(invalid)");
	entry = vb2_lookup_by_num(vb2_text_vs_hash, key->hash_alg);
	printf("  Hash Algorithm:      %d %s\n", key->hash_alg,
	       entry ? entry->name : "(invalid)");
	printf("  ID:                  ");
	vb2_print_bytes(&key->id, sizeof(key->id));
	printf("\n");
	if (vb2_private_key_sha1sum(key, sha1sum) &&
	    memcmp(&key->id, sha1sum, sizeof(key->id))) {
		printf("  Key sha1sum:         ");
		vb2_print_bytes(sha1sum, sizeof(sha1sum));
		printf("\n");
	}
	vb2_private_key_free(key);
	return 0;
}

static RSA *rsa_from_buffer(uint8_t *buf, uint32_t len)
{
	BIO *bp;
	RSA *rsa_key;

	bp = BIO_new_mem_buf(buf, len);
	if (!bp)
		return 0;

	rsa_key = PEM_read_bio_RSAPrivateKey(bp, NULL, NULL, NULL);
	if (!rsa_key) {
		if (BIO_reset(bp) < 0)
			return 0;
		rsa_key = PEM_read_bio_RSA_PUBKEY(bp, NULL, NULL, NULL);
	}
	if (!rsa_key) {
		BIO_free(bp);
		return 0;
	}

	BIO_free(bp);

	return rsa_key;
}

enum futil_file_type ft_recognize_pem(uint8_t *buf, uint32_t len)
{
	RSA *rsa_key = rsa_from_buffer(buf, len);

	if (rsa_key) {
		RSA_free(rsa_key);
		return FILE_TYPE_PEM;
	}

	return FILE_TYPE_UNKNOWN;
}

int ft_show_pem(const char *name, uint8_t *buf, uint32_t len, void *data)
{
	RSA *rsa_key;
	uint8_t *keyb;
	uint8_t digest[VB2_SHA1_DIGEST_SIZE];
	uint32_t keyb_len;
	int i, bits;

	/* We're called only after ft_recognize_pem, so this should work. */
	rsa_key = rsa_from_buffer(buf, len);
	if (!rsa_key)
		DIE;

	/* Use to presence of the private exponent to decide if it's public */
	printf("%s Key file:      %s\n", rsa_key->d ? "Private" : "Public",
					 name);

	bits = BN_num_bits(rsa_key->n);
	printf("  Key length:          %d\n", bits);

	if (vb_keyb_from_rsa(rsa_key, &keyb, &keyb_len)) {
		printf("  Key sha1sum:         <error>");
		RSA_free(rsa_key);
		return 1;
	}

	printf("  Key sha1sum:         ");
	vb2_digest_buffer(keyb, keyb_len, VB2_HASH_SHA1,
			  digest, sizeof(digest));
	for (i = 0; i < sizeof(digest); i++)
		printf("%02x", digest[i]);
	printf("\n");

	free(keyb);
	RSA_free(rsa_key);
	return 0;
}
