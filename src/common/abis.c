/* Abis/IP interface routines utilizing libosmo-abis (Pablo) */

/* (C) 2011 by Andreas Eversberg <jolly@eversberg.eu>
 * (C) 2011-2013 by Harald Welte <laforge@gnumonks.org>
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

#include "btsconfig.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <osmocom/core/select.h>
#include <osmocom/core/timer.h>
#include <osmocom/core/msgb.h>
#include <osmocom/core/signal.h>
#include <osmocom/core/macaddr.h>
#include <osmocom/abis/abis.h>
#include <osmocom/abis/e1_input.h>
#include <osmocom/abis/ipaccess.h>
#include <osmocom/gsm/ipa.h>

#include <osmo-bts/logging.h>
#include <osmo-bts/gsm_data.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/rsl.h>
#include <osmo-bts/oml.h>
#include <osmo-bts/bts_model.h>

static struct gsm_bts *g_bts;

int abis_oml_sendmsg(struct msgb *msg)
{
	struct gsm_bts *bts = msg->trx->bts;

	if (!bts->oml_link) {
		llist_add_tail(&msg->list, &bts->oml_queue);
		return 0;
	} else {
		/* osmo-bts uses msg->trx internally, but libosmo-abis uses
		 * the signalling link at msg->dst */
		msg->dst = bts->oml_link;
		return abis_sendmsg(msg);
	}
}

static void drain_oml_queue(struct gsm_bts *bts)
{
	struct msgb *msg, *msg2;

	llist_for_each_entry_safe(msg, msg2, &bts->oml_queue, list) {
		/* osmo-bts uses msg->trx internally, but libosmo-abis uses
		 * the signalling link at msg->dst */
		llist_del(&msg->list);
		msg->dst = bts->oml_link;
		abis_sendmsg(msg);
	}
}

int abis_bts_rsl_sendmsg(struct msgb *msg)
{
	OSMO_ASSERT(msg->trx);

	if (msg->trx->bts->variant == BTS_OSMO_OMLDUMMY) {
		msgb_free(msg);
		return 0;
	}

	/* osmo-bts uses msg->trx internally, but libosmo-abis uses
	 * the signalling link at msg->dst */
	msg->dst = msg->trx->rsl_link;
	return abis_sendmsg(msg);
}

static struct e1inp_sign_link *sign_link_up(void *unit, struct e1inp_line *line,
					    enum e1inp_sign_type type)
{
	struct e1inp_sign_link *sign_link = NULL;
	struct gsm_bts_trx *trx;
	int trx_nr;

	switch (type) {
	case E1INP_SIGN_OML:
		LOGP(DABIS, LOGL_INFO, "OML Signalling link up\n");
		e1inp_ts_config_sign(&line->ts[E1INP_SIGN_OML-1], line);
		sign_link = g_bts->oml_link =
			e1inp_sign_link_create(&line->ts[E1INP_SIGN_OML-1],
						E1INP_SIGN_OML, NULL, 255, 0);
		drain_oml_queue(g_bts);
		sign_link->trx = g_bts->c0;
		bts_link_estab(g_bts);
		break;
	default:
		trx_nr = type - E1INP_SIGN_RSL;
		LOGP(DABIS, LOGL_INFO, "RSL Signalling link for TRX%d up\n",
			trx_nr);
		trx = gsm_bts_trx_num(g_bts, trx_nr);
		if (!trx) {
			LOGP(DABIS, LOGL_ERROR, "TRX%d does not exixt!\n",
				trx_nr);
			break;
		}
		e1inp_ts_config_sign(&line->ts[type-1], line);
		sign_link = trx->rsl_link =
			e1inp_sign_link_create(&line->ts[type-1],
						E1INP_SIGN_RSL, NULL, 0, 0);
		sign_link->trx = trx;
		trx_link_estab(trx);
		break;
	}

	return sign_link;
}

