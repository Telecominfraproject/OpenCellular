/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for keyblocks
 */

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "vb2_common.h"
#include "host_common.h"
#include "host_key2.h"
#include "host_keyblock2.h"
#include "host_misc.h"
#include "host_signature2.h"

int vb2_keyblock_create(struct vb2_keyblock **kb_ptr,
			const struct vb2_public_key *data_key,
			const struct vb2_private_key **signing_keys,
			uint32_t signing_key_count,
			uint32_t flags,
			const char *desc)
{
	struct vb2_keyblock kb = {
		.c.magic = VB2_MAGIC_KEYBLOCK,
		.c.struct_version_major = VB2_KEYBLOCK_VERSION_MAJOR,
		.c.struct_version_minor = VB2_KEYBLOCK_VERSION_MAJOR,
		.c.fixed_size = sizeof(kb),
		.flags = flags,
		.sig_count = signing_key_count,
	};

	struct vb2_packed_key *key = NULL;
	uint32_t sig_size;
	uint8_t *buf;

	*kb_ptr = NULL;

	/* Determine component sizes */
	if (!desc)
		desc = data_key->desc;
	kb.c.desc_size = vb2_desc_size(desc);
	kb.key_offset = kb.c.fixed_size + kb.c.desc_size;

	if (vb2_sig_size_for_keys(&sig_size, signing_keys, signing_key_count))
		return VB2_KEYBLOCK_CREATE_SIG_SIZE;

	if (vb2_public_key_pack(&key, data_key))
		return VB2_KEYBLOCK_CREATE_DATA_KEY;

	kb.sig_offset = kb.key_offset + key->c.total_size;
	kb.c.total_size = kb.sig_offset + sig_size;

	/* Allocate buffer and copy header and data key */
	buf = calloc(1, kb.c.total_size);
	if (!buf) {
		free(key);
		return VB2_KEYBLOCK_CREATE_ALLOC;
	}

	memcpy(buf, &kb, sizeof(kb));
	if (kb.c.desc_size)
		strcpy((char *)buf + kb.c.fixed_size, desc);
	memcpy(buf + kb.key_offset, key, key->c.total_size);
	free(key);

	/* Sign the keyblock */
	if (vb2_sign_object_multiple(buf, kb.sig_offset, signing_keys,
				     signing_key_count)) {
		free(buf);
		return VB2_KEYBLOCK_CREATE_SIGN;
	}

	*kb_ptr = (struct vb2_keyblock *)buf;
	return VB2_SUCCESS;
}
