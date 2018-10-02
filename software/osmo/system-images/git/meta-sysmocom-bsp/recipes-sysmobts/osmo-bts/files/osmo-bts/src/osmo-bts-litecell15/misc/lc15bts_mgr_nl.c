/* NetworkListen for NuRAN Litecell 1.5 BTS management daemon */

/* Copyright (C) 2015 by Yves Godin <support@nuranwireless.com>
 * 
 * Based on sysmoBTS:
 *     sysmobts_mgr_nl.c
 *     (C) 2014 by Holger Hans Peter Freyther
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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "misc/lc15bts_mgr.h"
#include "misc/lc15bts_misc.h"
#include "misc/lc15bts_nl.h"
#include "misc/lc15bts_par.h"
#include "misc/lc15bts_bid.h"

#include <osmo-bts/logging.h>

#include <osmocom/gsm/protocol/ipaccess.h>

#include <osmocom/core/msgb.h>
#include <osmocom/core/socket.h>
#include <osmocom/core/select.h>

#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define ETH0_ADDR_SYSFS		"/var/lc15/net/eth0/address"

static struct osmo_fd nl_fd;

/*
 * The TLV structure in IPA messages in UDP packages is a bit
 * weird. First the header appears to have an extra NULL byte
 * and second the L16 of the L16TV needs to include +1 for the
 * tag. The default msgb/tlv and libosmo-abis routines do not
 * provide this.
 */

static void ipaccess_prepend_header_quirk(struct msgb *msg, int proto)
{
	struct ipaccess_head *hh;

	/* prepend the ip.access header */
	hh = (struct ipaccess_head *) msgb_push(msg, sizeof(*hh) + 1);
	hh->len = htons(msg->len - sizeof(*hh) - 1);
	hh->proto = proto;
}

static void quirk_l16tv_put(struct msgb *msg, uint16_t len, uint8_t tag,
			const uint8_t *val)
{
	uint8_t *buf = msgb_put(msg, len + 2 + 1);

	*buf++ = (len + 1) >> 8;
	*buf++ = (len + 1) & 0xff;
	*buf++ = tag;
	memcpy(buf, val, len);
}

/*
 * We don't look at the content of the request yet and lie
 * about most of the responses.
 */
static void respond_to(struct sockaddr_in *src, struct osmo_fd *fd,
			uint8_t *data, size_t len)
{
	static int fetched_info = 0;
	static char mac_str[20] = {0, };
	static char model_name[64] = {0, };
	static char ser_str[20] = {0, };

	struct sockaddr_in loc_addr;
	int rc;
	char loc_ip[INET_ADDRSTRLEN];
	struct msgb *msg = msgb_alloc_headroom(512, 128, "ipa get response");
	if (!msg) {
		LOGP(DFIND, LOGL_ERROR, "Failed to allocate msgb\n");
		return;
	}

	if (!fetched_info) {
		int fd_eth;
		int serno;
		int model;
		int rev;

		/* fetch the MAC */
		fd_eth = open(ETH0_ADDR_SYSFS, O_RDONLY);
		if (fd_eth >= 0) {
			read(fd_eth, mac_str, sizeof(mac_str)-1);
			mac_str[sizeof(mac_str)-1] = '\0';		
			close(fd_eth);	
		}

		/* fetch the serial number */
		lc15bts_par_get_int(tall_mgr_ctx, LC15BTS_PAR_SERNR, &serno);
		snprintf(ser_str, sizeof(ser_str), "%d", serno);

		/* fetch the model and trx number */
		snprintf(model_name, sizeof(model_name), "Litecell 1.5 BTS");

		rev = lc15bts_rev_get();
		if (rev >= 0) {
			snprintf(model_name, sizeof(model_name), "%s Rev %c", 
				model_name, rev);
		} 

		model = lc15bts_model_get();
		if (model >= 0) {
			snprintf(model_name, sizeof(model_name), "%s (%05X)", 
				model_name, model);
		}
		fetched_info = 1;
	}

	if (source_for_dest(&src->sin_addr, &loc_addr.sin_addr) != 0) {
		LOGP(DFIND, LOGL_ERROR, "Failed to determine local source\n");
		return;
	}

	msgb_put_u8(msg, IPAC_MSGT_ID_RESP);

	/* append MAC addr */
	quirk_l16tv_put(msg, strlen(mac_str) + 1, IPAC_IDTAG_MACADDR, (uint8_t *) mac_str);

	/* append ip address */
	inet_ntop(AF_INET, &loc_addr.sin_addr, loc_ip, sizeof(loc_ip));
	quirk_l16tv_put(msg, strlen(loc_ip) + 1, IPAC_IDTAG_IPADDR, (uint8_t *) loc_ip);

	/* append the serial number */
	quirk_l16tv_put(msg, strlen(ser_str) + 1, IPAC_IDTAG_SERNR, (uint8_t *) ser_str);

	/* abuse some flags */
	quirk_l16tv_put(msg, strlen(model_name) + 1, IPAC_IDTAG_UNIT, (uint8_t *) model_name);

	/* ip.access nanoBTS would reply to port==3006 */
	ipaccess_prepend_header_quirk(msg, IPAC_PROTO_IPACCESS);
	rc = sendto(fd->fd, msg->data, msg->len, 0, (struct sockaddr *)src, sizeof(*src));
	if (rc != msg->len)
		LOGP(DFIND, LOGL_ERROR,
			"Failed to send with rc(%d) errno(%d)\n", rc, errno);
}

static int ipaccess_bcast(struct osmo_fd *fd, unsigned int what)
{
	uint8_t data[2048];
	char src[INET_ADDRSTRLEN];
	struct sockaddr_in addr = {};
	socklen_t len = sizeof(addr);
	int rc;

	rc = recvfrom(fd->fd, data, sizeof(data), 0,
			(struct sockaddr *) &addr, &len);
	if (rc <= 0) {
		LOGP(DFIND, LOGL_ERROR,
			"Failed to read from socket errno(%d)\n", errno);
		return -1;
	}

	LOGP(DFIND, LOGL_DEBUG,
		"Received request from: %s size %d\n",
		inet_ntop(AF_INET, &addr.sin_addr, src, sizeof(src)), rc);

	if (rc < 6)
		return 0;

	if (data[2] != IPAC_PROTO_IPACCESS || data[4] != IPAC_MSGT_ID_GET)
		return 0;

	respond_to(&addr, fd, data + 6, rc - 6);
	return 0;
}

int lc15bts_mgr_nl_init(void)
{
	int rc;

	nl_fd.cb = ipaccess_bcast;
	rc = osmo_sock_init_ofd(&nl_fd, AF_INET, SOCK_DGRAM, IPPROTO_UDP,
				"0.0.0.0", 3006, OSMO_SOCK_F_BIND);
	if (rc < 0) {
		perror("Socket creation");
		return -1;
	}

	return 0;
}
