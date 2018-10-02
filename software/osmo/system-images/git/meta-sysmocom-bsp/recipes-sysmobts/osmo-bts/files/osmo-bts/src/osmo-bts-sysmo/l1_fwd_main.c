/* Sysmocom femtobts L1 proxy */

/* (C) 2011 by Harald Welte <laforge@gnumonks.org>
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

#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/utils.h>
#include <osmocom/core/select.h>
#include <osmocom/core/write_queue.h>
#include <osmocom/core/logging.h>
#include <osmocom/core/socket.h>
#include <osmocom/core/application.h>
#include <osmocom/gsm/gsm_utils.h>

#include <osmo-bts/logging.h>
#include <osmo-bts/gsm_data.h>

#include <sysmocom/femtobts/superfemto.h>
#include <sysmocom/femtobts/gsml1prim.h>
#include <sysmocom/femtobts/gsml1const.h>
#include <sysmocom/femtobts/gsml1types.h>

#include "femtobts.h"
#include "l1_if.h"
#include "l1_transp.h"
#include "l1_fwd.h"

static const uint16_t fwd_udp_ports[_NUM_MQ_WRITE] = {
	[MQ_SYS_READ]	= L1FWD_SYS_PORT,
	[MQ_L1_READ]	= L1FWD_L1_PORT,
#ifndef HW_SYSMOBTS_V1
	[MQ_TCH_READ]	= L1FWD_TCH_PORT,
	[MQ_PDTCH_READ]	= L1FWD_PDTCH_PORT,
#endif
};

struct l1fwd_hdl {
	struct sockaddr_storage remote_sa[_NUM_MQ_WRITE];
	socklen_t remote_sa_len[_NUM_MQ_WRITE];

	struct osmo_wqueue udp_wq[_NUM_MQ_WRITE];

	struct femtol1_hdl *fl1h;
};


/* callback when there's a new L1 primitive coming in from the HW */
int l1if_handle_l1prim(int wq, struct femtol1_hdl *fl1h, struct msgb *msg)
{
	struct l1fwd_hdl *l1fh = fl1h->priv;

	/* Enqueue message to UDP socket */
	if (osmo_wqueue_enqueue(&l1fh->udp_wq[wq], msg) != 0) {
		LOGP(DL1C, LOGL_ERROR, "Write queue %d full. dropping msg\n", wq);
		msgb_free(msg);
		return -EAGAIN;
	}
	return 0;
}

/* callback when there's a new SYS primitive coming in from the HW */
int l1if_handle_sysprim(struct femtol1_hdl *fl1h, struct msgb *msg)
{
	struct l1fwd_hdl *l1fh = fl1h->priv;

	/* Enqueue message to UDP socket */
	if (osmo_wqueue_enqueue(&l1fh->udp_wq[MQ_SYS_WRITE], msg) != 0) {
		LOGP(DL1C, LOGL_ERROR, "MQ_SYS_WRITE ful. dropping msg\n");
		msgb_free(msg);
		return -EAGAIN;
	}
	return 0;
}


/* data has arrived on the udp socket */
static int udp_read_cb(struct osmo_fd *ofd)
{
	struct msgb *msg = msgb_alloc_headroom(SYSMOBTS_PRIM_SIZE, 128, "udp_rx");
	struct l1fwd_hdl *l1fh = ofd->data;
	struct femtol1_hdl *fl1h = l1fh->fl1h;
	int rc;

	if (!msg)
		return -ENOMEM;

	msg->l1h = msg->data;

	l1fh->remote_sa_len[ofd->priv_nr] = sizeof(l1fh->remote_sa[ofd->priv_nr]);
	rc = recvfrom(ofd->fd, msg->l1h, msgb_tailroom(msg), 0,
		      (struct sockaddr *) &l1fh->remote_sa[ofd->priv_nr], &l1fh->remote_sa_len[ofd->priv_nr]);
	if (rc < 0) {
		perror("read from udp");
		msgb_free(msg);
		return rc;
	} else if (rc == 0) {
		perror("len=0 read from udp");
		msgb_free(msg);	
		return rc;
	}
	msgb_put(msg, rc);

	DEBUGP(DL1C, "UDP: Received %u bytes for queue %d\n", rc,
		ofd->priv_nr);

	/* put the message into the right queue */
	if (osmo_wqueue_enqueue(&fl1h->write_q[ofd->priv_nr], msg) != 0) {
		LOGP(DL1C, LOGL_ERROR, "Write queue %d full. dropping msg\n",
			ofd->priv_nr);
		msgb_free(msg);
		return -EAGAIN;
	}
	return 0;
}

