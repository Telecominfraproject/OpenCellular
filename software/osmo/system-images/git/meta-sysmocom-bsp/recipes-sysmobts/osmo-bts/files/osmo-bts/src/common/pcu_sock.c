/* pcu_sock.c: Connect from PCU via unix domain socket */

/* (C) 2008-2010 by Harald Welte <laforge@gnumonks.org>
 * (C) 2009-2012 by Andreas Eversberg <jolly@eversberg.eu>
 * (C) 2012 by Holger Hans Peter Freyther
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <inttypes.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/select.h>
#include <osmocom/core/socket.h>
#include <osmocom/gsm/gsm23003.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/gsm_data.h>
#include <osmo-bts/pcu_if.h>
#include <osmo-bts/pcuif_proto.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/rsl.h>
#include <osmo-bts/signal.h>
#include <osmo-bts/l1sap.h>

uint32_t trx_get_hlayer1(struct gsm_bts_trx *trx);

extern struct gsm_network bts_gsmnet;
int pcu_direct = 0;
static int avail_lai = 0, avail_nse = 0, avail_cell = 0, avail_nsvc[2] = {0, 0};

static const char *sapi_string[] = {
	[PCU_IF_SAPI_RACH] =	"RACH",
	[PCU_IF_SAPI_AGCH] =	"AGCH",
	[PCU_IF_SAPI_PCH] =	"PCH",
	[PCU_IF_SAPI_BCCH] =	"BCCH",
	[PCU_IF_SAPI_PDTCH] =	"PDTCH",
	[PCU_IF_SAPI_PRACH] =	"PRACH",
	[PCU_IF_SAPI_PTCCH] = 	"PTCCH",
};

static int pcu_sock_send(struct gsm_network *net, struct msgb *msg);

/*
 * PCU messages
 */

struct msgb *pcu_msgb_alloc(uint8_t msg_type, uint8_t bts_nr)
{
	struct msgb *msg;
	struct gsm_pcu_if *pcu_prim;

	msg = msgb_alloc(sizeof(struct gsm_pcu_if), "pcu_sock_tx");
	if (!msg)
		return NULL;
	msgb_put(msg, sizeof(struct gsm_pcu_if));
	pcu_prim = (struct gsm_pcu_if *) msg->data;
	pcu_prim->msg_type = msg_type;
	pcu_prim->bts_nr = bts_nr;

	return msg;
}

static bool ts_should_be_pdch(struct gsm_bts_trx_ts *ts) {
	if (ts->pchan == GSM_PCHAN_PDCH)
		return true;
	if (ts->pchan == GSM_PCHAN_TCH_F_PDCH) {
		/* When we're busy deactivating the PDCH, we first set
		 * DEACT_PENDING, tell the PCU about it and wait for a
		 * response. So DEACT_PENDING means "no PDCH" to the PCU.
		 * Similarly, when we're activating PDCH, we set the
		 * ACT_PENDING and wait for an activation response from the
		 * PCU, so ACT_PENDING means "is PDCH". */
		if (ts->flags & TS_F_PDCH_ACTIVE)
			return !(ts->flags & TS_F_PDCH_DEACT_PENDING);
		else
			return (ts->flags & TS_F_PDCH_ACT_PENDING);
	}
	if (ts->pchan == GSM_PCHAN_TCH_F_TCH_H_PDCH) {
		/*
		 * When we're busy de-/activating the PDCH, we first set
		 * ts->dyn.pchan_want, tell the PCU about it and wait for a
		 * response. So only care about dyn.pchan_want here.
		 */
		return ts->dyn.pchan_want == GSM_PCHAN_PDCH;
	}
	return false;
}

