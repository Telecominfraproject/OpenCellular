/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdint.h>

#ifndef VBOOT_REFERENCE_FUTILITY_H_
#define VBOOT_REFERENCE_FUTILITY_H_

/* Here's a structure to define the commands that futility implements. */
struct futil_cmd_t {
	const char *const name;
	int (*const handler) (int argc, char **argv);
	const char *const shorthelp;
};

/*
 * Macro to define a command.
 *
 * This defines the struct, then puts a pointer to it in a separate section.
 * We'll have a linker script to gather the pointers up later, so we can refer
 * to them without explictly declaring every function in a header somewhere.
 */
#define DECLARE_FUTIL_COMMAND(NAME, HANDLER, SHORTHELP)           \
        static const struct futil_cmd_t __cmd_##NAME = {          \
                .name = #NAME,                                    \
                .handler = HANDLER,                               \
                .shorthelp = SHORTHELP                            \
        };                                                        \
        const struct futil_cmd_t * const __cmd_ptr_##NAME         \
        __attribute__((section(".futil_cmds." #NAME)))            \
          = &__cmd_##NAME

/* This is the list of pointers to all commands. */
extern const struct futil_cmd_t *const futil_cmds[];

/* Size of an array */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
#endif

/* Test an important condition at compile time, not run time */
#ifndef BUILD_ASSERT
#define _BA1_(cond, line) \
        extern int __build_assertion_ ## line[1 - 2*!(cond)] \
        __attribute__ ((unused))
#define _BA0_(c, x) _BA1_(c, x)
#define BUILD_ASSERT(cond) _BA0_(cond, __LINE__)
#endif

#endif /* VBOOT_REFERENCE_FUTILITY_H_ */
