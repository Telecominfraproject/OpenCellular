/*
 * OpenBTS-style TRX interface/protocol handling
 *
 * This file contains the BTS-side implementation of the OpenBTS-style
 * UDP TRX protocol.  It manages the clock, control + burst-data UDP
 * sockets and their respective protocol encoding/parsing.
 *
 * Copyright (C) 2013  Andreas Eversberg <jolly@eversberg.eu>
 * Copyright (C) 2016-2017  Harald Welte <laforge@gnumonks.org>
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
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <netinet/in.h>

#include <osmocom/core/select.h>
#include <osmocom/core/socket.h>
#include <osmocom/core/timer.h>
#include <osmocom/core/talloc.h>
#include <osmocom/core/bits.h>

#include <osmo-bts/phy_link.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/scheduler.h>

#include "l1_if.h"
#include "trx_if.h"

/* enable to print RSSI level graph */
//#define TOA_RSSI_DEBUG

int transceiver_available = 0;

#define TRX_MAX_BURST_LEN	512

/*
 * socket helper functions
 */

/*! convenience wrapper to open socket + fill in osmo_fd */
static int trx_udp_open(void *priv, struct osmo_fd *ofd, const char *host_local,
			uint16_t port_local, const char *host_remote, uint16_t port_remote,
			int (*cb)(struct osmo_fd *fd, unsigned int what))
{
	int rc;

	/* Init */
	ofd->fd = -1;
	ofd->cb = cb;
	ofd->data = priv;

	/* Listen / Binds + Connect */
	rc = osmo_sock_init2_ofd(ofd, AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, host_local, port_local,
				host_remote, port_remote, OSMO_SOCK_F_BIND | OSMO_SOCK_F_CONNECT);
	if (rc < 0)
		return rc;

	return 0;
}

/* close socket + unregister osmo_fd */
static void trx_udp_close(struct osmo_fd *ofd)
{
	if (ofd->fd >= 0) {
		osmo_fd_unregister(ofd);
		close(ofd->fd);
		ofd->fd = -1;
	}
}


/*
 * TRX clock socket
 */

/* get clock from clock socket */
static int trx_clk_read_cb(struct osmo_fd *ofd, unsigned int what)
{
	struct phy_link *plink = ofd->data;
	struct phy_instance *pinst = phy_instance_by_num(plink, 0);
	char buf[1500];
	int len;
	uint32_t fn;

	OSMO_ASSERT(pinst);

	len = recv(ofd->fd, buf, sizeof(buf) - 1, 0);
	if (len <= 0)
		return len;
	buf[len] = '\0';

	if (!!strncmp(buf, "IND CLOCK ", 10)) {
		LOGP(DTRX, LOGL_NOTICE, "Unknown message on clock port: %s\n",
			buf);
		return 0;
	}

	if (sscanf(buf, "IND CLOCK %u", &fn) != 1) {
		LOGP(DTRX, LOGL_ERROR, "Unable to parse '%s'\n", buf);
		return 0;
	}

	LOGP(DTRX, LOGL_INFO, "Clock indication: fn=%u\n", fn);

	if (fn >= GSM_HYPERFRAME) {
		fn %= GSM_HYPERFRAME;
		LOGP(DTRX, LOGL_ERROR, "Indicated clock's FN is not wrapping "
			"correctly, correcting to fn=%u\n", fn);
	}

	/* inform core TRX clock handling code that a FN has been received */
	trx_sched_clock(pinst->trx->bts, fn);

	return 0;
}


/*
 * TRX ctrl socket
 */

static void trx_ctrl_timer_cb(void *data);

