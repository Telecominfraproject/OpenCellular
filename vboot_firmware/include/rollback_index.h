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
#define FIRMWARE_VERSIONS 0
#define KERNEL_VERSIONS   1

/* TPM NVRAM location indices. */
#define FIRMWARE_VERSIONS_NV_INDEX      0x1001
#define KERNEL_VERSIONS_NV_INDEX        0x1002
#define TPM_IS_INITIALIZED_NV_INDEX     0x1003
#define KERNEL_VERSIONS_BACKUP_NV_INDEX 0x1004
#define KERNEL_BACKUP_IS_VALID_NV_INDEX 0x1005


void SetupTPM(void);
void GetStoredVersions(int type, uint16_t* key_version, uint16_t* version);
int WriteStoredVersions(int type, uint16_t key_version, uint16_t version);
void LockFirmwareVersions();
void LockKernelVersionsByLockingPP();

#endif  /* VBOOT_REFERENCE_ROLLBACK_INDEX_H_ */
