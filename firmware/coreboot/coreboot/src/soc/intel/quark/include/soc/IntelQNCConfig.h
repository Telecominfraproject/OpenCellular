/** @file
Some configuration of QNC Package

Copyright (c) 2013-2017 Intel Corporation.

This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License.  The full text of the license
may be found at http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __INTEL_QNC_CONFIG_H__
#define __INTEL_QNC_CONFIG_H__

//
// QNC Fixed configurations.
//

//
// Memory arbiter fixed config values.
//
#define QNC_FIXED_CONFIG_ASTATUS  ((UINT32) (\
	(ASTATUS_PRI_NORMAL << ASTATUS0_DEFAULT_BP) | \
	(ASTATUS_PRI_NORMAL << ASTATUS1_DEFAULT_BP) | \
	(ASTATUS_PRI_URGENT << ASTATUS0_RASISED_BP) | \
	(ASTATUS_PRI_URGENT << ASTATUS1_RASISED_BP) \
	))

//
// Memory Manager fixed config values.
//
#define V_DRAM_NON_HOST_RQ_LIMIT                    2

//
// RMU Thermal config fixed config values for TS in Vref Mode.
//
#define V_TSCGF1_CONFIG_ISNSCURRENTSEL_VREF_MODE    0x04
#define V_TSCGF2_CONFIG2_ISPARECTRL_VREF_MODE       0x01
#define V_TSCGF1_CONFIG_IBGEN_VREF_MODE             1
#define V_TSCGF2_CONFIG_IDSCONTROL_VREF_MODE        0x011b
#define V_TSCGF2_CONFIG2_ICALCOARSETUNE_VREF_MODE   0x34

//
// RMU Thermal config fixed config values for TS in Ratiometric mode.
//
#define V_TSCGF1_CONFIG_ISNSCURRENTSEL_RATIO_MODE   0x04
#define V_TSCGF1_CONFIG_ISNSCHOPSEL_RATIO_MODE      0x02
#define V_TSCGF1_CONFIG_ISNSINTERNALVREFEN_RATIO_MODE 1
#define V_TSCGF2_CONFIG_IDSCONTROL_RATIO_MODE       0x011f
#define V_TSCGF2_CONFIG_IDSTIMING_RATIO_MODE        0x0001
#define V_TSCGF2_CONFIG2_ICALCONFIGSEL_RATIO_MODE   0x01
#define V_TSCGF2_CONFIG2_ISPARECTRL_RATIO_MODE      0x00
#define V_TSCGF1_CONFIG_IBGEN_RATIO_MODE            0
#define V_TSCGF1_CONFIG_IBGCHOPEN_RATIO_MODE        0
#define V_TSCGF3_CONFIG_ITSGAMMACOEFF_RATIO_MODE    0xC8
#define V_TSCGF2_CONFIG2_ICALCOARSETUNE_RATIO_MODE  0x17

//
// iCLK fixed config values.
//
#define V_MUXTOP_FLEX2                              3
#define V_MUXTOP_FLEX1                              1

//
// PCIe Root Port fixed config values.
//
#define V_PCIE_ROOT_PORT_SBIC_VALUE      (B_QNC_PCIE_IOSFSBCTL_SBIC_IDLE_NEVER)

#endif
