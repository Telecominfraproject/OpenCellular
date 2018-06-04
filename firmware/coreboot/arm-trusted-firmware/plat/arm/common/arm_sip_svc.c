/*
 * Copyright (c) 2016-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arm_sip_svc.h>
#include <debug.h>
#include <plat_arm.h>
#include <pmf.h>
#include <runtime_svc.h>
#include <stdint.h>
#include <uuid.h>


/* ARM SiP Service UUID */
DEFINE_SVC_UUID(arm_sip_svc_uid,
		0xe2756d55, 0x3360, 0x4bb5, 0xbf, 0xf3,
		0x62, 0x79, 0xfd, 0x11, 0x37, 0xff);

static int arm_sip_setup(void)
{
	if (pmf_setup() != 0)
		return 1;
	return 0;
}

/*
 * This function handles ARM defined SiP Calls
 */
static uintptr_t arm_sip_handler(unsigned int smc_fid,
			u_register_t x1,
			u_register_t x2,
			u_register_t x3,
			u_register_t x4,
			void *cookie,
			void *handle,
			u_register_t flags)
{
	int call_count = 0;

	/*
	 * Dispatch PMF calls to PMF SMC handler and return its return
	 * value
	 */
	if (is_pmf_fid(smc_fid)) {
		return pmf_smc_handler(smc_fid, x1, x2, x3, x4, cookie,
				handle, flags);
	}

	switch (smc_fid) {
	case ARM_SIP_SVC_EXE_STATE_SWITCH: {
		u_register_t pc;

		/* Allow calls from non-secure only */
		if (!is_caller_non_secure(flags))
			SMC_RET1(handle, STATE_SW_E_DENIED);

		/* Validate supplied entry point */
		pc = (u_register_t) ((x1 << 32) | (uint32_t) x2);
		if (arm_validate_ns_entrypoint(pc))
			SMC_RET1(handle, STATE_SW_E_PARAM);

		/*
		 * Pointers used in execution state switch are all 32 bits wide
		 */
		return arm_execution_state_switch(smc_fid, (uint32_t) x1,
				(uint32_t) x2, (uint32_t) x3, (uint32_t) x4,
				handle);
		}

	case ARM_SIP_SVC_CALL_COUNT:
		/* PMF calls */
		call_count += PMF_NUM_SMC_CALLS;

		/* State switch call */
		call_count += 1;

		SMC_RET1(handle, call_count);

	case ARM_SIP_SVC_UID:
		/* Return UID to the caller */
		SMC_UUID_RET(handle, arm_sip_svc_uid);

	case ARM_SIP_SVC_VERSION:
		/* Return the version of current implementation */
		SMC_RET2(handle, ARM_SIP_SVC_VERSION_MAJOR, ARM_SIP_SVC_VERSION_MINOR);

	default:
		WARN("Unimplemented ARM SiP Service Call: 0x%x \n", smc_fid);
		SMC_RET1(handle, SMC_UNK);
	}

}


/* Define a runtime service descriptor for fast SMC calls */
DECLARE_RT_SVC(
	arm_sip_svc,
	OEN_SIP_START,
	OEN_SIP_END,
	SMC_TYPE_FAST,
	arm_sip_setup,
	arm_sip_handler
);
