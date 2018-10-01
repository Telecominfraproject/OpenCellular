/* Interface handler for NuRAN Wireless OC-2G L1 */

/* Copyright (C) 2015 by Yves Godin <support@nuranwireless.com>
 * Copyright (C) 2016 by Harald Welte <laforge@gnumonks.org>
 * 
 * Based on sysmoBTS:
 *     (C) 2011-2014 by Harald Welte <laforge@gnumonks.org>
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

#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/utils.h>
#include <osmocom/core/select.h>
#include <osmocom/core/timer.h>
#include <osmocom/core/write_queue.h>
#include <osmocom/gsm/gsm_utils.h>
#include <osmocom/gsm/lapdm.h>

#include <osmo-bts/logging.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/oml.h>
#include <osmo-bts/rsl.h>
#include <osmo-bts/gsm_data.h>
#include <osmo-bts/phy_link.h>
#include <osmo-bts/paging.h>
#include <osmo-bts/measurement.h>
#include <osmo-bts/pcu_if.h>
#include <osmo-bts/handover.h>
#include <osmo-bts/cbch.h>
#include <osmo-bts/bts_model.h>
#include <osmo-bts/l1sap.h>
#include <osmo-bts/msg_utils.h>
#include <osmo-bts/dtx_dl_amr_fsm.h>

#include <nrw/oc2g/oc2g.h>
#include <nrw/oc2g/gsml1prim.h>
#include <nrw/oc2g/gsml1const.h>
#include <nrw/oc2g/gsml1types.h>

#include "oc2gbts.h"
#include "l1_if.h"
#include "l1_transp.h"
#include "hw_misc.h"
#include "misc/oc2gbts_par.h"
#include "misc/oc2gbts_bid.h"
#include "utils.h"
#include "osmo-bts/oml.h"

extern unsigned int dsp_trace;

struct wait_l1_conf {
	struct llist_head list;		/* internal linked list */
	struct osmo_timer_list timer;	/* timer for L1 timeout */
	unsigned int conf_prim_id;	/* primitive we expect in response */
	HANDLE conf_hLayer3;		/* layer 3 handle we expect in response */
	unsigned int is_sys_prim;	/* is this a system (1) or L1 (0) primitive */
	l1if_compl_cb *cb;
	void *cb_data;
};

static void release_wlc(struct wait_l1_conf *wlc)
{
	osmo_timer_del(&wlc->timer);
	talloc_free(wlc);
}

static void l1if_req_timeout(void *data)
{
	struct wait_l1_conf *wlc = data;

	if (wlc->is_sys_prim)
		LOGP(DL1C, LOGL_FATAL, "Timeout waiting for SYS primitive %s\n",
			get_value_string(oc2gbts_sysprim_names, wlc->conf_prim_id));
	else
		LOGP(DL1C, LOGL_FATAL, "Timeout waiting for L1 primitive %s\n",
			get_value_string(oc2gbts_l1prim_names, wlc->conf_prim_id));
	exit(23);
}

static HANDLE l1p_get_hLayer3(GsmL1_Prim_t *prim)
{
	switch (prim->id) {
	case GsmL1_PrimId_MphInitReq:
		return prim->u.mphInitReq.hLayer3;
	case GsmL1_PrimId_MphCloseReq:
		return prim->u.mphCloseReq.hLayer3;
	case GsmL1_PrimId_MphConnectReq:
		return prim->u.mphConnectReq.hLayer3;
	case GsmL1_PrimId_MphDisconnectReq:
		return prim->u.mphDisconnectReq.hLayer3;
	case GsmL1_PrimId_MphActivateReq:
		return prim->u.mphActivateReq.hLayer3;
	case GsmL1_PrimId_MphDeactivateReq:
		return prim->u.mphDeactivateReq.hLayer3;
	case GsmL1_PrimId_MphConfigReq:
		return prim->u.mphConfigReq.hLayer3;
	case GsmL1_PrimId_MphMeasureReq:
		return prim->u.mphMeasureReq.hLayer3;
	case GsmL1_PrimId_MphInitCnf:
		return prim->u.mphInitCnf.hLayer3;
	case GsmL1_PrimId_MphCloseCnf:
		return prim->u.mphCloseCnf.hLayer3;
	case GsmL1_PrimId_MphConnectCnf:
		return prim->u.mphConnectCnf.hLayer3;
	case GsmL1_PrimId_MphDisconnectCnf:
		return prim->u.mphDisconnectCnf.hLayer3;
	case GsmL1_PrimId_MphActivateCnf:
		return prim->u.mphActivateCnf.hLayer3;
	case GsmL1_PrimId_MphDeactivateCnf:
		return prim->u.mphDeactivateCnf.hLayer3;
	case GsmL1_PrimId_MphConfigCnf:
		return prim->u.mphConfigCnf.hLayer3;
	case GsmL1_PrimId_MphMeasureCnf:
		return prim->u.mphMeasureCnf.hLayer3;
	case GsmL1_PrimId_MphTimeInd:
	case GsmL1_PrimId_MphSyncInd:
	case GsmL1_PrimId_PhEmptyFrameReq:
	case GsmL1_PrimId_PhDataReq:
	case GsmL1_PrimId_PhConnectInd:
	case GsmL1_PrimId_PhReadyToSendInd:
	case GsmL1_PrimId_PhDataInd:
	case GsmL1_PrimId_PhRaInd:
		break;
	default:
		LOGP(DL1C, LOGL_ERROR, "unknown L1 primitive %u\n", prim->id);
		break;
	}
	return 0;
}

static int _l1if_req_compl(struct oc2gl1_hdl *fl1h, struct msgb *msg,
		   int is_system_prim, l1if_compl_cb *cb, void *data)
{
	struct wait_l1_conf *wlc;
	struct osmo_wqueue *wqueue;
	unsigned int timeout_secs;

	/* allocate new wsc and store reference to mutex and conf_id */
	wlc = talloc_zero(fl1h, struct wait_l1_conf);
	wlc->cb = cb;
	wlc->cb_data = data;

	/* Make sure we actually have received a REQUEST type primitive */
	if (is_system_prim == 0) {
		GsmL1_Prim_t *l1p = msgb_l1prim(msg);

		LOGP(DL1P, LOGL_DEBUG, "Tx L1 prim %s\n",
			get_value_string(oc2gbts_l1prim_names, l1p->id));

		if (oc2gbts_get_l1prim_type(l1p->id) != L1P_T_REQ) {
			LOGP(DL1C, LOGL_ERROR, "L1 Prim %s is not a Request!\n",
				get_value_string(oc2gbts_l1prim_names, l1p->id));
			talloc_free(wlc);
			return -EINVAL;
		}
		wlc->is_sys_prim = 0;
		wlc->conf_prim_id = oc2gbts_get_l1prim_conf(l1p->id);
		wlc->conf_hLayer3 = l1p_get_hLayer3(l1p);
		wqueue = &fl1h->write_q[MQ_L1_WRITE];
		timeout_secs = 30;
	} else {
		Oc2g_Prim_t *sysp = msgb_sysprim(msg);

		LOGP(DL1C, LOGL_DEBUG, "Tx SYS prim %s\n",
			get_value_string(oc2gbts_sysprim_names, sysp->id));

		if (oc2gbts_get_sysprim_type(sysp->id) != L1P_T_REQ) {
			LOGP(DL1C, LOGL_ERROR, "SYS Prim %s is not a Request!\n",
				get_value_string(oc2gbts_sysprim_names, sysp->id));
			talloc_free(wlc);
			return -EINVAL;
		}
		wlc->is_sys_prim = 1;
		wlc->conf_prim_id = oc2gbts_get_sysprim_conf(sysp->id);
		wqueue = &fl1h->write_q[MQ_SYS_WRITE];
		timeout_secs = 30;
	}

	/* enqueue the message in the queue and add wsc to list */
	if (osmo_wqueue_enqueue(wqueue, msg) != 0) {
		/* So we will get a timeout but the log message might help */
		LOGP(DL1C, LOGL_ERROR, "Write queue for %s full. dropping msg.\n",
			is_system_prim ? "system primitive" : "gsm");
		msgb_free(msg);
	}
	llist_add(&wlc->list, &fl1h->wlc_list);

	/* schedule a timer for timeout_secs seconds. If DSP fails to respond, we terminate */
	wlc->timer.data = wlc;
	wlc->timer.cb = l1if_req_timeout;
	osmo_timer_schedule(&wlc->timer, timeout_secs, 0);

	return 0;
}

/* send a request primitive to the L1 and schedule completion call-back */
int l1if_req_compl(struct oc2gl1_hdl *fl1h, struct msgb *msg,
		   l1if_compl_cb *cb, void *data)
{
	return _l1if_req_compl(fl1h, msg, 1, cb, data);
}

int l1if_gsm_req_compl(struct oc2gl1_hdl *fl1h, struct msgb *msg,
		   l1if_compl_cb *cb, void *data)
{
	return _l1if_req_compl(fl1h, msg, 0, cb, data);
}

/* allocate a msgb containing a GsmL1_Prim_t */
struct msgb *l1p_msgb_alloc(void)
{
	struct msgb *msg = msgb_alloc(sizeof(GsmL1_Prim_t), "l1_prim");

	if (msg)
		msg->l1h = msgb_put(msg, sizeof(GsmL1_Prim_t));

	return msg;
}

/* allocate a msgb containing a Oc2g_Prim_t */
struct msgb *sysp_msgb_alloc(void)
{
	struct msgb *msg = msgb_alloc(sizeof(Oc2g_Prim_t), "sys_prim");

