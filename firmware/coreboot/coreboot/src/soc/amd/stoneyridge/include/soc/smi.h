/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Advanced Micro Devices, Inc.
 * Copyright (C) 2014 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __SOUTHBRIDGE_AMD_PI_STONEYRIDGE_SMI_H__
#define __SOUTHBRIDGE_AMD_PI_STONEYRIDGE_SMI_H__

#include <arch/io.h>

#define SMI_GEVENTS			24
#define SCIMAPS				58
#define SCI_GPES			32

#define SMI_EVENT_STATUS		0x0
#define SMI_EVENT_ENABLE		0x04
#define SMI_SCI_TRIG			0x08
#define SMI_SCI_LEVEL			0x0c
#define SMI_SCI_STATUS			0x10
#define SMI_SCI_EN			0x14
#define SMI_SCI_MAP0			0x40
# define SMI_SCI_MAP(X)			(SMI_SCI_MAP0 + (X))

/* SMI source and status */
#define SMITYPE_AGPIO65				0
#define SMITYPE_AGPIO66				1
#define SMITYPE_AGPIO3				2
#define SMITYPE_LPCPME_AGPIO22			3
#define SMITYPE_GPIO4				4
#define SMITYPE_LPCPD_AGPIOG21			5
#define SMITYPE_IRTX1_G15			6
#define SMITYPE_AGPIO5_DEVSLP0			7
#define SMITYPE_WAKE_AGPIO2			8
#define SMITYPE_APIO68_SGPIOCLK			9
#define SMITYPE_AGPIO6				10
#define SMITYPE_GPIO7				11
#define SMITYPE_USBOC0_TRST_AGPIO16		12
#define SMITYPE_USB0C1_TDI_AGPIO17		13
#define SMITYPE_USBOC2_TCK_AGPIO18		14
#define SMITYPE_TDO_USB0C3_AGPIO24		15
#define SMITYPE_ACPRES_USBOC4_IRRX0_AGPIO23	16
/* 17 Reserved */
#define SMITYPE_BLINK_AGPIO11_USBOC7		18
#define SMITYPE_SYSRESET_AGPIO1			19
#define SMITYPE_IRRX1_AGPIO15			20
#define SMITYPE_IRTX0_USBOC5_AGPIO13		21
#define SMITYPE_GPIO9_SERPORTRX			22
#define SMITYPE_GPIO8_SEPORTTX			23
#define GEVENT_MASK ((1 << SMITYPE_AGPIO65)				\
			| (1 << SMITYPE_AGPIO66)			\
			| (1 << SMITYPE_AGPIO3)				\
			| (1 << SMITYPE_LPCPME_AGPIO22)			\
			| (1 << SMITYPE_GPIO4)				\
			| (1 << SMITYPE_LPCPD_AGPIOG21)			\
			| (1 << SMITYPE_IRTX1_G15)			\
			| (1 << SMITYPE_AGPIO5_DEVSLP0)			\
			| (1 << SMITYPE_WAKE_AGPIO2)			\
			| (1 << SMITYPE_APIO68_SGPIOCLK)		\
			| (1 << SMITYPE_AGPIO6)				\
			| (1 << SMITYPE_GPIO7)				\
			| (1 << SMITYPE_USBOC0_TRST_AGPIO16)		\
			| (1 << SMITYPE_USB0C1_TDI_AGPIO17)		\
			| (1 << SMITYPE_USBOC2_TCK_AGPIO18)		\
			| (1 << SMITYPE_TDO_USB0C3_AGPIO24)		\
			| (1 << SMITYPE_ACPRES_USBOC4_IRRX0_AGPIO23)	\
			| (1 << SMITYPE_BLINK_AGPIO11_USBOC7)		\
			| (1 << SMITYPE_SYSRESET_AGPIO1)		\
			| (1 << SMITYPE_IRRX1_AGPIO15)			\
			| (1 << SMITYPE_IRTX0_USBOC5_AGPIO13)		\
			| (1 << SMITYPE_GPIO9_SERPORTRX))
