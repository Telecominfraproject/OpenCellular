/*
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef VBOOT_REFERENCE_FUTILITY_VB1_HELPER_H_
#define VBOOT_REFERENCE_FUTILITY_VB1_HELPER_H_

struct vb2_kernel_preamble;
struct vb2_keyblock;
struct vb2_packed_key;

/**
 * Return the name of the vb1 crypto algorithm
 *
 * @param algo	Crypto algorithm
 * @return The name of the algorithm, or "(invalid)" if algo is not valid.
 */
const char *vb1_crypto_name(uint32_t algo);

/* Display a public key with variable indentation */
void show_pubkey(const struct vb2_packed_key *pubkey, const char *sp);

/* Other random functions needed for backward compatibility */

uint8_t *ReadConfigFile(const char *config_file, uint64_t *config_size);

uint8_t *CreateKernelBlob(uint8_t *vmlinuz_buf, uint64_t vmlinuz_size,
			  enum arch_t arch, uint64_t kernel_body_load_address,
			  uint8_t *config_data, uint64_t config_size,
			  uint8_t *bootloader_data, uint64_t bootloader_size,
			  uint64_t *blob_size_ptr);

uint8_t *SignKernelBlob(uint8_t *kernel_blob, uint64_t kernel_size,
			uint64_t padding,
			int version, uint64_t kernel_body_load_address,
			struct vb2_keyblock *keyblock,
			struct vb2_private_key *signpriv_key,
			uint32_t flags, uint64_t *vblock_size_ptr);

int WriteSomeParts(const char *outfile,
		   void *part1_data, uint64_t part1_size,
		   void *part2_data, uint64_t part2_size);

uint8_t *UnpackKPart(uint8_t *kpart_data, uint64_t kpart_size,
		     uint64_t padding,
		     struct vb2_keyblock **keyblock_ptr,
		     VbKernelPreambleHeader **preamble_ptr,
		     uint64_t *blob_size_ptr);

int UpdateKernelBlobConfig(uint8_t *kblob_data, uint64_t kblob_size,
			   uint8_t *config_data, uint64_t config_size);

int VerifyKernelBlob(uint8_t *kernel_blob,
		     uint64_t kernel_size,
		     struct vb2_packed_key *signpub_key,
		     const char *keyblock_outfile,
		     uint64_t min_version);

uint64_t kernel_cmd_line_offset(const struct vb2_kernel_preamble *preamble);

#endif	/* VBOOT_REFERENCE_FUTILITY_VB1_HELPER_H_ */
