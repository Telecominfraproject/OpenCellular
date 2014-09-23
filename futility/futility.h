/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef VBOOT_REFERENCE_FUTILITY_H_
#define VBOOT_REFERENCE_FUTILITY_H_
#include <stdint.h>

#include "vboot_common.h"
#include "gbb_header.h"
#include "host_key.h"

/* This program */
#define MYNAME "futility"

/* Here's a structure to define the commands that futility implements. */
struct futil_cmd_t {
	const char *const name;
	int (*const handler) (int argc, char **argv);
	const char *const shorthelp;
	void (*longhelp) (const char *cmd);
};

/* Macro to define a command */
#define DECLARE_FUTIL_COMMAND(NAME, HANDLER, SHORTHELP, LONGHELP) \
	const struct futil_cmd_t __cmd_##NAME = {                 \
		.name = #NAME,                                    \
		.handler = HANDLER,                               \
		.shorthelp = SHORTHELP,				  \
		.longhelp =  LONGHELP,				  \
	}

/* This is the list of pointers to all commands. */
extern const struct futil_cmd_t *const futil_cmds[];

/* Size of an array */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
#endif

/* Test an important condition at compile time, not run time */
#ifndef BUILD_ASSERT
#define _BA1_(cond, line) \
	extern int __build_assertion_ ## line[1 - 2*!(cond)]	\
	__attribute__ ((unused))
#define _BA0_(c, x) _BA1_(c, x)
#define BUILD_ASSERT(cond) _BA0_(cond, __LINE__)
#endif

/* Fatal internal stupidness */
#ifndef DIE
#define DIE do {							\
		fprintf(stderr, MYNAME ": internal error at %s:%d\n",	\
			__FILE__, __LINE__);				\
		exit(1);						\
	} while (0)
#endif

/* Debug output (off by default) */
extern int debugging_enabled;
void Debug(const char *format, ...);

/* Returns true if this looks enough like a GBB header to proceed. */
int futil_looks_like_gbb(GoogleBinaryBlockHeader *gbb, uint32_t len);

/*
 * Returns true if the gbb header is valid (and optionally updates *maxlen).
 * This doesn't verify the contents, though.
 */
int futil_valid_gbb_header(GoogleBinaryBlockHeader *gbb, uint32_t len,
			   uint32_t *maxlen);

/* Copies a file or dies with an error message */
void futil_copy_file_or_die(const char *infile, const char *outfile);

/* Wrapper for mmap/munmap. Returns 0 on success. Skips stupidly large files. */
#define MAP_RO 0
#define MAP_RW 1
int futil_map_file(int fd, int writeable, uint8_t **buf, uint32_t *len);
int futil_unmap_file(int fd, int writeable, uint8_t *buf, uint32_t len);

/* The CPU architecture is occasionally important */
enum arch_t {
	ARCH_UNSPECIFIED,
	ARCH_X86,
	ARCH_ARM,
	ARCH_MIPS
};

#endif /* VBOOT_REFERENCE_FUTILITY_H_ */
