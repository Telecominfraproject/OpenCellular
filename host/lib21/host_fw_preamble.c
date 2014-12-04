/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for keyblocks
 */

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "host_common.h"
#include "host_fw_preamble2.h"
#include "host_key2.h"
#include "host_keyblock2.h"
#include "host_misc.h"
#include "host_signature2.h"
#include "vb2_common.h"

int vb2_fw_preamble_create(struct vb2_fw_preamble **fp_ptr,
			   const struct vb2_private_key *signing_key,
			   const struct vb2_signature **hash_list,
			   uint32_t hash_count,
			   uint32_t fw_version,
			   uint32_t flags,
			   const char *desc)
{
	struct vb2_fw_preamble fp = {
		.c.magic = VB2_MAGIC_FW_PREAMBLE,
		.c.struct_version_major = VB2_FW_PREAMBLE_VERSION_MAJOR,
		.c.struct_version_minor = VB2_FW_PREAMBLE_VERSION_MAJOR,
		.c.fixed_size = sizeof(fp),
		.c.desc_size = vb2_desc_size(desc),
		.flags = flags,
		.fw_version = fw_version,
		.hash_count = hash_count,
	};

	uint32_t hash_next;
	uint32_t sig_size;
	uint8_t *buf;
	int i;

	*fp_ptr = NULL;

	/* Determine component sizes */
	hash_next = fp.hash_offset = fp.c.fixed_size + fp.c.desc_size;

	for (i = 0; i < hash_count; i++)
		hash_next += hash_list[i]->c.total_size;

	fp.sig_offset = hash_next;

	if (vb2_sig_size_for_key(&sig_size, signing_key, NULL))
		return VB2_FW_PREAMBLE_CREATE_SIG_SIZE;

	fp.c.total_size = fp.sig_offset + sig_size;

	/* Allocate buffer and copy components */
	buf = calloc(fp.c.total_size, 1);
	if (!buf)
		return VB2_FW_PREAMBLE_CREATE_ALLOC;

	memcpy(buf, &fp, sizeof(fp));
	if (fp.c.desc_size)
		strcpy((char *)buf + fp.c.fixed_size, desc);

	hash_next = fp.hash_offset;
	for (i = 0; i < hash_count; i++) {
		memcpy(buf + hash_next, hash_list[i],
		       hash_list[i]->c.total_size);
		hash_next += hash_list[i]->c.total_size;
	}

	/* Sign the preamble */
	if (vb2_sign_object(buf, fp.sig_offset, signing_key, NULL)) {
		free(buf);
		return VB2_FW_PREAMBLE_CREATE_SIGN;
	}

	*fp_ptr = (struct vb2_fw_preamble *)buf;
	return VB2_SUCCESS;
}
