/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* TPM Lightweight Command Library.
 *
 * A low-level library for interfacing to TPM hardware or an emulator.
 */

/* FIXME(gauravsh):
 * NOTE: This file is copied over from
 *       src/platform/tpm_lite/src/tlcl/tlcl.h
 * Ideally, we want to directly include it without having two maintain
 * duplicate copies in sync. But in the current model, this is hard
 * to do without breaking standalone compilation.
 * Eventually tpm_lite should be moved into vboot_reference.
 *
 * FURTHER NOTE: The subset of TPM error codes relevant to verified boot
 * (TPM_SUCCESS, etc.) are in tss_constants.h.  A full list of TPM error codes
 * are in /usr/include/tss/tpm_error.h, from the trousers package.
 */

#ifndef TPM_LITE_TLCL_H_
#define TPM_LITE_TLCL_H_

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Call this first.
 */
void TlclLibInit(void);

/* Sends a TPM_Startup(ST_CLEAR).  Note that this is a no-op for the emulator,
 * because it runs this command during initialization.  The TPM error code is
 * returned (0 for success).
 */
uint32_t TlclStartup(void);

/* Run the self test.  Note---this is synchronous.  To run this in parallel
 * with other firmware, use ContinueSelfTest.  The TPM error code is returned.
 */
uint32_t TlclSelftestfull(void);

/* Runs the self test in the background.  The TPM error code is returned.
 */
uint32_t TlclContinueSelfTest(void);

/* Defines a space with permission [perm].  [index] is the index for the space,
 * [size] the usable data size.  The TPM error code is returned.
 */
uint32_t TlclDefineSpace(uint32_t index, uint32_t perm, uint32_t size);

/* Writes [length] bytes of [data] to space at [index].  The TPM error code is
 * returned.
 */
uint32_t TlclWrite(uint32_t index, uint8_t *data, uint32_t length);

/* Reads [length] bytes from space at [index] into [data].  The TPM error code
 * is returned.
 */
uint32_t TlclRead(uint32_t index, uint8_t *data, uint32_t length);

/* Write-locks space at [index].  The TPM error code is returned.
 */
uint32_t TlclWriteLock(uint32_t index);

/* Read-locks space at [index].  The TPM error code is returned.
 */
uint32_t TlclReadLock(uint32_t index);

/* Asserts physical presence in software.  The TPM error code is returned.
 */
uint32_t TlclAssertPhysicalPresence(void);

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

/* Issues a SetEnable.  The TPM error code is returned.
 */
uint32_t TlclSetEnable(void);

/* Issues a SetDeactivated.  Pass 0 to activate.  Returns result code.
 */
uint32_t TlclSetDeactivated(uint8_t flag);

/* Gets flags of interest.  (Add more here as needed.)  The TPM error code is
 * returned.
 */
uint32_t TlclGetFlags(uint8_t* disable, uint8_t* deactivated);

/* Sets the bGlobalLock flag, which only a reboot can clear.  The TPM error
 * code is returned.
 */
uint32_t TlclSetGlobalLock(void);

#endif  /* TPM_LITE_TLCL_H_ */
