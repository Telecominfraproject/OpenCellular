/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdint.h>

#ifndef VBOOT_REFERENCE_FUTILITY_H_
#define VBOOT_REFERENCE_FUTILITY_H_

/*
  Here's a structure to define the commands that futility implements.
 */
typedef struct {
  const char const * name;
  int (*handler)(int argc, char **argv);
  const char const * shorthelp;
} __attribute__ ((aligned (16))) futil_cmd_t ; /* align for x86_64 ABI */

/*
 * Create an instance in a separate section. We'll have a linker script to
 * gather them all up later, so we can refer to them without explictly
 * declaring every function in a header somewhere
 */
#define DECLARE_FUTIL_COMMAND(name, handler, shorthelp)          \
        static const char __futil_cmd_name_##name[] = #name;     \
        const futil_cmd_t __futil_cmd_##name                     \
        __attribute__((section(".futil_cmds." #name)))           \
          = { __futil_cmd_name_##name, handler, shorthelp }

/*
 * Functions to find the command table. We have to play some games here,
 * because the x86_64 ABI says this:
 *
 *   An array uses the same alignment as its elements, except that a local or
 *   global array variable that requires at least 16 bytes, or a C99 local or
 *   global variable-length array variable, always has alignment of at least
 *   16 bytes.
 *
 * The linker script doesn't know what alignment to use for __futil_cmds_start,
 * because that's determined at compile-time and unavailable to the script
 * unless we define one global futil_cmd_t in advance.
 */
static inline futil_cmd_t *futil_cmds_start(void)
{
  extern uintptr_t __futil_cmds_start[]; /* from linker script */
  uintptr_t mask = sizeof(futil_cmd_t) - 1;
  uintptr_t addr = (uintptr_t)(__futil_cmds_start);
  return (futil_cmd_t *)((addr + mask) & ~mask);
}
static inline futil_cmd_t *futil_cmds_end(void)
{
  extern uintptr_t __futil_cmds_end[]; /* from linker script */
  return (futil_cmd_t *)(&__futil_cmds_end[0]);
}

#endif /* VBOOT_REFERENCE_FUTILITY_H_ */