static void sign_link_down(struct e1inp_line *line)
{
	struct gsm_bts_trx *trx;
	LOGP(DABIS, LOGL_ERROR, "Signalling link down\n");

	/* First remove the OML signalling link */
	if (g_bts->oml_link)
		e1inp_sign_link_destroy(g_bts->oml_link);
	g_bts->oml_link = NULL;

	/* Then iterate over the RSL signalling links */
	llist_for_each_entry(trx, &g_bts->trx_list, list) {
		if (trx->rsl_link) {
			e1inp_sign_link_destroy(trx->rsl_link);
			trx->rsl_link = NULL;
		}
	}

	bts_model_abis_close(g_bts);
}


/* callback for incoming mesages from A-bis/IP */
static int sign_link_cb(struct msgb *msg)
{
	struct e1inp_sign_link *link = msg->dst;

	/* osmo-bts code assumes msg->trx is set, but libosmo-abis works
	 * with the sign_link stored in msg->dst, so we have to convert
	 * here */
	msg->trx = link->trx;

	switch (link->type) {
	case E1INP_SIGN_OML:
		down_oml(link->trx->bts, msg);
		break;
	case E1INP_SIGN_RSL:
		down_rsl(link->trx, msg);
		break;
	default:
		msgb_free(msg);
		break;
	}

	return 0;
}

uint32_t get_signlink_remote_ip(struct e1inp_sign_link *link)
{
	int fd = link->ts->driver.ipaccess.fd.fd;
	struct sockaddr_in sin;
	socklen_t slen = sizeof(sin);
	int rc;

	rc = getpeername(fd, (struct sockaddr *)&sin, &slen);
	if (rc < 0) {
		LOGP(DOML, LOGL_ERROR, "Cannot determine remote IP Addr: %s\n",
			strerror(errno));
		return 0;
	}

	/* we assume that the soket is AF_INET.  As Abis/IP contains
	 * lots of hard-coded IPv4 addresses, this safe */
	OSMO_ASSERT(sin.sin_family == AF_INET);

	return ntohl(sin.sin_addr.s_addr);
}


static int inp_s_cbfn(unsigned int subsys, unsigned int signal,
		      void *hdlr_data, void *signal_data)
{
	if (subsys != SS_L_INPUT)
		return 0;

	DEBUGP(DABIS, "Input Signal %u received\n", signal);

	return 0;
}


static struct ipaccess_unit bts_dev_info = {
	.unit_name	= "sysmoBTS",
	.equipvers	= "",	/* FIXME: read this from hw */
	.swversion	= PACKAGE_VERSION,
	.location1	= "",
	.location2	= "",
	.serno		= "",
};

static struct e1inp_line_ops line_ops = {
	.cfg = {
		.ipa = {
			.role	= E1INP_LINE_R_BTS,
			.dev	= &bts_dev_info,
		},
	},
	.sign_link_up	= sign_link_up,
	.sign_link_down	= sign_link_down,
	.sign_link	= sign_link_cb,
};

void abis_init(struct gsm_bts *bts)
{
	g_bts = bts;

	oml_init(&bts->mo);
	libosmo_abis_init(NULL);

	osmo_signal_register_handler(SS_L_INPUT, &inp_s_cbfn, bts);
}

struct e1inp_line *abis_open(struct gsm_bts *bts, char *dst_host,
			     char *model_name)
{
	struct e1inp_line *line;

	/* patch in various data from VTY and othe sources */
	line_ops.cfg.ipa.addr = dst_host;
	osmo_get_macaddr(bts_dev_info.mac_addr, "eth0");
	bts_dev_info.site_id = bts->ip_access.site_id;
	bts_dev_info.bts_id = bts->ip_access.bts_id;
	bts_dev_info.unit_name = model_name;
	if (bts->description)
		bts_dev_info.unit_name = bts->description;
	bts_dev_info.location2 = model_name;

	line = e1inp_line_find(0);
	if (!line)
		line = e1inp_line_create(0, "ipa");
	if (!line)
		return NULL;
	e1inp_line_bind_ops(line, &line_ops);

	/* This will open the OML connection now */
	if (e1inp_line_update(line) < 0)
		return NULL;

	return line;
}
