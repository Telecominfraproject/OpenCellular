/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * This module provides some utility functions to use "flashrom" to read from
 * and write to NOR flash.
 */

#ifndef VBOOT_REFERCENCE_CGPT_CGPT_NOR_H_
#define VBOOT_REFERCENCE_CGPT_CGPT_NOR_H_

// Obtain the MTD size from its sysfs node. |mtd_device| should point to
// a dev node such as /dev/mtd0. This function returns 0 on success.
int GetMtdSize(const char *mtd_device, uint64_t *size);

// Exec |argv| in |cwd|. Return -1 on error, or exit code on success. |argv|
// must be terminated with a NULL element as is required by execv().
int ForkExecV(const char *cwd, const char *const argv[]);

// Similar to ForkExecV but with a vararg instead of an array of pointers.
int ForkExecL(const char *cwd, const char *cmd, ...);

// Exec "rm" to remove |dir|.
int RemoveDir(const char *dir);

// Read RW_GPT from NOR flash to "rw_gpt" in a temp dir |temp_dir_template|.
// |temp_dir_template| is passed to mkdtemp() so it must satisfy all
// requirements by mkdtemp().
int ReadNorFlash(char *temp_dir_template);

// Write "rw_gpt" back to NOR flash. We write the file in two parts for safety.
int WriteNorFlash(const char *dir);

#endif  // VBOOT_REFERCENCE_CGPT_CGPT_NOR_H_
