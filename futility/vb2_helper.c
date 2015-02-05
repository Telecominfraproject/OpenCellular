/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define OPENSSL_NO_SHA
#include <openssl/pem.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2guid.h"
#include "2rsa.h"
#include "util_misc.h"
#include "vb2_common.h"
#include "vb2_struct.h"

#include "host_common.h"
#include "host_key2.h"
#include "host_misc2.h"

#include "file_type.h"
#include "futility.h"
#include "traversal.h"

enum futil_file_type recognize_vb2_key(uint8_t *buf, uint32_t len)
{
	struct vb2_public_key pubkey;
	struct vb2_private_key *privkey = 0;

	/* The pubkey points into buf, so nothing to free */
	if (VB2_SUCCESS == vb2_unpack_key(&pubkey, buf, len))
		return FILE_TYPE_VB2_PUBKEY;

	/* The private key unpacks into new structs */
	if (VB2_SUCCESS == vb2_private_key_unpack(&privkey, buf, len)) {
		vb2_private_key_free(privkey);
		return FILE_TYPE_VB2_PRIVKEY;
	}

	return FILE_TYPE_UNKNOWN;
}

static void vb2_print_public_key_sha1sum(struct vb2_public_key *key)
{
	struct vb2_packed_key *pkey;
	uint8_t *digest;
	int i;

	if (vb2_public_key_pack(&pkey, key)) {
		printf("<error>");
		return;
	}

	digest = DigestBuf((uint8_t *)pkey + pkey->key_offset,
			   pkey->key_size, SHA1_DIGEST_ALGORITHM);
	for (i = 0; i < SHA1_DIGEST_SIZE; i++)
		printf("%02x", digest[i]);

	free(digest);
	free(pkey);
}

int futil_cb_show_vb2_pubkey(struct futil_traverse_state_s *state)
{
	struct vb2_public_key key;
	char guid_str[VB2_GUID_MIN_STRLEN];
	const struct vb2_text_vs_enum *entry;

	/* The key's members will point into the state buffer after this. Don't
	 * free anything. */
	if (VB2_SUCCESS != vb2_unpack_key(&key, state->my_area->buf,
					  state->my_area->len))
		return 1;

	if (VB2_SUCCESS != vb2_guid_to_str(key.guid, guid_str,
					   sizeof(guid_str)))
		return 1;

	printf("Public Key file:       %s\n", state->in_filename);
	printf("  Vboot API:           2.1\n");
	printf("  Desc:                \"%s\"\n", key.desc);
	entry = vb2_lookup_by_num(vb2_text_vs_sig, key.sig_alg);
	printf("  Signature Algorithm: %d %s\n", key.sig_alg,
	       entry ? entry->name : "(invalid)");
	entry = vb2_lookup_by_num(vb2_text_vs_hash, key.hash_alg);
	printf("  Hash Algorithm:      %d %s\n", key.hash_alg,
	       entry ? entry->name : "(invalid)");
	printf("  GUID:                %s\n", guid_str);
	printf("  Version:             0x%08x\n", key.version);
	printf("  Key sha1sum:         ");
	vb2_print_public_key_sha1sum(&key);
	printf("\n");

	return 0;
}

static void vb2_print_private_key_sha1sum(struct vb2_private_key *key)
{
	uint8_t *buf, *digest;
	uint32_t buflen;
	int i;

	if (vb_keyb_from_rsa(key->rsa_private_key, &buf, &buflen)) {
		printf("<error>");
		return;
	}

	digest = DigestBuf(buf, buflen, SHA1_DIGEST_ALGORITHM);
	for (i = 0; i < SHA1_DIGEST_SIZE; i++)
		printf("%02x", digest[i]);

	free(digest);
	free(buf);
}

int futil_cb_show_vb2_privkey(struct futil_traverse_state_s *state)
{
	struct vb2_private_key *key = 0;
	char guid_str[VB2_GUID_MIN_STRLEN];
	const struct vb2_text_vs_enum *entry;

	if (VB2_SUCCESS != vb2_private_key_unpack(&key, state->my_area->buf,
						  state->my_area->len))
		return 1;

	if (VB2_SUCCESS != vb2_guid_to_str(&key->guid, guid_str,
					   sizeof(guid_str))) {
		vb2_private_key_free(key);
		return 1;
	}


	printf("Private key file:      %s\n", state->in_filename);
	printf("  Vboot API:           2.1\n");
	printf("  Desc:                \"%s\"\n", key->desc ? key->desc : "");
	entry = vb2_lookup_by_num(vb2_text_vs_sig, key->sig_alg);
	printf("  Signature Algorithm: %d %s\n", key->sig_alg,
	       entry ? entry->name : "(invalid)");
	entry = vb2_lookup_by_num(vb2_text_vs_hash, key->hash_alg);
	printf("  Hash Algorithm:      %d %s\n", key->hash_alg,
	       entry ? entry->name : "(invalid)");
	printf("  GUID:                %s\n", guid_str);
	printf("  Key sha1sum:         ");
	vb2_print_private_key_sha1sum(key);
	printf("\n");

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
		BIO_free(bp);
		return 0;
	}

	BIO_free(bp);

	return rsa_key;
}

enum futil_file_type recognize_pem(uint8_t *buf, uint32_t len)
{
	RSA *rsa_key = rsa_from_buffer(buf, len);

	if (rsa_key) {
		RSA_free(rsa_key);
		return FILE_TYPE_PEM;
	}

	return FILE_TYPE_UNKNOWN;
}

int futil_cb_show_pem(struct futil_traverse_state_s *state)
{
	RSA *rsa_key;
	uint8_t *keyb, *digest;
	uint32_t keyb_len;
	int i, bits;

	printf("Private Key file:      %s\n", state->in_filename);

	/* We're called only after recognize_pem, so this should work. */
	rsa_key = rsa_from_buffer(state->my_area->buf, state->my_area->len);
	if (!rsa_key)
		DIE;

	bits = BN_num_bits(rsa_key->n);
	printf("  Key length:          %d\n", bits);

	if (vb_keyb_from_rsa(rsa_key, &keyb, &keyb_len)) {
		printf("  Key sha1sum:         <error>");
		RSA_free(rsa_key);
		return 1;
	}

	printf("  Key sha1sum:         ");
	digest = DigestBuf(keyb, keyb_len, SHA1_DIGEST_ALGORITHM);
	for (i = 0; i < SHA1_DIGEST_SIZE; i++)
		printf("%02x", digest[i]);
	printf("\n");

	free(digest);
	free(keyb);
	RSA_free(rsa_key);
	return 0;
}
