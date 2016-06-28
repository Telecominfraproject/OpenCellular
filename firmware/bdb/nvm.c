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
#include "nvm.h"
#include "secrets.h"

static int nvmrw_validate(const void *buf, uint32_t size)
{
	const struct nvmrw *nvm = buf;

	if (nvm->struct_magic != NVM_RW_MAGIC)
		return BDB_ERROR_NVM_RW_MAGIC;

	if (nvm->struct_major_version != NVM_HEADER_VERSION_MAJOR)
		return BDB_ERROR_NVM_STRUCT_VERSION;

	if (size < nvm->struct_size)
		return BDB_ERROR_NVM_STRUCT_SIZE;

	/*
	 * We allow any sizes between min and max so that we can handle minor
	 * version mismatches. Reader can be older than data or the other way
	 * around. FW in slot B can upgrade NVM-RW but fails to qualify as a
	 * stable boot path. Then, FW in slot A is invoked which is older than
	 * the NVM-RW written by FW in slot B.
	 */
	if (nvm->struct_size < NVM_RW_MIN_STRUCT_SIZE ||
			NVM_RW_MAX_STRUCT_SIZE < nvm->struct_size)
		return BDB_ERROR_NVM_STRUCT_SIZE;

	return BDB_SUCCESS;
}

static int nvmrw_verify(const struct bdb_ro_secrets *secrets,
			const struct nvmrw *nvm, uint32_t size)
{
	uint8_t mac[NVM_HMAC_SIZE];
	int rv;

	if (!secrets || !nvm)
		return BDB_ERROR_NVM_INVALID_PARAMETER;

	rv = nvmrw_validate(nvm, size);
	if (rv)
		return rv;

	/* Compute and verify HMAC */
	if (hmac(VB2_HASH_SHA256, secrets->nvm_rw, BDB_SECRET_SIZE,
		 nvm, nvm->struct_size - sizeof(mac), mac, sizeof(mac)))
		return BDB_ERROR_NVM_RW_HMAC;
	/* TODO: Use safe_memcmp */
	if (memcmp(mac, nvm->hmac, sizeof(mac)))
		return BDB_ERROR_NVM_RW_INVALID_HMAC;

	return BDB_SUCCESS;
}

int nvmrw_write(struct vba_context *ctx, enum nvm_type type)
{
	struct nvmrw *nvm = &ctx->nvmrw;
	int retry = NVM_MAX_WRITE_RETRY;
	int rv;

	if (!ctx)
		return BDB_ERROR_NVM_INVALID_PARAMETER;

	if (!ctx->ro_secrets)
		return BDB_ERROR_NVM_INVALID_SECRET;

	rv = nvmrw_validate(nvm, sizeof(*nvm));
	if (rv)
		return rv;

	/* Update HMAC */
	hmac(VB2_HASH_SHA256, ctx->ro_secrets->nvm_rw, BDB_SECRET_SIZE,
	     nvm, nvm->struct_size - sizeof(nvm->hmac),
	     nvm->hmac, sizeof(nvm->hmac));

	while (retry--) {
		uint8_t buf[sizeof(struct nvmrw)];
		if (vbe_write_nvm(type, nvm, nvm->struct_size))
			continue;
		if (vbe_read_nvm(type, buf, sizeof(buf)))
			continue;
		if (memcmp(buf, nvm, sizeof(buf)))
			continue;
		/* Write success */
		return BDB_SUCCESS;
	}

	/* NVM seems corrupted. Go to chip recovery mode */
	return BDB_ERROR_NVM_WRITE;
}

static int read_verify_nvmrw(enum nvm_type type,
			     const struct bdb_ro_secrets *secrets,
			     uint8_t *buf, uint32_t buf_size)
{
	struct nvmrw *nvm = (struct nvmrw *)buf;
	int rv;

	/* Read minimum amount */
	if (vbe_read_nvm(type, buf, NVM_MIN_STRUCT_SIZE))
		return BDB_ERROR_NVM_VBE_READ;

	/* Validate the content */
	rv = nvmrw_validate(buf, buf_size);
	if (rv)
		return rv;

	/* Read full body */
	if (vbe_read_nvm(type, buf, nvm->struct_size))
		return BDB_ERROR_NVM_VBE_READ;

	/* Verify the content */
	rv = nvmrw_verify(secrets, nvm, sizeof(*nvm));
		return rv;

	return BDB_SUCCESS;
}

