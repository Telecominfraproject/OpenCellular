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
#include "tpmextras.h"
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

/* Sends a TPM command and gets a response.  Returns 0 if success or the TPM
 * error code if error. */
static uint32_t TlclSendReceive(uint8_t* request, uint8_t* response,
                                int max_length) {

  uint32_t result;

#ifdef EXTRA_LOGGING
  VBDEBUG(("TPM: command: %x%x %x%x%x%x %x%x%x%x\n",
           request[0], request[1],
           request[2], request[3], request[4], request[5],
           request[6], request[7], request[8], request[9]));
#endif

  result = TlclStubSendReceive(request, TpmCommandSize(request),
                               response, max_length);
  if (0 != result) {
    /* Communication with TPM failed, so response is garbage */
    VBDEBUG(("TPM: command 0x%x send/receive failed: 0x%x\n",
             TpmCommandCode(request), result));
    return TPM_E_COMMUNICATION_ERROR;
  }
  /* Otherwise, use the result code from the response */
  result = TpmReturnCode(response);

#ifdef EXTRA_LOGGING
  VBDEBUG(("TPM: response: %x%x %x%x%x%x %x%x%x%x\n",
           response[0], response[1],
           response[2], response[3], response[4], response[5],
           response[6], response[7], response[8], response[9]));
#endif

  VBDEBUG(("TPM: command 0x%x returned 0x%x\n",
           TpmCommandCode(request), result));

  return result;
}


/* Sends a command and returns the error code. */
static uint32_t Send(uint8_t* command) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  return TlclSendReceive(command, response, sizeof(response));
}

/* Exported functions. */

void TlclLibInit(void) {
  TlclStubInit();
}

uint32_t TlclStartup(void) {
  VBDEBUG(("TPM: Startup\n"));
  return Send(tpm_startup_cmd.buffer);
}

uint32_t TlclResume(void) {
  VBDEBUG(("TPM: Resume\n"));
  return Send(tpm_resume_cmd.buffer);
}

uint32_t TlclSelfTestFull(void) {
  VBDEBUG(("TPM: Self test full\n"));
  return Send(tpm_selftestfull_cmd.buffer);
}

uint32_t TlclContinueSelfTest(void) {
  VBDEBUG(("TPM: Continue self test\n"));
  return Send(tpm_continueselftest_cmd.buffer);
}

uint32_t TlclDefineSpace(uint32_t index, uint32_t perm, uint32_t size) {
  struct s_tpm_nv_definespace_cmd cmd;
  VBDEBUG(("TPM: TlclDefineSpace(0x%x, 0x%x, %d)\n", index, perm, size));
  Memcpy(&cmd, &tpm_nv_definespace_cmd, sizeof(cmd));
  ToTpmUint32(cmd.buffer + tpm_nv_definespace_cmd.index, index);
  ToTpmUint32(cmd.buffer + tpm_nv_definespace_cmd.perm, perm);
  ToTpmUint32(cmd.buffer + tpm_nv_definespace_cmd.size, size);
  return Send(cmd.buffer);
}

uint32_t TlclWrite(uint32_t index, const void* data, uint32_t length) {
  struct s_tpm_nv_write_cmd cmd;
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  const int total_length =
    kTpmRequestHeaderLength + kWriteInfoLength + length;

  VBDEBUG(("TPM: TlclWrite(0x%x, %d)\n", index, length));
  Memcpy(&cmd, &tpm_nv_write_cmd, sizeof(cmd));
  assert(total_length <= TPM_LARGE_ENOUGH_COMMAND_SIZE);
  SetTpmCommandSize(cmd.buffer, total_length);

  ToTpmUint32(cmd.buffer + tpm_nv_write_cmd.index, index);
  ToTpmUint32(cmd.buffer + tpm_nv_write_cmd.length, length);
  Memcpy(cmd.buffer + tpm_nv_write_cmd.data, data, length);

  return TlclSendReceive(cmd.buffer, response, sizeof(response));
}

uint32_t TlclRead(uint32_t index, void* data, uint32_t length) {
  struct s_tpm_nv_read_cmd cmd;
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  uint32_t result_length;
  uint32_t result;

  VBDEBUG(("TPM: TlclRead(0x%x, %d)\n", index, length));
  Memcpy(&cmd, &tpm_nv_read_cmd, sizeof(cmd));
  ToTpmUint32(cmd.buffer + tpm_nv_read_cmd.index, index);
  ToTpmUint32(cmd.buffer + tpm_nv_read_cmd.length, length);

  result = TlclSendReceive(cmd.buffer, response, sizeof(response));
  if (result == TPM_SUCCESS && length > 0) {
    uint8_t* nv_read_cursor = response + kTpmResponseHeaderLength;
    FromTpmUint32(nv_read_cursor, &result_length);
    nv_read_cursor += sizeof(uint32_t);
    Memcpy(data, nv_read_cursor, result_length);
  }

  return result;
}

uint32_t TlclWriteLock(uint32_t index) {
  VBDEBUG(("TPM: Write lock 0x%x\n", index));
  return TlclWrite(index, NULL, 0);
}

uint32_t TlclReadLock(uint32_t index) {
  VBDEBUG(("TPM: Read lock 0x%x\n", index));
  return TlclRead(index, NULL, 0);
}

uint32_t TlclAssertPhysicalPresence(void) {
  VBDEBUG(("TPM: Asserting physical presence\n"));
  return Send(tpm_ppassert_cmd.buffer);
}

uint32_t TlclPhysicalPresenceCMDEnable(void) {
  VBDEBUG(("TPM: Enable the physical presence command\n"));
  return Send(tpm_ppenable_cmd.buffer);
}

