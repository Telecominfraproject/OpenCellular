/*
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef VBOOT_REFERENCE_FUTILITY_VB1_HELPER_H_
#define VBOOT_REFERENCE_FUTILITY_VB1_HELPER_H_

uint8_t *ReadConfigFile(const char *config_file, uint64_t *config_size);

uint8_t *CreateKernelBlob(uint8_t *vmlinuz_buf, uint64_t vmlinuz_size,
			  enum arch_t arch, uint64_t kernel_body_load_address,
			  uint8_t *config_data, uint64_t config_size,
			  uint8_t *bootloader_data, uint64_t bootloader_size,
			  uint64_t *blob_size_ptr);

uint8_t *SignKernelBlob(uint8_t *kernel_blob, uint64_t kernel_size,
			uint64_t padding,
			int version, uint64_t kernel_body_load_address,
			VbKeyBlockHeader *keyblock, VbPrivateKey *signpriv_key,
			uint32_t flags, uint64_t *vblock_size_ptr);

int WriteSomeParts(const char *outfile,
		   void *part1_data, uint64_t part1_size,
		   void *part2_data, uint64_t part2_size);

uint8_t *UnpackKPart(uint8_t *kpart_data, uint64_t kpart_size,
		     uint64_t padding,
		     VbKeyBlockHeader **keyblock_ptr,
		     VbKernelPreambleHeader **preamble_ptr,
		     uint64_t *blob_size_ptr);

int UpdateKernelBlobConfig(uint8_t *kblob_data, uint64_t kblob_size,
			   uint8_t *config_data, uint64_t config_size);

int VerifyKernelBlob(uint8_t *kernel_blob,
		     uint64_t kernel_size,
		     VbPublicKey *signpub_key,
		     const char *keyblock_outfile,
		     uint64_t min_version);

uint64_t KernelCmdLineOffset(VbKernelPreambleHeader *preamble);

#endif	/* VBOOT_REFERENCE_FUTILITY_VB1_HELPER_H_ */