	if (msg)
		msg->l1h = msgb_put(msg, sizeof(Oc2g_Prim_t));

	return msg;
}

static GsmL1_PhDataReq_t *
data_req_from_rts_ind(GsmL1_Prim_t *l1p,
		const GsmL1_PhReadyToSendInd_t *rts_ind)
{
	GsmL1_PhDataReq_t *data_req = &l1p->u.phDataReq;

	l1p->id = GsmL1_PrimId_PhDataReq;

	/* copy fields from PH-RSS.ind */
	data_req->hLayer1	= rts_ind->hLayer1;
	data_req->u8Tn 		= rts_ind->u8Tn;
	data_req->u32Fn		= rts_ind->u32Fn;
	data_req->sapi		= rts_ind->sapi;
	data_req->subCh		= rts_ind->subCh;
	data_req->u8BlockNbr	= rts_ind->u8BlockNbr;

	return data_req;
}

static GsmL1_PhEmptyFrameReq_t *
empty_req_from_rts_ind(GsmL1_Prim_t *l1p,
			const GsmL1_PhReadyToSendInd_t *rts_ind)
{
	GsmL1_PhEmptyFrameReq_t *empty_req = &l1p->u.phEmptyFrameReq;

	l1p->id = GsmL1_PrimId_PhEmptyFrameReq;

	empty_req->hLayer1 = rts_ind->hLayer1;
	empty_req->u8Tn = rts_ind->u8Tn;
	empty_req->u32Fn = rts_ind->u32Fn;
	empty_req->sapi = rts_ind->sapi;
	empty_req->subCh = rts_ind->subCh;
	empty_req->u8BlockNbr = rts_ind->u8BlockNbr;

	return empty_req;
}

/* fill PH-DATA.req from l1sap primitive */
static GsmL1_PhDataReq_t *
data_req_from_l1sap(GsmL1_Prim_t *l1p, struct oc2gl1_hdl *fl1,
		uint8_t tn, uint32_t fn, uint8_t sapi, uint8_t sub_ch,
		uint8_t block_nr, uint8_t len)
{
	GsmL1_PhDataReq_t *data_req = &l1p->u.phDataReq;

	l1p->id = GsmL1_PrimId_PhDataReq;

	/* copy fields from PH-RSS.ind */
	data_req->hLayer1	= (HANDLE)fl1->hLayer1;
	data_req->u8Tn 		= tn;
	data_req->u32Fn		= fn;
	data_req->sapi		= sapi;
	data_req->subCh		= sub_ch;
	data_req->u8BlockNbr	= block_nr;

	data_req->msgUnitParam.u8Size = len;

	return data_req;
}

/* fill PH-EMPTY_FRAME.req from l1sap primitive */
static GsmL1_PhEmptyFrameReq_t *
empty_req_from_l1sap(GsmL1_Prim_t *l1p, struct oc2gl1_hdl *fl1,
		     uint8_t tn, uint32_t fn, uint8_t sapi,
		     uint8_t subch, uint8_t block_nr)
{
	GsmL1_PhEmptyFrameReq_t *empty_req = &l1p->u.phEmptyFrameReq;

	l1p->id = GsmL1_PrimId_PhEmptyFrameReq;

	empty_req->hLayer1 = (HANDLE)fl1->hLayer1;
	empty_req->u8Tn = tn;
	empty_req->u32Fn = fn;
	empty_req->sapi = sapi;
	empty_req->subCh = subch;
	empty_req->u8BlockNbr = block_nr;

	return empty_req;
}

/* fill frame PH-DATA.req from l1sap primitive */
static GsmL1_PhDataReq_t *
fill_req_from_l1sap(GsmL1_Prim_t *l1p, struct oc2gl1_hdl *fl1,
		uint8_t tn, uint32_t fn, uint8_t sapi, uint8_t sub_ch,
		uint8_t block_nr)
{
	GsmL1_PhDataReq_t *data_req = &l1p->u.phDataReq;
	GsmL1_MsgUnitParam_t *msu_param;
	uint8_t *l1_payload;

	msu_param = &data_req->msgUnitParam;
	l1_payload = &msu_param->u8Buffer[0];
	l1p->id = GsmL1_PrimId_PhDataReq;

	memset(l1_payload, 0x2B, GSM_MACBLOCK_LEN);
	/* address field */
	l1_payload[0] = 0x03;
	/* control field */
	l1_payload[1] = 0x03;
	/* length field */
	l1_payload[2] = 0x01;

	/* copy fields from PH-RTS.ind */
	data_req->hLayer1	= (HANDLE)fl1->hLayer1;
	data_req->u8Tn 		= tn;
	data_req->u32Fn		= fn;
	data_req->sapi		= sapi;
	data_req->subCh		= sub_ch;
	data_req->u8BlockNbr	= block_nr;
	data_req->msgUnitParam.u8Size = GSM_MACBLOCK_LEN;


	LOGP(DL1C, LOGL_DEBUG, "Send fill frame on in none DTX mode Tn=%d, Fn=%d, SAPI=%d, SubCh=%d, BlockNr=%d dump=%s\n",
			tn,
			fn,
			sapi,
			sub_ch,
			block_nr,
			osmo_hexdump(data_req->msgUnitParam.u8Buffer, data_req->msgUnitParam.u8Size));

	return data_req;
}

static int ph_data_req(struct gsm_bts_trx *trx, struct msgb *msg,
		       struct osmo_phsap_prim *l1sap, bool use_cache)
{
	struct oc2gl1_hdl *fl1 = trx_oc2gl1_hdl(trx);
	struct msgb *l1msg = l1p_msgb_alloc();
	struct gsm_lchan *lchan;
	uint32_t u32Fn;
	uint8_t u8Tn, subCh, u8BlockNbr = 0, sapi = 0;
	uint8_t chan_nr, link_id;
	int len;

	if (!msg) {
		LOGPFN(DL1C, LOGL_FATAL, l1sap->u.data.fn, "PH-DATA.req without msg. Please fix!\n");
		abort();
	}

	len = msgb_l2len(msg);

	chan_nr = l1sap->u.data.chan_nr;
	link_id = l1sap->u.data.link_id;
	u32Fn = l1sap->u.data.fn;
	u8Tn = L1SAP_CHAN2TS(chan_nr);
	subCh = 0x1f;
	lchan = get_lchan_by_chan_nr(trx, chan_nr);
	if (L1SAP_IS_LINK_SACCH(link_id)) {
		sapi = GsmL1_Sapi_Sacch;
		if (!L1SAP_IS_CHAN_TCHF(chan_nr) && !L1SAP_IS_CHAN_PDCH(chan_nr))
			subCh = l1sap_chan2ss(chan_nr);
	} else if (L1SAP_IS_CHAN_TCHF(chan_nr) || L1SAP_IS_CHAN_PDCH(chan_nr)) {
		if (ts_is_pdch(&trx->ts[u8Tn])) {
			if (L1SAP_IS_PTCCH(u32Fn)) {
				sapi = GsmL1_Sapi_Ptcch;
				u8BlockNbr = L1SAP_FN2PTCCHBLOCK(u32Fn);
			} else {
				sapi = GsmL1_Sapi_Pdtch;
				u8BlockNbr = L1SAP_FN2MACBLOCK(u32Fn);
			}
		} else {
			sapi = GsmL1_Sapi_FacchF;
			u8BlockNbr = (u32Fn % 13) >> 2;
		}
	} else if (L1SAP_IS_CHAN_TCHH(chan_nr)) {
		subCh = L1SAP_CHAN2SS_TCHH(chan_nr);
		sapi = GsmL1_Sapi_FacchH;
		u8BlockNbr = (u32Fn % 26) >> 3;
	} else if (L1SAP_IS_CHAN_SDCCH4(chan_nr)) {
		subCh = L1SAP_CHAN2SS_SDCCH4(chan_nr);
		sapi = GsmL1_Sapi_Sdcch;
	} else if (L1SAP_IS_CHAN_SDCCH8(chan_nr)) {
		subCh = L1SAP_CHAN2SS_SDCCH8(chan_nr);
		sapi = GsmL1_Sapi_Sdcch;
	} else if (L1SAP_IS_CHAN_BCCH(chan_nr)) {
		sapi = GsmL1_Sapi_Bcch;
	} else if (L1SAP_IS_CHAN_AGCH_PCH(chan_nr)) {
		/* The sapi depends on DSP configuration, not
		 * on the actual SYSTEM INFORMATION 3. */
		u8BlockNbr = L1SAP_FN2CCCHBLOCK(u32Fn);
		if (u8BlockNbr >= num_agch(trx, "PH-DATA-REQ"))
			sapi = GsmL1_Sapi_Pch;
		else
			sapi = GsmL1_Sapi_Agch;

		LOGP(DL1C, LOGL_DEBUG, "PH-DATA.req on %s Fn=%d, Tn=%d, BlockNr=%d, SAPI=%d\n",
				u8BlockNbr >= num_agch(trx, "PH-DATA-REQ") ? "PCH" : "AGCH",
				u32Fn,
				u8Tn,
				u8BlockNbr,
				sapi);
	} else {
		LOGPFN(DL1C, LOGL_NOTICE, u32Fn, "unknown prim %d op %d "
			"chan_nr %d link_id %d\n", l1sap->oph.primitive,
			l1sap->oph.operation, chan_nr, link_id);
		msgb_free(l1msg);
		return -EINVAL;
	}

