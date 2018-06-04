/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Google Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * lib_helpers.h: All library function prototypes and macros are defined in this
 * file.
 */

#ifndef __ARCH_LIB_HELPERS_H__
#define __ARCH_LIB_HELPERS_H__

#define EL0               0
#define EL1               1
#define EL2               2
#define EL3               3

#define CURRENT_EL_MASK   0x3
#define CURRENT_EL_SHIFT  2

#define SPSR_USE_L           0
#define SPSR_USE_H           1
#define SPSR_L_H_MASK        1
#define SPSR_M_SHIFT         4
#define SPSR_ERET_32         (1 << SPSR_M_SHIFT)
#define SPSR_ERET_64         (0 << SPSR_M_SHIFT)
#define SPSR_FIQ             (1 << 6)
#define SPSR_IRQ             (1 << 7)
#define SPSR_SERROR          (1 << 8)
#define SPSR_DEBUG           (1 << 9)
#define SPSR_EXCEPTION_MASK  (SPSR_FIQ | SPSR_IRQ | SPSR_SERROR | SPSR_DEBUG)

#define SCR_NS_SHIFT         0
#define SCR_NS_MASK          (1 << SCR_NS_SHIFT)
#define SCR_NS_ENABLE        (1 << SCR_NS_SHIFT)
#define SCR_NS_DISABLE       (0 << SCR_NS_SHIFT)
#define SCR_NS               SCR_NS_ENABLE
#define SCR_RES1             (0x3 << 4)
#define SCR_IRQ_SHIFT        2
#define SCR_IRQ_MASK         (1 << SCR_IRQ_SHIFT)
#define SCR_IRQ_ENABLE       (1 << SCR_IRQ_SHIFT)
#define SCR_IRQ_DISABLE      (0 << SCR_IRQ_SHIFT)
#define SCR_FIQ_SHIFT        2
#define SCR_FIQ_MASK         (1 << SCR_FIQ_SHIFT)
#define SCR_FIQ_ENABLE       (1 << SCR_FIQ_SHIFT)
#define SCR_FIQ_DISABLE      (0 << SCR_FIQ_SHIFT)
#define SCR_EA_SHIFT         3
#define SCR_EA_MASK          (1 << SCR_EA_SHIFT)
#define SCR_EA_ENABLE        (1 << SCR_EA_SHIFT)
#define SCR_EA_DISABLE       (0 << SCR_EA_SHIFT)
#define SCR_SMD_SHIFT        7
#define SCR_SMD_MASK         (1 << SCR_SMD_SHIFT)
#define SCR_SMD_DISABLE      (1 << SCR_SMD_SHIFT)
#define SCR_SMD_ENABLE       (0 << SCR_SMD_SHIFT)
#define SCR_HVC_SHIFT        8
#define SCR_HVC_MASK         (1 << SCR_HVC_SHIFT)
#define SCR_HVC_DISABLE      (0 << SCR_HVC_SHIFT)
#define SCR_HVC_ENABLE       (1 << SCR_HVC_SHIFT)
#define SCR_SIF_SHIFT        9
#define SCR_SIF_MASK         (1 << SCR_SIF_SHIFT)
#define SCR_SIF_ENABLE       (1 << SCR_SIF_SHIFT)
#define SCR_SIF_DISABLE      (0 << SCR_SIF_SHIFT)
#define SCR_RW_SHIFT         10
#define SCR_RW_MASK          (1 << SCR_RW_SHIFT)
#define SCR_LOWER_AARCH64    (1 << SCR_RW_SHIFT)
#define SCR_LOWER_AARCH32    (0 << SCR_RW_SHIFT)
#define SCR_ST_SHIFT         11
#define SCR_ST_MASK          (1 << SCR_ST_SHIFT)
#define SCR_ST_ENABLE        (1 << SCR_ST_SHIFT)
#define SCR_ST_DISABLE       (0 << SCR_ST_SHIFT)
#define SCR_TWI_SHIFT        12
#define SCR_TWI_MASK         (1 << SCR_TWI_SHIFT)
#define SCR_TWI_ENABLE       (1 << SCR_TWI_SHIFT)
#define SCR_TWI_DISABLE      (0 << SCR_TWI_SHIFT)
#define SCR_TWE_SHIFT        13
#define SCR_TWE_MASK         (1 << SCR_TWE_SHIFT)
#define SCR_TWE_ENABLE       (1 << SCR_TWE_SHIFT)
#define SCR_TWE_DISABLE      (0 << SCR_TWE_SHIFT)

