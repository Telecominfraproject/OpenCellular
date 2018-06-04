/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * TPM Lightweight Command Library.
 *
 * A low-level library for interfacing to TPM hardware or an emulator.
 */

#ifndef TSS_H_
#define TSS_H_
#include <stdint.h>
#include <types.h>

#include "tss_constants.h"

/*****************************************************************************/
/* Functions implemented in tlcl.c */

/**
 * Call this first.  Returns 0 if success, nonzero if error.
 */
uint32_t tlcl_lib_init(void);

/**
 * Perform a raw TPM request/response transaction.
 */
uint32_t tlcl_send_receive(const uint8_t *request, uint8_t *response,
			   int max_length);

/* Commands */

/**
 * Send a TPM_Startup(ST_CLEAR).  The TPM error code is returned (0 for
 * success).
 */
uint32_t tlcl_startup(void);

/**
 * Resume by sending a TPM_Startup(ST_STATE).  The TPM error code is returned
 * (0 for success).
 */
uint32_t tlcl_resume(void);

/**
 * Run the self test.
 *
 * Note---this is synchronous.  To run this in parallel with other firmware,
 * use ContinueSelfTest().  The TPM error code is returned.
 */
uint32_t tlcl_self_test_full(void);

/**
 * Run the self test in the background.
 */
uint32_t tlcl_continue_self_test(void);

#if IS_ENABLED(CONFIG_TPM)
/**
 * Define a space with permission [perm].  [index] is the index for the space,
 * [size] the usable data size.  The TPM error code is returned.
 */
uint32_t tlcl_define_space(uint32_t index, uint32_t perm, uint32_t size);

#elif IS_ENABLED(CONFIG_TPM2)

/*
 * Define a TPM space. The define space command TPM command used by the tlcl
 * layer is enforcing the policy which would not allow to delete the created
 * space after any PCR0 change from its initial value.
 */
uint32_t tlcl_define_space(uint32_t space_index, size_t space_size);
#endif

/**
 * Write [length] bytes of [data] to space at [index].  The TPM error code is
 * returned.
 */
uint32_t tlcl_write(uint32_t index, const void *data, uint32_t length);

/**
 * Read [length] bytes from space at [index] into [data].  The TPM error code
 * is returned.
 */
uint32_t tlcl_read(uint32_t index, void *data, uint32_t length);

/**
 * Assert physical presence in software.  The TPM error code is returned.
 */
uint32_t tlcl_assert_physical_presence(void);

/**
 * Enable the physical presence command.  The TPM error code is returned.
 */
uint32_t tlcl_physical_presence_cmd_enable(void);

/**
 * Finalize the physical presence settings: sofware PP is enabled, hardware PP
 * is disabled, and the lifetime lock is set.  The TPM error code is returned.
 */
uint32_t tlcl_finalize_physical_presence(void);

/**
 * Set the nvLocked bit.  The TPM error code is returned.
 */
uint32_t tlcl_set_nv_locked(void);

/**
 * Issue a ForceClear.  The TPM error code is returned.
 */
uint32_t tlcl_force_clear(void);

/**
 * Issue a PhysicalEnable.  The TPM error code is returned.
 */
uint32_t tlcl_set_enable(void);

/**
 * Issue a SetDeactivated.  Pass 0 to activate.  Returns result code.
 */
uint32_t tlcl_set_deactivated(uint8_t flag);

/**
 * Get flags of interest.  Pointers for flags you aren't interested in may
 * be NULL.  The TPM error code is returned.
 */
uint32_t tlcl_get_flags(uint8_t *disable, uint8_t *deactivated,
			uint8_t *nvlocked);

/**
 * Set the bGlobalLock flag, which only a reboot can clear.  The TPM error
 * code is returned.
 */
uint32_t tlcl_set_global_lock(void);

/**
 * Make an NV Ram location read_only.  The TPM error code is returned.
 */
uint32_t tlcl_lock_nv_write(uint32_t index);

/**
 * Perform a TPM_Extend.
 */
uint32_t tlcl_extend(int pcr_num, const uint8_t *in_digest,
		     uint8_t *out_digest);

/**
 * Get the entire set of permanent flags.
 */
uint32_t tlcl_get_permanent_flags(TPM_PERMANENT_FLAGS *pflags);

/**
 * Disable platform hierarchy. Specific to TPM2. The TPM error code is returned.
 */
uint32_t tlcl_disable_platform_hierarchy(void);

/**
 * CR50 specific tpm command to enable nvmem commits before internal timeout
 * expires.
 */
uint32_t tlcl_cr50_enable_nvcommits(void);

/**
 * CR50 specific tpm command to restore header(s) of the dormant RO/RW
 * image(s) and in case there indeed was a dormant image, trigger reboot after
 * the timeout milliseconds. Note that timeout of zero means "NO REBOOT", not
 * "IMMEDIATE REBOOT".
 *
 * Return value indicates success or failure of accessing the TPM; in case of
 * success the number of restored headers is saved in num_restored_headers.
 */
uint32_t tlcl_cr50_enable_update(uint16_t timeout_ms,
				 uint8_t *num_restored_headers);

#endif  /* TSS_H_ */
