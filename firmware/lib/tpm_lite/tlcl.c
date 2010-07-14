/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* A lightweight TPM command library.
 *
 * The general idea is that TPM commands are array of bytes whose
 * fields are mostly compile-time constant.  The goal is to build much
 * of the commands at compile time (or build time) and change some of
 * the fields at run time as needed.  The code in
 * utility/tlcl_generator.c builds structures containing the commands,
 * as well as the offsets of the fields that need to be set at run
 * time.
 */

#include "sysincludes.h"
#include "tlcl.h"
#include "tlcl_internal.h"
#include "tlcl_structures.h"
#include "tss_constants.h"
#include "utility.h"


/* Sets the size field of a TPM command. */
static INLINE void SetTpmCommandSize(uint8_t* buffer, uint32_t size) {
  ToTpmUint32(buffer + sizeof(uint16_t), size);
}

/* Gets the size field of a TPM command. */
POSSIBLY_UNUSED static INLINE int TpmCommandSize(const uint8_t* buffer) {
  uint32_t size;
  FromTpmUint32(buffer + sizeof(uint16_t), &size);
  return (int) size;
}

/* Gets the code field of a TPM command. */
static INLINE int TpmCommandCode(const uint8_t* buffer) {
  uint32_t code;
  FromTpmUint32(buffer + sizeof(uint16_t) + sizeof(uint32_t), &code);
  return code;
}

/* Gets the return code field of a TPM result. */
static INLINE int TpmReturnCode(const uint8_t* buffer) {
  return TpmCommandCode(buffer);
}

/* Checks for errors in a TPM response. */
static void CheckResult(uint8_t* request, uint8_t* response, int warn_only) {
  int command = TpmCommandCode(request);
  int result = TpmReturnCode(response);
  if (result != TPM_SUCCESS) {
    if (warn_only)
      VBDEBUG(("TPM command %d 0x%x failed: %d 0x%x\n",
               command, command, result, result));
    else
      error("TPM command %d 0x%x failed: %d 0x%x\n",
            command, command, result, result);
  }
}

/* Sends a TPM command and gets a response. */
static void TlclSendReceive(uint8_t* request, uint8_t* response,
                            int max_length) {
  TlclStubSendReceive(request, TpmCommandSize(request),
                      response, max_length);
}


/* Sends a command and returns the error code. */
static uint32_t Send(uint8_t* command) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  TlclSendReceive(command, response, sizeof(response));
  return TpmReturnCode(response);
}

/* Exported functions. */

void TlclLibInit(void) {
  TlclStubInit();
}

uint32_t TlclStartup(void) {
  return Send(tpm_startup_cmd.buffer);
}

uint32_t TlclSelftestfull(void) {
  return Send(tpm_selftestfull_cmd.buffer);
}

uint32_t TlclContinueSelfTest(void) {
  return Send(tpm_continueselftest_cmd.buffer);
}

uint32_t TlclDefineSpace(uint32_t index, uint32_t perm, uint32_t size) {
  ToTpmUint32(tpm_nv_definespace_cmd.index, index);
  ToTpmUint32(tpm_nv_definespace_cmd.perm, perm);
  ToTpmUint32(tpm_nv_definespace_cmd.size, size);
  return Send(tpm_nv_definespace_cmd.buffer);
}

uint32_t TlclWrite(uint32_t index, uint8_t* data, uint32_t length) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  const int total_length =
    kTpmRequestHeaderLength + kWriteInfoLength + length;

  assert(total_length <= TPM_LARGE_ENOUGH_COMMAND_SIZE);
  SetTpmCommandSize(tpm_nv_write_cmd.buffer, total_length);

  ToTpmUint32(tpm_nv_write_cmd.index, index);
  ToTpmUint32(tpm_nv_write_cmd.length, length);
  Memcpy(tpm_nv_write_cmd.data, data, length);

  TlclSendReceive(tpm_nv_write_cmd.buffer, response, sizeof(response));
  CheckResult(tpm_nv_write_cmd.buffer, response, 1);

  return TpmReturnCode(response);
}