	/* convert l1sap message to GsmL1 primitive, keep payload */
	if (len) {
		/* data request */
		GsmL1_Prim_t *l1p = msgb_l1prim(l1msg);
		data_req_from_l1sap(l1p, fl1, u8Tn, u32Fn, sapi, subCh, u8BlockNbr, len);
		if (use_cache)
			memcpy(l1p->u.phDataReq.msgUnitParam.u8Buffer,
			       lchan->tch.dtx.facch, msgb_l2len(msg));
		else if (dtx_dl_amr_enabled(lchan) &&
			 ((lchan->tch.dtx.dl_amr_fsm->state == ST_ONSET_F) ||
			 (lchan->tch.dtx.dl_amr_fsm->state == ST_U_INH_F) ||
			 (lchan->tch.dtx.dl_amr_fsm->state == ST_F1_INH_F))) {
			if (sapi == GsmL1_Sapi_FacchF) {
				sapi = GsmL1_Sapi_TchF;
			}
			if (sapi == GsmL1_Sapi_FacchH) {
				sapi = GsmL1_Sapi_TchH;
				subCh = L1SAP_CHAN2SS_TCHH(chan_nr);
				u8BlockNbr = (u32Fn % 13) >> 2;
			}
			if (sapi == GsmL1_Sapi_TchH || sapi == GsmL1_Sapi_TchF) {
				/* FACCH interruption of DTX silence */
				/* cache FACCH data */
				memcpy(lchan->tch.dtx.facch, msg->l2h,
				       msgb_l2len(msg));
				/* prepare ONSET or INH message */
				if(lchan->tch.dtx.dl_amr_fsm->state == ST_ONSET_F)
					l1p->u.phDataReq.msgUnitParam.u8Buffer[0] =
								GsmL1_TchPlType_Amr_Onset;
				else if(lchan->tch.dtx.dl_amr_fsm->state == ST_U_INH_F)
					l1p->u.phDataReq.msgUnitParam.u8Buffer[0] =
								GsmL1_TchPlType_Amr_SidUpdateInH;
				else if(lchan->tch.dtx.dl_amr_fsm->state == ST_F1_INH_F)
					l1p->u.phDataReq.msgUnitParam.u8Buffer[0] =
								GsmL1_TchPlType_Amr_SidFirstInH;
				/* ignored CMR/CMI pair */
				l1p->u.phDataReq.msgUnitParam.u8Buffer[1] = 0;
				l1p->u.phDataReq.msgUnitParam.u8Buffer[2] = 0;
				/* update length */
				data_req_from_l1sap(l1p, fl1, u8Tn, u32Fn, sapi,
						    subCh, u8BlockNbr, 3);
				/* update FN so it can be checked by TCH silence
				   resume handler */
				lchan->tch.dtx.fn = LCHAN_FN_DUMMY;
			}
		} else if (dtx_dl_amr_enabled(lchan) &&
			   lchan->tch.dtx.dl_amr_fsm->state == ST_FACCH) {
			/* update FN so it can be checked by TCH silence
			   resume handler */
			lchan->tch.dtx.fn = LCHAN_FN_DUMMY;
		}
		else {
			OSMO_ASSERT(msgb_l2len(msg) <= sizeof(l1p->u.phDataReq.msgUnitParam.u8Buffer));
			memcpy(l1p->u.phDataReq.msgUnitParam.u8Buffer, msg->l2h,
			       msgb_l2len(msg));
		}
		LOGPFN(DL1P, LOGL_DEBUG, u32Fn, "PH-DATA.req(%s)\n",
		     osmo_hexdump(l1p->u.phDataReq.msgUnitParam.u8Buffer,
					  l1p->u.phDataReq.msgUnitParam.u8Size));
	} else {

		GsmL1_Prim_t *l1p = msgb_l1prim(l1msg);
		if (lchan->rsl_cmode == RSL_CMOD_SPD_SIGN)
			/* fill frame */
			fill_req_from_l1sap(l1p, fl1, u8Tn, u32Fn, sapi, subCh, u8BlockNbr);
		else {
			if (lchan->ts->trx->bts->dtxd)
				/* empty frame */
				empty_req_from_l1sap(l1p, fl1, u8Tn, u32Fn, sapi, subCh, u8BlockNbr);
			else
				/* fill frame */
				fill_req_from_l1sap(l1p, fl1, u8Tn, u32Fn, sapi, subCh, u8BlockNbr);
		}
	}

	/* send message to DSP's queue */
	if (osmo_wqueue_enqueue(&fl1->write_q[MQ_L1_WRITE], l1msg) != 0) {
		LOGPFN(DL1P, LOGL_ERROR, u32Fn, "MQ_L1_WRITE queue full. Dropping msg.\n");
		msgb_free(l1msg);
	} else
		dtx_int_signal(lchan);

	if (dtx_recursion(lchan))
		ph_data_req(trx, msg, l1sap, true);
	return 0;
}

static int ph_tch_req(struct gsm_bts_trx *trx, struct msgb *msg,
		      struct osmo_phsap_prim *l1sap, bool use_cache, bool marker)
{
	struct oc2gl1_hdl *fl1 = trx_oc2gl1_hdl(trx);
	struct gsm_lchan *lchan;
	uint32_t u32Fn;
	uint8_t u8Tn, subCh, u8BlockNbr = 0, sapi;
	uint8_t chan_nr;
	GsmL1_Prim_t *l1p;
	struct msgb *nmsg = NULL;
	int rc = -1;

	chan_nr = l1sap->u.tch.chan_nr;
	u32Fn = l1sap->u.tch.fn;
	u8Tn = L1SAP_CHAN2TS(chan_nr);
	u8BlockNbr = (u32Fn % 13) >> 2;
	if (L1SAP_IS_CHAN_TCHH(chan_nr)) {
		subCh = L1SAP_CHAN2SS_TCHH(chan_nr);
		sapi = GsmL1_Sapi_TchH;
	} else {
		subCh = 0x1f;
		sapi = GsmL1_Sapi_TchF;
	}

	lchan = get_lchan_by_chan_nr(trx, chan_nr);

	/* create new message and fill data */
	if (msg) {
		msgb_pull(msg, sizeof(*l1sap));
		/* create new message */
		nmsg = l1p_msgb_alloc();
		if (!nmsg)
			return -ENOMEM;
		l1p = msgb_l1prim(nmsg);
		rc = l1if_tch_encode(lchan,
				     l1p->u.phDataReq.msgUnitParam.u8Buffer,
				     &l1p->u.phDataReq.msgUnitParam.u8Size,
				     msg->data, msg->len, u32Fn, use_cache,
				     l1sap->u.tch.marker);
		if (rc < 0) {
		/* no data encoded for L1: smth will be generated below */
			msgb_free(nmsg);
			nmsg = NULL;
		}
	}

	/* no message/data, we might generate an empty traffic msg or re-send
	   cached SID in case of DTX */
	if (!nmsg)
		nmsg = gen_empty_tch_msg(lchan, u32Fn);

	/* no traffic message, we generate an empty msg */
	if (!nmsg) {
		nmsg = l1p_msgb_alloc();
		if (!nmsg)
			return -ENOMEM;
	}

	l1p = msgb_l1prim(nmsg);

	/* if we provide data, or if data is already in nmsg */
	if (l1p->u.phDataReq.msgUnitParam.u8Size) {
		/* data request */
		data_req_from_l1sap(l1p, fl1, u8Tn, u32Fn, sapi, subCh,
				    u8BlockNbr,
				    l1p->u.phDataReq.msgUnitParam.u8Size);
	} else {
		/* empty frame */
		if (trx->bts->dtxd && trx != trx->bts->c0)
			lchan->tch.dtx.dl_active = true;
		empty_req_from_l1sap(l1p, fl1, u8Tn, u32Fn, sapi, subCh, u8BlockNbr);
	}
	/* send message to DSP's queue */
	osmo_wqueue_enqueue(&fl1->write_q[MQ_L1_WRITE], nmsg);
	if (dtx_is_first_p1(lchan))
		dtx_dispatch(lchan, E_FIRST);
	else
		dtx_int_signal(lchan);

	if (dtx_recursion(lchan)) /* DTX: send voice after ONSET was sent */
		return ph_tch_req(trx, l1sap->oph.msg, l1sap, true, false);

	return 0;
}

static int mph_info_req(struct gsm_bts_trx *trx, struct msgb *msg,
		        struct osmo_phsap_prim *l1sap)
{
	struct oc2gl1_hdl *fl1 = trx_oc2gl1_hdl(trx);
	uint8_t chan_nr;
	struct gsm_lchan *lchan;
	int rc = 0;

