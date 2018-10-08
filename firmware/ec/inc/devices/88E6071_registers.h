/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _88E6071_REGISTERS_H_
#define _88E6071_REGISTERS_H_

/* SMI Device IDs */
#define PHY_PORT_0 0
#define PHY_PORT_1 1
#define PHY_PORT_2 2
#define PHY_PORT_3 3
#define PHY_PORT_4 4
#define GLOBAL_2 7
#define SW_PORT_0 8
#define SW_PORT_1 9
#define SW_PORT_2 10
#define SW_PORT_3 11
#define SW_PORT_4 12
#define SW_PORT_5 13
#define SW_PORT_6 14
#define GLOBAL_1 15

/* PHY Specific Register set */
#define REG_PHY_CONTROL 0x0
#define REG_PHY_STATUS 0x1
#define REG_PHY_ID_1 0x2
#define REG_PHY_ID_2 0x3
#define REG_AUTONEG_ADV 0x4
#define REG_LINK_PARTNER_ABILITY 0x5
#define REG_AUTO_NEG_EXPANSION 0x6
#define REG_NXT_PAGE_TRANSMIT 0x7
#define REG_LINK_PARTNER_NXT_PAGE 0x8
#define REG_MMD_ACCESS_CNTRL 0xD
#define REG_MMD_ADDR_DATA 0xC
#define REG_PHY_SPEC_CONTROL 0x10
#define REG_PHY_SPEC_STATUS 0x11
#define REG_PHY_INTERRUPT_EN 0x12
#define REG_PHY_INTERRUPT_STATUS 0x13
#define REG_RCV_ERR_COUNTER 0x15

/* GLOBAL - 1 */
#define REG_GLOBAL_STATUS 0x0
#define REG_GLOBAL_CONTROL 0x4
#define REG_VTU_CONTROL 0x5
#define REG_VTU_VID 0x6
#define REG_VTU_DATA_PORT_3_0 0x7
#define REG_VTU_DATA_PORT_6_4 0x8
#define REG_ATU_CONTROL 0xA
#define REG_ATU_OPERATION 0xB
#define REG_ATU_DATA 0xC

/* GLOBAL - 2 */
#define REG_INTERRUPT_SOURCE 0x0
#define REG_INTERRUPT_MASK 0x1
#define REG_MGMT_EN_2X 0x2
#define REG_MGMT_EN_0X 0x3
#define REG_MANAGEMENT 0x5
#define REG_TRUNK_MASK 0x7
#define REG_INGRESS_RATE_CMD 0x9
#define REG_INGRESS_RATE_DATA 0xA
#define REG_SWITCH_MAC 0xD
#define REG_ATU_STATS 0xE
#define REG_PRIORITY_OVERRIDES 0xF
#define REG_EEPROM_CMD 0x14
#define REG_EEPROM_DATA 0x15
#define REG_AVB_CMD 0x16
#define REG_AVB_DATA 0x17
#define REG_SMI_CMD 0x18
#define REG_SMI_DATA 0x19
#define REG_SCRATCH_MISC 0x1A
#define REG_WATCHDOG 0x1B

/* Switch Ports registers */
#define REG_PORT_STATUS 0x0
#define REG_MAC_CONTROL 0x1
#define REG_JAMING_CONTROL 0x2
#define REG_SW_IDENTIFIER 0x3
#define REG_PORT_CONTROL 0x4
#define REG_PORT_CONTROL_1 0x5
#define REG_VLAN_MAP 0x6
#define REG_VLAN_ID_PRIORITY 0x7
#define REG_PORT_ID_2 0x8
#define REG_EGRESS_RATE_CONTROL 0x9
#define REG_EGRESS_RATE_CONTROL_2 0xA
#define REG_PORT_ASSOCITATION_VECTOR 0xB
#define REG_PRIORITY_OVERRIDE 0xD
#define REG_POLICY_CONTROL 0xE
#define REG_PORT_ETYPE 0xF
#define REG_RX_FRAME_COUNTER 0x10
#define REG_TX_FRAME_COUNTER 0x11
#define REG_INDISCARD_COUNTER 0x12
#define REG_INFILTERED_COUNTER 0x13
#define REG_LED_CONTROL 0x16
#define REG_TAG_REMAP_3_0 0x18
#define REG_TAG_REMAP_7_4 0x19
#define REG_QUEUE_COUNTER 0x1B

#define REG_C45_PACKET_GEN 0x8030
#define REG_C45_CRC_ERROR_COUNTER 0x8031

/*
 * PHY Register fields SMI Device address 0x0 to 0x4
 */
//REG_PHY_CONTROL - 0x0
#define SOFT_RESET (1 << 0xF)
#define LOOPBACK_EN (1 << 0xE)
#define SPEED (1 << 0xD)
#define AUTONEG_EN (1 << 0xC)
#define PWR_DOWN (1 << 0xB)
#define RESTART_AUTONEG (1 << 0xA)
#define DUPLEX (1 << 0x8)

