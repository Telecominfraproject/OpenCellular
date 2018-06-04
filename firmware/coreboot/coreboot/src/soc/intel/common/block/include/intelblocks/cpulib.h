/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef SOC_INTEL_COMMON_BLOCK_CPULIB_H
#define SOC_INTEL_COMMON_BLOCK_CPULIB_H

#include <stdint.h>

/*
 * Set PERF_CTL MSR (0x199) P_Req (14:8 bits) with
 * Turbo Ratio which is the Maximum Ratio.
 */
void cpu_set_max_ratio(void);

/*
 * Get the TDP Nominal Ratio from MSR 0x648 Bits 7:0.
 */
u8 cpu_get_tdp_nominal_ratio(void);

/*
 * Read PLATFORM_INFO MSR (0xCE).
 * Return Value of Bit 34:33 (CONFIG_TDP_LEVELS).
 *
 * Possible values of Bit 34:33 are -
 * 00 : Config TDP not supported
 * 01 : One Additional TDP level supported
 * 10 : Two Additional TDP level supported
 * 11 : Reserved
 */
int cpu_config_tdp_levels(void);

/*
 * TURBO_RATIO_LIMIT MSR (0x1AD) Bits 31:0 indicates the
 * factory configured values for of 1-core, 2-core, 3-core
 * and 4-core turbo ratio limits for all processors.
 *
 * 7:0 -	MAX_TURBO_1_CORE
 * 15:8 -	MAX_TURBO_2_CORES
 * 23:16 -	MAX_TURBO_3_CORES
 * 31:24 -	MAX_TURBO_4_CORES
 *
 * Set PERF_CTL MSR (0x199) P_Req (14:8 bits) with that value.
 */
void cpu_set_p_state_to_turbo_ratio(void);

/*
 * CONFIG_TDP_NOMINAL MSR (0x648) Bits 7:0 tells Nominal
 * TDP level ratio to be used for specific processor (in units
 * of 100MHz).
 *
 * Set PERF_CTL MSR (0x199) P_Req (14:8 bits) with that value.
 */
void cpu_set_p_state_to_nominal_tdp_ratio(void);

/*
 * PLATFORM_INFO MSR (0xCE) Bits 15:8 tells
 * MAX_NON_TURBO_LIM_RATIO.
 *
 * Set PERF_CTL MSR (0x199) P_Req (14:8 bits) with that value.
 */
void cpu_set_p_state_to_max_non_turbo_ratio(void);

/*
 * Get the Burst/Turbo Mode State from MSR IA32_MISC_ENABLE 0x1A0
 * Bit 38 - TURBO_MODE_DISABLE Bit to get state ENABLED / DISABLED.
 * Also check for the cpuid 0x6 to check whether Burst mode unsupported.
 * Below are the possible cpu_get_burst_mode_state() return values-
 * These states are exposed to the User since user
 * need to know which is the current Burst Mode State.
 */
enum {
	BURST_MODE_UNKNOWN,
	BURST_MODE_UNAVAILABLE,
	BURST_MODE_DISABLED,
	BURST_MODE_ENABLED
};
int cpu_get_burst_mode_state(void);

/*
 * Enable Burst mode.
 */
void cpu_enable_burst_mode(void);

/*
 * Disable Burst mode.
 */
void cpu_disable_burst_mode(void);

/*
 * Enable Intel Enhanced Speed Step Technology.
 */
void cpu_enable_eist(void);

/*
 * Disable Intel Enhanced Speed Step Technology.
 */
void cpu_disable_eist(void);

/*
 * Set Bit 6 (ENABLE_IA_UNTRUSTED_MODE) of MSR 0x120
 * UCODE_PCR_POWER_MISC MSR to enter IA Untrusted Mode.
 */
void cpu_enable_untrusted_mode(void *unused);

/*
 * This function fills in the number of Cores(physical) and Threads(virtual)
 * of the CPU in the function arguments. It also returns if the number of cores
 * and number of threads are equal.
 */
int cpu_read_topology(unsigned int *num_phys, unsigned int *num_virt);

/*
 * cpu_get_bus_clock returns the bus clock frequency in KHz.
 * This is the value the clock ratio is multiplied with.
 */
uint32_t cpu_get_bus_clock(void);

/*
 * cpu_get_coord_type returns coordination type (SW_ANY or SW_ALL or HW_ALL)
 * which is used to populate _PSD object.
 */
int cpu_get_coord_type(void);

/*
 * cpu_get_min_ratio returns the minimum frequency ratio that is supported
 * by this processor
 */
uint32_t cpu_get_min_ratio(void);

/*
 * cpu_get_max_ratio returns the nominal TDP ratio if available or the
 * maximum non turbo frequency ratio for this processor
 */
uint32_t cpu_get_max_ratio(void);

/*
 * cpu_get_power_max calculates CPU TDP in mW
 */
uint32_t cpu_get_power_max(void);

/*
 * cpu_get_max_turbo_ratio returns the maximum turbo ratio limit for the
 * processor
 */
uint32_t cpu_get_max_turbo_ratio(void);

/* Configure Machine Check Architecture support */
void mca_configure(void);

#endif	/* SOC_INTEL_COMMON_BLOCK_CPULIB_H */
