/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Google Inc.
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

#ifndef _BROADWELL_MSR_H_
#define _BROADWELL_MSR_H_

#define MSR_PIC_MSG_CONTROL		0x2e
#define CORE_THREAD_COUNT_MSR		0x35
#define IA32_FEATURE_CONTROL		0x3a
#define  CPUID_VMX			(1 << 5)
#define  CPUID_SMX			(1 << 6)
#define MSR_PLATFORM_INFO		0xce
#define  PLATFORM_INFO_SET_TDP		(1 << 29)
#define MSR_PMG_CST_CONFIG_CONTROL	0xe2
#define MSR_PMG_IO_CAPTURE_BASE		0xe4
#define MSR_FEATURE_CONFIG		0x13c
#define SMM_MCA_CAP_MSR			0x17d
#define  SMM_CPU_SVRSTR_BIT		57
#define  SMM_CPU_SVRSTR_MASK		(1 << (SMM_CPU_SVRSTR_BIT - 32))
#define MSR_FLEX_RATIO			0x194
#define  FLEX_RATIO_LOCK		(1 << 20)
#define  FLEX_RATIO_EN			(1 << 16)
#define IA32_MISC_ENABLE		0x1a0
#define MSR_MISC_PWR_MGMT		0x1aa
#define  MISC_PWR_MGMT_EIST_HW_DIS	(1 << 0)
#define MSR_TURBO_RATIO_LIMIT		0x1ad
#define MSR_TEMPERATURE_TARGET		0x1a2
#define IA32_PERF_CTL			0x199
#define IA32_THERM_INTERRUPT		0x19b
#define IA32_ENERGY_PERFORMANCE_BIAS	0x1b0
#define  ENERGY_POLICY_PERFORMANCE	0
#define  ENERGY_POLICY_NORMAL		6
#define  ENERGY_POLICY_POWERSAVE	15
#define IA32_PACKAGE_THERM_INTERRUPT	0x1b2
#define EMRRphysBase_MSR		0x1f4
#define EMRRphysMask_MSR		0x1f5
#define IA32_PLATFORM_DCA_CAP		0x1f8
#define MSR_POWER_CTL			0x1fc
#define MSR_LT_LOCK_MEMORY		0x2e7
#define UNCORE_EMRRphysBase_MSR		0x2f4
#define UNCORE_EMRRphysMask_MSR		0x2f5
#define IA32_MC0_STATUS			0x401
#define SMM_FEATURE_CONTROL_MSR		0x4e0
#define  SMM_CPU_SAVE_EN		(1 << 1)

#define MSR_C_STATE_LATENCY_CONTROL_0	0x60a
#define MSR_C_STATE_LATENCY_CONTROL_1	0x60b
#define MSR_C_STATE_LATENCY_CONTROL_2	0x60c
#define MSR_C_STATE_LATENCY_CONTROL_3	0x633
#define MSR_C_STATE_LATENCY_CONTROL_4	0x634
#define MSR_C_STATE_LATENCY_CONTROL_5	0x635
#define  IRTL_VALID			(1 << 15)
#define  IRTL_1_NS			(0 << 10)
#define  IRTL_32_NS			(1 << 10)
#define  IRTL_1024_NS			(2 << 10)
#define  IRTL_32768_NS			(3 << 10)
#define  IRTL_1048576_NS		(4 << 10)
#define  IRTL_33554432_NS		(5 << 10)
#define  IRTL_RESPONSE_MASK		(0x3ff)
#define MSR_COUNTER_24_MHZ		0x637

/* long duration in low dword, short duration in high dword */
#define MSR_PKG_POWER_LIMIT		0x610
#define  PKG_POWER_LIMIT_MASK		0x7fff
#define  PKG_POWER_LIMIT_EN		(1 << 15)
#define  PKG_POWER_LIMIT_CLAMP		(1 << 16)
#define  PKG_POWER_LIMIT_TIME_SHIFT	17
#define  PKG_POWER_LIMIT_TIME_MASK	0x7f

#define MSR_VR_CURRENT_CONFIG		0x601
#define MSR_VR_MISC_CONFIG		0x603
#define MSR_PKG_POWER_SKU_UNIT		0x606
#define MSR_PKG_POWER_SKU		0x614
#define MSR_DDR_RAPL_LIMIT		0x618
#define MSR_VR_MISC_CONFIG2		0x636
#define MSR_PP0_POWER_LIMIT		0x638
#define MSR_PP1_POWER_LIMIT		0x640

#define MSR_CONFIG_TDP_NOMINAL		0x648
#define MSR_CONFIG_TDP_LEVEL1		0x649
#define MSR_CONFIG_TDP_LEVEL2		0x64a
#define MSR_CONFIG_TDP_CONTROL		0x64b
#define MSR_TURBO_ACTIVATION_RATIO	0x64c

/* SMM save state MSRs */
#define SMBASE_MSR			0xc20
#define IEDBASE_MSR			0xc22

/* MTRR_CAP_MSR bits */
#define SMRR_SUPPORTED (1<<11)
#define EMRR_SUPPORTED (1<<12)

#endif