	switch (l1sap->u.info.type) {
	case PRIM_INFO_ACT_CIPH:
		chan_nr = l1sap->u.info.u.ciph_req.chan_nr;
		lchan = get_lchan_by_chan_nr(trx, chan_nr);
		if (l1sap->u.info.u.ciph_req.uplink) {
			l1if_set_ciphering(fl1, lchan, 0);
			lchan->ciph_state = LCHAN_CIPH_RX_REQ;
		}
		if (l1sap->u.info.u.ciph_req.downlink) {
			l1if_set_ciphering(fl1, lchan, 1);
			lchan->ciph_state = LCHAN_CIPH_RX_CONF_TX_REQ;
		}
		if (l1sap->u.info.u.ciph_req.downlink
		 && l1sap->u.info.u.ciph_req.uplink)
			lchan->ciph_state = LCHAN_CIPH_RXTX_REQ;
		break;
	case PRIM_INFO_ACTIVATE:
	case PRIM_INFO_DEACTIVATE:
	case PRIM_INFO_MODIFY:
		chan_nr = l1sap->u.info.u.act_req.chan_nr;
		lchan = get_lchan_by_chan_nr(trx, chan_nr);
		if (l1sap->u.info.type == PRIM_INFO_ACTIVATE)
			l1if_rsl_chan_act(lchan);
		else if (l1sap->u.info.type == PRIM_INFO_MODIFY) {
			if (lchan->ho.active == HANDOVER_WAIT_FRAME)
				l1if_rsl_chan_mod(lchan);
			else
				l1if_rsl_mode_modify(lchan);
		} else if (l1sap->u.info.u.act_req.sacch_only)
			l1if_rsl_deact_sacch(lchan);
		else
			l1if_rsl_chan_rel(lchan);
		break;
	default:
		LOGP(DL1C, LOGL_NOTICE, "unknown MPH-INFO.req %d\n",
				l1sap->u.info.type);
		/* TODO (oramadan): Fix OML alarms
		alarm_sig_data.mo = &trx->mo;
		memcpy(alarm_sig_data.spare, &l1sap->u.info.type, sizeof(unsigned int));
		osmo_signal_dispatch(SS_NM, S_NM_OML_BTS_UNKN_MPH_INFO_REQ_ALARM, &alarm_sig_data);
		*/
		rc = -EINVAL;
	}

	return rc;
}

/* primitive from common part */
int bts_model_l1sap_down(struct gsm_bts_trx *trx, struct osmo_phsap_prim *l1sap)
{
	struct msgb *msg = l1sap->oph.msg;
	int rc = 0;

	/* called functions MUST NOT take ownership of msgb, as it is
	 * free()d below */
	switch (OSMO_PRIM_HDR(&l1sap->oph)) {
	case OSMO_PRIM(PRIM_PH_DATA, PRIM_OP_REQUEST):
		rc = ph_data_req(trx, msg, l1sap, false);
		break;
	case OSMO_PRIM(PRIM_TCH, PRIM_OP_REQUEST):
		rc = ph_tch_req(trx, msg, l1sap, false, l1sap->u.tch.marker);
		break;
	case OSMO_PRIM(PRIM_MPH_INFO, PRIM_OP_REQUEST):
		rc = mph_info_req(trx, msg, l1sap);
		break;
	default:
		LOGP(DL1C, LOGL_NOTICE, "unknown prim %d op %d\n",
			l1sap->oph.primitive, l1sap->oph.operation);

		/* TODO (oramadan): Fix OML alarms
		alarm_sig_data.mo = &trx->mo;
		memcpy(alarm_sig_data.spare, &l1sap->oph.primitive, sizeof(unsigned int));
		osmo_signal_dispatch(SS_NM, S_NM_OML_BTS_RX_UNKN_L1SAP_DOWN_MSG_ALARM, &alarm_sig_data);
		*/
		rc = -EINVAL;
	}

	msgb_free(msg);

	return rc;
}

static int handle_mph_time_ind(struct oc2gl1_hdl *fl1,
				GsmL1_MphTimeInd_t *time_ind,
				struct msgb *msg)
{
	struct gsm_bts_trx *trx = oc2gl1_hdl_trx(fl1);
	struct gsm_bts *bts = trx->bts;
	struct osmo_phsap_prim l1sap;
	uint32_t fn;

	/* increment the primitive count for the alive timer */
	fl1->alive_prim_cnt++;

	/* ignore every time indication, except for c0 */
	if (trx != bts->c0) {
		msgb_free(msg);
		return 0;
	}

	fn = time_ind->u32Fn;

	memset(&l1sap, 0, sizeof(l1sap));
	osmo_prim_init(&l1sap.oph, SAP_GSM_PH, PRIM_MPH_INFO,
		PRIM_OP_INDICATION, NULL);
	l1sap.u.info.type = PRIM_INFO_TIME;
	l1sap.u.info.u.time_ind.fn = fn;

	msgb_free(msg);

	return l1sap_up(trx, &l1sap);
}

static enum gsm_phys_chan_config pick_pchan(struct gsm_bts_trx_ts *ts)
{
	switch (ts->pchan) {
	case GSM_PCHAN_TCH_F_PDCH:
		if (ts->flags & TS_F_PDCH_ACTIVE)
			return GSM_PCHAN_PDCH;
		return GSM_PCHAN_TCH_F;
	case GSM_PCHAN_TCH_F_TCH_H_PDCH:
		return ts->dyn.pchan_is;
	default:
		return ts->pchan;
	}
}

static uint8_t chan_nr_by_sapi(struct gsm_bts_trx_ts *ts,
			       GsmL1_Sapi_t sapi, GsmL1_SubCh_t subCh,
			       uint8_t u8Tn, uint32_t u32Fn)
{
	uint8_t cbits = 0;
	enum gsm_phys_chan_config pchan = pick_pchan(ts);
	OSMO_ASSERT(pchan != GSM_PCHAN_TCH_F_PDCH);
	OSMO_ASSERT(pchan != GSM_PCHAN_TCH_F_TCH_H_PDCH);

	switch (sapi) {
	case GsmL1_Sapi_Bcch:
		cbits = 0x10;
		break;
	case GsmL1_Sapi_Sacch:
		switch(pchan) {
		case GSM_PCHAN_TCH_F:
			cbits = 0x01;
			break;
		case GSM_PCHAN_TCH_H:
			cbits = 0x02 + subCh;
			break;
		case GSM_PCHAN_CCCH_SDCCH4:
			cbits = 0x04 + subCh;
			break;
		case GSM_PCHAN_SDCCH8_SACCH8C:
			cbits = 0x08 + subCh;
			break;
		default:
			LOGP(DL1C, LOGL_ERROR, "SACCH for pchan %d?\n",
				pchan);
			return 0;
		}
		break;
	case GsmL1_Sapi_Sdcch:
		switch(pchan) {
		case GSM_PCHAN_CCCH_SDCCH4:
			cbits = 0x04 + subCh;
			break;
		case GSM_PCHAN_SDCCH8_SACCH8C:
			cbits = 0x08 + subCh;
			break;
		default:
			LOGP(DL1C, LOGL_ERROR, "SDCCH for pchan %d?\n",
				pchan);
			return 0;
		}
		break;
	case GsmL1_Sapi_Agch:
	case GsmL1_Sapi_Pch:
		cbits = 0x12;
		break;
	case GsmL1_Sapi_Pdtch:
	case GsmL1_Sapi_Pacch:
		switch(pchan) {
		case GSM_PCHAN_PDCH:
			cbits = 0x01;
			break;
		default:
			LOGP(DL1C, LOGL_ERROR, "PDTCH for pchan %d?\n",
				pchan);
			return 0;
		}
		break;
	case GsmL1_Sapi_TchF:
		cbits = 0x01;
		break;
	case GsmL1_Sapi_TchH:
		cbits = 0x02 + subCh;
		break;
	case GsmL1_Sapi_FacchF:
		cbits = 0x01;
		break;
	case GsmL1_Sapi_FacchH:
		cbits = 0x02 + subCh;
		break;
	case GsmL1_Sapi_Ptcch:
		if (!L1SAP_IS_PTCCH(u32Fn)) {
			LOGP(DL1C, LOGL_FATAL, "Not expecting PTCCH at frame "
				"number other than 12, got it at %u (%u). "
				"Please fix!\n", u32Fn % 52, u32Fn);
			abort();
		}
		switch(pchan) {
		case GSM_PCHAN_PDCH:
			cbits = 0x01;
			break;
		default:
			LOGP(DL1C, LOGL_ERROR, "PTCCH for pchan %d?\n",
				pchan);
			return 0;
		}
		break;
	default:
		return 0;
	}

	/* not reached due to default case above */
	return (cbits << 3) | u8Tn;
}

static int handle_ph_readytosend_ind(struct oc2gl1_hdl *fl1,
				     GsmL1_PhReadyToSendInd_t *rts_ind,
				     struct msgb *l1p_msg)
{
	struct gsm_bts_trx *trx = oc2gl1_hdl_trx(fl1);
	struct gsm_bts *bts = trx->bts;
	struct msgb *resp_msg;
	GsmL1_PhDataReq_t *data_req;
	GsmL1_MsgUnitParam_t *msu_param;
	struct gsm_time g_time;
	uint32_t t3p;
	int rc;
	struct osmo_phsap_prim *l1sap;
	uint8_t chan_nr, link_id;
	uint32_t fn;

	/* check if primitive should be handled by common part */
	chan_nr = chan_nr_by_sapi(&trx->ts[rts_ind->u8Tn], rts_ind->sapi,
		rts_ind->subCh, rts_ind->u8Tn, rts_ind->u32Fn);
	if (chan_nr) {
		fn = rts_ind->u32Fn;
		if (rts_ind->sapi == GsmL1_Sapi_Sacch)
			link_id = LID_SACCH;
		else
			link_id = LID_DEDIC;
		/* recycle the msgb and use it for the L1 primitive,
		 * which means that we (or our caller) must not free it */
		rc = msgb_trim(l1p_msg, sizeof(*l1sap));
		if (rc < 0)
			MSGB_ABORT(l1p_msg, "No room for primitive\n");
		l1sap = msgb_l1sap_prim(l1p_msg);
		if (rts_ind->sapi == GsmL1_Sapi_TchF
		 || rts_ind->sapi == GsmL1_Sapi_TchH) {
			osmo_prim_init(&l1sap->oph, SAP_GSM_PH, PRIM_TCH_RTS,
				PRIM_OP_INDICATION, l1p_msg);
			l1sap->u.tch.chan_nr = chan_nr;
			l1sap->u.tch.fn = fn;
		} else {
			osmo_prim_init(&l1sap->oph, SAP_GSM_PH, PRIM_PH_RTS,
				PRIM_OP_INDICATION, l1p_msg);
			l1sap->u.data.link_id = link_id;
			l1sap->u.data.chan_nr = chan_nr;
			l1sap->u.data.fn = fn;
		}

		return l1sap_up(trx, l1sap);
	}

