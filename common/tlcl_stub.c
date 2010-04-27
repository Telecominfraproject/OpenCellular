/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Stub implementations of TPM Lite Library.
 */

#include "tss_constants.h"

void TlclLibinit(void) { return; }
void TlclStartup(void) { return; }
void TlclSelftestfull(void) { return; }
void TlclDefineSpace(uint32_t index, uint32_t perm, uint32_t size) { return; }
uint32_t TlclWrite(uint32_t index, uint8_t *data, uint32_t length) {
  return TPM_SUCCESS;
}
uint32_t TlclRead(uint32_t index, uint8_t *data, uint32_t length) {
  return TPM_SUCCESS;
}
void TlclWriteLock(uint32_t index) { return; }
void TlclReadLock(uint32_t index) { return; }
void TlclAssertPhysicalPresence(void) { return; }
void TlclSetNvLocked(void) { return; }
int TlclIsOwned(void) { return 0; }
void TlclForceClear(void) { return; }
void TlclPhysicalEnable(void) { return; }
int TlclPhysicalSetDeactivated(uint8_t flag) { return TPM_SUCCESS; }
int TlclGetFlags(uint8_t* disable, uint8_t* deactivated) { return TPM_SUCCESS; }