int nvmrw_read(struct vba_context *ctx)
{
	uint8_t buf1[NVM_RW_MAX_STRUCT_SIZE];
	uint8_t buf2[NVM_RW_MAX_STRUCT_SIZE];
	struct nvmrw *nvm1 = (struct nvmrw *)buf1;
	struct nvmrw *nvm2 = (struct nvmrw *)buf2;
	int rv1, rv2;

	/* Read and verify the 1st copy */
	rv1 = read_verify_nvmrw(NVM_TYPE_RW_PRIMARY, ctx->ro_secrets,
				buf1, sizeof(buf1));

	/* Read and verify the 2nd copy */
	rv2 = read_verify_nvmrw(NVM_TYPE_RW_SECONDARY, ctx->ro_secrets,
				buf2, sizeof(buf2));

	if (rv1 == BDB_SUCCESS && rv2 == BDB_SUCCESS) {
		/* Sync primary and secondary based on update_count. */
		if (nvm1->update_count > nvm2->update_count)
			rv2 = !BDB_SUCCESS;
		else if (nvm1->update_count < nvm2->update_count)
			rv1 = !BDB_SUCCESS;
	} else if (rv1 != BDB_SUCCESS && rv2 != BDB_SUCCESS){
		/* Abort. Neither was successful. */
		return rv1;
	}

	if (rv1 == BDB_SUCCESS)
		/* both copies are good. use primary copy */
		memcpy(&ctx->nvmrw, buf1, sizeof(ctx->nvmrw));
	else
		/* primary is bad but secondary is good. */
		memcpy(&ctx->nvmrw, buf2, sizeof(ctx->nvmrw));

	if (ctx->nvmrw.struct_minor_version != NVM_HEADER_VERSION_MINOR) {
		/*
		 * Upgrade or downgrade is required. So, we need to write both.
		 * When upgrading, this is the place where new fields should be
		 * initialized. We don't increment update_count.
		 */
		ctx->nvmrw.struct_minor_version = NVM_HEADER_VERSION_MINOR;
		ctx->nvmrw.struct_size = sizeof(ctx->nvmrw);
		/* We don't worry about calculating hmac twice because
		 * this is a corner case. */
		rv1 = nvmrw_write(ctx, NVM_TYPE_RW_PRIMARY);
		rv2 = nvmrw_write(ctx, NVM_TYPE_RW_SECONDARY);
	} else if (rv1 != BDB_SUCCESS) {
		/* primary copy is bad. sync it with secondary copy */
		rv1 = nvmrw_write(ctx, NVM_TYPE_RW_PRIMARY);
	} else if (rv2 != BDB_SUCCESS){
		/* secondary copy is bad. sync it with primary copy */
		rv2 = nvmrw_write(ctx, NVM_TYPE_RW_SECONDARY);
	} else {
		/* Both copies are good and versions are same as the reader.
		 * Skip writing. This should be the common case. */
	}

	if (rv1 || rv2)
		return rv1 ? rv1 : rv2;

	return BDB_SUCCESS;
}

static int nvmrw_init(struct vba_context *ctx)
{
	if (nvmrw_read(ctx))
		return BDB_ERROR_NVM_INIT;

	return BDB_SUCCESS;
}

int vba_update_kernel_version(struct vba_context *ctx,
			      uint32_t kernel_data_key_version,
			      uint32_t kernel_version)
{
	struct nvmrw *nvm = &ctx->nvmrw;

	if (nvmrw_verify(ctx->ro_secrets, nvm, sizeof(*nvm))) {
		if (nvmrw_init(ctx))
			return BDB_ERROR_NVM_INIT;
	}

	if (nvm->min_kernel_data_key_version < kernel_data_key_version ||
			nvm->min_kernel_version < kernel_version) {
		int rv1, rv2;

		/* Roll forward versions */
		nvm->min_kernel_data_key_version = kernel_data_key_version;
		nvm->min_kernel_version = kernel_version;

		/* Increment update counter */
		nvm->update_count++;

		/* Update both copies */
		rv1 = nvmrw_write(ctx, NVM_TYPE_RW_PRIMARY);
		rv2 = nvmrw_write(ctx, NVM_TYPE_RW_SECONDARY);
		if (rv1 || rv2)
			return BDB_ERROR_RECOVERY_REQUEST;
	}

	return BDB_SUCCESS;
}