/* send first ctrl message and start timer */
static void trx_ctrl_send(struct trx_l1h *l1h)
{
	struct trx_ctrl_msg *tcm;
	char buf[1500];
	int len;

	/* get first command */
	if (llist_empty(&l1h->trx_ctrl_list))
		return;
	tcm = llist_entry(l1h->trx_ctrl_list.next, struct trx_ctrl_msg, list);

	len = snprintf(buf, sizeof(buf), "CMD %s%s%s", tcm->cmd, tcm->params_len ? " ":"", tcm->params);
	OSMO_ASSERT(len < sizeof(buf));

	LOGP(DTRX, LOGL_DEBUG, "Sending control '%s' to %s\n", buf, phy_instance_name(l1h->phy_inst));
	/* send command */
	send(l1h->trx_ofd_ctrl.fd, buf, len+1, 0);

	/* start timer */
	l1h->trx_ctrl_timer.cb = trx_ctrl_timer_cb;
	l1h->trx_ctrl_timer.data = l1h;
	osmo_timer_schedule(&l1h->trx_ctrl_timer, 2, 0);
}

/* send first ctrl message and start timer */
static void trx_ctrl_timer_cb(void *data)
{
	struct trx_l1h *l1h = data;
	struct trx_ctrl_msg *tcm = NULL;

	/* get first command */
	OSMO_ASSERT(!llist_empty(&l1h->trx_ctrl_list));
	tcm = llist_entry(l1h->trx_ctrl_list.next, struct trx_ctrl_msg, list);

	LOGP(DTRX, LOGL_NOTICE, "No response from transceiver for %s (CMD %s%s%s)\n",
		phy_instance_name(l1h->phy_inst),
		tcm->cmd, tcm->params_len ? " ":"", tcm->params);

	trx_ctrl_send(l1h);
}

/*! Send a new TRX control command.
 *  \param[inout] l1h TRX Layer1 handle to which to send command
 *  \param[in] criticial
 *  \param[in] cmd zero-terminated string containing command
 *  \param[in] fmt Format string (+ variable list of arguments)
 *  \returns 0 on success; negative on error
 *
 *  The new ocommand will be added to the end of the control command
 *  queue.
 */
static int trx_ctrl_cmd(struct trx_l1h *l1h, int critical, const char *cmd,
	const char *fmt, ...)
{
	struct trx_ctrl_msg *tcm;
	struct trx_ctrl_msg *prev = NULL;
	va_list ap;
	int pending;

	if (!transceiver_available &&
	    !(!strcmp(cmd, "POWEROFF") || !strcmp(cmd, "POWERON"))) {
		LOGP(DTRX, LOGL_ERROR, "CTRL %s ignored: No clock from "
		     "transceiver, please fix!\n", cmd);
		return -EIO;
	}

	pending = !llist_empty(&l1h->trx_ctrl_list);

	/* create message */
	tcm = talloc_zero(tall_bts_ctx, struct trx_ctrl_msg);
	if (!tcm)
		return -ENOMEM;
	snprintf(tcm->cmd, sizeof(tcm->cmd)-1, "%s", cmd);
	tcm->cmd[sizeof(tcm->cmd)-1] = '\0';
	tcm->cmd_len = strlen(tcm->cmd);
	if (fmt && fmt[0]) {
		va_start(ap, fmt);
		vsnprintf(tcm->params, sizeof(tcm->params) - 1, fmt, ap);
		va_end(ap);
		tcm->params[sizeof(tcm->params)-1] = '\0';
		tcm->params_len = strlen(tcm->params);
	} else {
		tcm->params[0] ='\0';
		tcm->params_len = 0;
	}
	tcm->critical = critical;

	/* Avoid adding consecutive duplicate messages, eg: two consecutive POWEROFF */
	if(pending)
		prev = llist_entry(l1h->trx_ctrl_list.prev, struct trx_ctrl_msg, list);

	if (!pending ||
	    !(strcmp(tcm->cmd, prev->cmd) == 0 && strcmp(tcm->params, prev->params) == 0)) {
		LOGP(DTRX, LOGL_INFO, "Enqueuing TRX control command 'CMD %s%s%s'\n",
			tcm->cmd, tcm->params_len ? " ":"", tcm->params);
		llist_add_tail(&tcm->list, &l1h->trx_ctrl_list);
	}

	/* send message, if we didn't already have pending messages */
	if (!pending)
		trx_ctrl_send(l1h);

	return 0;
}

