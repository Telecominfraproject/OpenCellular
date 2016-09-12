/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_FIRMWARE_BDB_SECRETS_H_
#define VBOOT_REFERENCE_FIRMWARE_BDB_SECRETS_H_

#define BDB_SECRET_SIZE		32
#define BDB_CONSTANT_BLOCK_SIZE	64

enum bdb_secret_type {
	BDB_SECRET_TYPE_WSR,
	BDB_SECRET_TYPE_NVM_WP,
	BDB_SECRET_TYPE_NVM_RW,
	BDB_SECRET_TYPE_BDB,
	BDB_SECRET_TYPE_BOOT_VERIFIED,
	BDB_SECRET_TYPE_BOOT_PATH,
	BDB_SECRET_TYPE_BUC,
	BDB_SECRET_TYPE_COUNT,	/* Last entry. Add new secrets before this. */
};

/*
 * Struct storing BDB secrets passed between SP-RO and SP-RW.
 */
struct bdb_secrets {
	uint8_t nvm_rw[BDB_SECRET_SIZE];
	uint8_t bdb[BDB_SECRET_SIZE];
	uint8_t boot_verified[BDB_SECRET_SIZE];
	uint8_t boot_path[BDB_SECRET_SIZE];
	uint8_t nvm_wp[BDB_SECRET_SIZE];
	uint8_t buc[BDB_SECRET_SIZE];
};

#endif