int pcu_tx_info_ind(void)
{
	struct gsm_network *net = &bts_gsmnet;
	struct msgb *msg;
	struct gsm_pcu_if *pcu_prim;
	struct gsm_pcu_if_info_ind *info_ind;
	struct gsm_bts *bts;
	struct gprs_rlc_cfg *rlcc;
	struct gsm_bts_gprs_nsvc *nsvc;
	struct gsm_bts_trx *trx;
	struct gsm_bts_trx_ts *ts;
	int i, j;

	LOGP(DPCU, LOGL_INFO, "Sending info\n");

	/* FIXME: allow multiple BTS */
	bts = llist_entry(net->bts_list.next, struct gsm_bts, list);
	rlcc = &bts->gprs.cell.rlc_cfg;

	msg = pcu_msgb_alloc(PCU_IF_MSG_INFO_IND, bts->nr);
	if (!msg)
		return -ENOMEM;
	pcu_prim = (struct gsm_pcu_if *) msg->data;
	info_ind = &pcu_prim->u.info_ind;
	info_ind->version = PCU_IF_VERSION;

	if (avail_lai && avail_nse && avail_cell && avail_nsvc[0]) {
		info_ind->flags |= PCU_IF_FLAG_ACTIVE;
		LOGP(DPCU, LOGL_INFO, "BTS is up\n");
	} else
		LOGP(DPCU, LOGL_INFO, "BTS is down\n");

	if (pcu_direct)
		info_ind->flags |= PCU_IF_FLAG_SYSMO;

	/* RAI */
	info_ind->mcc = net->plmn.mcc;
	info_ind->mnc = net->plmn.mnc;
	info_ind->mnc_3_digits = net->plmn.mnc_3_digits;
	info_ind->lac = bts->location_area_code;
	info_ind->rac = bts->gprs.rac;

	/* NSE */
	info_ind->nsei = bts->gprs.nse.nsei;
	memcpy(info_ind->nse_timer, bts->gprs.nse.timer, 7);
	memcpy(info_ind->cell_timer, bts->gprs.cell.timer, 11);

	/* cell attributes */
	info_ind->cell_id = bts->cell_identity;
	info_ind->repeat_time = rlcc->paging.repeat_time;
	info_ind->repeat_count = rlcc->paging.repeat_count;
	info_ind->bvci = bts->gprs.cell.bvci;
	info_ind->t3142 = rlcc->parameter[RLC_T3142];
	info_ind->t3169 = rlcc->parameter[RLC_T3169];
	info_ind->t3191 = rlcc->parameter[RLC_T3191];
	info_ind->t3193_10ms = rlcc->parameter[RLC_T3193];
	info_ind->t3195 = rlcc->parameter[RLC_T3195];
	info_ind->n3101 = rlcc->parameter[RLC_N3101];
	info_ind->n3103 = rlcc->parameter[RLC_N3103];
	info_ind->n3105 = rlcc->parameter[RLC_N3105];
	info_ind->cv_countdown = rlcc->parameter[CV_COUNTDOWN];
	if (rlcc->cs_mask & (1 << GPRS_CS1))
		info_ind->flags |= PCU_IF_FLAG_CS1;
	if (rlcc->cs_mask & (1 << GPRS_CS2))
		info_ind->flags |= PCU_IF_FLAG_CS2;
	if (rlcc->cs_mask & (1 << GPRS_CS3))
		info_ind->flags |= PCU_IF_FLAG_CS3;
	if (rlcc->cs_mask & (1 << GPRS_CS4))
		info_ind->flags |= PCU_IF_FLAG_CS4;
	if (rlcc->cs_mask & (1 << GPRS_MCS1))
		info_ind->flags |= PCU_IF_FLAG_MCS1;
	if (rlcc->cs_mask & (1 << GPRS_MCS2))
		info_ind->flags |= PCU_IF_FLAG_MCS2;
	if (rlcc->cs_mask & (1 << GPRS_MCS3))
		info_ind->flags |= PCU_IF_FLAG_MCS3;
	if (rlcc->cs_mask & (1 << GPRS_MCS4))
		info_ind->flags |= PCU_IF_FLAG_MCS4;
	if (rlcc->cs_mask & (1 << GPRS_MCS5))
		info_ind->flags |= PCU_IF_FLAG_MCS5;
	if (rlcc->cs_mask & (1 << GPRS_MCS6))
		info_ind->flags |= PCU_IF_FLAG_MCS6;
	if (rlcc->cs_mask & (1 << GPRS_MCS7))
		info_ind->flags |= PCU_IF_FLAG_MCS7;
	if (rlcc->cs_mask & (1 << GPRS_MCS8))
		info_ind->flags |= PCU_IF_FLAG_MCS8;
	if (rlcc->cs_mask & (1 << GPRS_MCS9))
		info_ind->flags |= PCU_IF_FLAG_MCS9;
#warning	"isn't dl_tbf_ext wrong?: * 10 and no ntohs"
	info_ind->dl_tbf_ext = rlcc->parameter[T_DL_TBF_EXT];
#warning	"isn't ul_tbf_ext wrong?: * 10 and no ntohs"
	info_ind->ul_tbf_ext = rlcc->parameter[T_UL_TBF_EXT];
	info_ind->initial_cs = rlcc->initial_cs;
	info_ind->initial_mcs = rlcc->initial_mcs;

	/* NSVC */
	for (i = 0; i < 2; i++) {
		nsvc = &bts->gprs.nsvc[i];
		info_ind->nsvci[i] = nsvc->nsvci;
		info_ind->local_port[i] = nsvc->local_port;
		info_ind->remote_port[i] = nsvc->remote_port;
		info_ind->remote_ip[i] = nsvc->remote_ip;
	}

	for (i = 0; i < 8; i++) {
		trx = gsm_bts_trx_num(bts, i);
		if (!trx)
			break;
		info_ind->trx[i].pdch_mask = 0;
		info_ind->trx[i].arfcn = trx->arfcn;
		info_ind->trx[i].hlayer1 = trx_get_hlayer1(trx);
		for (j = 0; j < 8; j++) {
			ts = &trx->ts[j];
			if (ts->mo.nm_state.operational == NM_OPSTATE_ENABLED
			    && ts_should_be_pdch(ts)) {
				info_ind->trx[i].pdch_mask |= (1 << j);
				info_ind->trx[i].tsc[j] = gsm_ts_tsc(ts);

				LOGP(DPCU, LOGL_INFO, "trx=%d ts=%d: "
					"available (tsc=%d arfcn=%d)\n",
					trx->nr, ts->nr,
					info_ind->trx[i].tsc[j],
					info_ind->trx[i].arfcn);
			}
		}
	}

	return pcu_sock_send(net, msg);
}

