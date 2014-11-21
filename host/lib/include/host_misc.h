/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host-side misc functions for verified boot.
 */

#ifndef VBOOT_REFERENCE_HOST_MISC_H_
#define VBOOT_REFERENCE_HOST_MISC_H_

#include "utility.h"
#include "vboot_struct.h"

/* Copy up to dest_size-1 characters from src to dest, ensuring null
   termination (which strncpy() doesn't do).  Returns the destination
   string. */
char* StrCopy(char* dest, const char* src, int dest_size);

/* Read data from [filename].  Store the size of returned data in [size].
 *
 * Returns the data buffer, which the caller must Free(), or NULL if
 * error. */
uint8_t* ReadFile(const char* filename, uint64_t* size);

/* Read a string from a file.  Passed the destination, dest size, and
 * filename to read.
 *
 * Returns the destination, or NULL if error. */
char* ReadFileString(char* dest, int size, const char* filename);

/* Read an unsigned integer from a file and save into passed pointer.
 *
 * Returns 0 if success, -1 if error. */
int ReadFileInt(const char* filename, unsigned* value);

/* Check if a bit is set in a file which contains an integer.
 *
 * Returns 1 if the bit is set, 0 if clear, or -1 if error. */
int ReadFileBit(const char* filename, int bitmask);

/* Writes [size] bytes of [data] to [filename].
 *
 * Returns 0 if success, 1 if error. */
int WriteFile(const char* filename, const void *data, uint64_t size);

/**
 * Read data from a file into a newly allocated buffer.
 *
 * @param filename	Name of file to read from
 * @param data_ptr	On exit, pointer to newly allocated buffer with data
 *			will be stored here.  Caller must free() the buffer
 *			when done with it.
 * @param size_ptr	On exit, size of data will be stored here.
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_read_file(const char *filename, uint8_t **data_ptr, uint32_t *size_ptr);

/**
 * Write data to a file from a buffer.
 *
 * @param filename	Name of file to write to
 * @param buf		Buffer to write
 * @param size		Number of bytes of data to write
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_write_file(const char *filename, const void *buf, uint32_t size);

/**
 * Write a buffer which starts with a standard vb2_struct_common header.
 *
 * Determines the buffer size from the common header total size field.
 *
 * @param filename	Name of file to write to
 * @param buf		Buffer to write
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_write_object(const char *filename, const void *buf);

/**
 * Round up a size to a multiple of 32 bits (4 bytes).
 */
static __inline const uint32_t roundup32(uint32_t v)
{
	return (v + 3) & ~3;
}

/**
 * Return the buffer size required to hold a description string.
 *
 * If the string is NULL or empty, the size is zero.  Otherwise, it is the
 * size of a buffer which can hold the string and its null terminator,
 * rounded up to the nerest multiple of 32 bits.
 *
 * @param desc		Description string
 * @return The buffer size in bytes.
 */
uint32_t vb2_desc_size(const char *desc);

#endif  /* VBOOT_REFERENCE_HOST_MISC_H_ */
