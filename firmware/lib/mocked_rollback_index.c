/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for querying, manipulating and locking rollback indices
 * stored in the TPM NVRAM.
 */

#include "sysincludes.h"
#include "utility.h"

#include "rollback_index.h"

#include "tss_constants.h"


uint32_t SetVirtualDevMode(int val) {
  return TPM_SUCCESS;
}


uint32_t TPMClearAndReenable(void) {
  return TPM_SUCCESS;
}


uint32_t SetupTPM(int developer_mode, int disable_dev_request,
                  int clear_tpm_owner_request, RollbackSpaceFirmware* rsf) {
  return TPM_SUCCESS;
}


uint32_t RollbackS3Resume(void) {
  return TPM_SUCCESS;
}


uint32_t RollbackFirmwareSetup(int is_hw_dev,
                               int disable_dev_request,
                               int clear_tpm_owner_request,
                               int *is_virt_dev, uint32_t *version) {
  *version = 0;
  return TPM_SUCCESS;
}


uint32_t RollbackFirmwareWrite(uint32_t version) {
  return TPM_SUCCESS;
}


uint32_t RollbackFirmwareLock(void) {
  return TPM_SUCCESS;
}


uint32_t RollbackKernelRead(uint32_t* version) {
  *version = 0;
  return TPM_SUCCESS;
}


uint32_t RollbackKernelWrite(uint32_t version) {
  return TPM_SUCCESS;
}


uint32_t RollbackKernelLock(int recovery_mode) {
  return TPM_SUCCESS;
}

static uint8_t rollback_backup[BACKUP_NV_SIZE];

uint32_t RollbackBackupRead(uint8_t *raw)
{
	Memcpy(raw, rollback_backup, BACKUP_NV_SIZE);
	return TPM_SUCCESS;
}

uint32_t RollbackBackupWrite(uint8_t *raw)
{
	Memcpy(rollback_backup, raw, BACKUP_NV_SIZE);
	return TPM_SUCCESS;
}
