/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * We centralize option parsing but may split operations into multiple files,
 * so let's declare the option structures in a single place (here).
 */

#ifndef VBOOT_REFERENCE_FUTILITY_OPTIONS_H_
#define VBOOT_REFERENCE_FUTILITY_OPTIONS_H_
#include <stdint.h>
#include "vboot_common.h"
#include "file_type.h"
#include "2rsa.h"

struct vb2_private_key;
struct vb2_packed_key;

struct show_option_s {
	VbPublicKey *k;
	uint8_t *fv;
	uint64_t fv_size;
	uint32_t padding;
	int strict;
	int t_flag;
	enum futil_file_type type;
	struct vb2_packed_key *pkey;
	uint32_t sig_size;
};
extern struct show_option_s show_option;

struct sign_option_s {
	VbPrivateKey *signprivate;
	VbKeyBlockHeader *keyblock;
	VbPublicKey *kernel_subkey;
	VbPrivateKey *devsignprivate;
	VbKeyBlockHeader *devkeyblock;
	uint32_t version;
	int version_specified;
	uint32_t flags;
	int flags_specified;
	char *loemdir;
	char *loemid;
	uint8_t *bootloader_data;
	uint64_t bootloader_size;
	uint8_t *config_data;
	uint64_t config_size;
	enum arch_t arch;
	int fv_specified;
	uint32_t kloadaddr;
	uint32_t padding;
	int vblockonly;
	char *outfile;
	int create_new_outfile;
	int inout_file_count;
	char *pem_signpriv;
	int pem_algo_specified;
	uint32_t pem_algo;
	char *pem_external;
	enum futil_file_type type;
	enum vb2_hash_algorithm hash_alg;
	uint32_t ro_size, rw_size;
	uint32_t ro_offset, rw_offset;
	uint32_t data_size, sig_size;
	struct vb2_private_key *prikey;
};
extern struct sign_option_s sign_option;

/* Return true if hash_alg was identified, either by name or number */
int vb2_lookup_hash_alg(const char *str, enum vb2_hash_algorithm *alg);

#endif	/* VBOOT_REFERENCE_FUTILITY_OPTIONS_H_ */
