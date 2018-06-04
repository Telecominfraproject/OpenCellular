/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_BDB_NVM_H_
#define VBOOT_REFERENCE_BDB_NVM_H_

#include <stdint.h>
#include "bdb_struct.h"
#include "bdb_api.h"

enum nvm_type {
	NVM_TYPE_WP_PRIMARY,
	NVM_TYPE_WP_SECONDARY,
	NVM_TYPE_RW_PRIMARY,
	NVM_TYPE_RW_SECONDARY,
};

#define NVM_RW_MAGIC			0x3052766e

/* Size in bytes of encrypted BUC (Boot Unlock Code) */
#define BUC_ENC_DIGEST_SIZE		32
/* Size in bytes of HMAC of struct NVM-RW */
#define NVM_HMAC_SIZE			BDB_SHA256_DIGEST_SIZE

#define NVM_RW_FLAG_BUC_PRESENT		(1 << 0)
#define NVM_RW_FLAG_DFM_DISABLE		(1 << 1)
#define NVM_RW_FLAG_DOSM		(1 << 2)

/* This is the minimum size of the data needed to learn the actual size */
#define NVM_MIN_STRUCT_SIZE		8

#define NVM_HEADER_VERSION_MAJOR	1
#define NVM_HEADER_VERSION_MINOR	1

/* Maximum number of retries for writing NVM */
#define NVM_MAX_WRITE_RETRY		2

struct nvmrw {
	/* Magic number to identify struct */
	uint32_t struct_magic;

	/* Structure version */
	uint8_t struct_major_version;
	uint8_t struct_minor_version;

	/* Size of struct in bytes. 96 for version 1.0 */
	uint16_t struct_size;

	/* Number of updates to structure contents */
	uint32_t update_count;

	/* Flags: NVM_RW_FLAG_* */
	uint32_t flags;

	/* Minimum valid kernel data key version */
	uint32_t min_kernel_data_key_version;

	/* Minimum valid kernel version */
	uint32_t min_kernel_version;

	/* Type of BUC */
	uint8_t buc_type;

	uint8_t reserved0[7];

	/* Encrypted BUC */
	uint8_t buc_enc_digest[BUC_ENC_DIGEST_SIZE];

	/* SHA-256 HMAC of the struct contents. Add new fields before this. */
	uint8_t hmac[NVM_HMAC_SIZE];
} __attribute__((packed));

/*
 * List of variables stored in NVM-RW. This should be exported and used by
 * firmware and futility to access data in NVM-RW.
 */
enum nvmrw_var {
	NVMRW_VAR_UPDATE_COUNT,
	NVMRW_VAR_FLAGS,
	NVMRW_VAR_MIN_KERNEL_DATA_KEY_VERSION,
	NVMRW_VAR_MIN_KERNEL_VERSION,
	NVMRW_VAR_BUC_TYPE,
	NVMRW_VAR_FLAG_BUC_PRESENT,
	NVMRW_VAR_FLAG_DFM_DISABLE,
	NVMRW_VAR_FLAG_DOSM,
};

/* Size of the version 1.0 */
#define NVM_RW_MIN_STRUCT_SIZE		96
/* 4 Kbit EEPROM divided by 4 regions (RO,RW) x (1st,2nd) = 128 KB */
#define NVM_RW_MAX_STRUCT_SIZE		128

/* For nvm_rw_read and nvm_write */
struct vba_context;

/**
 * Read NVM-RW contents into the context
 *
 * @param ctx	struct vba_context
 * @return	BDB_SUCCESS or BDB_ERROR_NVM_*
 */
int nvmrw_read(struct vba_context *ctx);

/**
 * Write to NVM-RW from the context
 *
 * @param ctx	struct vba_context
 * @param type	NVM_TYPE_RW_*
 * @return	BDB_SUCCESS or BDB_ERROR_NVM_*
 */
int nvmrw_write(struct vba_context *ctx, enum nvm_type type);

/**
 * Get a value of NVM-RW variable
 *
 * Callers are responsible for init and verify of ctx->nvmrw.
 *
 * @param ctx	struct vba_context
 * @param var	Index of the variable
 * @param val	Destination where the value is stored
 * @return	BDB_SUCCESS or BDB_ERROR_NVM_*
 */
int nvmrw_get(struct vba_context *ctx, enum nvmrw_var var, uint32_t *val);

/**
 * Set a value in NVM-RW variable
 *
 * Callers are responsible for init and verify of ctx->nvmrw.
 *
 * @param ctx	struct vba_context
 * @param var	Index of the variable
 * @param val	Value to be set
 * @return	BDB_SUCCESS or BDB_ERROR_NVM_*
 */
int nvmrw_set(struct vba_context *ctx, enum nvmrw_var var, uint32_t val);

#endif
