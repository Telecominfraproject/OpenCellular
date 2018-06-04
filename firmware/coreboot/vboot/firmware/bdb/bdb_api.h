/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_FIRMWARE_BDB_BDB_API_H
#define VBOOT_REFERENCE_FIRMWARE_BDB_BDB_API_H

#include <stdint.h>
#include "vboot_register.h"
#include "nvm.h"
#include "secrets.h"
#include "bdb_flag.h"

struct vba_context {
	/* Indicate which slot is being tried: 0 - primary, 1 - secondary */
	uint8_t slot;

	/* Defined by VBA_CONTEXT_FLAG_* in bdb_flag.h */
	uint32_t flags;

	/* BDB */
	uint8_t *bdb;

	/* Secrets */
	struct bdb_secrets *secrets;

	/* NVM-RW buffer */
	struct nvmrw nvmrw;
};

/**
 * Initialize vboot process
 *
 * @param ctx
 * @return	enum bdb_return_code
 */
int vba_bdb_init(struct vba_context *ctx);

/**
 * Finalize vboot process
 *
 * @param ctx
 * @return	enum bdb_return_code
 */
int vba_bdb_finalize(struct vba_context *ctx);

/**
 * Log failed boot attempt and reset the chip
 *
 * @param ctx
 */
void vba_bdb_fail(struct vba_context *ctx);

/**
 * Update kernel and its data key version in NVM
 *
 * This is the function called from SP-RW, which receives a kernel version
 * from an AP-RW after successful verification of a kernel.
 *
 * It checks whether the version in NVM-RW is older than the reported version
 * or not. If so, it updates the version in NVM-RW.
 *
 * @param ctx
 * @param kernel_data_key_version
 * @param kernel_version
 * @return BDB_SUCCESS or BDB_ERROR_*
 */
int vba_update_kernel_version(struct vba_context *ctx,
			      uint32_t kernel_data_key_version,
			      uint32_t kernel_version);

/**
 * Write new boot unlock code to NVM-RW
 *
 * @param ctx
 * @param new_buc	New BUC to be written
 * @return		BDB_SUCCESS or BDB_ERROR_*
 */
int vba_update_buc(struct vba_context *ctx, uint8_t *new_buc);

/**
 * Derive a secret
 *
 * This derives a new secret from a secret passed from SP-RO.
 *
 * @param ctx
 * @param type		Type of secret to derive
 * @param buf		Buffer containing data to derive secret from
 * @param buf_size	Size of <buf>
 * @return		BDB_SUCCESS or BDB_ERROR_*
 */
int vba_derive_secret(struct vba_context *ctx, enum bdb_secret_type type,
		      uint8_t *wsr, const uint8_t *buf, uint32_t buf_size);

/**
 * Clear a secret
 *
 * @param ctx
 * @param type		Type of secret to clear
 * @return		BDB_SUCCESS or BDB_ERROR_*
 */
int vba_clear_secret(struct vba_context *ctx, enum bdb_secret_type type);

/**
 * Extend secrets for SP-RO
 *
 * @param ctx		struct vba_context
 * @param bdb		BDB
 * @param wsr		Pointer to working secret register contents
 * @param extend	Function to be called for extending a secret
 * @return		BDB_SUCCESS or BDB_ERROR_*
 */
typedef void (*f_extend)(const uint8_t *from, const uint8_t *by, uint8_t *to);
int vba_extend_secrets_ro(struct vba_context *ctx, const uint8_t *bdb,
			  uint8_t *wsr, f_extend extend);

/**
 * Get vboot register value
 *
 * Implemented by each chip
 *
 * @param type	Type of register to get
 * @return	Register value
 */
uint32_t vbe_get_vboot_register(enum vboot_register type);

/**
 * Set vboot register value
 *
 * Implemented by each chip
 *
 * @param type	Type of register to set
 * @param val	Value to set
 */
void vbe_set_vboot_register(enum vboot_register type, uint32_t val);

/**
 * Reset the SoC
 *
 * Implemented by each chip. This is different from reboot (a.k.a. board reset,
 * cold reset).
 */
void vbe_reset(void);

/**
 * Read contents from Non-Volatile Memory
 *
 * Implemented by each chip.
 *
 * @param type	Type of NVM
 * @param buf	Buffer where the data will be read to
 * @param size	Size of data to read
 * @return	Zero if success or non-zero otherwise
 */
int vbe_read_nvm(enum nvm_type type, uint8_t *buf, uint32_t size);

/**
 * Write contents to Non-Volatile Memory
 *
 * Implemented by each chip.
 *
 * @param type	Type of NVM
 * @param buf	Buffer where the data will be written from
 * @param size	Size of data to write
 * @return	Zero if success or non-zero otherwise
 */
int vbe_write_nvm(enum nvm_type type, void *buf, uint32_t size);

/**
 * Encrypt data by AES-256
 *
 * @param msg	Message to be encrypted
 * @param len	Length of <msg> in bytes
 * @param key	Key used for encryption
 * @param out	Buffer where encrypted message is stored
 * @return	BDB_SUCCESS or BDB_ERROR_*
 */
int vbe_aes256_encrypt(const uint8_t *msg, uint32_t len, const uint8_t *key,
		       uint8_t *out);

/**
 * Decrypt data by AES-256
 *
 * @param msg	Message to be decrypted
 * @param len	Length of <msg> in bytes
 * @param key	Key used for decryption
 * @param out	Buffer where decrypted message is stored
 * @return	BDB_SUCCESS or BDB_ERROR_*
 */
int vbe_aes256_decrypt(const uint8_t *msg, uint32_t len, const uint8_t *key,
		       uint8_t *out);

#endif