#define HCR_RW_SHIFT         31
#define HCR_LOWER_AARCH64    (1 << HCR_RW_SHIFT)
#define HCR_LOWER_AARCH32    (0 << HCR_RW_SHIFT)

#define SCTLR_MMU_ENABLE     1
#define SCTLR_MMU_DISABLE    0
#define SCTLR_ACE_SHIFT      1
#define SCTLR_ACE_ENABLE     (1 << SCTLR_ACE_SHIFT)
#define SCTLR_ACE_DISABLE    (0 << SCTLR_ACE_SHIFT)
#define SCTLR_CACHE_SHIFT    2
#define SCTLR_CACHE_ENABLE   (1 << SCTLR_CACHE_SHIFT)
#define SCTLR_CACHE_DISABLE  (0 << SCTLR_CACHE_SHIFT)
#define SCTLR_SAE_SHIFT      3
#define SCTLR_SAE_ENABLE     (1 << SCTLR_SAE_SHIFT)
#define SCTLR_SAE_DISABLE    (0 << SCTLR_SAE_SHIFT)
#define SCTLR_RES1           ((0x3 << 4) | (0x1 << 11) | (0x1 << 16) |	\
			      (0x1 << 18) | (0x3 << 22) | (0x3 << 28))
#define SCTLR_ICE_SHIFT      12
#define SCTLR_ICE_ENABLE     (1 << SCTLR_ICE_SHIFT)
#define SCTLR_ICE_DISABLE    (0 << SCTLR_ICE_SHIFT)
#define SCTLR_WXN_SHIFT      19
#define SCTLR_WXN_ENABLE     (1 << SCTLR_WXN_SHIFT)
#define SCTLR_WXN_DISABLE    (0 << SCTLR_WXN_SHIFT)
#define SCTLR_ENDIAN_SHIFT   25
#define SCTLR_LITTLE_END     (0 << SCTLR_ENDIAN_SHIFT)
#define SCTLR_BIG_END        (1 << SCTLR_ENDIAN_SHIFT)

#define CPTR_EL3_TCPAC_SHIFT	(31)
#define CPTR_EL3_TTA_SHIFT	(20)
#define CPTR_EL3_TFP_SHIFT	(10)
#define CPTR_EL3_TCPAC_DISABLE	(0 << CPTR_EL3_TCPAC_SHIFT)
#define CPTR_EL3_TCPAC_ENABLE	(1 << CPTR_EL3_TCPAC_SHIFT)
#define CPTR_EL3_TTA_DISABLE	(0 << CPTR_EL3_TTA_SHIFT)
#define CPTR_EL3_TTA_ENABLE	(1 << CPTR_EL3_TTA_SHIFT)
#define CPTR_EL3_TFP_DISABLE	(0 << CPTR_EL3_TFP_SHIFT)
#define CPTR_EL3_TFP_ENABLE	(1 << CPTR_EL3_TFP_SHIFT)

#define CPACR_TTA_SHIFT	(28)
#define CPACR_TTA_ENABLE	(1 << CPACR_TTA_SHIFT)
#define CPACR_TTA_DISABLE	(0 << CPACR_TTA_SHIFT)
#define CPACR_FPEN_SHIFT	(20)
/*
 * ARMv8-A spec: Values 0b00 and 0b10 both seem to enable traps from el0 and el1
 * for fp reg access.
 */
#define CPACR_TRAP_FP_EL0_EL1	(0 << CPACR_FPEN_SHIFT)
#define CPACR_TRAP_FP_EL0	(1 << CPACR_FPEN_SHIFT)
#define CPACR_TRAP_FP_DISABLE	(3 << CPACR_FPEN_SHIFT)

#ifdef __ASSEMBLY__

/* Macro to switch to label based on current el */
.macro switch_el xreg label1 label2 label3
	mrs	\xreg, CurrentEL
	/* Currently at EL1 */
	 cmp	\xreg, #(EL1 << CURRENT_EL_SHIFT)
	b.eq	\label1
	/* Currently at EL2 */
	cmp	\xreg, #(EL2 << CURRENT_EL_SHIFT)
	b.eq	\label2
	/* Currently at EL3 */
	cmp	\xreg, #(EL3 << CURRENT_EL_SHIFT)
	b.eq	\label3
.endm

/* Macro to read sysreg at current EL
   xreg - reg in which read value needs to be stored
   sysreg - system reg that is to be read
*/
.macro read_current xreg sysreg
	switch_el \xreg, 101f, 102f, 103f
