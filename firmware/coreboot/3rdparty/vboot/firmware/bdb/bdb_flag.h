/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_FIRMWARE_BDB_BDB_FLAG_H
#define VBOOT_REFERENCE_FIRMWARE_BDB_BDB_FLAG_H

/* Indicate whether BDB key is verified */
#define VBA_CONTEXT_FLAG_BDB_KEY_EFUSED			(1 << 0)

/* Indicate whether kernel data key is verified */
#define VBA_CONTEXT_FLAG_KERNEL_DATA_KEY_VERIFIED	(1 << 1)

#endif