/*! Send "POWEROFF" command to TRX */
int trx_if_cmd_poweroff(struct trx_l1h *l1h)
{
	struct phy_instance *pinst = l1h->phy_inst;
	if (pinst->num == 0)
		return trx_ctrl_cmd(l1h, 1, "POWEROFF", "");
	else
		return 0;
}

/*! Send "POWERON" command to TRX */
int trx_if_cmd_poweron(struct trx_l1h *l1h)
{
	struct phy_instance *pinst = l1h->phy_inst;
	if (pinst->num == 0)
		return trx_ctrl_cmd(l1h, 1, "POWERON", "");
	else
		return 0;
}

/*! Send "SETTSC" command to TRX */
int trx_if_cmd_settsc(struct trx_l1h *l1h, uint8_t tsc)
{
	struct phy_instance *pinst = l1h->phy_inst;
	if (pinst->phy_link->u.osmotrx.use_legacy_setbsic)
		return 0;

	return trx_ctrl_cmd(l1h, 1, "SETTSC", "%d", tsc);
}

/*! Send "SETBSIC" command to TRX */
int trx_if_cmd_setbsic(struct trx_l1h *l1h, uint8_t bsic)
{
	struct phy_instance *pinst = l1h->phy_inst;
	if (!pinst->phy_link->u.osmotrx.use_legacy_setbsic)
		return 0;

	return trx_ctrl_cmd(l1h, 1, "SETBSIC", "%d", bsic);
}

/*! Send "SETRXGAIN" command to TRX */
int trx_if_cmd_setrxgain(struct trx_l1h *l1h, int db)
{
	return trx_ctrl_cmd(l1h, 0, "SETRXGAIN", "%d", db);
}

/*! Send "SETPOWER" command to TRX */
int trx_if_cmd_setpower(struct trx_l1h *l1h, int db)
{
	return trx_ctrl_cmd(l1h, 0, "SETPOWER", "%d", db);
}

/*! Send "SETMAXDLY" command to TRX, i.e. maximum delay for RACH bursts */
int trx_if_cmd_setmaxdly(struct trx_l1h *l1h, int dly)
{
	return trx_ctrl_cmd(l1h, 0, "SETMAXDLY", "%d", dly);
}

/*! Send "SETMAXDLYNB" command to TRX, i.e. maximum delay for normal bursts */
int trx_if_cmd_setmaxdlynb(struct trx_l1h *l1h, int dly)
{
	return trx_ctrl_cmd(l1h, 0, "SETMAXDLYNB", "%d", dly);
}

/*! Send "SETSLOT" command to TRX: Configure Channel Combination for TS */
int trx_if_cmd_setslot(struct trx_l1h *l1h, uint8_t tn, uint8_t type)
{
	return trx_ctrl_cmd(l1h, 1, "SETSLOT", "%d %d", tn, type);
}

/*! Send "RXTUNE" command to TRX: Tune Receiver to given ARFCN */
int trx_if_cmd_rxtune(struct trx_l1h *l1h, uint16_t arfcn)
{
	struct phy_instance *pinst = l1h->phy_inst;
	uint16_t freq10;

	if (pinst->trx->bts->band == GSM_BAND_1900)
		arfcn |= ARFCN_PCS;

	freq10 = gsm_arfcn2freq10(arfcn, 1); /* RX = uplink */
	if (freq10 == 0xffff) {
		LOGP(DTRX, LOGL_ERROR, "Arfcn %d not defined.\n",
		     arfcn & ~ARFCN_FLAG_MASK);
		return -ENOTSUP;
	}

	return trx_ctrl_cmd(l1h, 1, "RXTUNE", "%d", freq10 * 100);
}

