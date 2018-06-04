/*
 * Copyright (c) 2017-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <bl31.h>
#include <context_mgmt.h>
#include <debug.h>
#include <errno.h>
#include <mm_svc.h>
#include <platform.h>
#include <runtime_svc.h>
#include <secure_partition.h>
#include <smccc.h>
#include <smccc_helpers.h>
#include <spinlock.h>
#include <spm_svc.h>
#include <utils.h>
#include <xlat_tables_v2.h>

#include "spm_private.h"

/*******************************************************************************
 * Secure Partition context information.
 ******************************************************************************/
static sp_context_t sp_ctx;

/*******************************************************************************
 * Set state of a Secure Partition context.
 ******************************************************************************/
void sp_state_set(sp_context_t *sp_ptr, sp_state_t state)
{
	spin_lock(&(sp_ptr->state_lock));
	sp_ptr->state = state;
	spin_unlock(&(sp_ptr->state_lock));
}

/*******************************************************************************
 * Wait until the state of a Secure Partition is the specified one and change it
 * to the desired state.
 ******************************************************************************/
void sp_state_wait_switch(sp_context_t *sp_ptr, sp_state_t from, sp_state_t to)
{
	int success = 0;

	while (success == 0) {
		spin_lock(&(sp_ptr->state_lock));

		if (sp_ptr->state == from) {
			sp_ptr->state = to;

			success = 1;
		}

		spin_unlock(&(sp_ptr->state_lock));
	}
}

/*******************************************************************************
 * Check if the state of a Secure Partition is the specified one and, if so,
 * change it to the desired state. Returns 0 on success, -1 on error.
 ******************************************************************************/
int sp_state_try_switch(sp_context_t *sp_ptr, sp_state_t from, sp_state_t to)
{
	int ret = -1;

	spin_lock(&(sp_ptr->state_lock));

	if (sp_ptr->state == from) {
		sp_ptr->state = to;

		ret = 0;
	}

	spin_unlock(&(sp_ptr->state_lock));

	return ret;
}

/*******************************************************************************
 * This function takes an SP context pointer and prepares the CPU to enter.
 ******************************************************************************/
static void spm_sp_prepare_enter(sp_context_t *sp_ctx)
{
	assert(sp_ctx != NULL);

	/* Assign the context of the SP to this CPU */
	cm_set_context(&(sp_ctx->cpu_ctx), SECURE);

	/* Restore the context assigned above */
	cm_el1_sysregs_context_restore(SECURE);
	cm_set_next_eret_context(SECURE);

	/* Invalidate TLBs at EL1. */
	tlbivmalle1();
	dsbish();
}

/*******************************************************************************
 * Enter SP after preparing it with spm_sp_prepare_enter().
 ******************************************************************************/
static uint64_t spm_sp_enter(sp_context_t *sp_ctx)
{
	/* Enter Secure Partition */
	return spm_secure_partition_enter(&sp_ctx->c_rt_ctx);
}

/*******************************************************************************
 * Jump to each Secure Partition for the first time.
 ******************************************************************************/
static int32_t spm_init(void)
{
	uint64_t rc = 0;
	sp_context_t *ctx;

	INFO("Secure Partition init...\n");

	ctx = &sp_ctx;

	ctx->state = SP_STATE_RESET;

	spm_sp_prepare_enter(ctx);
	rc |= spm_sp_enter(ctx);
	assert(rc == 0);

	ctx->state = SP_STATE_IDLE;

	INFO("Secure Partition initialized.\n");

	return rc;
}

/*******************************************************************************
 * Initialize contexts of all Secure Partitions.
 ******************************************************************************/
int32_t spm_setup(void)
{
	sp_context_t *ctx;

	/* Disable MMU at EL1 (initialized by BL2) */
	disable_mmu_icache_el1();

	/* Initialize context of the SP */
	INFO("Secure Partition context setup start...\n");

	ctx = &sp_ctx;

	/* Assign translation tables context. */
	ctx->xlat_ctx_handle = spm_get_sp_xlat_context();

	spm_sp_setup(ctx);

	/* Register init function for deferred init.  */
	bl31_register_bl32_init(&spm_init);

	INFO("Secure Partition setup done.\n");

	return 0;
}

/*******************************************************************************
 * MM_COMMUNICATE handler
 ******************************************************************************/
static uint64_t mm_communicate(uint32_t smc_fid, uint64_t mm_cookie,
			       uint64_t comm_buffer_address,
			       uint64_t comm_size_address, void *handle)
{
	sp_context_t *ctx = &sp_ctx;

	/* Cookie. Reserved for future use. It must be zero. */
	if (mm_cookie != 0U) {
		ERROR("MM_COMMUNICATE: cookie is not zero\n");
		SMC_RET1(handle, SPM_INVALID_PARAMETER);
	}

	if (comm_buffer_address == 0U) {
		ERROR("MM_COMMUNICATE: comm_buffer_address is zero\n");
		SMC_RET1(handle, SPM_INVALID_PARAMETER);
	}

	if (comm_size_address != 0U) {
		VERBOSE("MM_COMMUNICATE: comm_size_address is not 0 as recommended.\n");
	}

	/* Save the Normal world context */
	cm_el1_sysregs_context_save(NON_SECURE);

	/* Wait until the Secure Partition is IDLE and set it to BUSY. */
	sp_state_wait_switch(ctx, SP_STATE_IDLE, SP_STATE_BUSY);

	/* Jump to the Secure Partition. */
	spm_sp_prepare_enter(ctx);

	SMC_RET4(&(ctx->cpu_ctx), smc_fid, comm_buffer_address,
		 comm_size_address, plat_my_core_pos());
}

