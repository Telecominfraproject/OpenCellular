/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for verified boot.
 *
 * TODO: change all 'return 0', 'return 1' into meaningful return codes.
 */

#include <string.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "host_common.h"
#include "host_key2.h"
#include "cryptolib.h"
#include "utility.h"
#include "vb2_common.h"
#include "vboot_common.h"

struct vb2_fw_preamble *vb2_create_fw_preamble(
	uint32_t firmware_version,
	const struct vb2_packed_key *kernel_subkey,
	const struct vb2_signature *body_signature,
	const struct vb2_private_key *signing_key,
	uint32_t flags)
{
	uint32_t signed_size = (sizeof(struct vb2_fw_preamble) +
				kernel_subkey->key_size +
				body_signature->sig_size);
	uint32_t block_size = signed_size +
		vb2_rsa_sig_size(signing_key->sig_alg);

	/* Allocate preamble */
	struct vb2_fw_preamble *h =
		(struct vb2_fw_preamble *)calloc(block_size, 1);
	if (!h)
		return NULL;

	uint8_t *kernel_subkey_dest = (uint8_t *)(h + 1);
	uint8_t *body_sig_dest = kernel_subkey_dest + kernel_subkey->key_size;
	uint8_t *block_sig_dest = body_sig_dest + body_signature->sig_size;

	h->header_version_major = FIRMWARE_PREAMBLE_HEADER_VERSION_MAJOR;
	h->header_version_minor = FIRMWARE_PREAMBLE_HEADER_VERSION_MINOR;
	h->preamble_size = block_size;
	h->firmware_version = firmware_version;
	h->flags = flags;

	/* Copy data key */
	PublicKeyInit((VbPublicKey *)&h->kernel_subkey, kernel_subkey_dest,
		      kernel_subkey->key_size);
	PublicKeyCopy((VbPublicKey *)&h->kernel_subkey,
		      (VbPublicKey *)kernel_subkey);

	/* Copy body signature */
	vb2_init_signature(&h->body_signature,
			   body_sig_dest, body_signature->sig_size, 0);
	vb2_copy_signature(&h->body_signature, body_signature);

	/* Set up signature struct so we can calculate the signature */
	vb2_init_signature(&h->preamble_signature, block_sig_dest,
			   vb2_rsa_sig_size(signing_key->sig_alg), signed_size);

	/* Calculate signature */
	struct vb2_signature *sig =
		vb2_calculate_signature((uint8_t *)h, signed_size, signing_key);
	vb2_copy_signature(&h->preamble_signature, sig);
	free(sig);

	/* Return the header */
	return h;
}
