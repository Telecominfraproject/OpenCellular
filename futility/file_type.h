/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef VBOOT_REFERENCE_FUTILITY_FILE_TYPE_H_
#define VBOOT_REFERENCE_FUTILITY_FILE_TYPE_H_

/* What type of things do I know how to handle? */
enum futil_file_type {
	FILE_TYPE_UNKNOWN,
	FILE_TYPE_PUBKEY,			/* VbPublicKey */
	FILE_TYPE_KEYBLOCK,			/* VbKeyBlockHeader */
	FILE_TYPE_FW_PREAMBLE,			/* VbFirmwarePreambleHeader */
	FILE_TYPE_GBB,				/* GoogleBinaryBlockHeader */
	FILE_TYPE_BIOS_IMAGE,			/* Chrome OS BIOS image */
	FILE_TYPE_OLD_BIOS_IMAGE,		/* Old Chrome OS BIOS image */
	FILE_TYPE_KERN_PREAMBLE,		/* VbKernelPreambleHeader */

	/* These are FILE_TYPE_UNKNOWN, but we've been told more about them */
	FILE_TYPE_RAW_FIRMWARE,			/* FW_MAIN_A, etc. */
	FILE_TYPE_RAW_KERNEL,			/* vmlinuz, *.uimg, etc. */

	FILE_TYPE_CHROMIUMOS_DISK,		/* At least it has a GPT */
	FILE_TYPE_PRIVKEY,			/* VbPrivateKey */

	NUM_FILE_TYPES
};

/* Names for them */
const char * const futil_file_type_str(enum futil_file_type type);

/*
 * This tries to match the buffer content to one of the known file types.
 */
enum futil_file_type futil_file_type_buf(uint8_t *buf, uint32_t len);

/*
 * This opens a file and tries to match it to one of the known file types.
 * It's not an error if it returns FILE_TYPE_UKNOWN.
 */
enum futil_file_err futil_file_type(const char *filename,
				    enum futil_file_type *type);

/* Routines to identify particular file types. */
enum futil_file_type recognize_bios_image(uint8_t *buf, uint32_t len);
enum futil_file_type recognize_gbb(uint8_t *buf, uint32_t len);
enum futil_file_type recognize_vblock1(uint8_t *buf, uint32_t len);
enum futil_file_type recognize_gpt(uint8_t *buf, uint32_t len);
enum futil_file_type recognize_privkey(uint8_t *buf, uint32_t len);

#endif	/* VBOOT_REFERENCE_FUTILITY_FILE_TYPE_H_ */