/*! Send "TXTUNE" command to TRX: Tune Transmitter to given ARFCN */
int trx_if_cmd_txtune(struct trx_l1h *l1h, uint16_t arfcn)
{
	struct phy_instance *pinst = l1h->phy_inst;
	uint16_t freq10;

	if (pinst->trx->bts->band == GSM_BAND_1900)
		arfcn |= ARFCN_PCS;

	freq10 = gsm_arfcn2freq10(arfcn, 0); /* TX = downlink */
	if (freq10 == 0xffff) {
		LOGP(DTRX, LOGL_ERROR, "Arfcn %d not defined.\n",
		     arfcn & ~ARFCN_FLAG_MASK);
		return -ENOTSUP;
	}

	return trx_ctrl_cmd(l1h, 1, "TXTUNE", "%d", freq10 * 100);
}

/*! Send "HANDOVER" command to TRX: Enable handover RACH Detection on timeslot/sub-slot */
int trx_if_cmd_handover(struct trx_l1h *l1h, uint8_t tn, uint8_t ss)
{
	return trx_ctrl_cmd(l1h, 1, "HANDOVER", "%d %d", tn, ss);
}

/*! Send "NOHANDOVER" command to TRX: Disable handover RACH Detection on timeslot/sub-slot */
int trx_if_cmd_nohandover(struct trx_l1h *l1h, uint8_t tn, uint8_t ss)
{
	return trx_ctrl_cmd(l1h, 1, "NOHANDOVER", "%d %d", tn, ss);
}

static int parse_rsp(const char *buf_in, size_t len_in, char *cmdname_out, size_t cmdname_len,
			char *params_out, size_t params_len, int *status)
{
	char *p, *k;

	if (strncmp(buf_in, "RSP ", 4))
		goto parse_err;

	/* Get the RSP cmd name */
	if (!(p = strchr(buf_in + 4, ' ')))
		goto parse_err;

	if (p - buf_in >= cmdname_len) {
		LOGP(DTRX, LOGL_ERROR, "cmdname buffer too small %lu >= %lu\n",
			p - buf_in, cmdname_len);
		goto parse_err;
	}

	cmdname_out[0] = '\0';
	strncat(cmdname_out, buf_in + 4, p - buf_in - 4);

	/* Now comes the status code of the response */
	p++;
	if (sscanf(p, "%d", status) != 1)
		goto parse_err;

	/* Now copy back the parameters */
	k = strchr(p, ' ');
	if (k)
		k++;
	else
		k = p + strlen(p);

	if (strlen(k) >= params_len) {
		LOGP(DTRX, LOGL_ERROR, "params buffer too small %lu >= %lu\n",
			strlen(k), params_len);
		goto parse_err;
	}
	params_out[0] = '\0';
	strcat(params_out, k);
	return 0;

parse_err:
	LOGP(DTRX, LOGL_NOTICE, "Unknown message on ctrl port: %s\n",
		buf_in);
	return -1;
}

static bool cmd_matches_rsp(struct trx_ctrl_msg *tcm, char *rspname, char* params)
{
	if (strcmp(tcm->cmd, rspname))
		return false;

	/* For SETSLOT we also need to check if it's the response for the
	   specific timeslot. For other commands such as SETRXGAIN, it is
	   expected that they can return different values */
	if (strcmp(tcm->cmd, "SETSLOT") == 0 && strcmp(tcm->params, params))
		return false;

	return true;
}

