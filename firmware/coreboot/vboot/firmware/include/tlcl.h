/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * TPM Lightweight Command Library.
 *
 * A low-level library for interfacing to TPM hardware or an emulator.
 */

#ifndef TPM_LITE_TLCL_H_
#define TPM_LITE_TLCL_H_
#include <stdint.h>

#include "tss_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/* Functions implemented in tlcl.c */

/**
 * Call this first.  Returns 0 if success, nonzero if error.
 */
uint32_t TlclLibInit(void);

/**
 * Call this on shutdown.  Returns 0 if success, nonzero if error.
 */
uint32_t TlclLibClose(void);

/* Low-level operations */

/**
 * Perform a raw TPM request/response transaction.
 */
uint32_t TlclSendReceive(const uint8_t *request, uint8_t *response,
                         int max_length);

/**
 * Return the size of a TPM request or response packet.
 */
int TlclPacketSize(const uint8_t *packet);

/* Commands */

/**
 * Send a TPM_Startup(ST_CLEAR).  The TPM error code is returned (0 for
 * success).
 */
uint32_t TlclStartup(void);

/**
 * Save the TPM state.  Normally done by the kernel before a suspend, included
 * here for tests.  The TPM error code is returned (0 for success).
 */
uint32_t TlclSaveState(void);

/**
 * Resume by sending a TPM_Startup(ST_STATE).  The TPM error code is returned
 * (0 for success).
 */
uint32_t TlclResume(void);

/**
 * Run the self test.
 *
 * Note---this is synchronous.  To run this in parallel with other firmware,
 * use ContinueSelfTest().  The TPM error code is returned.
 */
uint32_t TlclSelfTestFull(void);

/**
 * Run the self test in the background.
 */
uint32_t TlclContinueSelfTest(void);

/**
 * Define a space with permission [perm].  [index] is the index for the space,
 * [size] the usable data size.  The TPM error code is returned.
 */
uint32_t TlclDefineSpace(uint32_t index, uint32_t perm, uint32_t size);

/**
 * Define a space using owner authorization secret [owner_auth]. The space is
 * set up to have permission [perm].  [index] is the index for the space, [size]
 * the usable data size. Optional auth policy (such as PCR selections) can be
 * passed via [auth_policy]. The TPM error code is returned.
 */
uint32_t TlclDefineSpaceEx(const uint8_t* owner_auth, uint32_t owner_auth_size,
                           uint32_t index, uint32_t perm, uint32_t size,
                           const void* auth_policy, uint32_t auth_policy_size);

/**
 * Initializes [auth_policy] to require PCR binding of the given
 * [pcr_selection_bitmap]. The PCR values are passed in the [pcr_values]
 * parameter with each entry corresponding to the sequence of indexes that
 * corresponds to the bits that are set in [pcr_selection_bitmap]. Returns
 * TPM_SUCCESS if successful, TPM_E_BUFFER_SIZE if the provided buffer is too
 * short. The actual size of the policy will be set in [auth_policy_size] upon
 * return, also for the case of insufficient buffer size.
 */
uint32_t TlclInitNvAuthPolicy(uint32_t pcr_selection_bitmap,
                              const uint8_t pcr_values[][TPM_PCR_DIGEST],
                              void* auth_policy, uint32_t* auth_policy_size);

/**
 * Write [length] bytes of [data] to space at [index].  The TPM error code is
 * returned.
 */
uint32_t TlclWrite(uint32_t index, const void *data, uint32_t length);

/**
 * Read [length] bytes from space at [index] into [data].  The TPM error code
 * is returned.
 */
uint32_t TlclRead(uint32_t index, void *data, uint32_t length);

/**
 * Read PCR at [index] into [data].  [length] must be TPM_PCR_DIGEST or
 * larger. The TPM error code is returned.
 */
uint32_t TlclPCRRead(uint32_t index, void *data, uint32_t length);

/**
 * Write-lock space at [index].  The TPM error code is returned.
 */
uint32_t TlclWriteLock(uint32_t index);

/**
 * Read-lock space at [index].  The TPM error code is returned.
 */
uint32_t TlclReadLock(uint32_t index);

/**
 * Assert physical presence in software.  The TPM error code is returned.
 */
uint32_t TlclAssertPhysicalPresence(void);

/**
 * Enable the physical presence command.  The TPM error code is returned.
 */
uint32_t TlclPhysicalPresenceCMDEnable(void);

/**
 * Finalize the physical presence settings: sofware PP is enabled, hardware PP
 * is disabled, and the lifetime lock is set.  The TPM error code is returned.
 */
uint32_t TlclFinalizePhysicalPresence(void);

uint32_t TlclAssertPhysicalPresenceResult(void);

/**
 * Turn off physical presence and locks it off until next reboot.  The TPM
 * error code is returned.
 */
uint32_t TlclLockPhysicalPresence(void);

/**
 * Set the nvLocked bit.  The TPM error code is returned.
 */
uint32_t TlclSetNvLocked(void);

/**
 * Return 1 if the TPM is owned, 0 otherwise.
 */