	gsm_fn2gsmtime(&g_time, rts_ind->u32Fn);

	DEBUGPGT(DL1P, &g_time, "Rx PH-RTS.ind SAPI=%s\n",
		get_value_string(oc2gbts_l1sapi_names, rts_ind->sapi));

	/* in all other cases, we need to allocate a new PH-DATA.ind
	 * primitive msgb and start to fill it */
	resp_msg = l1p_msgb_alloc();
	data_req = data_req_from_rts_ind(msgb_l1prim(resp_msg), rts_ind);
	msu_param = &data_req->msgUnitParam;

	/* set default size */
	msu_param->u8Size = GSM_MACBLOCK_LEN;

	switch (rts_ind->sapi) {
	case GsmL1_Sapi_Sch:
		/* compute T3prime */
		t3p = (g_time.t3 - 1) / 10;
		/* fill SCH burst with data */
		msu_param->u8Size = 4;
		msu_param->u8Buffer[0] = (bts->bsic << 2) | (g_time.t1 >> 9);
		msu_param->u8Buffer[1] = (g_time.t1 >> 1);
		msu_param->u8Buffer[2] = (g_time.t1 << 7) | (g_time.t2 << 2) | (t3p >> 1);
		msu_param->u8Buffer[3] = (t3p & 1);
		break;
	case GsmL1_Sapi_Prach:
		goto empty_frame;
		break;
	case GsmL1_Sapi_Cbch:
		/* get them from bts->si_buf[] */
		bts_cbch_get(bts, msu_param->u8Buffer, &g_time);
		break;
	default:
		memcpy(msu_param->u8Buffer, fill_frame, GSM_MACBLOCK_LEN);
		break;
	}
tx:

	/* transmit */
	if (osmo_wqueue_enqueue(&fl1->write_q[MQ_L1_WRITE], resp_msg) != 0) {
		LOGPGT(DL1C, LOGL_ERROR, &g_time, "MQ_L1_WRITE queue full. Dropping msg.\n");
		msgb_free(resp_msg);
	}

	/* free the msgb, as we have not handed it to l1sap and thus
	 * need to release its memory */
	msgb_free(l1p_msg);
	return 0;

empty_frame:
	/* in case we decide to send an empty frame... */
	empty_req_from_rts_ind(msgb_l1prim(resp_msg), rts_ind);

	goto tx;
}

static void dump_meas_res(int ll, GsmL1_MeasParam_t *m)
{
	LOGPC(DL1C, ll, ", Meas: RSSI %-3.2f dBm,  Qual %-3.2f dB,  "
		"BER %-3.2f,  Timing %d\n", m->fRssi, m->fLinkQuality,
		m->fBer, m->i16BurstTiming);
}

static int process_meas_res(struct gsm_bts_trx *trx, uint8_t chan_nr,
				GsmL1_MeasParam_t *m, uint32_t fn)
{
	struct osmo_phsap_prim l1sap;
	memset(&l1sap, 0, sizeof(l1sap));
	osmo_prim_init(&l1sap.oph, SAP_GSM_PH, PRIM_MPH_INFO,
		PRIM_OP_INDICATION, NULL);
	l1sap.u.info.type = PRIM_INFO_MEAS;
	l1sap.u.info.u.meas_ind.chan_nr = chan_nr;
	l1sap.u.info.u.meas_ind.ta_offs_256bits = m->i16BurstTiming*64;
	l1sap.u.info.u.meas_ind.ber10k = (unsigned int) (m->fBer * 10000);
	l1sap.u.info.u.meas_ind.inv_rssi = (uint8_t) (m->fRssi * -1);
	l1sap.u.info.u.meas_ind.fn = fn;

	/* l1sap wants to take msgb ownership.  However, as there is no
	 * msg, it will msgb_free(l1sap.oph.msg == NULL) */
	return l1sap_up(trx, &l1sap);
}

static int handle_ph_data_ind(struct oc2gl1_hdl *fl1, GsmL1_PhDataInd_t *data_ind,
			      struct msgb *l1p_msg)
{
	struct gsm_bts_trx *trx = oc2gl1_hdl_trx(fl1);
	uint8_t chan_nr, link_id;
	struct osmo_phsap_prim *l1sap;
	uint32_t fn;
	struct gsm_time g_time;
	uint8_t *data, len;
	int rc = 0;
	int8_t rssi;

	chan_nr = chan_nr_by_sapi(&trx->ts[data_ind->u8Tn], data_ind->sapi,
		data_ind->subCh, data_ind->u8Tn, data_ind->u32Fn);
	fn = data_ind->u32Fn;
	link_id =  (data_ind->sapi == GsmL1_Sapi_Sacch) ? LID_SACCH : LID_DEDIC;
	gsm_fn2gsmtime(&g_time, fn);

	if (!chan_nr) {
		LOGPGT(DL1C, LOGL_ERROR, &g_time, "PH-DATA-INDICATION for unknown sapi %s (%d)\n",
		     get_value_string(oc2gbts_l1sapi_names, data_ind->sapi), data_ind->sapi);
		msgb_free(l1p_msg);
		return ENOTSUP;
	}

	process_meas_res(trx, chan_nr, &data_ind->measParam, fn);


	DEBUGPGT(DL1P, &g_time, "Rx PH-DATA.ind %s (hL2 %08x): %s\n",
		get_value_string(oc2gbts_l1sapi_names, data_ind->sapi), (uint32_t)data_ind->hLayer2,
		osmo_hexdump(data_ind->msgUnitParam.u8Buffer, data_ind->msgUnitParam.u8Size));
	dump_meas_res(LOGL_DEBUG, &data_ind->measParam);

	/* check for TCH */
	if (data_ind->sapi == GsmL1_Sapi_TchF
	 || data_ind->sapi == GsmL1_Sapi_TchH) {
		/* TCH speech frame handling */
		rc = l1if_tch_rx(trx, chan_nr, l1p_msg);
		msgb_free(l1p_msg);
		return rc;
	}

	/* get rssi */
	rssi = (int8_t) (data_ind->measParam.fRssi);
	/* get data pointer and length */
	data = data_ind->msgUnitParam.u8Buffer;
	len = data_ind->msgUnitParam.u8Size;
	/* pull lower header part before data */
	msgb_pull(l1p_msg, data - l1p_msg->data);
	/* trim remaining data to it's size, to get rid of upper header part */
	rc = msgb_trim(l1p_msg, len);
	if (rc < 0)
		MSGB_ABORT(l1p_msg, "No room for primitive data\n");
	l1p_msg->l2h = l1p_msg->data;
	/* push new l1 header */
	l1p_msg->l1h = msgb_push(l1p_msg, sizeof(*l1sap));
	/* fill header */
	l1sap = msgb_l1sap_prim(l1p_msg);
	osmo_prim_init(&l1sap->oph, SAP_GSM_PH, PRIM_PH_DATA,
		PRIM_OP_INDICATION, l1p_msg);
	l1sap->u.data.link_id = link_id;
	l1sap->u.data.chan_nr = chan_nr;
	l1sap->u.data.fn = fn;
	l1sap->u.data.rssi = rssi;
	if (!pcu_direct) {
		l1sap->u.data.ber10k = data_ind->measParam.fBer * 10000;
		l1sap->u.data.ta_offs_256bits = data_ind->measParam.i16BurstTiming*64;
		l1sap->u.data.lqual_cb = data_ind->measParam.fLinkQuality * 10;
	}
	return l1sap_up(trx, l1sap);
}

static int handle_ph_ra_ind(struct oc2gl1_hdl *fl1, GsmL1_PhRaInd_t *ra_ind,
			    struct msgb *l1p_msg)
{
	struct gsm_bts_trx *trx = oc2gl1_hdl_trx(fl1);
	struct gsm_bts *bts = trx->bts;
	struct gsm_lchan *lchan;
	struct osmo_phsap_prim *l1sap;
	int rc;
	struct ph_rach_ind_param rach_ind_param;

	/* FIXME: this should be deprecated/obsoleted as it bypasses rach.busy counting */
	if (ra_ind->measParam.fLinkQuality < bts->min_qual_rach) {
		msgb_free(l1p_msg);
		return 0;
	}

	dump_meas_res(LOGL_DEBUG, &ra_ind->measParam);

	if ((ra_ind->msgUnitParam.u8Size != 1) &&
		(ra_ind->msgUnitParam.u8Size != 2)) {
		LOGPFN(DL1P, LOGL_ERROR, ra_ind->u32Fn, "PH-RACH-INDICATION has %d bits\n", ra_ind->sapi);
		msgb_free(l1p_msg);
		return 0;
	}

	/* We need to evaluate ra_ind before below msgb_trim(), since that invalidates *ra_ind. */
	rach_ind_param = (struct ph_rach_ind_param) {
		/* .chan_nr set below */
		/* .ra set below */
		.acc_delay = 0,
		.fn = ra_ind->u32Fn,
		/* .is_11bit set below */
		/* .burst_type set below */
		.rssi = (int8_t) ra_ind->measParam.fRssi,
		.ber10k = (unsigned int) (ra_ind->measParam.fBer * 10000.0),
		.acc_delay_256bits = ra_ind->measParam.i16BurstTiming * 64,
	};