/*! Get + parse response from TRX ctrl socket */
static int trx_ctrl_read_cb(struct osmo_fd *ofd, unsigned int what)
{
	struct trx_l1h *l1h = ofd->data;
	struct phy_instance *pinst = l1h->phy_inst;
	char buf[1500], cmdname[50], params[100];
	int len, resp;
	struct trx_ctrl_msg *tcm;

	len = recv(ofd->fd, buf, sizeof(buf) - 1, 0);
	if (len <= 0)
		return len;
	buf[len] = '\0';

	if (parse_rsp(buf, len, cmdname, sizeof(cmdname), params, sizeof(params), &resp) < 0)
		return 0;

	LOGP(DTRX, LOGL_INFO, "Response message: '%s'\n", buf);

	/* abort timer and send next message, if any */
	if (osmo_timer_pending(&l1h->trx_ctrl_timer))
		osmo_timer_del(&l1h->trx_ctrl_timer);

	/* get command for response message */
	if (llist_empty(&l1h->trx_ctrl_list)) {
		/* RSP from a retransmission, skip it */
		if (l1h->last_acked && cmd_matches_rsp(l1h->last_acked, cmdname, params)) {
			LOGP(DTRX, LOGL_NOTICE, "Discarding duplicated RSP "
				"from old CMD '%s'\n", buf);
			return 0;
		}
		LOGP(DTRX, LOGL_NOTICE, "Response message without "
			"command\n");
		return -EINVAL;
	}
	tcm = llist_entry(l1h->trx_ctrl_list.next, struct trx_ctrl_msg,
		list);

	/* check if response matches command */
	if (!cmd_matches_rsp(tcm, cmdname, params)) {
		/* RSP from a retransmission, skip it */
		if (l1h->last_acked && cmd_matches_rsp(l1h->last_acked, cmdname, params)) {
			LOGP(DTRX, LOGL_NOTICE, "Discarding duplicated RSP "
				"from old CMD '%s'\n", buf);
			return 0;
		}
		LOGP(DTRX, (tcm->critical) ? LOGL_FATAL : LOGL_NOTICE,
			"Response message '%s' does not match command "
			"message 'CMD %s%s%s'\n",
			buf, tcm->cmd, tcm->params_len ? " ":"", tcm->params);
		goto rsp_error;
	}

	/* check for response code */
	if (resp) {
		LOGP(DTRX, (tcm->critical) ? LOGL_FATAL : LOGL_NOTICE,
			"transceiver (%s) rejected TRX command "
			"with response: '%s'\n",
			phy_instance_name(pinst), buf);
		if (tcm->critical)
			goto rsp_error;
	}

	/* remove command from list, save it to last_acked and removed previous last_acked */
	llist_del(&tcm->list);
	talloc_free(l1h->last_acked);
	l1h->last_acked = tcm;

	trx_ctrl_send(l1h);

	return 0;

rsp_error:
	bts_shutdown(pinst->trx->bts, "TRX-CTRL-MSG: CRITICAL");
	/* keep tcm list, so process is stopped */
	return -EIO;
}


/*
 * TRX burst data socket
 */

static int trx_data_read_cb(struct osmo_fd *ofd, unsigned int what)
{
	struct trx_l1h *l1h = ofd->data;
	uint8_t buf[TRX_MAX_BURST_LEN];
	int len;
	uint8_t tn;
	int8_t rssi;
	int16_t toa256 = 0;
	uint32_t fn;
	sbit_t bits[EGPRS_BURST_LEN];
	int i, burst_len = GSM_BURST_LEN;

	len = recv(ofd->fd, buf, sizeof(buf), 0);
	if (len <= 0) {
		return len;
	} else if (len == EGPRS_BURST_LEN + 10) {
		burst_len = EGPRS_BURST_LEN;
	/* Accept bursts ending with 2 bytes of padding (OpenBTS compatible trx) or without them: */
	} else if (len != GSM_BURST_LEN + 10 && len != GSM_BURST_LEN + 8) {
		LOGP(DTRX, LOGL_NOTICE, "Got data message with invalid lenght "
			"'%d'\n", len);
		return -EINVAL;
	}
	tn = buf[0];
	fn = (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4];
	rssi = -(int8_t)buf[5];
	toa256 = ((int16_t)(buf[6] << 8) | buf[7]);

	/* copy and convert bits {254..0} to sbits {-127..127} */
	for (i = 0; i < burst_len; i++) {
		if (buf[8 + i] == 255)
			bits[i] = -127;
		else
			bits[i] = 127 - buf[8 + i];
	}

	if (tn >= 8) {
		LOGP(DTRX, LOGL_ERROR, "Illegal TS %d\n", tn);
		return -EINVAL;
	}
	if (fn >= GSM_HYPERFRAME) {
		LOGP(DTRX, LOGL_ERROR, "Illegal FN %u\n", fn);
		return -EINVAL;
	}

	LOGP(DTRX, LOGL_DEBUG, "RX burst tn=%u fn=%u rssi=%d toa256=%d\n",
		tn, fn, rssi, toa256);

#ifdef TOA_RSSI_DEBUG
	char deb[128];

	sprintf(deb, "|                                0              "
		"                 | rssi=%4d  toa=%5d fn=%u", rssi, toa256, fn);
	deb[1 + (128 + rssi) / 4] = '*';
	fprintf(stderr, "%s\n", deb);
#endif

	/* feed received burst into scheduler code */
	trx_sched_ul_burst(&l1h->l1s, tn, fn, bits, burst_len, rssi, toa256);

	return 0;
}