static int pcu_if_signal_cb(unsigned int subsys, unsigned int signal,
	void *hdlr_data, void *signal_data)
{
	struct gsm_network *net = &bts_gsmnet;
	struct gsm_bts_gprs_nsvc *nsvc;
	struct gsm_bts *bts;
	struct gsm48_system_information_type_3 *si3;
	int id;

	if (subsys != SS_GLOBAL)
		return -EINVAL;

	switch(signal) {
	case S_NEW_SYSINFO:
		bts = signal_data;
		if (!(bts->si_valid & (1 << SYSINFO_TYPE_3)))
			break;
		si3 = (struct gsm48_system_information_type_3 *)
						bts->si_buf[SYSINFO_TYPE_3];
		osmo_plmn_from_bcd(si3->lai.digits, &net->plmn);
		bts->location_area_code = ntohs(si3->lai.lac);
		bts->cell_identity = si3->cell_identity;
		avail_lai = 1;
		break;
	case S_NEW_NSE_ATTR:
		bts = signal_data;
		avail_nse = 1;
		break;
	case S_NEW_CELL_ATTR:
		bts = signal_data;
		avail_cell = 1;
		break;
	case S_NEW_NSVC_ATTR:
		nsvc = signal_data;
		id = nsvc->id;
		if (id < 0 || id > 1)
			return -EINVAL;
		avail_nsvc[id] = 1;
		break;
	case S_NEW_OP_STATE:
		break;
	default:
		return -EINVAL;
	}

	/* If all infos have been received, of if one info is updated after
	 * all infos have been received, transmit info update. */
	if (avail_lai && avail_nse && avail_cell && avail_nsvc[0])
		pcu_tx_info_ind();
	return 0;
}


int pcu_tx_rts_req(struct gsm_bts_trx_ts *ts, uint8_t is_ptcch, uint32_t fn,
	uint16_t arfcn, uint8_t block_nr)
{
	struct msgb *msg;
	struct gsm_pcu_if *pcu_prim;
	struct gsm_pcu_if_rts_req *rts_req;
	struct gsm_bts *bts = ts->trx->bts;

	LOGP(DPCU, LOGL_DEBUG, "Sending rts request: is_ptcch=%d arfcn=%d "
		"block=%d\n", is_ptcch, arfcn, block_nr);

	msg = pcu_msgb_alloc(PCU_IF_MSG_RTS_REQ, bts->nr);
	if (!msg)
		return -ENOMEM;
	pcu_prim = (struct gsm_pcu_if *) msg->data;
	rts_req = &pcu_prim->u.rts_req;

	rts_req->sapi = (is_ptcch) ? PCU_IF_SAPI_PTCCH : PCU_IF_SAPI_PDTCH;
	rts_req->fn = fn;
	rts_req->arfcn = arfcn;
	rts_req->trx_nr = ts->trx->nr;
	rts_req->ts_nr = ts->nr;
	rts_req->block_nr = block_nr;

	return pcu_sock_send(&bts_gsmnet, msg);
}