101:
	mrs	\xreg, \sysreg\()_el1
	b	104f
102:
	mrs	\xreg, \sysreg\()_el2
	b	104f
103:
	mrs	\xreg, \sysreg\()_el3
	b	104f
104:
.endm

/* Macro to write sysreg at current EL
   xreg - reg from which value needs to be written
   sysreg - system reg that is to be written
   temp - temp reg that can be used to read current EL
*/
.macro write_current sysreg xreg temp
	switch_el \temp, 101f, 102f, 103f
101:
	msr	\sysreg\()_el1, \xreg
	b	104f
102:
	msr	\sysreg\()_el2, \xreg
	b	104f
103:
	msr	\sysreg\()_el3, \xreg
	b	104f
104:
.endm

/* Macro to read sysreg at current EL - 1
   xreg - reg in which read value needs to be stored
   sysreg - system reg that is to be read
*/
.macro read_lower xreg sysreg
	switch_el \xreg, 101f, 102f, 103f
101:
	b	104f
102:
	mrs	\xreg, \sysreg\()_el1
	b	104f
103:
	mrs	\xreg, \sysreg\()_el2
	b	104f
104:
.endm

/* Macro to write sysreg at current EL - 1
   xreg - reg from which value needs to be written
   sysreg - system reg that is to be written
   temp - temp reg that can be used to read current EL
*/
.macro write_lower sysreg xreg temp
	switch_el \temp, 101f, 102f, 103f
101:
	b	104f
102:
	msr	\sysreg\()_el1, \xreg
	b	104f
103:
	msr	\sysreg\()_el2, \xreg
	b	104f
104:
.endm

/* Macro to read from a register at EL3 only if we are currently at that
   level. This is required to ensure that we do not attempt to read registers
   from a level lower than el3. e.g. SCR is available for read only at EL3.
   IMPORTANT: if EL != EL3, macro silently doesn't perform the read.
*/
.macro read_el3 xreg sysreg
	switch_el \xreg, 402f, 402f, 401f
401:
	mrs	\xreg, \sysreg\()_el3
402:
.endm

/* Macro to write to a register at EL3 only if we are currently at that
   level. This is required to ensure that we do not attempt to write to
   registers from a level lower than el3. e.g. SCR is available to write only at
   EL3.
   IMPORTANT: if EL != EL3, macro silently doesn't perform the write.
*/
.macro write_el3 sysreg xreg temp
	switch_el \temp, 402f, 402f, 401f
401:
	msr	\sysreg\()_el3, \xreg
402:
.endm

/* Macro to read from an el1 register */
.macro read_el1 xreg sysreg
	mrs	\xreg, \sysreg\()_el1
.endm

/* Macro to write to an el1 register */
.macro write_el1 sysreg xreg temp
	msr	\sysreg\()_el1, \xreg
.endm

/* Macro to read from an el0 register */
.macro read_el0 xreg sysreg
	mrs	\xreg, \sysreg\()_el0
.endm

/* Macro to write to an el0 register */
.macro write_el0 sysreg xreg temp
	msr	\sysreg\()_el0, \xreg
.endm

/* Macro to invalidate all stage 1 TLB entries for current EL */
.macro tlbiall_current temp
	switch_el \temp, 401f, 402f, 403f
401:
	tlbi	alle1
	b	404f
402:
	tlbi	alle2
	b	404f
403:
	tlbi	alle3
	b	404f
404:
.endm

#else

#include <stdint.h>

#define DAIF_DBG_BIT      (1<<3)
#define DAIF_ABT_BIT      (1<<2)
#define DAIF_IRQ_BIT      (1<<1)
#define DAIF_FIQ_BIT      (1<<0)

#define SWITCH_CASE_READ(func, var, type, el)	 do {	\
	type var = -1;					\
	switch(el) {					\
	case EL1:					\
		var = func##_el1();			\
		break;					\
	case EL2:					\
		var = func##_el2();			\
		break;					\
	case EL3:					\
		var = func##_el3();			\
		break;					\
	}						\
	return var;					\
	} while (0)

#define SWITCH_CASE_WRITE(func, var, el)	do {	\
	switch(el) {					\
	case EL1:					\
		func##_el1(var);			\
		break;					\
	case EL2:					\
		func##_el2(var);			\
		break;					\
	case EL3:					\
		func##_el3(var);			\
		break;					\
	}						\
	} while (0)