/*! Send burst data for given FN/timeslot to TRX
 *  \param[inout] l1h TRX Layer1 handle referring to TX
 *  \param[in] tn Timeslot Number (0..7)
 *  \param[in] fn GSM Frame Number
 *  \param[in] pwr Transmit Power to use
 *  \param[in] bits Unpacked bits to be transmitted
 *  \param[in] nbits Number of \a bits
 *  \returns 0 on success; negative on error */
int trx_if_send_burst(struct trx_l1h *l1h, uint8_t tn, uint32_t fn, uint8_t pwr,
	const ubit_t *bits, uint16_t nbits)
{
	uint8_t buf[TRX_MAX_BURST_LEN];

	if ((nbits != GSM_BURST_LEN) && (nbits != EGPRS_BURST_LEN)) {
		LOGP(DTRX, LOGL_ERROR, "Tx burst length %u invalid\n", nbits);
		return -1;
	}

	LOGP(DTRX, LOGL_DEBUG, "TX burst tn=%u fn=%u pwr=%u\n", tn, fn, pwr);

	buf[0] = tn;
	buf[1] = (fn >> 24) & 0xff;
	buf[2] = (fn >> 16) & 0xff;
	buf[3] = (fn >>  8) & 0xff;
	buf[4] = (fn >>  0) & 0xff;
	buf[5] = pwr;

	/* copy ubits {0,1} */
	memcpy(buf + 6, bits, nbits);

	/* we must be sure that we have clock, and we have sent all control
	 * data */
	if (transceiver_available && llist_empty(&l1h->trx_ctrl_list)) {
		send(l1h->trx_ofd_data.fd, buf, nbits + 6, 0);
	} else
		LOGP(DTRX, LOGL_DEBUG, "Ignoring TX data, transceiver "
			"offline.\n");

	return 0;
}


/*
 * open/close
 */

/*! flush (delete) all pending control messages */
void trx_if_flush(struct trx_l1h *l1h)
{
	struct trx_ctrl_msg *tcm;

	/* free ctrl message list */
	while (!llist_empty(&l1h->trx_ctrl_list)) {
		tcm = llist_entry(l1h->trx_ctrl_list.next, struct trx_ctrl_msg,
			list);
		llist_del(&tcm->list);
		talloc_free(tcm);
	}
	talloc_free(l1h->last_acked);
}

/*! close the TRX for given handle (data + control socket) */
void trx_if_close(struct trx_l1h *l1h)
{
	struct phy_instance *pinst = l1h->phy_inst;
	LOGP(DTRX, LOGL_NOTICE, "Close transceiver for %s\n",
		phy_instance_name(pinst));

	trx_if_flush(l1h);

	/* close sockets */
	trx_udp_close(&l1h->trx_ofd_ctrl);
	trx_udp_close(&l1h->trx_ofd_data);
}

/*! compute UDP port number used for TRX protocol */
static uint16_t compute_port(struct phy_instance *pinst, int remote, int is_data)
{
	struct phy_link *plink = pinst->phy_link;
	uint16_t inc = 1;

	if (is_data)
		inc = 2;

	if (remote)
		return plink->u.osmotrx.base_port_remote + (pinst->num << 1) + inc;
	else
		return plink->u.osmotrx.base_port_local + (pinst->num << 1) + inc;
}

