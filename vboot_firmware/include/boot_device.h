/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Helper functions/wrappers for raw sector access to current boot device. */

#ifndef VBOOT_REFERENCE_BOOT_DEVICE_H_
#define VBOOT_REFERENCE_BOOT_DEVICE_H_

#include "sysincludes.h"

int BootDeviceReadLBA(uint64_t lba_start, uint64_t lba_count, void *buffer);
/* Reads lba_count LBA sectors, starting at sector lba_start, from the current
 * boot device, into the buffer.
 *
 * Returns 0 if successful or 1 if error. */

int BootDeviceWriteLBA(uint64_t lba_start, uint64_t lba_count,
                       const void *buffer);
/* Writes lba_count LBA sectors, starting at sector lba_start, to the current
 * boot device, from the buffer.
 *
 * Returns 0 if successful or 1 if error. */

#endif  /* VBOOT_REFERENCE_BOOT_DEVICE_H_ */
