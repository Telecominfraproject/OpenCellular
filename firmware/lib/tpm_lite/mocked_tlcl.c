/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "sysincludes.h"

#include "tlcl.h"
#include "tlcl_internal.h"
#include "utility.h"
#include "vboot_api.h"

uint32_t TlclLibInit(void) {
  return VbExTpmInit();
}

uint32_t TlclLibClose(void) {
  return TPM_SUCCESS;
}

uint32_t TlclStartup(void) {
  return TPM_SUCCESS;
}

uint32_t TlclSaveState(void) {
  return TPM_SUCCESS;
}

uint32_t TlclResume(void) {
  return TPM_SUCCESS;
}

uint32_t TlclSelfTestFull(void) {
  return TPM_SUCCESS;
}

uint32_t TlclContinueSelfTest(void) {
  return TPM_SUCCESS;
}

uint32_t TlclDefineSpace(uint32_t index, uint32_t perm, uint32_t size) {
  return TPM_SUCCESS;
}

uint32_t TlclWrite(uint32_t index, const void* data, uint32_t length) {
  return TPM_SUCCESS;
}

uint32_t TlclRead(uint32_t index, void* data, uint32_t length) {
  Memset(data, '\0', length);
  return TPM_SUCCESS;
}

uint32_t TlclPCRRead(uint32_t index, void* data, uint32_t length) {
  Memset(data, '\0', length);
  return TPM_SUCCESS;
}

uint32_t TlclWriteLock(uint32_t index) {
  return TPM_SUCCESS;
}

uint32_t TlclReadLock(uint32_t index) {
  return TPM_SUCCESS;
}

uint32_t TlclAssertPhysicalPresence(void) {
  return TPM_SUCCESS;
}

uint32_t TlclPhysicalPresenceCMDEnable(void) {
  return TPM_SUCCESS;
}

uint32_t TlclFinalizePhysicalPresence(void) {
  return TPM_SUCCESS;
}

uint32_t TlclAssertPhysicalPresenceResult(void) {
  return TPM_SUCCESS;
}

uint32_t TlclLockPhysicalPresence(void) {
  return TPM_SUCCESS;
}

uint32_t TlclSetNvLocked(void) {
  return TPM_SUCCESS;
}

int TlclIsOwned(void) {
  return 0;
}

uint32_t TlclForceClear(void) {
  return TPM_SUCCESS;
}

uint32_t TlclSetEnable(void) {
  return TPM_SUCCESS;
}

uint32_t TlclClearEnable(void) {
  return TPM_SUCCESS;
}

uint32_t TlclSetDeactivated(uint8_t flag) {
  return TPM_SUCCESS;
}

uint32_t TlclGetPermanentFlags(TPM_PERMANENT_FLAGS* pflags) {
  Memset(pflags, '\0', sizeof(*pflags));
  return TPM_SUCCESS;
}

uint32_t TlclGetSTClearFlags(TPM_STCLEAR_FLAGS* vflags) {
  Memset(vflags, '\0', sizeof(*vflags));
  return TPM_SUCCESS;
}

uint32_t TlclGetFlags(uint8_t* disable,
                      uint8_t* deactivated,
                      uint8_t *nvlocked) {
  *disable = 0;
  *deactivated = 0;
  *nvlocked = 0;
  return TPM_SUCCESS;
}

uint32_t TlclSetGlobalLock(void) {
  return TPM_SUCCESS;
}

uint32_t TlclExtend(int pcr_num, const uint8_t* in_digest,
                    uint8_t* out_digest) {
  Memcpy(out_digest, in_digest, kPcrDigestLength);
  return TPM_SUCCESS;
}

uint32_t TlclGetPermissions(uint32_t index, uint32_t* permissions) {
  *permissions = 0;
  return TPM_SUCCESS;
}

uint32_t TlclGetOwnership(uint8_t* owned) {
  *owned = 0;
  return TPM_SUCCESS;
}

uint32_t TlclGetRandom(uint8_t* data, uint32_t length, uint32_t *size) {
  *size = length;
  /* http://dilbert.com/strips/comic/2001-10-25/ */
  Memset(data, '\x9', *size);
  return TPM_SUCCESS;
}

int TlclPacketSize(const uint8_t* packet)
{
  uint32_t size;
  FromTpmUint32(packet + sizeof(uint16_t), &size);
  return (int) size;
}

uint32_t TlclSendReceive(const uint8_t* request, uint8_t* response,
                         int max_length)
{
  return TPM_SUCCESS;
}