	lchan = l1if_hLayer_to_lchan(trx, (uint32_t)ra_ind->hLayer2);
	if (!lchan || lchan->ts->pchan == GSM_PCHAN_CCCH ||
	    lchan->ts->pchan == GSM_PCHAN_CCCH_SDCCH4 ||
	    lchan->ts->pchan == GSM_PCHAN_CCCH_SDCCH4_CBCH)
		rach_ind_param.chan_nr = 0x88;
	else
		rach_ind_param.chan_nr = gsm_lchan2chan_nr(lchan);

	if (ra_ind->msgUnitParam.u8Size == 2) {
		uint16_t temp;
		uint16_t ra = ra_ind->msgUnitParam.u8Buffer[0];
		ra = ra << 3;
		temp = (ra_ind->msgUnitParam.u8Buffer[1] & 0x7);
		ra = ra | temp;
		rach_ind_param.is_11bit = 1;
		rach_ind_param.ra = ra;
	} else {
		rach_ind_param.is_11bit = 0;
		rach_ind_param.ra = ra_ind->msgUnitParam.u8Buffer[0];
	}

	/* the old legacy full-bits acc_delay cannot express negative values */
	if (ra_ind->measParam.i16BurstTiming > 0)
		rach_ind_param.acc_delay = ra_ind->measParam.i16BurstTiming >> 2;

	/* mapping of the burst type, the values are specific to
	 * osmo-bts-oc2g */
	switch (ra_ind->burstType) {
	case GsmL1_BurstType_Access_0:
		rach_ind_param.burst_type =
			GSM_L1_BURST_TYPE_ACCESS_0;
		break;
	case GsmL1_BurstType_Access_1:
		rach_ind_param.burst_type =
			GSM_L1_BURST_TYPE_ACCESS_1;
		break;
	case GsmL1_BurstType_Access_2:
		rach_ind_param.burst_type =
			GSM_L1_BURST_TYPE_ACCESS_2;
		break;
	default:
		rach_ind_param.burst_type =
			GSM_L1_BURST_TYPE_NONE;
		break;
	}

	/* msgb_trim() invalidates ra_ind, make that abundantly clear: */
	ra_ind = NULL;
	rc = msgb_trim(l1p_msg, sizeof(*l1sap));
	if (rc < 0)
		MSGB_ABORT(l1p_msg, "No room for primitive data\n");
	l1sap = msgb_l1sap_prim(l1p_msg);
	osmo_prim_init(&l1sap->oph, SAP_GSM_PH, PRIM_PH_RACH, PRIM_OP_INDICATION,
		l1p_msg);
	l1sap->u.rach_ind = rach_ind_param;

	return l1sap_up(trx, l1sap);
}

/* handle any random indication from the L1 */
static int l1if_handle_ind(struct oc2gl1_hdl *fl1, struct msgb *msg)
{
	GsmL1_Prim_t *l1p = msgb_l1prim(msg);
	int rc = 0;

	/* all the below called functions must take ownership of the msgb */
	switch (l1p->id) {
	case GsmL1_PrimId_MphTimeInd:
		rc = handle_mph_time_ind(fl1, &l1p->u.mphTimeInd, msg);
		break;
	case GsmL1_PrimId_MphSyncInd:
		msgb_free(msg);
		break;
	case GsmL1_PrimId_PhConnectInd:
		msgb_free(msg);
		break;
	case GsmL1_PrimId_PhReadyToSendInd:
		rc = handle_ph_readytosend_ind(fl1, &l1p->u.phReadyToSendInd,
					       msg);
		break;
	case GsmL1_PrimId_PhDataInd:
		rc = handle_ph_data_ind(fl1, &l1p->u.phDataInd, msg);
		break;
	case GsmL1_PrimId_PhRaInd:
		rc = handle_ph_ra_ind(fl1, &l1p->u.phRaInd, msg);
		break;
	default:
		msgb_free(msg);
	}

	return rc;
}

static inline int is_prim_compat(GsmL1_Prim_t *l1p, struct wait_l1_conf *wlc)
{
	if (wlc->is_sys_prim != 0)
		return 0;
	if (l1p->id != wlc->conf_prim_id)
		return 0;
	if (l1p_get_hLayer3(l1p) != wlc->conf_hLayer3)
		return 0;
	return 1;
}

int l1if_handle_l1prim(int wq, struct oc2gl1_hdl *fl1h, struct msgb *msg)
{
	GsmL1_Prim_t *l1p = msgb_l1prim(msg);
	struct wait_l1_conf *wlc;
	int rc;

	switch (l1p->id) {
	case GsmL1_PrimId_MphTimeInd:
		/* silent, don't clog the log file */
		break;
	default:
		LOGP(DL1P, LOGL_DEBUG, "Rx L1 prim %s on queue %d\n",
			get_value_string(oc2gbts_l1prim_names, l1p->id), wq);
	}

	/* check if this is a resposne to a sync-waiting request */
	llist_for_each_entry(wlc, &fl1h->wlc_list, list) {
		if (is_prim_compat(l1p, wlc)) {
			llist_del(&wlc->list);
			if (wlc->cb) {
				/* call-back function must take
				 * ownership of msgb */
				rc = wlc->cb(oc2gl1_hdl_trx(fl1h), msg,
					     wlc->cb_data);
			} else {
				rc = 0;
				msgb_free(msg);
			}
			release_wlc(wlc);
			return rc;
		}
	}

	/* if we reach here, it is not a Conf for a pending Req */
	return l1if_handle_ind(fl1h, msg);
}

int l1if_handle_sysprim(struct oc2gl1_hdl *fl1h, struct msgb *msg)
{
	Oc2g_Prim_t *sysp = msgb_sysprim(msg);
	struct wait_l1_conf *wlc;
	int rc;

	LOGP(DL1P, LOGL_DEBUG, "Rx SYS prim %s\n",
		get_value_string(oc2gbts_sysprim_names, sysp->id));

	/* check if this is a resposne to a sync-waiting request */
	llist_for_each_entry(wlc, &fl1h->wlc_list, list) {
		/* the limitation here is that we cannot have multiple callers
		 * sending the same primitive */
		if (wlc->is_sys_prim && sysp->id == wlc->conf_prim_id) {
			llist_del(&wlc->list);
			if (wlc->cb) {
				/* call-back function must take
				 * ownership of msgb */
				rc = wlc->cb(oc2gl1_hdl_trx(fl1h), msg,
					     wlc->cb_data);
			} else {
				rc = 0;
				msgb_free(msg);
			}
			release_wlc(wlc);
			return rc;
		}
	}
	/* if we reach here, it is not a Conf for a pending Req */
	return l1if_handle_ind(fl1h, msg);
}

static int activate_rf_compl_cb(struct gsm_bts_trx *trx, struct msgb *resp,
				void *data)
{
	Oc2g_Prim_t *sysp = msgb_sysprim(resp);
	GsmL1_Status_t status;
	int on = 0;
	unsigned int i;
	struct gsm_bts *bts = trx->bts;

	if (sysp->id == Oc2g_PrimId_ActivateRfCnf)
		on = 1;

	if (on)
		status = sysp->u.activateRfCnf.status;
	else
		status = sysp->u.deactivateRfCnf.status;

	LOGP(DL1C, LOGL_INFO, "Rx RF-%sACT.conf (status=%s)\n", on ? "" : "DE",
		get_value_string(oc2gbts_l1status_names, status));


	if (on) {
		if (status != GsmL1_Status_Success) {
			LOGP(DL1C, LOGL_FATAL, "RF-ACT.conf with status %s\n",
				get_value_string(oc2gbts_l1status_names, status));
			bts_shutdown(trx->bts, "RF-ACT failure");
		} else {
			if(bts->oc2g.led_ctrl_mode == OC2G_LED_CONTROL_BTS)
				bts_update_status(BTS_STATUS_RF_ACTIVE, 1);
		}

		/* signal availability */
		oml_mo_state_chg(&trx->mo, NM_OPSTATE_DISABLED, NM_AVSTATE_OK);
		oml_mo_tx_sw_act_rep(&trx->mo);
		oml_mo_state_chg(&trx->bb_transc.mo, -1, NM_AVSTATE_OK);
		oml_mo_tx_sw_act_rep(&trx->bb_transc.mo);

		for (i = 0; i < ARRAY_SIZE(trx->ts); i++)
			oml_mo_state_chg(&trx->ts[i].mo, NM_OPSTATE_DISABLED, NM_AVSTATE_DEPENDENCY);
	} else {
		if(bts->oc2g.led_ctrl_mode == OC2G_LED_CONTROL_BTS)
			bts_update_status(BTS_STATUS_RF_ACTIVE, 0);
		oml_mo_state_chg(&trx->mo, NM_OPSTATE_DISABLED, NM_AVSTATE_OFF_LINE);
		oml_mo_state_chg(&trx->bb_transc.mo, NM_OPSTATE_DISABLED, NM_AVSTATE_OFF_LINE);
	}

	msgb_free(resp);

	return 0;
}