int pcu_tx_data_ind(struct gsm_bts_trx_ts *ts, uint8_t sapi, uint32_t fn,
	uint16_t arfcn, uint8_t block_nr, uint8_t *data, uint8_t len,
	int8_t rssi, uint16_t ber10k, int16_t bto, int16_t lqual)
{
	struct msgb *msg;
	struct gsm_pcu_if *pcu_prim;
	struct gsm_pcu_if_data *data_ind;
	struct gsm_bts *bts = ts->trx->bts;

	LOGP(DPCU, LOGL_DEBUG, "Sending data indication: sapi=%s arfcn=%d block=%d data=%s\n",
	     sapi_string[sapi], arfcn, block_nr, osmo_hexdump(data, len));

	if (lqual / 10 < bts->min_qual_norm) {
		LOGP(DPCU, LOGL_DEBUG, "Link quality %"PRId16" is below threshold %f, dropping packet\n",
			lqual, bts->min_qual_norm);
		return 0;
	}

	msg = pcu_msgb_alloc(PCU_IF_MSG_DATA_IND, bts->nr);
	if (!msg)
		return -ENOMEM;
	pcu_prim = (struct gsm_pcu_if *) msg->data;
	data_ind = &pcu_prim->u.data_ind;

	data_ind->sapi = sapi;
	data_ind->rssi = rssi;
	data_ind->fn = fn;
	data_ind->arfcn = arfcn;
	data_ind->trx_nr = ts->trx->nr;
	data_ind->ts_nr = ts->nr;
	data_ind->block_nr = block_nr;
	data_ind->ber10k = ber10k;
	data_ind->ta_offs_qbits = bto;
	data_ind->lqual_cb = lqual;
	memcpy(data_ind->data, data, len);
	data_ind->len = len;

	return pcu_sock_send(&bts_gsmnet, msg);
}

int pcu_tx_rach_ind(struct gsm_bts *bts, int16_t qta, uint16_t ra, uint32_t fn,
	uint8_t is_11bit, enum ph_burst_type burst_type)
{
	struct msgb *msg;
	struct gsm_pcu_if *pcu_prim;
	struct gsm_pcu_if_rach_ind *rach_ind;

	LOGP(DPCU, LOGL_INFO, "Sending RACH indication: qta=%d, ra=%d, "
		"fn=%d\n", qta, ra, fn);

	msg = pcu_msgb_alloc(PCU_IF_MSG_RACH_IND, bts->nr);
	if (!msg)
		return -ENOMEM;
	pcu_prim = (struct gsm_pcu_if *) msg->data;
	rach_ind = &pcu_prim->u.rach_ind;

	rach_ind->sapi = PCU_IF_SAPI_RACH;
	rach_ind->ra = ra;
	rach_ind->qta = qta;
	rach_ind->fn = fn;
	rach_ind->is_11bit = is_11bit;
	rach_ind->burst_type = burst_type;

	return pcu_sock_send(&bts_gsmnet, msg);
}

int pcu_tx_time_ind(uint32_t fn)
{
	struct msgb *msg;
	struct gsm_pcu_if *pcu_prim;
	struct gsm_pcu_if_time_ind *time_ind;
	uint8_t fn13 = fn % 13;

	/* omit frame numbers not starting at a MAC block */
	if (fn13 != 0 && fn13 != 4 && fn13 != 8)
		return 0;

	msg = pcu_msgb_alloc(PCU_IF_MSG_TIME_IND, 0);
	if (!msg)
		return -ENOMEM;
	pcu_prim = (struct gsm_pcu_if *) msg->data;
	time_ind = &pcu_prim->u.time_ind;

	time_ind->fn = fn;

	return pcu_sock_send(&bts_gsmnet, msg);
}

int pcu_tx_pag_req(const uint8_t *identity_lv, uint8_t chan_needed)
{
	struct pcu_sock_state *state = bts_gsmnet.pcu_state;
	struct msgb *msg;
	struct gsm_pcu_if *pcu_prim;
	struct gsm_pcu_if_pag_req *pag_req;

	/* check if identity does not fit: length > sizeof(lv) - 1 */
	if (identity_lv[0] >= sizeof(pag_req->identity_lv)) {
		LOGP(DPCU, LOGL_ERROR, "Paging identity too large (%d)\n",
			identity_lv[0]);
		return -EINVAL;
	}

	/* socket not created */
	if (!state) {
		LOGP(DPCU, LOGL_DEBUG, "PCU socket not created, ignoring "
			"paging message\n");
		return 0;
	}

	msg = pcu_msgb_alloc(PCU_IF_MSG_PAG_REQ, 0);
	if (!msg)
		return -ENOMEM;
	pcu_prim = (struct gsm_pcu_if *) msg->data;
	pag_req = &pcu_prim->u.pag_req;

	pag_req->chan_needed = chan_needed;
	memcpy(pag_req->identity_lv, identity_lv, identity_lv[0] + 1);

	return pcu_sock_send(&bts_gsmnet, msg);
}

