/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Misc functions which need access to vb2_context but are not public APIs
 */

#include "2sysincludes.h"
#include "2api.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "2secdata.h"
#include "2sha.h"
#include "2rsa.h"
#include "vb2_common.h"

int vb2_load_fw_keyblock(struct vb2_context *ctx)
{
	struct vb2_shared_data *sd = vb2_get_sd(ctx);
	struct vb2_workbuf wb;

	uint8_t *key_data;
	uint32_t key_size;
	struct vb2_packed_key *packed_key;
	struct vb2_public_key root_key;

	struct vb2_keyblock *kb;
	uint32_t block_size;

	int rv;

	vb2_workbuf_from_ctx(ctx, &wb);

	/* Read the root key */
	key_size = sd->gbb_rootkey_size;
	key_data = vb2_workbuf_alloc(&wb, key_size);
	if (!key_data)
		return VB2_ERROR_FW_KEYBLOCK_WORKBUF_ROOT_KEY;

	rv = vb2ex_read_resource(ctx, VB2_RES_GBB, sd->gbb_rootkey_offset,
				 key_data, key_size);
	if (rv)
		return rv;

	/* Unpack the root key */
	rv = vb2_unpack_key(&root_key, key_data, key_size);
	if (rv)
		return rv;

	/* Load the firmware keyblock header after the root key */
	kb = vb2_workbuf_alloc(&wb, sizeof(*kb));
	if (!kb)
		return VB2_ERROR_FW_KEYBLOCK_WORKBUF_HEADER;

	rv = vb2ex_read_resource(ctx, VB2_RES_FW_VBLOCK, 0, kb, sizeof(*kb));
	if (rv)
		return rv;

	block_size = kb->keyblock_size;

	/*
	 * Load the entire keyblock, now that we know how big it is.  Note that
	 * we're loading the entire keyblock instead of just the piece after
	 * the header.  That means we re-read the header.  But that's a tiny
	 * amount of data, and it makes the code much more straightforward.
	 */
	kb = vb2_workbuf_realloc(&wb, sizeof(*kb), block_size);
	if (!kb)
		return VB2_ERROR_FW_KEYBLOCK_WORKBUF;

	rv = vb2ex_read_resource(ctx, VB2_RES_FW_VBLOCK, 0, kb, block_size);
	if (rv)
		return rv;

	/* Verify the keyblock */
	rv = vb2_verify_keyblock(kb, block_size, &root_key, &wb);
	if (rv) {
		vb2_fail(ctx, VB2_RECOVERY_FW_KEYBLOCK, rv);
		return rv;
	}

	/* Key version is the upper 16 bits of the composite firmware version */
	if (kb->data_key.key_version > 0xffff)
		rv = VB2_ERROR_FW_KEYBLOCK_VERSION_RANGE;
	if (!rv && kb->data_key.key_version < (sd->fw_version_secdata >> 16))
		rv = VB2_ERROR_FW_KEYBLOCK_VERSION_ROLLBACK;
	if (rv) {
		vb2_fail(ctx, VB2_RECOVERY_FW_KEY_ROLLBACK, rv);
		return rv;
	}

	sd->fw_version = kb->data_key.key_version << 16;

	/*
	 * Save the data key in the work buffer.  This overwrites the root key
	 * we read above.  That's ok, because now that we have the data key we
	 * no longer need the root key.
	 */
	packed_key = (struct vb2_packed_key *)key_data;

	packed_key->algorithm = kb->data_key.algorithm;
	packed_key->key_version = kb->data_key.key_version;
	packed_key->key_size = kb->data_key.key_size;

	/*
	 * Use memmove() instead of memcpy().  In theory, the destination will
	 * never overlap because with the source because the root key is likely
	 * to be at least as large as the data key, but there's no harm here in
	 * being paranoid.
	 */
	memmove(key_data + packed_key->key_offset,
		(uint8_t*)&kb->data_key + kb->data_key.key_offset,
		packed_key->key_size);

	/* Save the packed key offset and size */
	sd->workbuf_data_key_offset = vb2_offset_of(ctx->workbuf, key_data);
	sd->workbuf_data_key_size =
		packed_key->key_offset + packed_key->key_size;

	/* Preamble follows the keyblock in the vblock */
	sd->vblock_preamble_offset = kb->keyblock_size;

	/* Data key will persist in the workbuf after we return */
	ctx->workbuf_used = sd->workbuf_data_key_offset +
		sd->workbuf_data_key_size;

	return VB2_SUCCESS;
}