int vba_update_buc(struct vba_context *ctx, uint8_t *new_buc)
{
	struct nvmrw *nvm = &ctx->nvmrw;
	uint8_t buc[BUC_ENC_DIGEST_SIZE];
	int rv1, rv2;

	if (nvmrw_verify(ctx->ro_secrets, nvm, sizeof(*nvm))) {
		if (nvmrw_init(ctx))
			return BDB_ERROR_NVM_INIT;
	}

	/* Encrypt new BUC
	 * Note that we do not need to decide whether we should use hardware
	 * crypto or not because this is supposed to be running in RW code. */
	if (vbe_aes256_encrypt(new_buc, BUC_ENC_DIGEST_SIZE,
			       ctx->rw_secrets->buc, buc))
		return BDB_ERROR_ENCRYPT_BUC;

	/* Return if new BUC is same as current one. */
	if (!memcmp(buc, nvm->buc_enc_digest, sizeof(buc)))
		return BDB_SUCCESS;

	memcpy(nvm->buc_enc_digest, buc, sizeof(buc));

	/* Increment update counter */
	nvm->update_count++;

	/* Write new BUC */
	rv1 = nvmrw_write(ctx, NVM_TYPE_RW_PRIMARY);
	rv2 = nvmrw_write(ctx, NVM_TYPE_RW_SECONDARY);
	if (rv1 || rv2)
		return BDB_ERROR_WRITE_BUC;

	return BDB_SUCCESS;
}

int nvmrw_get(struct vba_context *ctx, enum nvmrw_var var, uint32_t *val)
{
	struct nvmrw *nvm = &ctx->nvmrw;

	/* No init or verify so that this can be called from futility.
	 * Callers are responsible for init and verify. */

	switch (var) {
	case NVMRW_VAR_UPDATE_COUNT:
		*val = nvm->update_count;
		break;
	case NVMRW_VAR_MIN_KERNEL_DATA_KEY_VERSION:
		*val = nvm->min_kernel_data_key_version;
		break;
	case NVMRW_VAR_MIN_KERNEL_VERSION:
		*val = nvm->min_kernel_version;
		break;
	case NVMRW_VAR_BUC_TYPE:
		*val = nvm->buc_type;
		break;
	case NVMRW_VAR_FLAG_BUC_PRESENT:
		*val = nvm->flags & NVM_RW_FLAG_BUC_PRESENT;
		break;
	case NVMRW_VAR_FLAG_DFM_DISABLE:
		*val = nvm->flags & NVM_RW_FLAG_DFM_DISABLE;
		break;
	case NVMRW_VAR_FLAG_DOSM:
		*val = nvm->flags & NVM_RW_FLAG_DOSM;
		break;
	default:
		return BDB_ERROR_NVM_INVALID_PARAMETER;
	}

	return BDB_SUCCESS;
}

#define MAX_8BIT_UINT ((((uint64_t)1) << 8) - 1)

int nvmrw_set(struct vba_context *ctx, enum nvmrw_var var, uint32_t val)
{
	struct nvmrw *nvm = &ctx->nvmrw;

	/* No init or verify so that this can be called from futility.
	 * Callers are responsible for init and verify. */

	switch (var) {
	case NVMRW_VAR_UPDATE_COUNT:
		nvm->update_count = val;
		break;
	case NVMRW_VAR_MIN_KERNEL_DATA_KEY_VERSION:
		nvm->min_kernel_data_key_version = val;
		break;
	case NVMRW_VAR_MIN_KERNEL_VERSION:
		nvm->min_kernel_version = val;
		break;
	case NVMRW_VAR_BUC_TYPE:
		if (val > MAX_8BIT_UINT)
			return BDB_ERROR_NVM_INVALID_PARAMETER;
		nvm->buc_type = val;
		break;
	case NVMRW_VAR_FLAG_BUC_PRESENT:
		nvm->flags &= ~NVM_RW_FLAG_BUC_PRESENT;
		nvm->flags |= val ? NVM_RW_FLAG_BUC_PRESENT : 0;
		break;
	case NVMRW_VAR_FLAG_DFM_DISABLE:
		nvm->flags &= ~NVM_RW_FLAG_DFM_DISABLE;
		nvm->flags |= val ? NVM_RW_FLAG_DFM_DISABLE : 0;
		break;
	case NVMRW_VAR_FLAG_DOSM:
		nvm->flags &= ~NVM_RW_FLAG_DOSM;
		nvm->flags |= val ? NVM_RW_FLAG_DOSM : 0;
		break;
	default:
		return BDB_ERROR_NVM_INVALID_PARAMETER;
	}

	return BDB_SUCCESS;
}
