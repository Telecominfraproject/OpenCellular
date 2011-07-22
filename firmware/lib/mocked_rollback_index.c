/* Copyright (c) 2010-2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for querying, manipulating and locking rollback indices
 * stored in the TPM NVRAM.
 */

#include "rollback_index.h"

#include "tss_constants.h"


uint32_t TPMClearAndReenable(void) {
  return TPM_SUCCESS;
}


uint32_t SetupTPM(int recovery_mode, int developer_mode,
                  RollbackSpaceFirmware* rsf) {
  return TPM_SUCCESS;
}


uint32_t RollbackS3Resume(void) {
  return TPM_SUCCESS;
}


uint32_t RollbackFirmwareSetup(int recovery_mode, int developer_mode,
                               uint32_t* version) {
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


uint32_t RollbackKernelLock(void) {
  return TPM_SUCCESS;
}
