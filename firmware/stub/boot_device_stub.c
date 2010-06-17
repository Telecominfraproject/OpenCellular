/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Stub implementations of boot device functions.
 */

#include "boot_device.h"

int BootDeviceReadLBA(uint64_t lba_start, uint64_t lba_count, void *buffer) {
  return 1;
}

int BootDeviceWriteLBA(uint64_t lba_start, uint64_t lba_count,
                       const void *buffer) {
  return 1;
}