/* activate or de-activate the entire RF-Frontend */
int l1if_activate_rf(struct oc2gl1_hdl *hdl, int on)
{
	struct msgb *msg = sysp_msgb_alloc();
	Oc2g_Prim_t *sysp = msgb_sysprim(msg);
	struct phy_instance *pinst = hdl->phy_inst;

	if (on) {
		sysp->id = Oc2g_PrimId_ActivateRfReq;
		sysp->u.activateRfReq.msgq.u8UseTchMsgq = 0;
		sysp->u.activateRfReq.msgq.u8UsePdtchMsgq = pcu_direct;

		sysp->u.activateRfReq.u8UnusedTsMode = pinst->u.oc2g.pedestal_mode;

		/* maximum cell size in quarter-bits, 90 == 12.456 km */
		sysp->u.activateRfReq.u8MaxCellSize = pinst->u.oc2g.max_cell_size;

		/* auto tx power adjustment mode 0:none, 1: automatic*/
		sysp->u.activateRfReq.u8EnAutoPowerAdjust = pinst->u.oc2g.tx_pwr_adj_mode;

	} else {
		sysp->id = Oc2g_PrimId_DeactivateRfReq;
	}

	return l1if_req_compl(hdl, msg, activate_rf_compl_cb, NULL);
}

static void mute_handle_ts(struct gsm_bts_trx_ts *ts, int is_muted)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ts->lchan); i++) {
		struct gsm_lchan *lchan = &ts->lchan[i];

		if (!is_muted)
			continue;

		if (lchan->state != LCHAN_S_ACTIVE)
			continue;

		/* skip channels that might be active for another reason */
		if (lchan->type == GSM_LCHAN_CCCH)
			continue;
		if (lchan->type == GSM_LCHAN_PDTCH)
			continue;

		if (lchan->s <= 0)
			continue;

		lchan->s = 0;
		rsl_tx_conn_fail(lchan, RSL_ERR_RADIO_LINK_FAIL);
	}
}

static int mute_rf_compl_cb(struct gsm_bts_trx *trx, struct msgb *resp,
			    void *data)
{
	struct oc2gl1_hdl *fl1h = trx_oc2gl1_hdl(trx);
	Oc2g_Prim_t *sysp = msgb_sysprim(resp);
	GsmL1_Status_t status;

	status = sysp->u.muteRfCnf.status;

	if (status != GsmL1_Status_Success) {
		LOGP(DL1C, LOGL_ERROR, "Rx RF-MUTE.conf with status %s\n",
		     get_value_string(oc2gbts_l1status_names, status));
		oml_mo_rf_lock_chg(&trx->mo, fl1h->last_rf_mute, 0);
	} else {
		int i;
		struct gsm_bts *bts = trx->bts;
		LOGP(DL1C, LOGL_INFO, "Rx RF-MUTE.conf with status=%s\n",
		     get_value_string(oc2gbts_l1status_names, status));
		if(bts->oc2g.led_ctrl_mode == OC2G_LED_CONTROL_BTS)
			bts_update_status(BTS_STATUS_RF_MUTE, fl1h->last_rf_mute[0]);
		oml_mo_rf_lock_chg(&trx->mo, fl1h->last_rf_mute, 1);

		osmo_static_assert(
			ARRAY_SIZE(trx->ts) >= ARRAY_SIZE(fl1h->last_rf_mute),
			ts_array_size);

		for (i = 0; i < ARRAY_SIZE(fl1h->last_rf_mute); ++i)
			mute_handle_ts(&trx->ts[i], fl1h->last_rf_mute[i]);
	}

	msgb_free(resp);

	return 0;
}

/* mute/unmute RF time slots */
int l1if_mute_rf(struct oc2gl1_hdl *hdl, uint8_t mute[8], l1if_compl_cb *cb)
{
	struct msgb *msg = sysp_msgb_alloc();
	Oc2g_Prim_t *sysp = msgb_sysprim(msg);

	LOGP(DL1C, LOGL_INFO, "Tx RF-MUTE.req (%d, %d, %d, %d, %d, %d, %d, %d)\n",
	     mute[0], mute[1], mute[2], mute[3],
	     mute[4], mute[5], mute[6], mute[7]
	    );

	sysp->id = Oc2g_PrimId_MuteRfReq;
	memcpy(sysp->u.muteRfReq.u8Mute, mute, sizeof(sysp->u.muteRfReq.u8Mute));
	/* save for later use */
	memcpy(hdl->last_rf_mute, mute, sizeof(hdl->last_rf_mute));

	return l1if_req_compl(hdl, msg, cb ? cb : mute_rf_compl_cb, NULL);
}

/* call-back on arrival of DSP+FPGA version + band capability */
static int info_compl_cb(struct gsm_bts_trx *trx, struct msgb *resp,
			 void *data)
{
	Oc2g_Prim_t *sysp = msgb_sysprim(resp);
	Oc2g_SystemInfoCnf_t *sic = &sysp->u.systemInfoCnf;
	struct oc2gl1_hdl *fl1h = trx_oc2gl1_hdl(trx);
	int rc;

	fl1h->hw_info.dsp_version[0] = sic->dspVersion.major;
	fl1h->hw_info.dsp_version[1] = sic->dspVersion.minor;
	fl1h->hw_info.dsp_version[2] = sic->dspVersion.build;

	fl1h->hw_info.fpga_version[0] = sic->fpgaVersion.major;
	fl1h->hw_info.fpga_version[1] = sic->fpgaVersion.minor;
	fl1h->hw_info.fpga_version[2] = sic->fpgaVersion.build;

	LOGP(DL1C, LOGL_INFO, "DSP v%u.%u.%u, FPGA v%u.%u.%u, Band support %s\n",
		sic->dspVersion.major, sic->dspVersion.minor,
		sic->dspVersion.build, sic->fpgaVersion.major,
		sic->fpgaVersion.minor, sic->fpgaVersion.build,
		gsm_band_name(fl1h->hw_info.band_support));

	if (!(fl1h->hw_info.band_support & trx->bts->band))
		LOGP(DL1C, LOGL_FATAL, "BTS band %s not supported by hw\n",
		     gsm_band_name(trx->bts->band));

	/* Request the activation */
	l1if_activate_rf(fl1h, 1);

	/* load calibration tables */
	rc = calib_load(fl1h);
	if (rc < 0)
		LOGP(DL1C, LOGL_ERROR, "Operating without calibration; "
			"unable to load tables!\n");

	msgb_free(resp);
	return 0;
}

/* request DSP+FPGA code versions */
static int l1if_get_info(struct oc2gl1_hdl *hdl)
{
	struct msgb *msg = sysp_msgb_alloc();
	Oc2g_Prim_t *sysp = msgb_sysprim(msg);

	sysp->id = Oc2g_PrimId_SystemInfoReq;

	return l1if_req_compl(hdl, msg, info_compl_cb, NULL);
}

static int reset_compl_cb(struct gsm_bts_trx *trx, struct msgb *resp,
			  void *data)
{
	struct oc2gl1_hdl *fl1h = trx_oc2gl1_hdl(trx);
	Oc2g_Prim_t *sysp = msgb_sysprim(resp);
	GsmL1_Status_t status = sysp->u.layer1ResetCnf.status;

	LOGP(DL1C, LOGL_NOTICE, "Rx L1-RESET.conf (status=%s)\n",
		get_value_string(oc2gbts_l1status_names, status));

	msgb_free(resp);

	/* If we're coming out of reset .. */
	if (status != GsmL1_Status_Success) {
		LOGP(DL1C, LOGL_FATAL, "L1-RESET.conf with status %s\n",
			get_value_string(oc2gbts_l1status_names, status));
		bts_shutdown(trx->bts, "L1-RESET failure");
	}

	/* as we cannot get the current DSP trace flags, we simply
	 * set them to zero (or whatever dsp_trace_f has been initialized to */
	l1if_set_trace_flags(fl1h, fl1h->dsp_trace_f);

	/* obtain version information on DSP/FPGA and band capabilities */
	l1if_get_info(fl1h);

	return 0;
}

int l1if_reset(struct oc2gl1_hdl *hdl)
{
	struct msgb *msg = sysp_msgb_alloc();
	Oc2g_Prim_t *sysp = msgb_sysprim(msg);
	sysp->id = Oc2g_PrimId_Layer1ResetReq;

	return l1if_req_compl(hdl, msg, reset_compl_cb, NULL);
}

/* set the trace flags within the DSP */
int l1if_set_trace_flags(struct oc2gl1_hdl *hdl, uint32_t flags)
{
	struct msgb *msg = sysp_msgb_alloc();
	Oc2g_Prim_t *sysp = msgb_sysprim(msg);

	LOGP(DL1C, LOGL_INFO, "Tx SET-TRACE-FLAGS.req (0x%08x)\n",
		flags);

	sysp->id = Oc2g_PrimId_SetTraceFlagsReq;
	sysp->u.setTraceFlagsReq.u32Tf = flags;

	hdl->dsp_trace_f = flags;

	/* There is no confirmation we could wait for */
	if (osmo_wqueue_enqueue(&hdl->write_q[MQ_SYS_WRITE], msg) != 0) {
		LOGP(DL1C, LOGL_ERROR, "MQ_SYS_WRITE queue full. Dropping msg\n");
		msgb_free(msg);
		return -EAGAIN;
	}
	return 0;
}