#define SWITCH_CASE_TLBI(func, el) do {		\
	switch(el) {				\
	case EL1:				\
		func##_el1();			\
			break;			\
	case EL2:				\
		func##_el2();			\
			break;			\
	case EL3:				\
		func##_el3();			\
			break;			\
	}					\
	} while (0)

/* PSTATE and special purpose register access functions */
uint32_t raw_read_current_el(void);
uint32_t get_current_el(void);
uint32_t raw_read_daif(void);
void raw_write_daif(uint32_t daif);
void enable_debug_exceptions(void);
void enable_serror_exceptions(void);
void enable_irq(void);
void enable_fiq(void);
void disable_debug_exceptions(void);
void disable_serror_exceptions(void);
void disable_irq(void);
void disable_fiq(void);
uint64_t raw_read_dlr_el0(void);
void raw_write_dlr_el0(uint64_t dlr_el0);
uint64_t raw_read_dspsr_el0(void);
void raw_write_dspsr_el0(uint64_t dspsr_el0);
uint64_t raw_read_elr_el1(void);
void raw_write_elr_el1(uint64_t elr_el1);
uint64_t raw_read_elr_el2(void);
void raw_write_elr_el2(uint64_t elr_el2);
uint64_t raw_read_elr_el3(void);
void raw_write_elr_el3(uint64_t elr_el3);
uint64_t raw_read_elr_current(void);
void raw_write_elr_current(uint64_t elr);
uint64_t raw_read_elr(uint32_t el);
void raw_write_elr(uint64_t elr, uint32_t el);
uint32_t raw_read_fpcr(void);
void raw_write_fpcr(uint32_t fpcr);
uint32_t raw_read_fpsr(void);
void raw_write_fpsr(uint32_t fpsr);
uint32_t raw_read_nzcv(void);
void raw_write_nzcv(uint32_t nzcv);
uint64_t raw_read_sp_el0(void);
void raw_write_sp_el0(uint64_t sp_el0);
uint64_t raw_read_sp_el1(void);
void raw_write_sp_el1(uint64_t sp_el1);
uint64_t raw_read_sp_el2(void);
void raw_write_sp_el2(uint64_t sp_el2);
uint32_t raw_read_spsel(void);
void raw_write_spsel(uint32_t spsel);
uint64_t raw_read_sp_el3(void);
void raw_write_sp_el3(uint64_t sp_el3);
uint64_t raw_read_sp_elx(uint32_t el);
void raw_write_sp_elx(uint64_t sp_elx, uint32_t el);
uint32_t raw_read_spsr_abt(void);
void raw_write_spsr_abt(uint32_t spsr_abt);
uint32_t raw_read_spsr_el1(void);
void raw_write_spsr_el1(uint32_t spsr_el1);
uint32_t raw_read_spsr_el2(void);
void raw_write_spsr_el2(uint32_t spsr_el2);
uint32_t raw_read_spsr_el3(void);
void raw_write_spsr_el3(uint32_t spsr_el3);
uint32_t raw_read_spsr_current(void);
void raw_write_spsr_current(uint32_t spsr);
uint32_t raw_read_spsr(uint32_t el);
void raw_write_spsr(uint32_t spsr, uint32_t el);
uint32_t raw_read_spsr_fiq(void);
void raw_write_spsr_fiq(uint32_t spsr_fiq);
uint32_t raw_read_spsr_irq(void);
void raw_write_spsr_irq(uint32_t spsr_irq);
uint32_t raw_read_spsr_und(void);
void raw_write_spsr_und(uint32_t spsr_und);

