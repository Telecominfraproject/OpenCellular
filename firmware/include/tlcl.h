/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* TPM Lightweight Command Library.
 *
 * A low-level library for interfacing to TPM hardware or an emulator.
 */

#ifndef TPM_LITE_TLCL_H_
#define TPM_LITE_TLCL_H_

#include "sysincludes.h"
#include "tss_constants.h"

/*****************************************************************************/
/* Functions implemented in tlcl.c */

/* Call this first.  Returns 0 if success, nonzero if error.
 */
uint32_t TlclLibInit(void);

/* Call this on shutdown.  Returns 0 if success, nonzero if error.
 */
uint32_t TlclLibClose(void);

/* Logs to stdout.  Arguments like printf.
 */
void TlclLog(char* format, ...);

/* Sets the log level.  0 is quietest.
 */
void TlclSetLogLevel(int level);

/* Sends a TPM_Startup(ST_CLEAR).  The TPM error code is returned (0
 * for success).
 */
uint32_t TlclStartup(void);

/* Save the TPM state.  Normally done by the kernel before a suspend, included
 * here for tests.  The TPM error code is returned (0 for success).
 */
uint32_t TlclSaveState(void);

/* Resumes by sending a TPM_Startup(ST_STATE).  The TPM error code is returned
 * (0 for success).
 */
uint32_t TlclResume(void);

/* Runs the self test.  Note---this is synchronous.  To run this in parallel
 * with other firmware, use ContinueSelfTest.  The TPM error code is returned.
 */
uint32_t TlclSelfTestFull(void);

/* Runs the self test in the background.
 */
uint32_t TlclContinueSelfTest(void);

/* Defines a space with permission [perm].  [index] is the index for the space,
 * [size] the usable data size.  The TPM error code is returned.
 */
uint32_t TlclDefineSpace(uint32_t index, uint32_t perm, uint32_t size);

/* Writes [length] bytes of [data] to space at [index].  The TPM error code is
 * returned.
 */
uint32_t TlclWrite(uint32_t index, const void* data, uint32_t length);

/* Reads [length] bytes from space at [index] into [data].  The TPM error code
 * is returned.
 */
uint32_t TlclRead(uint32_t index, void* data, uint32_t length);

/* Reads PCR at [index] into [data].  [length] must be TPM_PCR_DIGEST or
 * larger. The TPM error code is returned.
 */
uint32_t TlclPCRRead(uint32_t index, void* data, uint32_t length);

/* Write-locks space at [index].  The TPM error code is returned.
 */
uint32_t TlclWriteLock(uint32_t index);

/* Read-locks space at [index].  The TPM error code is returned.
 */
uint32_t TlclReadLock(uint32_t index);

/* Asserts physical presence in software.  The TPM error code is returned.
 */
uint32_t TlclAssertPhysicalPresence(void);

/* Enables the physical presence command.  The TPM error code is returned.
 */
uint32_t TlclPhysicalPresenceCMDEnable(void);

/* Finalizes the physical presence settings: sofware PP is enabled, hardware PP
 * is disabled, and the lifetime lock is set.  The TPM error code is returned.
 */
uint32_t TlclFinalizePhysicalPresence(void);

/* Turns off physical presence and locks it off until next reboot.  The TPM
 * error code is returned.
 */
uint32_t TlclLockPhysicalPresence(void);

/* Sets the nvLocked bit.  The TPM error code is returned.
 */
uint32_t TlclSetNvLocked(void);

/* Returns 1 if the TPM is owned, 0 otherwise.
 */
int TlclIsOwned(void);

/* Issues a ForceClear.  The TPM error code is returned.
 */
uint32_t TlclForceClear(void);

/* Issues a PhysicalEnable.  The TPM error code is returned.
 */
uint32_t TlclSetEnable(void);

/* Issues a PhysicalDisable.  The TPM error code is returned.
 */
uint32_t TlclClearEnable(void);

/* Issues a SetDeactivated.  Pass 0 to activate.  Returns result code.
 */
uint32_t TlclSetDeactivated(uint8_t flag);

/* Gets flags of interest.  Pointers for flags you aren't interested in may
 * be NULL.  The TPM error code is returned.
 */
uint32_t TlclGetFlags(uint8_t* disable, uint8_t* deactivated,
                      uint8_t* nvlocked);

/* Sets the bGlobalLock flag, which only a reboot can clear.  The TPM error
 * code is returned.
 */
uint32_t TlclSetGlobalLock(void);

/* Performs a TPM_Extend.
 */
uint32_t TlclExtend(int pcr_num, const uint8_t* in_digest, uint8_t* out_digest);

/* Gets the permission bits for the NVRAM space with |index|.
 */
uint32_t TlclGetPermissions(uint32_t index, uint32_t* permissions);

/* Gets the entire set of permanent flags.
 */
uint32_t TlclGetPermanentFlags(TPM_PERMANENT_FLAGS* pflags);

/* Gets the entire set of volatile (ST_CLEAR) flags.
 */
uint32_t TlclGetSTClearFlags(TPM_STCLEAR_FLAGS* pflags);

/* Requests [length] bytes from TPM RNG to be stored in [data]. Actual
 * number of bytes read is stored in [size]. The TPM error code is returned.
 */
uint32_t TlclGetRandom(uint8_t* data, uint32_t length, uint32_t* size);

#endif  /* TPM_LITE_TLCL_H_ */