int pcu_tx_pch_data_cnf(uint32_t fn, uint8_t *data, uint8_t len)
{
	struct gsm_network *net = &bts_gsmnet;
	struct gsm_bts *bts;
	struct msgb *msg;
	struct gsm_pcu_if *pcu_prim;
	struct gsm_pcu_if_data *data_cnf;

	/* FIXME: allow multiple BTS */
	bts = llist_entry(net->bts_list.next, struct gsm_bts, list);

	LOGP(DPCU, LOGL_INFO, "Sending PCH confirm\n");

	msg = pcu_msgb_alloc(PCU_IF_MSG_DATA_CNF, bts->nr);
	if (!msg)
		return -ENOMEM;
	pcu_prim = (struct gsm_pcu_if *) msg->data;
	data_cnf = &pcu_prim->u.data_cnf;

	data_cnf->sapi = PCU_IF_SAPI_PCH;
	data_cnf->fn = fn;
	memcpy(data_cnf->data, data, len);
	data_cnf->len = len;

	return pcu_sock_send(&bts_gsmnet, msg);
}

static int pcu_rx_data_req(struct gsm_bts *bts, uint8_t msg_type,
	struct gsm_pcu_if_data *data_req)
{
	uint8_t is_ptcch;
	struct gsm_bts_trx *trx;
	struct gsm_bts_trx_ts *ts;
	struct msgb *msg;
	int rc = 0;

	LOGP(DPCU, LOGL_DEBUG, "Data request received: sapi=%s arfcn=%d "
		"block=%d data=%s\n", sapi_string[data_req->sapi],
		data_req->arfcn, data_req->block_nr,
		osmo_hexdump(data_req->data, data_req->len));

	switch (data_req->sapi) {
	case PCU_IF_SAPI_PCH:
		if (msg_type == PCU_IF_MSG_PAG_REQ) {
			/* FIXME: Add function to schedule paging request.
			 * This might not be required, if PCU_IF_MSG_DATA_REQ
			 * is used instead. */
		} else {
			paging_add_imm_ass(bts->paging_state, data_req->data, data_req->len);
		}
		break;
	case PCU_IF_SAPI_AGCH:
		msg = msgb_alloc(data_req->len, "pcu_agch");
		if (!msg) {
			rc = -ENOMEM;
			break;
		}
		msg->l3h = msgb_put(msg, data_req->len);
		memcpy(msg->l3h, data_req->data, data_req->len);
		if (bts_agch_enqueue(bts, msg) < 0) {
			msgb_free(msg);
			rc = -EIO;
		}
		break;
	case PCU_IF_SAPI_PDTCH:
	case PCU_IF_SAPI_PTCCH:
		trx = gsm_bts_trx_num(bts, data_req->trx_nr);
		if (!trx) {
			LOGP(DPCU, LOGL_ERROR, "Received PCU data request with "
				"not existing TRX %d\n", data_req->trx_nr);
			rc = -EINVAL;
			break;
		}
		if (data_req->ts_nr >= ARRAY_SIZE(trx->ts)) {
			LOGP(DPCU, LOGL_ERROR, "Received PCU data request with "
				"not existing TS %u\n", data_req->ts_nr);
			rc = -EINVAL;
			break;
		}
		ts = &trx->ts[data_req->ts_nr];
		if (!ts_should_be_pdch(ts)) {
			LOGP(DPCU, LOGL_ERROR, "%s: Received PCU DATA request for non-PDCH TS\n",
				gsm_ts_name(ts));
			rc = -EINVAL;
			break;
		}
		if (ts->lchan[0].state != LCHAN_S_ACTIVE) {
			LOGP(DPCU, LOGL_ERROR, "%s: Received PCU DATA request for inactive lchan\n",
				gsm_ts_name(ts));
			rc = -EINVAL;
			break;
		}
		is_ptcch = (data_req->sapi == PCU_IF_SAPI_PTCCH);
		rc = l1sap_pdch_req(ts, is_ptcch, data_req->fn, data_req->arfcn,
			data_req->block_nr, data_req->data, data_req->len);
		break;
	default:
		LOGP(DPCU, LOGL_ERROR, "Received PCU data request with "
			"unsupported sapi %d\n", data_req->sapi);
		rc = -EINVAL;
	}

	return rc;
}

int pcu_tx_si13(const struct gsm_bts *bts, bool enable)
{
	/* the SI is per-BTS so it doesn't matter which TRX we use */
	struct gsm_bts_trx *trx = gsm_bts_trx_num(bts, 0);

	/* The low-level data like FN, ARFCN etc will be ignored but we have to set lqual high enough to bypass
	   the check at lower levels */
	int rc = pcu_tx_data_ind(&trx->ts[0], PCU_IF_SAPI_BCCH, 0, 0, 0, GSM_BTS_SI(bts, SYSINFO_TYPE_13),
				 enable ? GSM_MACBLOCK_LEN : 0, 0, 0, 0, INT16_MAX);
	if (rc < 0)
		LOGP(DPCU, LOGL_NOTICE, "Failed to send SI13 to PCU: %d\n", rc);

	return rc;
}

