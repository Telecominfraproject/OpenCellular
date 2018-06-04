/*
 * This file is part of the coreboot project.
 *
 * Copyright 2015 Google Inc.
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

#ifndef _SOC_GPE_H_
#define _SOC_GPE_H_

/* GPE_31_0 */
#define GPE0_DW0_00		0
#define GPE0_DW0_01		1
#define GPE0_DW0_02		2
#define GPE0_DW0_03		3
#define GPE0_DW0_04		4
#define GPE0_DW0_05		5
#define GPE0_DW0_06		6
#define GPE0_DW0_07		7
#define GPE0_DW0_08		8
#define GPE0_DW0_09		9
#define GPE0_DW0_10		10
#define GPE0_DW0_11		11
#define GPE0_DW0_12		12
#define GPE0_DW0_13		13
#define GPE0_DW0_14		14
#define GPE0_DW0_15		15
#define GPE0_DW0_16		16
#define GPE0_DW0_17		17
#define GPE0_DW0_18		18
#define GPE0_DW0_19		19
#define GPE0_DW0_20		20
#define GPE0_DW0_21		21
#define GPE0_DW0_22		22
#define GPE0_DW0_23		23
#define GPE0_DW0_24		24
#define GPE0_DW0_25		25
#define GPE0_DW0_26		26
#define GPE0_DW0_27		27
#define GPE0_DW0_28		28
#define GPE0_DW0_29		29
#define GPE0_DW0_30		30
#define GPE0_DW0_31		31
/* GPE_63_32 */
#define GPE0_DW1_00		32
#define GPE0_DW1_01		33
#define GPE0_DW1_02		34
#define GPE0_DW1_03		36
#define GPE0_DW1_04		36
#define GPE0_DW1_05		37
#define GPE0_DW1_06		38
#define GPE0_DW1_07		39
#define GPE0_DW1_08		40
#define GPE0_DW1_09		41
#define GPE0_DW1_10		42
#define GPE0_DW1_11		43
#define GPE0_DW1_12		44
#define GPE0_DW1_13		45
#define GPE0_DW1_14		46
#define GPE0_DW1_15		47
#define GPE0_DW1_16		48
#define GPE0_DW1_17		49
#define GPE0_DW1_18		50
#define GPE0_DW1_19		51
#define GPE0_DW1_20		52
#define GPE0_DW1_21		53
#define GPE0_DW1_22		54
#define GPE0_DW1_23		55
#define GPE0_DW1_24		56
#define GPE0_DW1_25		57
#define GPE0_DW1_26		58
#define GPE0_DW1_27		59
#define GPE0_DW1_28		60
#define GPE0_DW1_29		61
#define GPE0_DW1_30		62
#define GPE0_DW1_31		63
/* GPE_95_64 */
#define GPE0_DW2_00		64
#define GPE0_DW2_01		65
#define GPE0_DW2_02		66
#define GPE0_DW2_03		67
#define GPE0_DW2_04		68
#define GPE0_DW2_05		69
#define GPE0_DW2_06		70
#define GPE0_DW2_07		71
#define GPE0_DW2_08		72
#define GPE0_DW2_09		73
#define GPE0_DW2_10		74
#define GPE0_DW2_11		75
#define GPE0_DW2_12		76
#define GPE0_DW2_13		77
#define GPE0_DW2_14		78
#define GPE0_DW2_15		79
#define GPE0_DW2_16		80
#define GPE0_DW2_17		81
#define GPE0_DW2_18		82
#define GPE0_DW2_19		83
#define GPE0_DW2_20		84
#define GPE0_DW2_21		85
#define GPE0_DW2_22		86
#define GPE0_DW2_23		87
#define GPE0_DW2_24		88
#define GPE0_DW2_25		89
#define GPE0_DW2_26		90
#define GPE0_DW2_27		91
#define GPE0_DW2_28		92
#define GPE0_DW2_29		93
#define GPE0_DW2_30		94
#define GPE0_DW2_31		95
/* GPE_STD */
#define GPE0_HOT_PLUG		97
#define GPE0_SWGPE		98
#define GPE0_TCOSCI		102
#define GPE0_SMB_WAK		103
#define GPE0_PCI_EXP		105
#define GPE0_BATLOW		106
#define GPE0_PME		107
#define GPE0_ME_SCI		108
#define GPE0_PME_B0		109
#define GPE0_ESPI		110
#define GPE0_GPIO_T2		111
#define GPE0_LAN_WAK		112
#define GPE0_WADT		114

#define GPE_MAX			GPE0_WADT

#endif /* _SOC_GPE_H_ */
