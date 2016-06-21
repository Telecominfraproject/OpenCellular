/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for verified boot.
 */

#include <stdio.h>

#include "2sysincludes.h"

#include "2api.h"
#include "2common.h"
#include "2rsa.h"
#include "2sha.h"
#include "cryptolib.h"
#include "host_common.h"
#include "host_key.h"
#include "host_key2.h"
#include "host_keyblock.h"
#include "vb2_common.h"
#include "vb2_struct.h"
#include "vboot_common.h"

struct vb2_keyblock *vb2_create_keyblock(
		const struct vb2_packed_key *data_key,
		const struct vb2_private_key *signing_key,
		uint32_t flags)
{
	/* Allocate key block */
	uint32_t signed_size = sizeof(struct vb2_keyblock) + data_key->key_size;
	uint32_t sig_data_size =
		(signing_key ? vb2_rsa_sig_size(signing_key->sig_alg) : 0);
	uint32_t block_size =
		signed_size + VB2_SHA512_DIGEST_SIZE + sig_data_size;
	struct vb2_keyblock *h = (struct vb2_keyblock *)calloc(block_size, 1);
	if (!h)
		return NULL;

	uint8_t *data_key_dest = (uint8_t *)(h + 1);
	uint8_t *block_chk_dest = data_key_dest + data_key->key_size;
	uint8_t *block_sig_dest = block_chk_dest + VB2_SHA512_DIGEST_SIZE;

	memcpy(h->magic, KEY_BLOCK_MAGIC, KEY_BLOCK_MAGIC_SIZE);
	h->header_version_major = KEY_BLOCK_HEADER_VERSION_MAJOR;
	h->header_version_minor = KEY_BLOCK_HEADER_VERSION_MINOR;
	h->keyblock_size = block_size;
	h->keyblock_flags = flags;

	/* Copy data key */
	vb2_init_packed_key(&h->data_key, data_key_dest, data_key->key_size);
	vb2_copy_packed_key(&h->data_key, data_key);

	/* Set up signature structs so we can calculate the signatures */
	vb2_init_signature(&h->keyblock_hash, block_chk_dest,
			   VB2_SHA512_DIGEST_SIZE, signed_size);
	if (signing_key) {
		vb2_init_signature(&h->keyblock_signature, block_sig_dest,
				   sig_data_size, signed_size);
	} else {
		memset(&h->keyblock_signature, 0,
		       sizeof(h->keyblock_signature));
	}

	/* Calculate hash */
	struct vb2_signature *chk =
		vb2_sha512_signature((uint8_t*)h, signed_size);
	vb2_copy_signature(&h->keyblock_hash, chk);
	free(chk);

	/* Calculate signature */
	if (signing_key) {
		struct vb2_signature *sigtmp =
			vb2_calculate_signature((uint8_t*)h,
						signed_size,
						signing_key);
		vb2_copy_signature(&h->keyblock_signature, sigtmp);
		free(sigtmp);
	}

	/* Return the header */
	return h;
}

/* TODO(gauravsh): This could easily be integrated into the function above
 * since the code is almost a mirror - I have kept it as such to avoid changing
 * the existing interface. */
struct vb2_keyblock *vb2_create_keyblock_external(
		const struct vb2_packed_key *data_key,
                const char *signing_key_pem_file,
                uint32_t algorithm,
                uint32_t flags,
                const char *external_signer)
{
	uint32_t signed_size = sizeof(struct vb2_keyblock) + data_key->key_size;
	uint32_t sig_data_size = vb2_rsa_sig_size(algorithm);
	uint32_t block_size =
		signed_size + VB2_SHA512_DIGEST_SIZE + sig_data_size;

	/* Allocate key block */
	struct vb2_keyblock *h = (struct vb2_keyblock *)calloc(block_size, 1);
	if (!h)
		return NULL;
	if (!signing_key_pem_file || !data_key || !external_signer)
		return NULL;

	uint8_t *data_key_dest = (uint8_t *)(h + 1);
	uint8_t *block_chk_dest = data_key_dest + data_key->key_size;
	uint8_t *block_sig_dest = block_chk_dest + VB2_SHA512_DIGEST_SIZE;

	memcpy(h->magic, KEY_BLOCK_MAGIC, KEY_BLOCK_MAGIC_SIZE);
	h->header_version_major = KEY_BLOCK_HEADER_VERSION_MAJOR;
	h->header_version_minor = KEY_BLOCK_HEADER_VERSION_MINOR;
	h->keyblock_size = block_size;
	h->keyblock_flags = flags;

	/* Copy data key */
	vb2_init_packed_key(&h->data_key, data_key_dest, data_key->key_size);
	vb2_copy_packed_key(&h->data_key, data_key);

	/* Set up signature structs so we can calculate the signatures */
	vb2_init_signature(&h->keyblock_hash, block_chk_dest,
			   VB2_SHA512_DIGEST_SIZE, signed_size);
	vb2_init_signature(&h->keyblock_signature, block_sig_dest,
			   sig_data_size, signed_size);

	/* Calculate checksum */
	struct vb2_signature *chk =
		vb2_sha512_signature((uint8_t*)h, signed_size);
	vb2_copy_signature(&h->keyblock_hash, chk);
	free(chk);

	/* Calculate signature */
struct vb2_signature *sigtmp = (struct vb2_signature *)
		CalculateSignature_external((uint8_t*)h, signed_size,
					    signing_key_pem_file, algorithm,
					    external_signer);
	free(sigtmp);

	/* Return the header */
	return h;
}

struct vb2_keyblock *vb2_read_keyblock(const char *filename)
{
	uint8_t workbuf[VB2_WORKBUF_RECOMMENDED_SIZE];
	struct vb2_workbuf wb;
	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	struct vb2_keyblock *block;
	uint32_t file_size;
	if (VB2_SUCCESS !=
	    vb2_read_file(filename, (uint8_t **)&block, &file_size)) {
		fprintf(stderr, "Error reading key block file: %s\n", filename);
		return NULL;
	}

	/* Verify the hash of the key block, since we can do that without
	 * the public signing key. */
	if (VB2_SUCCESS != vb2_verify_keyblock_hash(block, file_size, &wb)) {
		fprintf(stderr, "Invalid key block file: %s\n", filename);
		free(block);
		return NULL;
	}

	return block;
}


int vb2_write_keyblock(const char *filename,
		       const struct vb2_keyblock *keyblock)
{
	return vb2_write_file(filename, keyblock, keyblock->keyblock_size);
}
