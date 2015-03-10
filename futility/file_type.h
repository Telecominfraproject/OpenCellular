/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef VBOOT_REFERENCE_FUTILITY_FILE_TYPE_H_
#define VBOOT_REFERENCE_FUTILITY_FILE_TYPE_H_

/* What type of things do I know how to handle? */
enum futil_file_type {
#define FILE_TYPE(A, B, C) FILE_TYPE_ ## A,
#include "file_type.inc"
#undef FILE_TYPE
	NUM_FILE_TYPES
};

/* Short name for file types */
const char * const futil_file_type_name(enum futil_file_type type);
/* Description of file type */
const char * const futil_file_type_desc(enum futil_file_type type);

/* Name to enum. Returns true on success. */
int futil_file_str_to_type(const char *str, enum futil_file_type *tptr);

/* Print the list of type names and exit with the given value. */
void print_file_types_and_exit(int retval);

/* Lookup an type by name. Return true on success */
int futil_str_to_file_type(const char *str, enum futil_file_type *type);

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
enum futil_file_type recognize_vb1_key(uint8_t *buf, uint32_t len);
enum futil_file_type recognize_vb2_key(uint8_t *buf, uint32_t len);
enum futil_file_type recognize_pem(uint8_t *buf, uint32_t len);

#endif	/* VBOOT_REFERENCE_FUTILITY_FILE_TYPE_H_ */