uint32_t TlclFinalizePhysicalPresence(void) {
  VBDEBUG(("TPM: Enable PP cmd, disable HW pp, and set lifetime lock\n"));
  return Send(tpm_finalizepp_cmd.buffer);
}

uint32_t TlclAssertPhysicalPresenceResult(void) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  return TlclSendReceive(tpm_ppassert_cmd.buffer, response, sizeof(response));
}

uint32_t TlclLockPhysicalPresence(void) {
  VBDEBUG(("TPM: Lock physical presence\n"));
  return Send(tpm_pplock_cmd.buffer);
}

uint32_t TlclSetNvLocked(void) {
  VBDEBUG(("TPM: Set NV locked\n"));
  return TlclDefineSpace(TPM_NV_INDEX_LOCK, 0, 0);
}

int TlclIsOwned(void) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE + TPM_PUBEK_SIZE];
  uint32_t result;
  result = TlclSendReceive(tpm_readpubek_cmd.buffer,
                           response, sizeof(response));
  return (result != TPM_SUCCESS);
}

uint32_t TlclForceClear(void) {
  VBDEBUG(("TPM: Force clear\n"));
  return Send(tpm_forceclear_cmd.buffer);
}

uint32_t TlclSetEnable(void) {
  VBDEBUG(("TPM: Enabling TPM\n"));
  return Send(tpm_physicalenable_cmd.buffer);
}

uint32_t TlclClearEnable(void) {
  VBDEBUG(("TPM: Disabling TPM\n"));
  return Send(tpm_physicaldisable_cmd.buffer);
}

uint32_t TlclSetDeactivated(uint8_t flag) {
  struct s_tpm_physicalsetdeactivated_cmd cmd;
  VBDEBUG(("TPM: SetDeactivated(%d)\n", flag));
  Memcpy(&cmd, &tpm_physicalsetdeactivated_cmd, sizeof(cmd));
  *(cmd.buffer + cmd.deactivated) = flag;
  return Send(cmd.buffer);
}

uint32_t TlclGetPermanentFlags(TPM_PERMANENT_FLAGS* pflags) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  uint32_t size;
  uint32_t result =
    TlclSendReceive(tpm_getflags_cmd.buffer, response, sizeof(response));
  if (result != TPM_SUCCESS)
    return result;
  FromTpmUint32(response + kTpmResponseHeaderLength, &size);
  assert(size == sizeof(TPM_PERMANENT_FLAGS));
  Memcpy(pflags,
         response + kTpmResponseHeaderLength + sizeof(size),
         sizeof(TPM_PERMANENT_FLAGS));
  return result;
}

uint32_t TlclGetSTClearFlags(TPM_STCLEAR_FLAGS* vflags) {
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  uint32_t size;
  uint32_t result =
    TlclSendReceive(tpm_getstclearflags_cmd.buffer, response, sizeof(response));
  if (result != TPM_SUCCESS)
    return result;
  FromTpmUint32(response + kTpmResponseHeaderLength, &size);
  /* Ugly assertion, but the struct is padded up by one byte. */
  assert(size == 7 && sizeof(TPM_STCLEAR_FLAGS) - 1 == 7);
  Memcpy(vflags,
         response + kTpmResponseHeaderLength + sizeof(size),
         sizeof(TPM_STCLEAR_FLAGS));
  return result;
}

uint32_t TlclGetFlags(uint8_t* disable,
                      uint8_t* deactivated,
                      uint8_t *nvlocked) {
  TPM_PERMANENT_FLAGS pflags;
  uint32_t result = TlclGetPermanentFlags(&pflags);
  if (result == TPM_SUCCESS) {
    if (disable)
      *disable = pflags.disable;
    if (deactivated)
      *deactivated = pflags.deactivated;
    if (nvlocked)
      *nvlocked = pflags.nvLocked;
    VBDEBUG(("TPM: Got flags disable=%d, deactivated=%d, nvlocked=%d\n",
             pflags.disable, pflags.deactivated, pflags.nvLocked));
  }
  return result;
}

uint32_t TlclSetGlobalLock(void) {
  uint32_t x;
  VBDEBUG(("TPM: Set global lock\n"));
  return TlclWrite(TPM_NV_INDEX0, (uint8_t*) &x, 0);
}

uint32_t TlclExtend(int pcr_num, uint8_t* in_digest, uint8_t* out_digest) {
  struct s_tpm_extend_cmd cmd;
  uint8_t response[kTpmResponseHeaderLength + kPcrDigestLength];
  uint32_t result;

  Memcpy(&cmd, &tpm_extend_cmd, sizeof(cmd));
  ToTpmUint32(cmd.buffer + tpm_extend_cmd.pcrNum, pcr_num);
  Memcpy(cmd.buffer + cmd.inDigest, in_digest, kPcrDigestLength);

  result = TlclSendReceive(cmd.buffer, response, sizeof(response));
  if (result != TPM_SUCCESS)
    return result;

  Memcpy(out_digest, response + kTpmResponseHeaderLength, kPcrDigestLength);
  return result;
}

uint32_t TlclGetPermissions(uint32_t index, uint32_t* permissions) {
  struct s_tpm_getpermissions_cmd cmd;
  uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
  uint8_t* nvdata;
  uint32_t result;
  uint32_t size;

  Memcpy(&cmd, &tpm_getpermissions_cmd, sizeof(cmd));
  ToTpmUint32(cmd.buffer + tpm_getpermissions_cmd.index, index);
  result = TlclSendReceive(cmd.buffer, response, sizeof(response));
  if (result != TPM_SUCCESS)
    return result;

  nvdata = response + kTpmResponseHeaderLength + sizeof(size);
  FromTpmUint32(nvdata + kNvDataPublicPermissionsOffset, permissions);
  return result;
}