#define SMITYPE_EHCI0_WAKE			24
#define SMITYPE_EHCI1_WAKE			25
#define SMITYPE_ESPI_SYS			26
#define SMITYPE_ESPI_WAKE_PME			27
/* 28-32 Reserved */
#define SMITYPE_FCH_FAKE0			33
#define SMITYPE_FCH_FAKE1			34
#define SMITYPE_FCH_FAKE2			35
/* 36 Reserved */
#define SMITYPE_SATA_GEVENT0			37
#define SMITYPE_SATA_GEVENT1			38
#define SMITYPE_ACP_WAKE			39
#define SMITYPE_ECG				40
#define SMITYPE_GPIO_CTL			41
#define SMITYPE_CIR_PME				42
#define SMITYPE_ALT_HPET_ALARM			43
#define SMITYPE_FAN_THERMAL			44
#define SMITYPE_ASF_MASTER_SLAVE		45
#define SMITYPE_I2S_WAKE			46
#define SMITYPE_SMBUS0_MASTER			47
#define SMITYPE_TWARN				48
#define SMITYPE_TRAFFIC_MON			49
#define SMITYPE_ILLB				50
#define SMITYPE_PWRBUTTON_UP			51
#define SMITYPE_PROCHOT				52
#define SMITYPE_APU_HW				53
#define SMITYPE_NB_SCI				54
#define SMITYPE_RAS_SERR			55
#define SMITYPE_XHC0_PME			56
/* 57 Reserved */
#define SMITYPE_ACDC_TIMER			58
/* 59-62 Reserved */
#define SMITYPE_TEMP_TSI			63
#define SMITYPE_KB_RESET			64
#define SMITYPE_SLP_TYP				65
#define SMITYPE_AL2H_ACPI			66
#define SMITYPE_AHCI				67
/* 68-71 Reserved */
#define SMITYPE_GBL_RLS				72
#define SMITYPE_BIOS_RLS			73
#define SMITYPE_PWRBUTTON_DOWN			74
#define SMITYPE_SMI_CMD_PORT			75
#define SMITYPE_USB_SMI				76
#define SMITYPE_SERIRQ				77
#define SMITYPE_SMBUS0_INTR			78
#define SMITYPE_IMC				79
#define SMITYPE_XHC_ERROR			80
#define SMITYPE_INTRUDER			81
#define SMITYPE_VBAT_LOW			82
#define SMITYPE_PROTHOT				83
#define SMITYPE_PCI_SERR			84
#define SMITYPE_GPP_SERR			85
/* 85-88 Reserved */
#define SMITYPE_TMERTRIP			89
#define SMITYPE_EMUL60_64			90
#define SMITYPE_USB_FLR				91
#define SMITYPE_SATA_FLR			92
#define SMITYPE_AZ_FLR				93
/* 94-132 Reserved */
#define SMITYPE_FANIN0				133
/* 134-137 Reserved */
#define SMITYPE_FAKE0				138
#define SMITYPE_FAKE1				139
#define SMITYPE_FAKE2				140
/* 141 Reserved */
#define SMITYPE_SHORT_TIMER			142
#define SMITYPE_LONG_TIMER			143
#define SMITYPE_AB_SMI				144
#define SMITYPE_SOFT_RESET			145
/* 146-147 Reserved */
#define SMITYPE_IOTRAP0				148
/* 149-151 Reserved */
#define SMITYPE_MEMTRAP0			152
/* 153-155 Reserved */
#define SMITYPE_CFGTRAP0			156
/* 157-159 Reserved */
#define NUMBER_SMITYPES				160
#define TYPE_TO_MASK(X)				(1 << (X) % 32)

#define SMI_REG_SMISTS0			0x80
#define SMI_REG_SMISTS1			0x84
#define SMI_REG_SMISTS2			0x88
#define SMI_REG_SMISTS3			0x8c
#define SMI_REG_SMISTS4			0x90

#define SMI_REG_POINTER			0x94
# define SMI_STATUS_SRC_SCI			(1 << 0)
# define SMI_STATUS_SRC_0			(1 << 1) /* SMIx80 */
# define SMI_STATUS_SRC_1			(1 << 2) /* SMIx84... */
# define SMI_STATUS_SRC_2			(1 << 3)
# define SMI_STATUS_SRC_3			(1 << 4)
# define SMI_STATUS_SRC_4			(1 << 5)

#define SMI_TIMER			0x96
#define SMI_TIMER_MASK				0x7fff
#define SMI_TIMER_EN				(1 << 15)

#define SMI_REG_SMITRIG0		0x98
# define SMITRG0_EOS				(1 << 28)
# define SMI_TIMER_SEL				(1 << 29)
# define SMITRG0_SMIENB				(1 << 31)

#define SMI_REG_CONTROL0		0xa0
#define SMI_REG_CONTROL1		0xa4
#define SMI_REG_CONTROL2		0xa8
#define SMI_REG_CONTROL3		0xac
#define SMI_REG_CONTROL4		0xb0
#define SMI_REG_CONTROL5		0xb4
#define SMI_REG_CONTROL6		0xb8
#define SMI_REG_CONTROL7		0xbc
#define SMI_REG_CONTROL8		0xc0
#define SMI_REG_CONTROL9		0xc4

enum smi_mode {
	SMI_MODE_DISABLE = 0,
	SMI_MODE_SMI = 1,
	SMI_MODE_NMI = 2,
	SMI_MODE_IRQ13 = 3,
};

enum smi_sci_type {
	INTERRUPT_NONE,
	INTERRUPT_SCI,
	INTERRUPT_SMI,
	INTERRUPT_BOTH,
};

enum smi_sci_lvl {
	SMI_SCI_LVL_LOW,
	SMI_SCI_LVL_HIGH,
};

enum smi_sci_dir {
	SMI_SCI_EDG,
	SMI_SCI_LVL,
};

struct smi_sources_t {
	int type;
	void (*handler)(void);
};

struct sci_source {
	uint8_t scimap;		/* SCIMAP 0-57 */
	uint8_t gpe;		/* 32 GPEs */
	uint8_t direction;	/* Active High or Low,  smi_sci_lvl */
	uint8_t level;		/* Edge or Level,  smi_sci_dir */
};

uint16_t pm_acpi_smi_cmd_port(void);
void configure_smi(uint8_t smi_num, uint8_t mode);
void configure_gevent_smi(uint8_t gevent, uint8_t mode, uint8_t level);
void configure_scimap(const struct sci_source *sci);
void disable_gevent_smi(uint8_t gevent);
void gpe_configure_sci(const struct sci_source *scis, size_t num_gpes);

#ifndef __SMM__
void enable_smi_generation(void);
#endif

#endif /* __SOUTHBRIDGE_AMD_PI_STONEYRIDGE_SMI_H__ */
