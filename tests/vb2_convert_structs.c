/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Convert structs from vboot1 data format to new vboot2 structs
 */

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "vb2_convert_structs.h"
#include "vboot_struct.h"  /* For old struct sizes */

#include "test_common.h"

struct vb2_packed_key2 *vb2_convert_packed_key2(
				const struct vb2_packed_key *key,
				const char *desc, uint32_t *out_size)
{
	struct vb2_packed_key2 k2 = {
		.c.magic = VB2_MAGIC_PACKED_KEY2,
		.c.struct_version_major = VB2_PACKED_KEY2_VERSION_MAJOR,
		.c.struct_version_minor = VB2_PACKED_KEY2_VERSION_MINOR,
	};
	uint8_t *kbuf;

	/* Calculate description size */
	k2.c.desc_offset = sizeof(k2);
	k2.c.desc_size = roundup32(strlen(desc) + 1);

	/* Copy/initialize fields */
	k2.key_offset = k2.c.desc_offset + k2.c.desc_size;
	k2.key_size = key->key_size;
	k2.key_version = key->key_version;
	k2.sig_algorithm = vb2_crypto_to_signature(key->algorithm);
	k2.hash_algorithm = vb2_crypto_to_hash(key->algorithm);
	/* TODO: fill in a non-zero GUID */

	/* Allocate the new buffer */
	*out_size = k2.key_offset + k2.key_size;
	kbuf = malloc(*out_size);
	memset(kbuf, 0, *out_size);

	/* Copy data into the buffer */
	memcpy(kbuf, &k2, sizeof(k2));

	/* strcpy() is safe because we allocated above based on strlen() */
	strcpy((char *)(kbuf + k2.c.desc_offset), desc);
	kbuf[k2.c.desc_offset + k2.c.desc_size - 1] = 0;

	memcpy(kbuf + k2.key_offset,
	       (const uint8_t *)key + key->key_offset,
	       key->key_size);

	/* Return the newly allocated buffer */
	return (struct vb2_packed_key2 *)kbuf;
}
