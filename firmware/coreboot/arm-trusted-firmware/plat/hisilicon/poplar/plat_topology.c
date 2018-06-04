/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <plat_arm.h>
#include <psci.h>
#include "platform_def.h"

const unsigned char hisi_power_domain_tree_desc[] = {
	PLATFORM_CLUSTER_COUNT,
	PLATFORM_CORE_COUNT,
};

const unsigned char *plat_get_power_domain_tree_desc(void)
{
	return hisi_power_domain_tree_desc;
}

int plat_core_pos_by_mpidr(u_register_t mpidr)
{
	if (mpidr & MPIDR_CLUSTER_MASK)
		return -1;

	if ((mpidr & MPIDR_CPU_MASK) >= PLATFORM_CORE_COUNT)
		return -1;

	return plat_arm_calc_core_pos(mpidr);
}