static int get_hwinfo(struct oc2gl1_hdl *fl1h)
{
	int rc = 0;
	char rev_maj = 0, rev_min = 0;

	oc2gbts_rev_get(&rev_maj, &rev_min);
	if (rc < 0)
		return rc;
	fl1h->hw_info.ver_major = (uint8_t)rev_maj;
	fl1h->hw_info.ver_minor = (uint8_t)rev_min;

	rc = oc2gbts_model_get();
	if (rc < 0)
		return rc;
	fl1h->hw_info.options = (uint32_t)rc;

	rc = oc2gbts_option_get(OC2GBTS_OPTION_BAND);
	if (rc < 0) 
		return rc;

	switch (rc) {
	case OC2GBTS_BAND_850: 
		fl1h->hw_info.band_support = GSM_BAND_850; 
		break;
	case OC2GBTS_BAND_900: 
		fl1h->hw_info.band_support = GSM_BAND_900; 
		break;
	case OC2GBTS_BAND_1800: 
		fl1h->hw_info.band_support = GSM_BAND_1800; 
		break;
	case OC2GBTS_BAND_1900: 
		fl1h->hw_info.band_support = GSM_BAND_1900; 
		break;
	default:
		return -1;
	}
	return 0;
}

struct oc2gl1_hdl *l1if_open(struct phy_instance *pinst)
{
	struct oc2gl1_hdl *fl1h;
	int rc;

	LOGP(DL1C, LOGL_INFO, "OC-2G BTS L1IF compiled against API headers "
			"v%u.%u.%u\n", OC2G_API_VERSION >> 16,
			(OC2G_API_VERSION >> 8) & 0xff,
			 OC2G_API_VERSION & 0xff);

	fl1h = talloc_zero(pinst, struct oc2gl1_hdl);
	if (!fl1h)
		return NULL;
	INIT_LLIST_HEAD(&fl1h->wlc_list);
	INIT_LLIST_HEAD(&fl1h->alarm_list);

	fl1h->phy_inst = pinst;
	fl1h->dsp_trace_f = pinst->u.oc2g.dsp_trace_f;

	get_hwinfo(fl1h);

	rc = l1if_transport_open(MQ_SYS_WRITE, fl1h);
	if (rc < 0) {
		talloc_free(fl1h);
		return NULL;
	}

	rc = l1if_transport_open(MQ_L1_WRITE, fl1h);
	if (rc < 0) {
		l1if_transport_close(MQ_SYS_WRITE, fl1h);
		talloc_free(fl1h);
		return NULL;
	}

	return fl1h;
}

int l1if_close(struct oc2gl1_hdl *fl1h)
{
	l1if_transport_close(MQ_L1_WRITE, fl1h);
	l1if_transport_close(MQ_SYS_WRITE, fl1h);
	return 0;
}

/* TODO (oramadan): Fix OML alarms
static void dsp_alive_compl_cb(struct gsm_bts_trx *trx, struct msgb *resp, void *data)
{
	Oc2g_Prim_t *sysp = msgb_sysprim(resp);
	Oc2g_IsAliveCnf_t *sac = &sysp->u.isAliveCnf;
	struct oc2gl1_hdl *fl1h = trx_oc2gl1_hdl(trx);

	fl1h->hw_alive.dsp_alive_cnt++;
	LOGP(DL1C, LOGL_DEBUG, "Rx SYS prim %s, status=%d (%d)\n",
			get_value_string(oc2gbts_sysprim_names, sysp->id), sac->status, trx->nr);

	msgb_free(resp);
}

static int dsp_alive_timer_cb(void *data)
{
	struct oc2gl1_hdl *fl1h = data;
	struct gsm_bts_trx *trx = fl1h->phy_inst->trx;
	struct msgb *msg = sysp_msgb_alloc();
	int rc;
	struct oml_alarm_list *alarm_sent;

	Oc2g_Prim_t *sys_prim =  msgb_sysprim(msg);
	sys_prim->id = Oc2g_PrimId_IsAliveReq;

	if (fl1h->hw_alive.dsp_alive_cnt == 0) {
		/* check for the alarm has already sent or not * /
		llist_for_each_entry(alarm_sent, &fl1h->alarm_list, list) {
			llist_del(&alarm_sent->list);
			if (alarm_sent->alarm_signal != S_NM_OML_BTS_DSP_ALIVE_ALARM)
				continue;

			LOGP(DL1C, LOGL_ERROR, "Alarm %d has removed from sent alarm list (%d)\n", alarm_sent->alarm_signal, trx->nr);
			exit(23);
		}

		LOGP(DL1C, LOGL_ERROR, "Timeout waiting for SYS prim %s primitive (%d)\n",
				get_value_string(oc2gbts_sysprim_names, sys_prim->id + 1), trx->nr);

		if( fl1h->phy_inst->trx ){
			fl1h->phy_inst->trx->mo.obj_inst.trx_nr = fl1h->phy_inst->trx->nr;

			alarm_sig_data.mo = &fl1h->phy_inst->trx->mo;
			memcpy(alarm_sig_data.spare, &sys_prim->id, sizeof(unsigned int));
			osmo_signal_dispatch(SS_NM, S_NM_OML_BTS_DSP_ALIVE_ALARM, &alarm_sig_data);
			if (!alarm_sig_data.rc) {
				/* allocate new list of sent alarms * /
				alarm_sent = talloc_zero(fl1h, struct oml_alarm_list);
				if (!alarm_sent)
					return -EIO;

				alarm_sent->alarm_signal = S_NM_OML_BTS_DSP_ALIVE_ALARM;
				/* add alarm to sent list * /
				llist_add(&alarm_sent->list, &fl1h->alarm_list);
				LOGP(DL1C, LOGL_ERROR, "Alarm %d has added to sent alarm list (%d)\n", alarm_sent->alarm_signal, trx->nr);
			}
		}
	}

	LOGP(DL1C, LOGL_DEBUG, "Tx SYS prim %s (%d)\n",
			get_value_string(oc2gbts_sysprim_names, sys_prim->id), trx->nr);

	rc = l1if_req_compl(fl1h, msg, dsp_alive_compl_cb, NULL);
	if (rc < 0) {
		LOGP(DL1C, LOGL_FATAL, "Failed to send %s primitive\n", get_value_string(oc2gbts_sysprim_names, sys_prim->id));
		return -EIO;
	}

	/* restart timer * /
	fl1h->hw_alive.dsp_alive_cnt = 0;
	osmo_timer_schedule(&fl1h->hw_alive.dsp_alive_timer, fl1h->hw_alive.dsp_alive_period, 0);

	return 0;
}
*/

int bts_model_phy_link_open(struct phy_link *plink)
{
	struct phy_instance *pinst = phy_instance_by_num(plink, 0);

	OSMO_ASSERT(pinst);

	if (!pinst->trx) {
		LOGP(DL1C, LOGL_NOTICE, "Ignoring phy link %d instance %d "
		     "because no TRX is associated with it\n", plink->num, pinst->num);
		return 0;
	}
	phy_link_state_set(plink, PHY_LINK_CONNECTING);

	pinst->u.oc2g.hdl = l1if_open(pinst);
	if (!pinst->u.oc2g.hdl) {
		LOGP(DL1C, LOGL_FATAL, "Cannot open L1 interface\n");
		return -EIO;
	}

	/* Set default PHY parameters */
	if (!pinst->u.oc2g.max_cell_size)
		pinst->u.oc2g.max_cell_size = OC2G_BTS_MAX_CELL_SIZE_DEFAULT;

	if (!pinst->u.oc2g.pedestal_mode)
		pinst->u.oc2g.pedestal_mode = OC2G_BTS_PEDESTAL_MODE_DEFAULT;

	if (!pinst->u.oc2g.dsp_alive_period)
		pinst->u.oc2g.dsp_alive_period = OC2G_BTS_DSP_ALIVE_TMR_DEFAULT;

	if (!pinst->u.oc2g.tx_pwr_adj_mode)
		pinst->u.oc2g.tx_pwr_adj_mode = OC2G_BTS_TX_PWR_ADJ_DEFAULT;

	if (!pinst->u.oc2g.tx_pwr_red_8psk)
		pinst->u.oc2g.tx_pwr_red_8psk = OC2G_BTS_TX_RED_PWR_8PSK_DEFAULT;

	if (!pinst->u.oc2g.tx_c0_idle_pwr_red)
		pinst->u.oc2g.tx_c0_idle_pwr_red = OC2G_BTS_TX_C0_IDLE_RED_PWR_DEFAULT;

	struct oc2gl1_hdl *fl1h = pinst->u.oc2g.hdl;
	fl1h->dsp_trace_f = dsp_trace;

	l1if_reset(pinst->u.oc2g.hdl);

	phy_link_state_set(plink, PHY_LINK_CONNECTED);

	/* TODO (oramadan): Fix OML alarms
	/* Send first IS_ALIVE primitive * /
	struct msgb *msg = sysp_msgb_alloc();
	int rc;

	Oc2g_Prim_t *sys_prim =  msgb_sysprim(msg);
	sys_prim->id = Oc2g_PrimId_IsAliveReq;

	rc = l1if_req_compl(fl1h, msg, dsp_alive_compl_cb, NULL);
	if (rc < 0) {
		LOGP(DL1C, LOGL_FATAL, "Failed to send %s primitive\n", get_value_string(oc2gbts_sysprim_names, sys_prim->id));
		return -EIO;
	}

	/* initialize DSP heart beat alive timer * /
	fl1h->hw_alive.dsp_alive_timer.cb = dsp_alive_timer_cb;
	fl1h->hw_alive.dsp_alive_timer.data = fl1h;
	fl1h->hw_alive.dsp_alive_cnt = 0;
	fl1h->hw_alive.dsp_alive_period = pinst->u.oc2g.dsp_alive_period;
	osmo_timer_schedule(&fl1h->hw_alive.dsp_alive_timer, fl1h->hw_alive.dsp_alive_period, 0);
	*/
	return 0;
}
