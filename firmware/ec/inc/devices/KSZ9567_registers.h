/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef INC_DEVICES_KSZ9567_REGISTERS_H_
#define INC_DEVICES_KSZ9567_REGISTERS_H_

//GLOBAL REG ADDRESS
#define	GLOBAL_PORT_INT_STATUS_REG			0x0018
#define	GLOBAL_PORT_INT_MASK_REG			0x001C
#define	PORT_INT_CONTROL_REG				0x0136
#define	PORT_STATUS_REG						0x0030
#define	PHY_BASIC_CONTROL_REG				0x0100
#define	PHY_BASIC_STATUS_REG				0x0102
#define	PHY_AUTO_NEG_LINK_PART_ABI_REG		0x010A
#define	PHY_AUTO_NEG_EXP_STATUS_REG			0x010c

//PORT INTRRUPT STATUS BIT
#define	LINK_UP_INT_STATUS					(1 << 0x0)
#define	REMOTE_FAULT_INT_STATUS				(1 << 0x1)
#define	LINK_DOWN_INT_STATUS				(1 << 0x2)
#define	PR_DET_FAULT_INT_STATUS				(1 << 0x4) 		/*Parallel Detect Fault Interrupt*/
#define	RECEIVE_ERROR_INT_STATUS			(1 << 0x6)
#define	JABBER_INT_STATUS           		(1 << 0x7)

//REG BIT MASK
#define	PORT_INT_EN							0x0000
#define	SPEED_STATUS						0x18
#define	DUPLEX_STATUS						0x04
#define	AUTO_NEGO_STATUS 					0x1000
#define	PWR_DWN_STATUS						0x0800
#define	AUTO_NEGO_COM_STATUS				0x0020
#define	LINK_UP_STATUS						0x0004
#define	PWR_DWN_SET							0x800
#define	RESART_AUTO_NEGO					0x200
#define	PORT_SPEED_SET						0x2040
#define	PORT_DUPLEX_SET						0x0100
#define	PORT_PWR_DWN_SET					0x0800
#define	PORT_REM_FAULT_STATUS				0x2000
#define	PORT_JABBER_STATUS					0x0002
#define	PORT_PARA_FALT_STATUS				0x0010

#endif /* INC_DEVICES_KSZ9567_REGISTERS_H_ */