/*! open a TRX interface. creates contro + data sockets */
static int trx_if_open(struct trx_l1h *l1h)
{
	struct phy_instance *pinst = l1h->phy_inst;
	struct phy_link *plink = pinst->phy_link;
	int rc;

	LOGP(DTRX, LOGL_NOTICE, "Open transceiver for %s\n",
		phy_instance_name(pinst));

	/* initialize ctrl queue */
	INIT_LLIST_HEAD(&l1h->trx_ctrl_list);

	/* open sockets */
	rc = trx_udp_open(l1h, &l1h->trx_ofd_ctrl,
			  plink->u.osmotrx.local_ip,
			  compute_port(pinst, 0, 0),
			  plink->u.osmotrx.remote_ip,
			  compute_port(pinst, 1, 0), trx_ctrl_read_cb);
	if (rc < 0)
		goto err;
	rc = trx_udp_open(l1h, &l1h->trx_ofd_data,
			  plink->u.osmotrx.local_ip,
			  compute_port(pinst, 0, 1),
			  plink->u.osmotrx.remote_ip,
			  compute_port(pinst, 1, 1), trx_data_read_cb);
	if (rc < 0)
		goto err;

	/* enable all slots */
	l1h->config.slotmask = 0xff;

	/* FIXME: why was this only for TRX0 ? */
	//if (l1h->trx->nr == 0)
	trx_if_cmd_poweroff(l1h);

	return 0;

err:
	trx_if_close(l1h);
	return rc;
}

/*! close the control + burst data sockets for one phy_instance */
static void trx_phy_inst_close(struct phy_instance *pinst)
{
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;

	trx_if_close(l1h);
	trx_sched_exit(&l1h->l1s);
}

/*! open the control + burst data sockets for one phy_instance */
static int trx_phy_inst_open(struct phy_instance *pinst)
{
	struct trx_l1h *l1h;
	int rc;

	l1h = pinst->u.osmotrx.hdl;
	if (!l1h)
		return -EINVAL;

	rc = trx_sched_init(&l1h->l1s, pinst->trx);
	if (rc < 0) {
		LOGP(DL1C, LOGL_FATAL, "Cannot initialize scheduler for phy "
		     "instance %d\n", pinst->num);
		return -EIO;
	}

	rc = trx_if_open(l1h);
	if (rc < 0) {
		LOGP(DL1C, LOGL_FATAL, "Cannot open TRX interface for phy "
		     "instance %d\n", pinst->num);
		trx_phy_inst_close(pinst);
		return -EIO;
	}

	return 0;
}

/*! open the PHY link using TRX protocol */
int bts_model_phy_link_open(struct phy_link *plink)
{
	struct phy_instance *pinst;
	int rc;

	phy_link_state_set(plink, PHY_LINK_CONNECTING);

	/* open the shared/common clock socket */
	rc = trx_udp_open(plink, &plink->u.osmotrx.trx_ofd_clk,
			  plink->u.osmotrx.local_ip,
			  plink->u.osmotrx.base_port_local,
			  plink->u.osmotrx.remote_ip,
			  plink->u.osmotrx.base_port_remote,
			  trx_clk_read_cb);
	if (rc < 0) {
		phy_link_state_set(plink, PHY_LINK_SHUTDOWN);
		return -1;
	}

	/* open the individual instances with their ctrl+data sockets */
	llist_for_each_entry(pinst, &plink->instances, list) {
		if (trx_phy_inst_open(pinst) < 0)
			goto cleanup;
	}
	/* FIXME: is there better way to check/report TRX availability? */
	transceiver_available = 1;
	phy_link_state_set(plink, PHY_LINK_CONNECTED);
	return 0;

cleanup:
	phy_link_state_set(plink, PHY_LINK_SHUTDOWN);
	llist_for_each_entry(pinst, &plink->instances, list) {
		if (pinst->u.osmotrx.hdl) {
			trx_if_close(pinst->u.osmotrx.hdl);
			pinst->u.osmotrx.hdl = NULL;
		}
	}
	trx_udp_close(&plink->u.osmotrx.trx_ofd_clk);
	return -1;
}

/*! determine if the TRX for given handle is powered up */
int trx_if_powered(struct trx_l1h *l1h)
{
	return l1h->config.poweron;
}
