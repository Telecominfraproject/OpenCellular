/* Utility routines for dealing with OCTPKT/OCTVC1 in msgb */

/* Copyright (c) 2015 Harald Welte <laforge@gnumonks.org>
 *
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <errno.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <osmocom/core/select.h>
#include <osmocom/core/msgb.h>
#include <osmocom/core/socket.h>

#include <osmo-bts/gsm_data.h>

#include <octphy/octpkt/octpkt_hdr.h>
#include <octphy/octpkt/octpkt_hdr_swap.h>
#include <octphy/octvc1/octvocnet_pkt.h>
#include <octphy/octvc1/octvc1_msg.h>
#include <octphy/octvc1/octvc1_msg_swap.h>
#include <octphy/octvc1/gsm/octvc1_gsm_api.h>

#include "l1_if.h"
#include "octpkt.h"

/* push a common header (1 dword) to the start of a msgb */
void octpkt_push_common_hdr(struct msgb *msg, uint8_t format,
			    uint8_t trace, uint32_t ptype)
{
	uint32_t ch;
	uint32_t *chptr;
	uint32_t tot_len = msgb_length(msg) + sizeof(ch);

	ch = ((format & cOCTPKT_HDR_FORMAT_PROTO_TYPE_LEN_MASK_FORMAT_BIT_MASK)
		<< cOCTPKT_HDR_FORMAT_PROTO_TYPE_LEN_MASK_FORMAT_BIT_OFFSET) |
	     ((trace & cOCTPKT_HDR_FORMAT_PROTO_TYPE_LEN_MASK_TRACE_BIT_MASK)
	      	<< cOCTPKT_HDR_FORMAT_PROTO_TYPE_LEN_MASK_TRACE_BIT_OFFSET) |
	     ((ptype & cOCTPKT_HDR_FORMAT_PROTO_TYPE_LEN_MASK_CONTROL_PROTOCOL_TYPE_BIT_MASK)
	        << cOCTPKT_HDR_FORMAT_PROTO_TYPE_LEN_MASK_CONTROL_PROTOCOL_TYPE_BIT_OFFSET) |
	     (tot_len & cOCTPKT_HDR_FORMAT_PROTO_TYPE_LEN_MASK_LENGTH_BIT_MASK);

	chptr = (uint32_t *) msgb_push(msg, sizeof(ch));
	*chptr = htonl(ch);
}

/* push a control header (3 dwords) to the start of a msgb. This format
 * is used for command and response packets */
void octvocnet_push_ctl_hdr(struct msgb *msg, uint32_t dest_fifo_id,
			uint32_t src_fifo_id, uint32_t socket_id)
{
	tOCTVOCNET_PKT_CTL_HEADER *ch;

	ch = (tOCTVOCNET_PKT_CTL_HEADER *) msgb_push(msg, sizeof(*ch));

	ch->ulDestFifoId = htonl(dest_fifo_id);
	ch->ulSourceFifoId = htonl(src_fifo_id);
	ch->ulSocketId = htonl(socket_id);
}

/* common msg_header shared by all control messages. host byte order! */
void octvc1_fill_msg_hdr(tOCTVC1_MSG_HEADER *mh, uint32_t len,
			uint32_t sess_id, uint32_t trans_id,
			uint32_t user_info, uint32_t msg_type,
			uint32_t flags, uint32_t api_cmd)
{
	uint32_t type_r_cmdid;
	type_r_cmdid = ((msg_type & cOCTVC1_MSG_TYPE_BIT_MASK)
				<< cOCTVC1_MSG_TYPE_BIT_OFFSET) |
		       ((api_cmd & cOCTVC1_MSG_ID_BIT_MASK)
				<< cOCTVC1_MSG_ID_BIT_OFFSET);
	/* Resync?  Flags? */

	mh->ulLength = len;
	mh->ulTransactionId = trans_id;
	mh->ul_Type_R_CmdId = type_r_cmdid;
	mh->ulSessionId = sess_id;
	mh->ulReturnCode = 0;
	mh->ulUserInfo = user_info;
}

#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

/*! \brief Initialize a packet socket
 *  \param[in] tye Socket type like SOCK_RAW or SOCK_DGRAM
 *  \param[in] proto The link-layer protocol in network byte order
 *  \param[in] bind_dev The name of the interface to bind to (if any)
 *  \param[in] flags flags like \ref OSMO_SOCK_F_BIND
 *
 * This function creates a new packet socket of \a type and \a proto
 * and optionally bnds to it, if stated in the \a flags parameter.
 */
int osmo_sock_packet_init(uint16_t type, uint16_t proto, const char *bind_dev,
			  unsigned int flags)
{
	int sfd, rc, on = 1;

	if (flags & OSMO_SOCK_F_CONNECT)
		return -EINVAL;

	sfd = socket(AF_PACKET, type, proto);
	if (sfd < 0)
		return -1;

	if (flags & OSMO_SOCK_F_NONBLOCK) {
		if (ioctl(sfd, FIONBIO, (unsigned char *)&on) < 0) {
			perror("cannot set this socket unblocking");
			close(sfd);
			return -EINVAL;
		}
	}

	if (bind_dev) {
		struct sockaddr_ll sa;
		struct ifreq ifr;

		/* resolve the string device name to an ifindex */
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, bind_dev, sizeof(ifr.ifr_name));
		rc = ioctl(sfd, SIOCGIFINDEX, &ifr);
		if (rc < 0)
			goto err;

		memset(&sa, 0, sizeof(sa));
		sa.sll_family = AF_PACKET;
		sa.sll_protocol = htons(proto);
		sa.sll_ifindex = ifr.ifr_ifindex;
		/* according to the packet(7) man page, bind() will only
		 * use sll_protocol nad sll_ifindex */
		rc = bind(sfd, (struct sockaddr *)&sa, sizeof(sa));
		if (rc < 0)
			goto err;
	}

	return sfd;
err:
	close(sfd);
	return -1;
}
