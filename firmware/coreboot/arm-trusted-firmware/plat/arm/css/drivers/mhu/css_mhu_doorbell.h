/*
 * Copyright (c) 2014-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CSS_MHU_DOORBELL_H
#define CSS_MHU_DOORBELL_H

#include <mmio.h>
#include <stdint.h>

/* MHUv2 Base Address */
#define MHUV2_BASE_ADDR		PLAT_CSS_MHU_BASE

/* MHUv2 Control Registers Offsets */
#define MHU_V2_MSG_NO_CAP_OFFSET		0xF80
#define MHU_V2_ACCESS_REQ_OFFSET		0xF88
#define MHU_V2_ACCESS_READY_OFFSET		0xF8C

#define SENDER_REG_STAT(CHANNEL)	(0x20 * (CHANNEL))
#define SENDER_REG_SET(CHANNEL)		(0x20 * (CHANNEL)) + 0xC

/* Helper macro to ring doorbell */
#define MHU_RING_DOORBELL(addr, modify_mask, preserve_mask)	do {	\
		uint32_t db = mmio_read_32(addr) & (preserve_mask);	\
		mmio_write_32(addr, db | (modify_mask));		\
	} while (0)

#define MHU_V2_ACCESS_REQUEST(addr)	\
	mmio_write_32((addr) + MHU_V2_ACCESS_REQ_OFFSET, 0x1)

#define MHU_V2_CLEAR_REQUEST(addr)	\
	mmio_write_32((addr) + MHU_V2_ACCESS_REQ_OFFSET, 0x0)

#define MHU_V2_IS_ACCESS_READY(addr)	\
	(mmio_read_32((addr) + MHU_V2_ACCESS_READY_OFFSET) & 0x1)

struct scmi_channel_plat_info;
void mhu_ring_doorbell(struct scmi_channel_plat_info *plat_info);
void mhuv2_ring_doorbell(struct scmi_channel_plat_info *plat_info);

#endif	/* CSS_MHU_DOORBELL_H */
