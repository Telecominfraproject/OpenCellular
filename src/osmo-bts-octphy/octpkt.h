#pragma once
#include <stdint.h>

/* push a common header (1 dword) to the start of a msgb */
void octpkt_push_common_hdr(struct msgb *msg, uint8_t format,
			    uint8_t trace, uint32_t ptype);

/* common msg_header shared by all control messages */
void octvc1_fill_msg_hdr(tOCTVC1_MSG_HEADER *mh, uint32_t len,
			uint32_t sess_id, uint32_t trans_id,
			uint32_t user_info, uint32_t msg_type,
			uint32_t flags, uint32_t api_cmd);

/* push a control header (3 dwords) to the start of a msgb. This format
 * is used for command and response packets */
void octvocnet_push_ctl_hdr(struct msgb *msg, uint32_t dest_fifo_id,
			uint32_t src_fifo_id, uint32_t socket_id);

int osmo_sock_packet_init(uint16_t type, uint16_t proto, const char *bind_dev,
			  unsigned int flags);

int tx_trx_open(struct gsm_bts_trx *trx);
