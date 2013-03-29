/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Stub implementations of disk APIs.
 */

#include <stdint.h>

#define _STUB_IMPLEMENTATION_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "vboot_api.h"


VbError_t VbExDiskGetInfo(VbDiskInfo** infos_ptr, uint32_t* count,
                          uint32_t disk_flags) {
  *infos_ptr = NULL;
  *count = 0;
  return VBERROR_SUCCESS;
}


VbError_t VbExDiskFreeInfo(VbDiskInfo* infos_ptr,
                           VbExDiskHandle_t preserve_handle) {
  return VBERROR_SUCCESS;
}


VbError_t VbExDiskRead(VbExDiskHandle_t handle, uint64_t lba_start,
                       uint64_t lba_count, void* buffer) {
  return VBERROR_SUCCESS;
}


VbError_t VbExDiskWrite(VbExDiskHandle_t handle, uint64_t lba_start,
                        uint64_t lba_count, const void* buffer) {
  return VBERROR_SUCCESS;
}