//REG_PHY_STATUS - 0x1
#define AUTONEG_DONE (1 << 0x5)
#define LINK_UP (1 << 0x2)

//REG_MMD_ACCESS_CNTRL - 0xD
#define DEVADDR (0x1F << 0x0)
#define FUNCTION (0x03 << 0xD)

//REG_PHY_SPEC_CONTROL - 0x10
#define ENERGY_DET (1 << 0xE)
#define DIS_NLP_CHECK (1 << 0xD)
#define EXT_DISTANCE (1 << 0x7)
#define SIGDET_POL (1 << 0x6)
#define AUTOMDI_CROSSOVER (0x03 << 0x4)
#define AUTOPOL_REVERSE (1 << 0x1)

//REG_PHY_SPEC_STATUS - 0x11
#define RES_SPEED (1 << 0xE)
#define RES_DUPLEX (1 << 0xD)
#define RT_LINK (1 << 0xA)
#define MDI_CROSSOVER_STATUS (1 << 0x6)
#define SLEEP_MODE (1 << 0x4)
#define POLARITY (1 << 0x1)
#define JABBER_DET (1 << 0x0)

//REG_PHY_INTERRUPT_EN - 0x12
#define SPEED_INT_EN (1 << 0xE)
#define DUPLEX_INT_EN (1 << 0xD)
#define PAGE_RX_INT_STATUS_EN (1 << 0xC)
#define AUTONEG_COMPLETE_INT_EN (1 << 0xB)
#define LINK_CHANGE_INT_EN (1 << 0xA)
#define MDI_CROSSOVER_INT_EN (1 << 0x6)
#define ENERGY_DET_INT_EN (1 << 0x4)
#define POLARITY_INT_EN (1 << 0x1)
#define JABBER_INT_EN (1 << 0x0)

//REG_PHY_INTERRUPT_STATUS - 0x13
#define SPEED_INT_STATUS (1 << 0xE)
#define DUPLEX_INT_STATUS (1 << 0xD)
#define PAGE_RX_INT_STATUS (1 << 0xC)
#define AUTONEG_COMPLETE_INT_STATUS (1 << 0xB)
#define LINK_CHANGE_INT_STATUS (1 << 0xA)
#define MDI_CROSSOVER_INT_STATUS (1 << 0x6)
#define ENERGY_DET_INT_STATUS (1 << 0x4)
#define POLARITY_INT_STATUS (1 << 0x1)
#define JABBER_INT_STATUS (1 << 0x0)

/*
 * GLOBAL -1 Register fields (SMI Device address 0xF)
 */
// REG_GLOBAL_STATUS    0x1
#define INIT_RDY (1 << 11)
#define AVB_INT (1 << 8)
#define DEV_INT (1 << 7)
#define STATS_DONE (1 << 6)
#define VLAN_PROB (1 << 5)
#define VLAN_DONE (1 << 4)
#define ATU_PROB (1 << 3)
#define ATU_DONE (1 << 2)
#define EE_INT (1 << 0)

// REG_GLOBAL_CONTROL   0x4
#define SW_RESET (1 << 15)
#define DISCARD_EXCESSIVE (1 << 13)
#define ARP_WO_BROADCAST (1 << 12)
#define MAX_FRAME_SIZE (1 << 10)
#define RELOAD (1 << 9)
#define AVB_INT_EN (1 << 8)
#define DEV_INT_EN (1 << 7)
#define STATS_DONE_INT_EN (1 << 6)
#define VTU_PROB_INT_EN (1 << 5)
#define ATU_DONE_INT_EN (1 << 4)
#define EE_INT_EN (1 << 0)

/*
 * GLOBAL - 2 Register fields (SMI Device address 0x7)
 */
//REG_INTERRUPT_SOURCE      0x0
#define WATCHDOG_INT (1 << 15)
#define JAM_INT (1 << 14)
#define WAKE_EVENT_INT (1 << 12)
#define PHY_4_INT (1 << 4)
#define PHY_3_INT (1 << 3)
#define PHY_2_INT (1 << 2)
#define PHY_1_INT (1 << 1)
#define PHY_0_INT (1 << 0)

//REG_INTERRUPT_MASK        0x1
#define WATCHDOG_INT_EN (1 << 15)
#define JAM_INT_EN (1 << 14)
#define WAKE_EVENT_INT_EN (1 << 12)
#define PHY_4_INT_EN (1 << 4)
#define PHY_3_INT_EN (1 << 3)
#define PHY_2_INT_EN (1 << 2)
#define PHY_1_INT_EN (1 << 1)
#define PHY_0_INT_EN (1 << 0)

//REG_C45_PACKET_GEN        0x8030
#define CRC_ENABLE (1 << 6)
#define FRAME_COUNT_EN (1 << 5)
#define FORCE_BURST_STOP (1 << 4)
#define PACKET_GEN_EN (1 << 3)
#define PAYLOAD_TYPE (1 << 2)
#define PACKET_LENGTH (1 << 1)
#define ERROR_PACKET_INJECTION (1 << 0)
#endif /* _88E6071_REGISTERS_H_ */
