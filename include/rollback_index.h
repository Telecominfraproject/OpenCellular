/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for querying, manipulating and locking rollback indices
 * stored in the TPM NVRAM.
 */

#ifndef VBOOT_REFERENCE_ROLLBACK_INDEX_H_
#define VBOOT_REFERENCE_ROLLBACK_INDEX_H_

#include <stdint.h>

extern uint16_t g_firmware_key_version;
extern uint16_t g_firmware_version;
extern uint16_t g_kernel_key_version;
extern uint16_t g_kernel_version;

/* Rollback version types. */
#define FIRMWARE_KEY_VERSION 0
#define FIRMWARE_VERSION 1
#define KERNEL_KEY_VERSION 2
#define KERNEL_VERSION 3

/* TPM NVRAM location indices. */
#define FIRMWARE_KEY_VERSION_NV_INDEX  0x1001
#define FIRMWARE_VERSION_NV_INDEX 0x1002
#define KERNEL_KEY_VERSION_NV_INDEX 0x1003
#define KERNEL_VERSION_NV_INDEX 0x1004

void SetupTPM(void);
uint16_t GetStoredVersion(int type);
int WriteStoredVersion(int type, uint16_t version);
void LockStoredVersion(int type);

#endif  /* VBOOT_REFERENCE_ROLLBACK_INDEX_H_ */