/* System control register access */
uint32_t raw_read_actlr_el1(void);
void raw_write_actlr_el1(uint32_t actlr_el1);
uint32_t raw_read_actlr_el2(void);
void raw_write_actlr_el2(uint32_t actlr_el2);
uint32_t raw_read_actlr_el3(void);
void raw_write_actlr_el3(uint32_t actlr_el3);
uint32_t raw_read_actlr_current(void);
void raw_write_actlr_current(uint32_t actlr);
uint32_t raw_read_actlr(uint32_t el);
void raw_write_actlr(uint32_t actlr, uint32_t el);
uint32_t raw_read_afsr0_el1(void);
void raw_write_afsr0_el1(uint32_t afsr0_el1);
uint32_t raw_read_afsr0_el2(void);
void raw_write_afsr0_el2(uint32_t afsr0_el2);
uint32_t raw_read_afsr0_el3(void);
void raw_write_afsr0_el3(uint32_t afsr0_el3);
uint32_t raw_read_afsr0_current(void);
void raw_write_afsr0_current(uint32_t afsr0);
uint32_t raw_read_afsr0(uint32_t el);
void raw_write_afsr0(uint32_t afsr0, uint32_t el);
uint32_t raw_read_afsr1_el1(void);
void raw_write_afsr1_el1(uint32_t afsr1_el1);
uint32_t raw_read_afsr1_el2(void);
void raw_write_afsr1_el2(uint32_t afsr1_el2);
uint32_t raw_read_afsr1_el3(void);
void raw_write_afsr1_el3(uint32_t afsr1_el3);
uint32_t raw_read_afsr1_current(void);
void raw_write_afsr1_current(uint32_t afsr1);
uint32_t raw_read_afsr1(uint32_t el);
void raw_write_afsr1(uint32_t afsr1, uint32_t el);
uint32_t raw_read_aidr_el1(void);
uint64_t raw_read_amair_el1(void);
void raw_write_amair_el1(uint64_t amair_el1);
uint64_t raw_read_amair_el2(void);
void raw_write_amair_el2(uint64_t amair_el2);
uint64_t raw_read_amair_el3(void);
void raw_write_amair_el3(uint64_t amair_el3);
uint64_t raw_read_amair_current(void);
void raw_write_amair_current(uint64_t amair);
uint64_t raw_read_amair(uint32_t el);
void raw_write_amair(uint64_t amair, uint32_t el);
uint32_t raw_read_ccsidr_el1(void);
uint32_t raw_read_clidr_el1(void);
uint32_t raw_read_cpacr_el1(void);
void raw_write_cpacr_el1(uint32_t cpacr_el1);
uint32_t raw_read_cptr_el2(void);
void raw_write_cptr_el2(uint32_t cptr_el2);
uint32_t raw_read_cptr_el3(void);
void raw_write_cptr_el3(uint32_t cptr_el3);
uint32_t raw_read_cptr_current(void);
void raw_write_cptr_current(uint32_t cptr);
uint32_t raw_read_csselr_el1(void);
void raw_write_csselr_el1(uint32_t csselr_el1);
uint32_t raw_read_ctr_el0(void);
uint32_t raw_read_esr_el1(void);
void raw_write_esr_el1(uint32_t esr_el1);
uint32_t raw_read_esr_el2(void);
void raw_write_esr_el2(uint32_t esr_el2);
uint32_t raw_read_esr_el3(void);
void raw_write_esr_el3(uint32_t esr_el3);
uint32_t raw_read_esr_current(void);
void raw_write_esr_current(uint32_t esr);
uint32_t raw_read_esr(uint32_t el);
void raw_write_esr(uint32_t esr, uint32_t el);
uint64_t raw_read_far_el1(void);
void raw_write_far_el1(uint64_t far_el1);
uint64_t raw_read_far_el2(void);
void raw_write_far_el2(uint64_t far_el2);
uint64_t raw_read_far_el3(void);
void raw_write_far_el3(uint64_t far_el3);
uint64_t raw_read_far_current(void);
void raw_write_far_current(uint64_t far);
uint64_t raw_read_far(uint32_t el);
void raw_write_far(uint64_t far, uint32_t el);
uint64_t raw_read_hcr_el2(void);
void raw_write_hcr_el2(uint64_t hcr_el2);
uint64_t raw_read_aa64pfr0_el1(void);
uint64_t raw_read_mair_el1(void);
void raw_write_mair_el1(uint64_t mair_el1);
uint64_t raw_read_mair_el2(void);
void raw_write_mair_el2(uint64_t mair_el2);
uint64_t raw_read_mair_el3(void);
void raw_write_mair_el3(uint64_t mair_el3);
uint64_t raw_read_mair_current(void);
void raw_write_mair_current(uint64_t mair);
uint64_t raw_read_mair(uint32_t el);
void raw_write_mair(uint64_t mair, uint32_t el);
uint32_t raw_read_midr_el1(void);
uint64_t raw_read_mpidr_el1(void);
uint32_t raw_read_rmr_el1(void);
void raw_write_rmr_el1(uint32_t rmr_el1);
uint32_t raw_read_rmr_el2(void);
void raw_write_rmr_el2(uint32_t rmr_el2);
uint32_t raw_read_rmr_el3(void);
void raw_write_rmr_el3(uint32_t rmr_el3);
uint32_t raw_read_rmr_current(void);
void raw_write_rmr_current(uint32_t rmr);
uint32_t raw_read_rmr(uint32_t el);
void raw_write_rmr(uint32_t rmr, uint32_t el);
uint64_t raw_read_rvbar_el1(void);
void raw_write_rvbar_el1(uint64_t rvbar_el1);
uint64_t raw_read_rvbar_el2(void);
void raw_write_rvbar_el2(uint64_t rvbar_el2);
uint64_t raw_read_rvbar_el3(void);
void raw_write_rvbar_el3(uint64_t rvbar_el3);
uint64_t raw_read_rvbar_current(void);
void raw_write_rvbar_current(uint64_t rvbar);
uint64_t raw_read_rvbar(uint32_t el);
void raw_write_rvbar(uint64_t rvbar, uint32_t el);
uint32_t raw_read_scr_el3(void);
void raw_write_scr_el3(uint32_t scr_el3);
uint32_t raw_read_sctlr_el1(void);
void raw_write_sctlr_el1(uint32_t sctlr_el1);
uint32_t raw_read_sctlr_el2(void);
void raw_write_sctlr_el2(uint32_t sctlr_el2);
uint32_t raw_read_sctlr_el3(void);
void raw_write_sctlr_el3(uint32_t sctlr_el3);
uint32_t raw_read_sctlr_current(void);
void raw_write_sctlr_current(uint32_t sctlr);
uint32_t raw_read_sctlr(uint32_t el);
void raw_write_sctlr(uint32_t sctlr, uint32_t el);
uint64_t raw_read_tcr_el1(void);
void raw_write_tcr_el1(uint64_t tcr_el1);
uint32_t raw_read_tcr_el2(void);
void raw_write_tcr_el2(uint32_t tcr_el2);
uint32_t raw_read_tcr_el3(void);
void raw_write_tcr_el3(uint32_t tcr_el3);
uint64_t raw_read_tcr_current(void);
void raw_write_tcr_current(uint64_t tcr);
uint64_t raw_read_tcr(uint32_t el);
void raw_write_tcr(uint64_t tcr, uint32_t el);
uint64_t raw_read_ttbr0_el1(void);
void raw_write_ttbr0_el1(uint64_t ttbr0_el1);
uint64_t raw_read_ttbr0_el2(void);
void raw_write_ttbr0_el2(uint64_t ttbr0_el2);
uint64_t raw_read_ttbr0_el3(void);
void raw_write_ttbr0_el3(uint64_t ttbr0_el3);
uint64_t raw_read_ttbr0_current(void);
void raw_write_ttbr0_current(uint64_t ttbr0);
uint64_t raw_read_ttbr0(uint32_t el);
void raw_write_ttbr0(uint64_t ttbr0, uint32_t el);
uint64_t raw_read_ttbr1_el1(void);
void raw_write_ttbr1_el1(uint64_t ttbr1_el1);
uint64_t raw_read_vbar_el1(void);
void raw_write_vbar_el1(uint64_t vbar_el1);
uint64_t raw_read_vbar_el2(void);
void raw_write_vbar_el2(uint64_t vbar_el2);
uint64_t raw_read_vbar_el3(void);
void raw_write_vbar_el3(uint64_t vbar_el3);
uint64_t raw_read_vbar_current(void);
void raw_write_vbar_current(uint64_t vbar);
uint64_t raw_read_vbar(uint32_t el);
void raw_write_vbar(uint64_t vbar, uint32_t el);
uint32_t raw_read_cntfrq_el0(void);
void raw_write_cntfrq_el0(uint32_t cntfrq_el0);
uint64_t raw_read_cntpct_el0(void);

/* Cache maintenance system instructions */
void dccisw(uint64_t cisw);
void dccivac(uint64_t civac);
void dccsw(uint64_t csw);
void dccvac(uint64_t cvac);
void dccvau(uint64_t cvau);
void dcisw(uint64_t isw);
void dcivac(uint64_t ivac);
void dczva(uint64_t zva);
void iciallu(void);
void icialluis(void);
void icivau(uint64_t ivau);

/* TLB maintenance instructions */
void tlbiall_el1(void);
void tlbiall_el2(void);
void tlbiall_el3(void);
void tlbiall_current(void);
void tlbiall(uint32_t el);
void tlbiallis_el1(void);
void tlbiallis_el2(void);
void tlbiallis_el3(void);
void tlbiallis_current(void);
void tlbiallis(uint32_t el);
void tlbivaa_el1(uint64_t va);

#endif // __ASSEMBLY__

#endif /* __ARCH_LIB_HELPERS_H__ */