int vb2_load_fw_preamble(struct vb2_context *ctx)
{
	struct vb2_shared_data *sd = vb2_get_sd(ctx);
	struct vb2_workbuf wb;

	uint8_t *key_data = ctx->workbuf + sd->workbuf_data_key_offset;
	uint32_t key_size = sd->workbuf_data_key_size;
	struct vb2_public_key data_key;

	/* Preamble goes in the next unused chunk of work buffer */
	struct vb2_fw_preamble *pre;
	uint32_t pre_size;

	int rv;

	vb2_workbuf_from_ctx(ctx, &wb);

	/* Unpack the firmware data key */
	if (!sd->workbuf_data_key_size)
		return VB2_ERROR_FW_PREAMBLE2_DATA_KEY;

	rv = vb2_unpack_key(&data_key, key_data, key_size);
	if (rv)
		return rv;

	/* Load the firmware preamble header */
	pre = vb2_workbuf_alloc(&wb, sizeof(*pre));
	if (!pre)
		return VB2_ERROR_FW_PREAMBLE2_WORKBUF_HEADER;

	rv = vb2ex_read_resource(ctx, VB2_RES_FW_VBLOCK,
				 sd->vblock_preamble_offset,
				 pre, sizeof(*pre));
	if (rv)
		return rv;

	pre_size = pre->preamble_size;

	/* Load the entire firmware preamble, now that we know how big it is */
	pre = vb2_workbuf_realloc(&wb, sizeof(*pre), pre_size);
	if (!pre)
		return VB2_ERROR_FW_PREAMBLE2_WORKBUF;

	rv = vb2ex_read_resource(ctx, VB2_RES_FW_VBLOCK,
				 sd->vblock_preamble_offset,
				 pre, pre_size);
	if (rv)
		return rv;

	/* Work buffer now contains the data subkey data and the preamble */

	/* Verify the preamble */
	rv = vb2_verify_fw_preamble(pre, pre_size, &data_key, &wb);
	if (rv) {
		vb2_fail(ctx, VB2_RECOVERY_FW_PREAMBLE, rv);
		return rv;
	}

	/*
	 * Firmware version is the lower 16 bits of the composite firmware
	 * version.
	 */
	if (pre->firmware_version > 0xffff)
		rv = VB2_ERROR_FW_PREAMBLE_VERSION_RANGE;
	/* Combine with the key version from vb2_load_fw_keyblock() */
	sd->fw_version |= pre->firmware_version;
	if (!rv && sd->fw_version < sd->fw_version_secdata)
		rv = VB2_ERROR_FW_PREAMBLE_VERSION_ROLLBACK;
	if (rv) {
		vb2_fail(ctx, VB2_RECOVERY_FW_ROLLBACK, rv);
		return rv;
	}

	/*
	 * If this is a newer version than in secure storage, and we
	 * successfully booted the same slot last boot, roll forward the
	 * version in secure storage.
	 */
	if (sd->fw_version > sd->fw_version_secdata &&
	    sd->last_fw_slot == sd->fw_slot &&
	    sd->last_fw_result == VB2_FW_RESULT_SUCCESS) {

		sd->fw_version_secdata = sd->fw_version;
		rv = vb2_secdata_set(ctx, VB2_SECDATA_VERSIONS, sd->fw_version);
		if (rv)
			return rv;
	}

	/* Keep track of where we put the preamble */
	sd->workbuf_preamble_offset = vb2_offset_of(ctx->workbuf, pre);
	sd->workbuf_preamble_size = pre_size;

	/* Preamble will persist in work buffer after we return */
	ctx->workbuf_used = sd->workbuf_preamble_offset + pre_size;

	return VB2_SUCCESS;
}
