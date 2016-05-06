/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "2sysincludes.h"
#include "2hmac.h"
#include "2sha.h"
#include "bdb_api.h"
#include "bdb_struct.h"
#include "bdb.h"
#include "secrets.h"

static int get_constant(const uint8_t *buf, uint32_t buf_size,
			const uint8_t *constant, uint8_t *out)
{
	int digest_size = vb2_digest_size(VB2_HASH_SHA256);
	const struct bdb_key *key = (const struct bdb_key *)buf;

	if (!buf)
		return !BDB_SUCCESS;

	if (bdb_check_key(key, buf_size))
		return !BDB_SUCCESS;

	if (vb2_digest_buffer(buf, buf_size, VB2_HASH_SHA256, out, digest_size))
		return !BDB_SUCCESS;

	memcpy(out + digest_size, constant,
	       BDB_CONSTANT_BLOCK_SIZE - digest_size);

	return BDB_SUCCESS;
}

int vba_derive_secret(struct vba_context *ctx, enum bdb_secret_type type,
		      const uint8_t *buf, uint32_t buf_size)
{
	uint8_t c[BDB_CONSTANT_BLOCK_SIZE];
	const uint8_t *b = (const uint8_t *)c;
	uint8_t *s;
	uint8_t *o;

	switch (type) {
	case BDB_SECRET_TYPE_BDB:
		s = o = ctx->ro_secrets->bdb;
		if (get_constant(buf, buf_size, secret_constant_q, c))
			return BDB_ERROR_SECRET_BDB;
		break;
	case BDB_SECRET_TYPE_BOOT_PATH:
		s = o = ctx->ro_secrets->boot_path;
		if (get_constant(buf, buf_size, secret_constant_l, c))
			return BDB_ERROR_SECRET_BOOT_PATH;
		break;
	case BDB_SECRET_TYPE_BOOT_VERIFIED:
		s = o = ctx->ro_secrets->boot_verified;
		if (ctx->flags & VBA_CONTEXT_FLAG_KERNEL_DATA_KEY_VERIFIED)
			b = secret_constant_kv1;
		else
			b = secret_constant_kv0;
		break;
	case BDB_SECRET_TYPE_BUC:
		s = ctx->ro_secrets->boot_verified;
		b = secret_constant_c;
		o = ctx->rw_secrets->buc;
		break;
	default:
		return BDB_ERROR_SECRET_TYPE;
	}

	vb2_sha256_extend(s, b, o);

	return BDB_SUCCESS;
}

int vba_clear_secret(struct vba_context *ctx, enum bdb_secret_type type)
{
	uint8_t *s;

	switch (type) {
	case BDB_SECRET_TYPE_NVM_RW:
		s = ctx->ro_secrets->nvm_rw;
		break;
	case BDB_SECRET_TYPE_BDB:
		s = ctx->ro_secrets->bdb;
		break;
	case BDB_SECRET_TYPE_BOOT_PATH:
		s = ctx->ro_secrets->boot_path;
		break;
	case BDB_SECRET_TYPE_BOOT_VERIFIED:
		s = ctx->ro_secrets->boot_verified;
		break;
	case BDB_SECRET_TYPE_BUC:
		s = ctx->rw_secrets->buc;
		break;
	default:
		return BDB_ERROR_SECRET_TYPE;
	}

	memset(s, 0, BDB_SECRET_SIZE);
	return BDB_SUCCESS;
}