/*******************************************************************************
 * SP_EVENT_COMPLETE_AARCH64 handler
 ******************************************************************************/
static uint64_t sp_event_complete(uint64_t x1)
{
	sp_context_t *ctx = &sp_ctx;

	/* Save secure state */
	cm_el1_sysregs_context_save(SECURE);

	if (ctx->state == SP_STATE_RESET) {
		/*
		 * SPM reports completion. The SPM must have initiated the
		 * original request through a synchronous entry into the secure
		 * partition. Jump back to the original C runtime context.
		 */
		spm_secure_partition_exit(ctx->c_rt_ctx, x1);

		/* spm_secure_partition_exit doesn't return */
	}

	/*
	 * This is the result from the Secure partition of an earlier request.
	 * Copy the result into the non-secure context and return to the
	 * non-secure state.
	 */

	/* Mark Secure Partition as idle */
	assert(ctx->state == SP_STATE_BUSY);

	sp_state_set(ctx, SP_STATE_IDLE);

	/* Get a reference to the non-secure context */
	cpu_context_t *ns_cpu_context = cm_get_context(NON_SECURE);

	assert(ns_cpu_context != NULL);

	/* Restore non-secure state */
	cm_el1_sysregs_context_restore(NON_SECURE);
	cm_set_next_eret_context(NON_SECURE);

	/* Return to non-secure world */
	SMC_RET1(ns_cpu_context, x1);
}

/*******************************************************************************
 * Secure Partition Manager SMC handler.
 ******************************************************************************/
uint64_t spm_smc_handler(uint32_t smc_fid,
			 uint64_t x1,
			 uint64_t x2,
			 uint64_t x3,
			 uint64_t x4,
			 void *cookie,
			 void *handle,
			 uint64_t flags)
{
	unsigned int ns;

	/* Determine which security state this SMC originated from */
	ns = is_caller_non_secure(flags);

	if (ns == SMC_FROM_SECURE) {

		/* Handle SMCs from Secure world. */

		assert(handle == cm_get_context(SECURE));

		/* Make next ERET jump to S-EL0 instead of S-EL1. */
		cm_set_elr_spsr_el3(SECURE, read_elr_el1(), read_spsr_el1());

		switch (smc_fid) {

		case SPM_VERSION_AARCH32:
			SMC_RET1(handle, SPM_VERSION_COMPILED);

		case SP_EVENT_COMPLETE_AARCH64:
			return sp_event_complete(x1);

		case SP_MEMORY_ATTRIBUTES_GET_AARCH64:
			INFO("Received SP_MEMORY_ATTRIBUTES_GET_AARCH64 SMC\n");

			if (sp_ctx.state != SP_STATE_RESET) {
				WARN("SP_MEMORY_ATTRIBUTES_GET_AARCH64 is available at boot time only\n");
				SMC_RET1(handle, SPM_NOT_SUPPORTED);
			}
			SMC_RET1(handle,
				 spm_memory_attributes_get_smc_handler(
					 &sp_ctx, x1));

		case SP_MEMORY_ATTRIBUTES_SET_AARCH64:
			INFO("Received SP_MEMORY_ATTRIBUTES_SET_AARCH64 SMC\n");

			if (sp_ctx.state != SP_STATE_RESET) {
				WARN("SP_MEMORY_ATTRIBUTES_SET_AARCH64 is available at boot time only\n");
				SMC_RET1(handle, SPM_NOT_SUPPORTED);
			}
			SMC_RET1(handle,
				 spm_memory_attributes_set_smc_handler(
					&sp_ctx, x1, x2, x3));
		default:
			break;
		}
	} else {

		/* Handle SMCs from Non-secure world. */

		switch (smc_fid) {

		case MM_VERSION_AARCH32:
			SMC_RET1(handle, MM_VERSION_COMPILED);

		case MM_COMMUNICATE_AARCH32:
		case MM_COMMUNICATE_AARCH64:
			return mm_communicate(smc_fid, x1, x2, x3, handle);

		case SP_MEMORY_ATTRIBUTES_GET_AARCH64:
		case SP_MEMORY_ATTRIBUTES_SET_AARCH64:
			/* SMC interfaces reserved for secure callers. */
			SMC_RET1(handle, SPM_NOT_SUPPORTED);

		default:
			break;
		}
	}

	SMC_RET1(handle, SMC_UNK);
}
