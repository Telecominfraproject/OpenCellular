/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Misc functions which need access to vb2_context but are not public APIs
 */

#include "2sysincludes.h"
#include "2api.h"
#include "2common.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "2secdata.h"
#include "2sha.h"
#include "2rsa.h"
#include "vb2_common.h"

/**
 * Read an object with a common struct header from a verified boot resource.
 *
 * On success, an object buffer will be allocated in the work buffer, the
 * object will be stored into the buffer, and *buf_ptr will point to the
 * object.
 *
 * @param ctx		Vboot context
 * @param index		Resource index to read
 * @param offset	Byte offset within resource to start at
 * @param buf_ptr	Destination for object pointer
 * @return VB2_SUCCESS, or error code on error.
 */
int vb2_read_resource_object(struct vb2_context *ctx,
			     enum vb2_resource_index index,
			     uint32_t offset,
			     struct vb2_workbuf *wb,
			     void **buf_ptr)
{
	struct vb2_struct_common c;
	void *buf;
	int rv;

	*buf_ptr = NULL;

	/* Read the common header */
	rv = vb2ex_read_resource(ctx, index, offset, &c, sizeof(c));
	if (rv)
		return rv;

	/* Allocate a buffer for the object, now that we know how big it is */
	buf = vb2_workbuf_alloc(wb, c.total_size);
	if (!buf)
		return VB2_ERROR_READ_RESOURCE_OBJECT_BUF;

	/* Read the object */
	rv = vb2ex_read_resource(ctx, index, offset, buf, c.total_size);
	if (rv) {
		vb2_workbuf_free(wb, c.total_size);
		return rv;
	}

	/* Save the pointer */
	*buf_ptr = buf;
	return VB2_SUCCESS;
}

int vb2_load_fw_keyblock(struct vb2_context *ctx)
{
	struct vb2_shared_data *sd = vb2_get_sd(ctx);
	struct vb2_workbuf wb;

	uint8_t *key_data;
	uint32_t key_size;
	struct vb2_packed_key *packed_key;
	struct vb2_public_key root_key;
	struct vb2_keyblock *kb;

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

	/*
	 * Load the firmware keyblock common header into the work buffer after
	 * the root key.
	 */
	rv = vb2_read_resource_object(ctx, VB2_RES_FW_VBLOCK, 0, &wb,
				      (void **)&kb);
	if (rv)
		return rv;

	/* Verify the keyblock */
	rv = vb2_verify_keyblock(kb, kb->c.total_size, &root_key, &wb);
	if (rv) {
		vb2_fail(ctx, VB2_RECOVERY_FW_KEYBLOCK, rv);
		return rv;
	}

	/* Preamble follows the keyblock in the vblock */
	sd->vblock_preamble_offset = kb->c.total_size;

	packed_key = (struct vb2_packed_key *)((uint8_t *)kb + kb->key_offset);

	/* Key version is the upper 16 bits of the composite firmware version */
	if (packed_key->key_version > 0xffff)
		rv = VB2_ERROR_FW_KEYBLOCK_VERSION_RANGE;
	if (!rv && packed_key->key_version < (sd->fw_version_secdata >> 16))
		rv = VB2_ERROR_FW_KEYBLOCK_VERSION_ROLLBACK;
	if (rv) {
		vb2_fail(ctx, VB2_RECOVERY_FW_KEY_ROLLBACK, rv);
		return rv;
	}

	sd->fw_version = packed_key->key_version << 16;

	/*
	 * Save the data key in the work buffer.  This overwrites the root key
	 * we read above.  That's ok, because now that we have the data key we
	 * no longer need the root key.
	 *
	 * Use memmove() instead of memcpy().  In theory, the destination will
	 * never overlap with the source because the root key is likely to be
	 * at least as large as the data key, but there's no harm here in being
	 * paranoid.
	 */
	memmove(key_data, packed_key, packed_key->c.total_size);
	packed_key = (struct vb2_packed_key *)key_data;

	/* Save the packed key offset and size */
	sd->workbuf_data_key_offset = vb2_offset_of(ctx->workbuf, key_data);
	sd->workbuf_data_key_size = packed_key->c.total_size;

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

	int rv;

	vb2_workbuf_from_ctx(ctx, &wb);

	/* Unpack the firmware data key */
	if (!sd->workbuf_data_key_size)
		return VB2_ERROR_FW_PREAMBLE2_DATA_KEY;

	rv = vb2_unpack_key(&data_key, key_data, key_size);
	if (rv)
		return rv;

	/* Load the firmware preamble */
	rv = vb2_read_resource_object(ctx, VB2_RES_FW_VBLOCK,
				      sd->vblock_preamble_offset, &wb,
				      (void **)&pre);
	if (rv)
		return rv;

	/* Work buffer now contains the data subkey data and the preamble */

	/* Verify the preamble */
	rv = vb2_verify_fw_preamble(pre, pre->c.total_size, &data_key, &wb);
	if (rv) {
		vb2_fail(ctx, VB2_RECOVERY_FW_PREAMBLE, rv);
		return rv;
	}

	/* Move the preamble down now that the data key is no longer used */
	memmove(key_data, pre, pre->c.total_size);
	pre = (struct vb2_fw_preamble *)key_data;

	/* Data key is now gone */
	sd->workbuf_data_key_offset = sd->workbuf_data_key_size = 0;

	/*
	 * Firmware version is the lower 16 bits of the composite firmware
	 * version.
	 */
	if (pre->fw_version > 0xffff)
		rv = VB2_ERROR_FW_PREAMBLE_VERSION_RANGE;
	/* Combine with the key version from vb2_load_fw_keyblock() */
	sd->fw_version |= pre->fw_version;
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
	sd->workbuf_preamble_size = pre->c.total_size;

	/* Preamble will persist in work buffer after we return */
	ctx->workbuf_used = sd->workbuf_preamble_offset +
		sd->workbuf_preamble_size;

	return VB2_SUCCESS;
}