uint32_t TlclRead(uint32_t index, uint8_t* data, uint32_t length) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  uint32_t result_length;
  uint32_t result;

  ToTpmUint32(tpm_nv_read_cmd.index, index);
  ToTpmUint32(tpm_nv_read_cmd.length, length);

  TlclSendReceive(tpm_nv_read_cmd.buffer, response, sizeof(response));
  result = TpmReturnCode(response);
  if (result == TPM_SUCCESS && length > 0) {
    uint8_t* nv_read_cursor = response + kTpmResponseHeaderLength;
    FromTpmUint32(nv_read_cursor, &result_length);
    nv_read_cursor += sizeof(uint32_t);
    Memcpy(data, nv_read_cursor, result_length);
  }

  return result;
}

uint32_t TlclWriteLock(uint32_t index) {
  return TlclWrite(index, NULL, 0);
}

uint32_t TlclReadLock(uint32_t index) {
  return TlclRead(index, NULL, 0);
}

uint32_t TlclAssertPhysicalPresence(void) {
  return Send(tpm_ppassert_cmd.buffer);
}

uint32_t TlclAssertPhysicalPresenceResult(void) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  TlclSendReceive(tpm_ppassert_cmd.buffer, response, sizeof(response));
  return TpmReturnCode(response);
}

uint32_t TlclLockPhysicalPresence(void) {
  return Send(tpm_pplock_cmd.buffer);
}

uint32_t TlclSetNvLocked(void) {
  return TlclDefineSpace(TPM_NV_INDEX_LOCK, 0, 0);
}

int TlclIsOwned(void) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE + TPM_PUBEK_SIZE];
  uint32_t result;
  TlclSendReceive(tpm_readpubek_cmd.buffer, response, sizeof(response));
  result = TpmReturnCode(response);
  return (result != TPM_SUCCESS);
}

uint32_t TlclForceClear(void) {
  return Send(tpm_forceclear_cmd.buffer);
}

uint32_t TlclSetEnable(void) {
  return Send(tpm_physicalenable_cmd.buffer);
}

uint32_t TlclClearEnable(void) {
  return Send(tpm_physicaldisable_cmd.buffer);
}

uint32_t TlclSetDeactivated(uint8_t flag) {
  *((uint8_t*)tpm_physicalsetdeactivated_cmd.deactivated) = flag;
  return Send(tpm_physicalsetdeactivated_cmd.buffer);
}

uint32_t TlclGetFlags(uint8_t* disable, uint8_t* deactivated) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  TPM_PERMANENT_FLAGS* pflags;
  uint32_t result;
  uint32_t size;

  TlclSendReceive(tpm_getflags_cmd.buffer, response, sizeof(response));
  result = TpmReturnCode(response);
  if (result != TPM_SUCCESS) {
    return result;
  }
  FromTpmUint32(response + kTpmResponseHeaderLength, &size);
  assert(size == sizeof(TPM_PERMANENT_FLAGS));
  pflags =
    (TPM_PERMANENT_FLAGS*) (response + kTpmResponseHeaderLength + sizeof(size));
  *disable = pflags->disable;
  *deactivated = pflags->deactivated;
  return result;
}

uint32_t TlclSetGlobalLock(void) {
  uint32_t x;
  return TlclWrite(TPM_NV_INDEX0, (uint8_t*) &x, 0);
}

uint32_t TlclExtend(int pcr_num, uint8_t* in_digest, uint8_t* out_digest) {
  uint8_t response[kTpmResponseHeaderLength + kPcrDigestLength];
  ToTpmUint32(tpm_extend_cmd.pcrNum, pcr_num);
  Memcpy(tpm_extend_cmd.inDigest, in_digest, kPcrDigestLength);
  TlclSendReceive(tpm_extend_cmd.buffer, response, sizeof(response));
  Memcpy(out_digest, response + kTpmResponseHeaderLength, kPcrDigestLength);
  return TpmReturnCode(response);
}

uint32_t TlclGetPermissions(uint32_t index, uint32_t* permissions) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  uint8_t* nvdata;
  uint32_t result;
  uint32_t size;

  ToTpmUint32(tpm_getpermissions_cmd.index, index);
  TlclSendReceive(tpm_getpermissions_cmd.buffer, response, sizeof(response));
  result = TpmReturnCode(response);
  if (result != TPM_SUCCESS) {
    return result;
  }
  nvdata = response + kTpmResponseHeaderLength + sizeof(size);
  FromTpmUint32(nvdata + kNvDataPublicPermissionsOffset, permissions);
  return result;
}
