/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Stub implementations of TPM Lite Library.
 */

#include "tss_constants.h"

/* disable MSVC warnings on unused arguments */
__pragma(warning (disable: 4100))

void TlclLibInit(void) { return; }
void TlclCloseDevice(void) { return; }
void TlclOpenDevice(void) { return; }
uint32_t TlclStartup(void) { return TPM_SUCCESS; }
uint32_t TlclSelftestfull(void) { return TPM_SUCCESS; }
uint32_t TlclContinueSelfTest(void) { return TPM_SUCCESS; }
uint32_t TlclDefineSpace(uint32_t index, uint32_t perm, uint32_t size) {
  return TPM_SUCCESS;
}
uint32_t TlclWrite(uint32_t index, uint8_t *data, uint32_t length) {
  return TPM_SUCCESS;
}
uint32_t TlclRead(uint32_t index, uint8_t *data, uint32_t length) {
  return TPM_SUCCESS;
}
uint32_t TlclWriteLock(uint32_t index) { return TPM_SUCCESS; }
uint32_t TlclReadLock(uint32_t index) { return TPM_SUCCESS; }
uint32_t TlclAssertPhysicalPresence(void) { return TPM_SUCCESS; }
uint32_t TlclLockPhysicalPresence(void) { return TPM_SUCCESS; }
uint32_t TlclSetNvLocked(void) { return TPM_SUCCESS; }
int TlclIsOwned(void) { return TPM_SUCCESS; }
uint32_t TlclForceClear(void) { return TPM_SUCCESS; }
uint32_t TlclSetEnable(void) { return TPM_SUCCESS; }
uint32_t TlclClearEnable(void) { return TPM_SUCCESS; }
uint32_t TlclSetDeactivated(int deactivated) { return TPM_SUCCESS; }
uint32_t TlclSetGlobalLock(void) { return TPM_SUCCESS; }
uint32_t TlclGetFlags(uint8_t* disable, uint8_t* deactivated) {
  return TPM_SUCCESS;
}
uint32_t TlclGetPermissions(uint32_t index, uint32_t* permissions) {
  return TPM_SUCCESS;
}