int TlclIsOwned(void);

/**
 * Issue a ForceClear.  The TPM error code is returned.
 */
uint32_t TlclForceClear(void);

/**
 * Issue a PhysicalEnable.  The TPM error code is returned.
 */
uint32_t TlclSetEnable(void);

/**
 * Issue a PhysicalDisable.  The TPM error code is returned.
 */
uint32_t TlclClearEnable(void);

/**
 * Issue a SetDeactivated.  Pass 0 to activate.  Returns result code.
 */
uint32_t TlclSetDeactivated(uint8_t flag);

/**
 * Get flags of interest.  Pointers for flags you aren't interested in may
 * be NULL.  The TPM error code is returned.
 */
uint32_t TlclGetFlags(uint8_t *disable, uint8_t *deactivated,
                      uint8_t *nvlocked);

/**
 * Set the bGlobalLock flag, which only a reboot can clear.  The TPM error
 * code is returned.
 */
uint32_t TlclSetGlobalLock(void);

/**
 * Perform a TPM_Extend.
 */
uint32_t TlclExtend(int pcr_num, const uint8_t *in_digest, uint8_t *out_digest);

/**
 * Get the permission bits for the NVRAM space with |index|.
 */
uint32_t TlclGetPermissions(uint32_t index, uint32_t *permissions);

/**
 * Get the public information about the NVRAM space identified by |index|. All
 * other parameters are filled in with the respective information.
 * |auth_policy_size| is both an input an output parameter. It should contain
 * the available buffer size in |auth_policy| and will be updated to indicate
 * the size of the filled in auth policy upon return. If the buffer size is not
 * sufficient, the return value will be TPM_E_BUFFER_SIZE.
 */
uint32_t TlclGetSpaceInfo(uint32_t index, uint32_t *attributes, uint32_t *size,
                          void* auth_policy, uint32_t* auth_policy_size);

/**
 * Get the entire set of permanent flags.
 */
uint32_t TlclGetPermanentFlags(TPM_PERMANENT_FLAGS *pflags);

/**
 * Get the entire set of volatile (ST_CLEAR) flags.
 */
uint32_t TlclGetSTClearFlags(TPM_STCLEAR_FLAGS *pflags);

/**
 * Get the ownership flag. The TPM error code is returned.
 */
uint32_t TlclGetOwnership(uint8_t *owned);

/**
 * Request [length] bytes from TPM RNG to be stored in [data]. Actual number of
 * bytes read is stored in [size]. The TPM error code is returned.
 */
uint32_t TlclGetRandom(uint8_t *data, uint32_t length, uint32_t *size);

/**
 * Requests version information from the TPM.
 * If vendor_specific_buf_size != NULL, requests also the vendor-specific
 * variable-length part of the version:
 *   if vendor_specific_buf == NULL, determines its size and returns in
 *       *vendor_specific_buf_size;
 *   if vendor_specific_buf != NULL, fills the buffer until either the
 *       end of the vendor specific data or the end of the buffer, sets
 *       *vendor_specific_buf_size to the length of the filled data.
 */
uint32_t TlclGetVersion(uint32_t* vendor, uint64_t* firmware_version,
                        uint8_t* vendor_specific_buf,
                        size_t* vendor_specific_buf_size);

/**
 * Issues the IFX specific FieldUpgradeInfoRequest2 TPM_FieldUpgrade subcommand
 * and fills in [info] with results.
 */
uint32_t TlclIFXFieldUpgradeInfo(TPM_IFX_FIELDUPGRADEINFO *info);

#ifdef CHROMEOS_ENVIRONMENT
#ifndef TPM2_MODE

/**
 * Read the public half of the EK.
 */
uint32_t TlclReadPubek(uint32_t* public_exponent,
                       uint8_t* modulus,
                       uint32_t* modulus_size);

/**
 * Takes ownership of the TPM. [enc_owner_auth] and [enc_srk_auth] are the owner
 * and SRK authorization secrets encrypted under the endorsement key. The clear
 * text [owner_auth] needs to be passed as well for command auth.
 */
uint32_t TlclTakeOwnership(uint8_t enc_owner_auth[TPM_RSA_2048_LEN],
                           uint8_t enc_srk_auth[TPM_RSA_2048_LEN],
                           uint8_t owner_auth[TPM_AUTH_DATA_LEN]);

/**
 * Create a delegation family with the specified [family_label].
 */
uint32_t TlclCreateDelegationFamily(uint8_t family_label);

/**
 * Read the delegation family table. Entries are stored in [table]. The size of
 * the family table array must be specified in [table_size]. [table_size] gets
 * updated to indicate actual number of table entries available.
 */
uint32_t TlclReadDelegationFamilyTable(TPM_FAMILY_TABLE_ENTRY *table,
                                       uint32_t* table_size);

#endif  /* TPM2_MODE */
#endif  /* CHROMEOS_ENVIRONMENT */

#ifdef __cplusplus
}
#endif

#endif  /* TPM_LITE_TLCL_H_ */