static int pcu_rx_txt_ind(struct gsm_bts *bts,
			  struct gsm_pcu_if_txt_ind *txt)
{
	switch (txt->type) {
	case PCU_VERSION:
		LOGP(DPCU, LOGL_INFO, "OsmoPCU version %s connected\n",
		     txt->text);
		osmo_signal_dispatch(SS_FAIL, OSMO_EVT_PCU_VERS, txt->text);
		osmo_strlcpy(bts->pcu_version, txt->text, MAX_VERSION_LENGTH);

		if (GSM_BTS_HAS_SI(bts, SYSINFO_TYPE_13))
			return pcu_tx_si13(bts, true);

		LOGP(DPCU, LOGL_INFO, "SI13 is not available on PCU connection\n");
		break;
	case PCU_OML_ALERT:
		osmo_signal_dispatch(SS_FAIL, OSMO_EVT_EXT_ALARM, txt->text);
		break;
	default:
		LOGP(DPCU, LOGL_ERROR, "Unknown TXT_IND type %u received\n",
		     txt->type);
		return -EINVAL;
	}

	return 0;
}

static int pcu_rx_act_req(struct gsm_bts *bts,
	struct gsm_pcu_if_act_req *act_req)
{
	struct gsm_bts_trx *trx;
	struct gsm_lchan *lchan;

	LOGP(DPCU, LOGL_INFO, "%s request received: TRX=%d TX=%d\n",
		(act_req->activate) ? "Activate" : "Deactivate",
		act_req->trx_nr, act_req->ts_nr);

	trx = gsm_bts_trx_num(bts, act_req->trx_nr);
	if (!trx || act_req->ts_nr >= 8)
		return -EINVAL;

	lchan = trx->ts[act_req->ts_nr].lchan;
	lchan->rel_act_kind = LCHAN_REL_ACT_PCU;
	if (lchan->type != GSM_LCHAN_PDTCH) {
		LOGP(DPCU, LOGL_ERROR,
		     "%s request, but lchan is not of type PDTCH (is %s)\n",
		     (act_req->activate) ? "Activate" : "Deactivate",
		     gsm_lchant_name(lchan->type));
		return -EINVAL;
	}
	if (act_req->activate)
		l1sap_chan_act(trx, gsm_lchan2chan_nr(lchan), NULL);
	else
		l1sap_chan_rel(trx, gsm_lchan2chan_nr(lchan));

	return 0;
}

static int pcu_rx(struct gsm_network *net, uint8_t msg_type,
	struct gsm_pcu_if *pcu_prim)
{
	int rc = 0;
	struct gsm_bts *bts;

	/* FIXME: allow multiple BTS */
	if (pcu_prim->bts_nr != 0) {
		LOGP(DPCU, LOGL_ERROR, "Received PCU Prim for non-existent BTS %u\n", pcu_prim->bts_nr);
		return -EINVAL;
	}
	bts = llist_entry(net->bts_list.next, struct gsm_bts, list);

	switch (msg_type) {
	case PCU_IF_MSG_DATA_REQ:
	case PCU_IF_MSG_PAG_REQ:
		rc = pcu_rx_data_req(bts, msg_type, &pcu_prim->u.data_req);
		break;
	case PCU_IF_MSG_ACT_REQ:
		rc = pcu_rx_act_req(bts, &pcu_prim->u.act_req);
		break;
	case PCU_IF_MSG_TXT_IND:
		rc = pcu_rx_txt_ind(bts, &pcu_prim->u.txt_ind);
		break;
	default:
		LOGP(DPCU, LOGL_ERROR, "Received unknwon PCU msg type %d\n",
			msg_type);
		rc = -EINVAL;
	}

	return rc;
}

/*
 * PCU socket interface
 */

struct pcu_sock_state {
	struct gsm_network *net;
	struct osmo_fd listen_bfd;	/* fd for listen socket */
	struct osmo_fd conn_bfd;	/* fd for connection to lcr */
	struct llist_head upqueue;	/* queue for sending messages */
};

