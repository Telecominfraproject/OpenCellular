/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_FIRMWARE_BDB_SECRETS_H_
#define VBOOT_REFERENCE_FIRMWARE_BDB_SECRETS_H_

#define BDB_SECRET_SIZE		32

/*
 * Secrets passed to SP-RW by SP-RO. How it's passed depends on chips.
 * These are hash-extended by SP-RW.
 */
struct bdb_ro_secrets {
	uint8_t nvm_wp[BDB_SECRET_SIZE];
	uint8_t nvm_rw[BDB_SECRET_SIZE];
	uint8_t bdb[BDB_SECRET_SIZE];
	uint8_t boot_verified[BDB_SECRET_SIZE];
	uint8_t boot_path[BDB_SECRET_SIZE];
};

/*
 * Additional secrets SP-RW derives from RO secrets. This can be independently
 * updated as more secrets are needed.
 */
struct bdb_rw_secrets {
	uint8_t buc[BDB_SECRET_SIZE];
};

#endif