/* callback when we can write to the UDP socket */
static int udp_write_cb(struct osmo_fd *ofd, struct msgb *msg)
{
	int rc;
	struct l1fwd_hdl *l1fh = ofd->data;

	DEBUGP(DL1C, "UDP: Writing %u bytes for queue %d\n", msgb_l1len(msg),
		ofd->priv_nr);

	rc = sendto(ofd->fd, msg->l1h, msgb_l1len(msg), 0,
		    (const struct sockaddr *)&l1fh->remote_sa[ofd->priv_nr], l1fh->remote_sa_len[ofd->priv_nr]);
	if (rc < 0) {
		LOGP(DL1C, LOGL_ERROR, "error writing to L1 msg_queue: %s\n",
			strerror(errno));
		return rc;
	} else if (rc < msgb_l1len(msg)) {
		LOGP(DL1C, LOGL_ERROR, "short write to L1 msg_queue: "
			"%u < %u\n", rc, msgb_l1len(msg));
		return -EIO;
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct l1fwd_hdl *l1fh;
	struct femtol1_hdl *fl1h;
	int rc, i;
	void *ctx = talloc_named_const(NULL, 0, "l1_fwd");

	printf("sizeof(GsmL1_Prim_t) = %zu\n", sizeof(GsmL1_Prim_t));
	printf("sizeof(SuperFemto_Prim_t) = %zu\n", sizeof(SuperFemto_Prim_t));

	osmo_init_logging2(ctx, &bts_log_info);

	/*
	 * hack and prevent that two l1fwd-proxy/sysmobts run at the same
	 * time. This is done by binding to the same VTY port.
	 */
	rc = osmo_sock_init(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP,
				"127.0.0.1", 4241, OSMO_SOCK_F_BIND);
	if (rc < 0) {
		fprintf(stderr, "Failed to bind to the BTS VTY port.\n");
		return EXIT_FAILURE;
	}

	/* allocate new femtol1_handle */
	fl1h = talloc_zero(ctx, struct femtol1_hdl);
	INIT_LLIST_HEAD(&fl1h->wlc_list);

	/* open the actual hardware transport */
	for (i = 0; i < ARRAY_SIZE(fl1h->write_q); i++) {
		rc = l1if_transport_open(i, fl1h);
		if (rc < 0)
			exit(1);
	}

	/* create our fwd handle */
	l1fh = talloc_zero(ctx, struct l1fwd_hdl);

	l1fh->fl1h = fl1h;
	fl1h->priv = l1fh;

	/* Open UDP */
	for (i = 0; i < ARRAY_SIZE(l1fh->udp_wq); i++) {
		struct osmo_wqueue *wq = &l1fh->udp_wq[i];

		osmo_wqueue_init(wq, 10);
		wq->write_cb = udp_write_cb;
		wq->read_cb = udp_read_cb;

		wq->bfd.when |= BSC_FD_READ;
		wq->bfd.data = l1fh;
		wq->bfd.priv_nr = i;
		rc = osmo_sock_init_ofd(&wq->bfd, AF_UNSPEC, SOCK_DGRAM,
					IPPROTO_UDP, NULL, fwd_udp_ports[i],
					OSMO_SOCK_F_BIND);
		if (rc < 0) {
			perror("sock_init");
			exit(1);
		}
	}

	while (1) {
		rc = osmo_select_main(0);		
		if (rc < 0) {
			perror("select");
			exit(1);
		}
	}
	exit(0);
}