static int pcu_sock_send(struct gsm_network *net, struct msgb *msg)
{
	struct pcu_sock_state *state = net->pcu_state;
	struct osmo_fd *conn_bfd;
	struct gsm_pcu_if *pcu_prim = (struct gsm_pcu_if *) msg->data;

	if (!state) {
		if (pcu_prim->msg_type != PCU_IF_MSG_TIME_IND)
			LOGP(DPCU, LOGL_INFO, "PCU socket not created, "
				"dropping message\n");
		msgb_free(msg);
		return -EINVAL;
	}
	conn_bfd = &state->conn_bfd;
	if (conn_bfd->fd <= 0) {
		if (pcu_prim->msg_type != PCU_IF_MSG_TIME_IND)
			LOGP(DPCU, LOGL_NOTICE, "PCU socket not connected, "
				"dropping message\n");
		msgb_free(msg);
		return -EIO;
	}
	msgb_enqueue(&state->upqueue, msg);
	conn_bfd->when |= BSC_FD_WRITE;

	return 0;
}

static void pcu_sock_close(struct pcu_sock_state *state)
{
	struct osmo_fd *bfd = &state->conn_bfd;
	struct gsm_bts *bts;
	struct gsm_bts_trx *trx;
	struct gsm_bts_trx_ts *ts;
	int i, j;

	/* FIXME: allow multiple BTS */
	bts = llist_entry(state->net->bts_list.next, struct gsm_bts, list);

	LOGP(DPCU, LOGL_NOTICE, "PCU socket has LOST connection\n");
	osmo_signal_dispatch(SS_FAIL, OSMO_EVT_PCU_VERS, NULL);
	bts->pcu_version[0] = '\0';

	close(bfd->fd);
	bfd->fd = -1;
	osmo_fd_unregister(bfd);

	/* re-enable the generation of ACCEPT for new connections */
	state->listen_bfd.when |= BSC_FD_READ;

#if 0
	/* remove si13, ... */
	bts->si_valid &= ~(1 << SYSINFO_TYPE_13);
	osmo_signal_dispatch(SS_GLOBAL, S_NEW_SYSINFO, bts);
#endif

	/* release PDCH */
	for (i = 0; i < 8; i++) {
		trx = gsm_bts_trx_num(bts, i);
		if (!trx)
			break;
		for (j = 0; j < 8; j++) {
			ts = &trx->ts[j];
			if (ts->mo.nm_state.operational == NM_OPSTATE_ENABLED
			 && ts->pchan == GSM_PCHAN_PDCH) {
				ts->lchan[0].rel_act_kind = LCHAN_REL_ACT_PCU;
				l1sap_chan_rel(trx,
					gsm_lchan2chan_nr(&ts->lchan[0]));
			}
		}
	}

	/* flush the queue */
	while (!llist_empty(&state->upqueue)) {
		struct msgb *msg = msgb_dequeue(&state->upqueue);
		msgb_free(msg);
	}
}

static int pcu_sock_read(struct osmo_fd *bfd)
{
	struct pcu_sock_state *state = (struct pcu_sock_state *)bfd->data;
	struct gsm_pcu_if *pcu_prim;
	struct msgb *msg;
	int rc;

	msg = msgb_alloc(sizeof(*pcu_prim), "pcu_sock_rx");
	if (!msg)
		return -ENOMEM;

	pcu_prim = (struct gsm_pcu_if *) msg->tail;

	rc = recv(bfd->fd, msg->tail, msgb_tailroom(msg), 0);
	if (rc == 0)
		goto close;

	if (rc < 0) {
		if (errno == EAGAIN)
			return 0;
		goto close;
	}

	if (rc < sizeof(*pcu_prim)) {
		LOGP(DPCU, LOGL_ERROR, "Received %d bytes on PCU Socket, but primitive size "
			"is %lu, discarding\n", rc, sizeof(*pcu_prim));
		return 0;
	}

	rc = pcu_rx(state->net, pcu_prim->msg_type, pcu_prim);

	/* as we always synchronously process the message in pcu_rx() and
	 * its callbacks, we can free the message here. */
	msgb_free(msg);

	return rc;

close:
	msgb_free(msg);
	pcu_sock_close(state);
	return -1;
}

static int pcu_sock_write(struct osmo_fd *bfd)
{
	struct pcu_sock_state *state = bfd->data;
	int rc;

	while (!llist_empty(&state->upqueue)) {
		struct msgb *msg, *msg2;
		struct gsm_pcu_if *pcu_prim;

		/* peek at the beginning of the queue */
		msg = llist_entry(state->upqueue.next, struct msgb, list);
		pcu_prim = (struct gsm_pcu_if *)msg->data;

		bfd->when &= ~BSC_FD_WRITE;

		/* bug hunter 8-): maybe someone forgot msgb_put(...) ? */
		if (!msgb_length(msg)) {
			LOGP(DPCU, LOGL_ERROR, "message type (%d) with ZERO "
				"bytes!\n", pcu_prim->msg_type);
			goto dontsend;
		}

		/* try to send it over the socket */
		rc = write(bfd->fd, msgb_data(msg), msgb_length(msg));
		if (rc == 0)
			goto close;
		if (rc < 0) {
			if (errno == EAGAIN) {
				bfd->when |= BSC_FD_WRITE;
				break;
			}
			goto close;
		}

dontsend:
		/* _after_ we send it, we can deueue */
		msg2 = msgb_dequeue(&state->upqueue);
		assert(msg == msg2);
		msgb_free(msg);
	}
	return 0;

close:
	pcu_sock_close(state);

	return -1;
}

static int pcu_sock_cb(struct osmo_fd *bfd, unsigned int flags)
{
	int rc = 0;

	if (flags & BSC_FD_READ)
		rc = pcu_sock_read(bfd);
	if (rc < 0)
		return rc;

	if (flags & BSC_FD_WRITE)
		rc = pcu_sock_write(bfd);

	return rc;
}

/* accept connection comming from PCU */
static int pcu_sock_accept(struct osmo_fd *bfd, unsigned int flags)
{
	struct pcu_sock_state *state = (struct pcu_sock_state *)bfd->data;
	struct osmo_fd *conn_bfd = &state->conn_bfd;
	struct sockaddr_un un_addr;
	socklen_t len;
	int rc;

	len = sizeof(un_addr);
	rc = accept(bfd->fd, (struct sockaddr *) &un_addr, &len);
	if (rc < 0) {
		LOGP(DPCU, LOGL_ERROR, "Failed to accept a new connection\n");
		return -1;
	}

	if (conn_bfd->fd >= 0) {
		LOGP(DPCU, LOGL_NOTICE, "PCU connects but we already have "
			"another active connection ?!?\n");
		/* We already have one PCU connected, this is all we support */
		state->listen_bfd.when &= ~BSC_FD_READ;
		close(rc);
		return 0;
	}

	conn_bfd->fd = rc;
	conn_bfd->when = BSC_FD_READ;
	conn_bfd->cb = pcu_sock_cb;
	conn_bfd->data = state;

	if (osmo_fd_register(conn_bfd) != 0) {
		LOGP(DPCU, LOGL_ERROR, "Failed to register new connection "
			"fd\n");
		close(conn_bfd->fd);
		conn_bfd->fd = -1;
		return -1;
	}

	LOGP(DPCU, LOGL_NOTICE, "PCU socket connected to external PCU\n");

	/* send current info */
	pcu_tx_info_ind();

	return 0;
}

int pcu_sock_init(const char *path)
{
	struct pcu_sock_state *state;
	struct osmo_fd *bfd;
	int rc;

	state = talloc_zero(NULL, struct pcu_sock_state);
	if (!state)
		return -ENOMEM;

	INIT_LLIST_HEAD(&state->upqueue);
	state->net = &bts_gsmnet;
	state->conn_bfd.fd = -1;

	bfd = &state->listen_bfd;

	bfd->fd = osmo_sock_unix_init(SOCK_SEQPACKET, 0, path,
		OSMO_SOCK_F_BIND);
	if (bfd->fd < 0) {
		LOGP(DPCU, LOGL_ERROR, "Could not create %s unix socket: %s\n",
		     path, strerror(errno));
		talloc_free(state);
		return -1;
	}

	bfd->when = BSC_FD_READ;
	bfd->cb = pcu_sock_accept;
	bfd->data = state;

	rc = osmo_fd_register(bfd);
	if (rc < 0) {
		LOGP(DPCU, LOGL_ERROR, "Could not register listen fd: %d\n",
			rc);
		close(bfd->fd);
		talloc_free(state);
		return rc;
	}

	osmo_signal_register_handler(SS_GLOBAL, pcu_if_signal_cb, NULL);

	bts_gsmnet.pcu_state = state;

	return 0;
}

void pcu_sock_exit(void)
{
	struct pcu_sock_state *state = bts_gsmnet.pcu_state;
	struct osmo_fd *bfd, *conn_bfd;

	if (!state)
		return;

	osmo_signal_unregister_handler(SS_GLOBAL, pcu_if_signal_cb, NULL);
	conn_bfd = &state->conn_bfd;
	if (conn_bfd->fd > 0)
		pcu_sock_close(state);
	bfd = &state->listen_bfd;
	close(bfd->fd);
	osmo_fd_unregister(bfd);
	talloc_free(state);
	bts_gsmnet.pcu_state = NULL;
}

bool pcu_connected(void) {
	struct gsm_network *net = &bts_gsmnet;
	struct pcu_sock_state *state = net->pcu_state;

	if (!state)
		return false;
	if (state->conn_bfd.fd <= 0)
		return false;
	return true;
}
