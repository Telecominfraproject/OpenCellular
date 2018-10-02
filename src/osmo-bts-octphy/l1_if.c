/* Layer 1 (PHY) interface of osmo-bts OCTPHY integration */

/* Copyright (c) 2014 Octasic Inc. All rights reserved.
 * Copyright (c) 2015-2016 Harald Welte <laforge@gnumonks.org>
 *
 * based on a copy of osmo-bts-sysmo/l1_if.c, which is
 * Copyright (C) 2011-2014 by Harald Welte <laforge@gnumonks.org>
 * Copyright (C) 2014 by Holger Hans Peter Freyther
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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/socket.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/bts_model.h>
#include <osmo-bts/oml.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/l1sap.h>
#include <osmo-bts/handover.h>
#include <osmo-bts/cbch.h>

#include "l1_if.h"
#include "l1_oml.h"
#include "l1_utils.h"

#include "octpkt.h"
#include <octphy/octvc1/main/octvc1_main_version.h>

/* NOTE: The octphy GPRS frame number handling changed with
 * OCTSDR-2G-02.07.00-B1314-BETA. From that version on, each ph_data_ind must
 * subtract 3 from the frame number before passing the frame to the PCU */
#define cOCTVC1_MAIN_VERSION_ID_FN_PARADIGM_CHG 0x41c0522

#include <octphy/octpkt/octpkt_hdr.h>
#define OCTVC1_RC2STRING_DECLARE
#include <octphy/octvc1/octvc1_rc2string.h>
#define OCTVC1_ID2STRING_DECLARE
#include <octphy/octvc1/octvc1_id2string.h>
#include <octphy/octvc1/gsm/octvc1_gsm_evt_swap.h>
#define OCTVC1_OPT_DECLARE_DEFAULTS
#include <octphy/octvc1/gsm/octvc1_gsm_default.h>
#include <octphy/octvc1/main/octvc1_main_default.h>

#define cPKTAPI_FIFO_ID_MSG                                0xAAAA0001

/* maximum window of unacknowledged commands */
#define UNACK_CMD_WINDOW	8
/* maximum number of re-transmissions of a command */
#define MAX_RETRANS		3
/* timeout until which we expect PHY to respond */
#define CMD_TIMEOUT		5

/* allocate a msgb for a Layer1 primitive */
struct msgb *l1p_msgb_alloc(void)
{
	struct msgb *msg = msgb_alloc_headroom(1500, 24, "l1_prim");
	if (!msg)
		return msg;

	msg->l2h = msg->data;
	return msg;
}

void l1if_fill_msg_hdr(tOCTVC1_MSG_HEADER *mh, struct msgb *msg,
			struct octphy_hdl *fl1h, uint32_t msg_type, uint32_t api_cmd)
{
	octvc1_fill_msg_hdr(mh, msgb_l2len(msg), fl1h->session_id,
			    fl1h->next_trans_id++, 0 /* user_info */,
			    msg_type, 0, api_cmd);
}

/* Map OSMOCOM BAND type to Octasic type */
tOCTVC1_RADIO_STANDARD_FREQ_BAND_GSM_ENUM
osmocom_to_octphy_band(enum gsm_band osmo_band, unsigned int arfcn)
{
	switch (osmo_band) {
	case GSM_BAND_450:
		return cOCTVC1_RADIO_STANDARD_FREQ_BAND_GSM_ENUM_450;
	case GSM_BAND_850:
		return cOCTVC1_RADIO_STANDARD_FREQ_BAND_GSM_ENUM_850;
	case GSM_BAND_900:
		if (arfcn == 0)
			return cOCTVC1_RADIO_STANDARD_FREQ_BAND_GSM_ENUM_E_900;
		else if (arfcn >= 955 && arfcn <= 974)
			return cOCTVC1_RADIO_STANDARD_FREQ_BAND_GSM_ENUM_R_900;
		else if (arfcn >= 975 && arfcn <= 1023)
			return cOCTVC1_RADIO_STANDARD_FREQ_BAND_GSM_ENUM_E_900;
		else
			return cOCTVC1_RADIO_STANDARD_FREQ_BAND_GSM_ENUM_P_900;
	case GSM_BAND_1800:
		return cOCTVC1_RADIO_STANDARD_FREQ_BAND_GSM_ENUM_DCS_1800;
	case GSM_BAND_1900:
		return cOCTVC1_RADIO_STANDARD_FREQ_BAND_GSM_ENUM_PCS_1900;
	default:
		return -EINVAL;
	}
};

struct gsm_bts_trx *trx_by_l1h(struct octphy_hdl *fl1h, unsigned int trx_id)
{
	struct phy_instance *pinst;

	pinst = phy_instance_by_num(fl1h->phy_link, trx_id);
	if (!pinst)
		return NULL;

	return pinst->trx;
}

struct gsm_lchan *get_lchan_by_lchid(struct gsm_bts_trx *trx,
				tOCTVC1_GSM_LOGICAL_CHANNEL_ID *lch_id)
{
	unsigned int lchan_idx;

	OSMO_ASSERT(lch_id->byTimeslotNb < ARRAY_SIZE(trx->ts));
	if (lch_id->bySubChannelNb == cOCTVC1_GSM_ID_SUB_CHANNEL_NB_ENUM_ALL) {
		switch (lch_id->bySAPI) {
		case cOCTVC1_GSM_SAPI_ENUM_FCCH:
		case cOCTVC1_GSM_SAPI_ENUM_SCH:
		case cOCTVC1_GSM_SAPI_ENUM_BCCH:
		case cOCTVC1_GSM_SAPI_ENUM_PCH_AGCH:
		case cOCTVC1_GSM_SAPI_ENUM_RACH:
			lchan_idx = 4;
			break;
		case cOCTVC1_GSM_SAPI_ENUM_CBCH:
			/* it is always index 2 (3rd element), whether in a
			 * combined CCCH+SDCCH4 or in a SDCCH8 */
			lchan_idx = 2;
			break;
		default:
			lchan_idx = 0;
			break;
		}
	} else
		lchan_idx = lch_id->bySubChannelNb;

	OSMO_ASSERT(lchan_idx < ARRAY_SIZE(trx->ts[0].lchan));

	return &trx->ts[lch_id->byTimeslotNb].lchan[lchan_idx];
}


/* TODO: Unify with sysmobts? */
struct wait_l1_conf {
	/* list of wait_l1_conf in the phy handle */
	struct llist_head list;
	/* expiration timer */
	struct osmo_timer_list timer;
	/* primtivie / command ID */
	uint32_t prim_id;
	/* transaction ID */
	uint32_t trans_id;
	/* copy of the msgb containing the command */
	struct msgb *cmd_msg;
	/* call-back to call on response */
	l1if_compl_cb *cb;
	/* data to hand to call-back on response */
	void *cb_data;
	/* number of re-transmissions so far */
	uint32_t num_retrans;
};

static void release_wlc(struct wait_l1_conf *wlc)
{
	osmo_timer_del(&wlc->timer);
	msgb_free(wlc->cmd_msg);
	talloc_free(wlc);
}

static void l1if_req_timeout(void *data)
{
	struct wait_l1_conf *wlc = data;

	/* FIXME: Implement re-transmission of command on timer expiration */

	LOGP(DL1C, LOGL_FATAL, "Timeout waiting for L1 primitive %s\n",
		get_value_string(octphy_cid_vals, wlc->prim_id));
	exit(23);
}

/* FIXME: this should be in libosmocore */
static struct llist_head *llist_first(struct llist_head *head)
{
	if (llist_empty(head))
		return NULL;
	return head->next;
}

static void check_refill_window(struct octphy_hdl *fl1h, struct wait_l1_conf *recent)
{
	struct wait_l1_conf *wlc;
	int space = UNACK_CMD_WINDOW - fl1h->wlc_list_len;
	int i;

	for (i = 0; i < space; i++) {
		/* get head of queue */
		struct llist_head *first = llist_first(&fl1h->wlc_postponed);
		struct msgb *msg;
		if (!first)
			break;
		wlc = llist_entry(first, struct wait_l1_conf, list);

		/* remove from head of postponed queue */
		llist_del(&wlc->list);
		fl1h->wlc_postponed_len--;

		/* add to window */
		llist_add_tail(&wlc->list, &fl1h->wlc_list);
		fl1h->wlc_list_len++;

		if (wlc != recent) {
			LOGP(DL1C, LOGL_INFO, "Txing formerly postponed "
			     "command %s (trans_id=%u)\n",
			     get_value_string(octphy_cid_vals, wlc->prim_id),
			     wlc->trans_id);
		}
		msg = msgb_copy(wlc->cmd_msg, "Tx from wlc_postponed");
		/* queue for execution and response handling */
		if (osmo_wqueue_enqueue(&fl1h->phy_wq, msg) != 0) {
			LOGP(DL1C, LOGL_ERROR, "Tx Write queue full. dropping msg\n");
			llist_del(&wlc->list);
			msgb_free(msg);
			exit(24);
		}
		/* schedule a timer for CMD_TIMEOUT seconds. If PHY fails to
		 * respond, we terminate */
		osmo_timer_schedule(&wlc->timer, CMD_TIMEOUT, 0);

	}
}

/* send a request(command) to L1, scheduling a call-back to be executed
 * on receiving the response*/
int l1if_req_compl(struct octphy_hdl *fl1h, struct msgb *msg,
		   l1if_compl_cb *cb, void *data)
{
	struct wait_l1_conf *wlc;

	/* assume that there is a VC1 Message header and that it
	 * contains a command ID in network byte order */
	tOCTVC1_MSG_HEADER *msg_hdr = (tOCTVC1_MSG_HEADER *) msg->l2h;
	uint32_t type_r_cmdid = ntohl(msg_hdr->ul_Type_R_CmdId);
	uint32_t cmd_id = (type_r_cmdid >> cOCTVC1_MSG_ID_BIT_OFFSET) & cOCTVC1_MSG_ID_BIT_MASK;

	LOGP(DL1C, LOGL_DEBUG, "l1if_req_compl(msg_len=%u, cmd_id=%s, trans_id=%u)\n",
	     msgb_length(msg), octvc1_id2string(cmd_id),
	     ntohl(msg_hdr->ulTransactionId));

	/* push the two common headers in front */
	octvocnet_push_ctl_hdr(msg, cOCTVC1_FIFO_ID_MGW_CONTROL,
			       cPKTAPI_FIFO_ID_MSG, fl1h->socket_id);
	octpkt_push_common_hdr(msg, cOCTVOCNET_PKT_FORMAT_CTRL, 0,
			       cOCTPKT_HDR_CONTROL_PROTOCOL_TYPE_ENUM_OCTVOCNET);

	wlc = talloc_zero(fl1h, struct wait_l1_conf);
	wlc->cmd_msg = msg;
	wlc->cb = cb;
	wlc->cb_data = data;
	wlc->prim_id = cmd_id;
	wlc->trans_id = ntohl(msg_hdr->ulTransactionId);
	wlc->timer.data = wlc;
	wlc->timer.cb = l1if_req_timeout;

	/* unconditionally add t to the tail of postponed commands */
	llist_add_tail(&wlc->list, &fl1h->wlc_postponed);
	fl1h->wlc_postponed_len++;

	/* check if the unacknowledged window has some space to transmit */
	check_refill_window(fl1h, wlc);

	/* if any messages are in the queue, it must be at least 'our' message,
	 * as we always enqueue from the tail */
	if (fl1h->wlc_postponed_len) {
		fl1h->stats.wlc_postponed++;
		LOGP(DL1C, LOGL_INFO, "Postponed command %s (trans_id=%u)\n",
		     get_value_string(octphy_cid_vals, cmd_id), wlc->trans_id);
	}

	return 0;
}

/* For OctPHY, this only about sending state changes to BSC */
int l1if_activate_rf(struct gsm_bts_trx *trx, int on)
{
	int i;
	if (on) {
		/* signal availability */
		oml_mo_state_chg(&trx->mo, NM_OPSTATE_DISABLED, NM_AVSTATE_OK);
		oml_mo_tx_sw_act_rep(&trx->mo);
		oml_mo_state_chg(&trx->bb_transc.mo, -1, NM_AVSTATE_OK);
		oml_mo_tx_sw_act_rep(&trx->bb_transc.mo);

		for (i = 0; i < ARRAY_SIZE(trx->ts); i++)
			oml_mo_state_chg(&trx->ts[i].mo, NM_OPSTATE_DISABLED,
					 NM_AVSTATE_DEPENDENCY);
	} else {
		oml_mo_state_chg(&trx->mo, NM_OPSTATE_DISABLED,
				 NM_AVSTATE_OFF_LINE);
		oml_mo_state_chg(&trx->bb_transc.mo, NM_OPSTATE_DISABLED,
				 NM_AVSTATE_OFF_LINE);
	}

	return 0;
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
		      tOCTVC1_GSM_SAPI_ENUM sapi, uint8_t subCh,
		      uint8_t u8Tn, uint32_t u32Fn)
{
	uint8_t cbits = 0;
	enum gsm_phys_chan_config pchan = pick_pchan(ts);

	OSMO_ASSERT(pchan != GSM_PCHAN_TCH_F_PDCH);
	OSMO_ASSERT(pchan != GSM_PCHAN_TCH_F_TCH_H_PDCH);

	switch (sapi) {
	case cOCTVC1_GSM_SAPI_ENUM_BCCH:
		cbits = 0x10;
		break;
	case cOCTVC1_GSM_SAPI_ENUM_SACCH:
		switch (pchan) {
		case GSM_PCHAN_TCH_F:
			cbits = 0x01;
			break;
		case GSM_PCHAN_TCH_H:
			cbits = 0x02 + subCh;
			break;
		case GSM_PCHAN_CCCH_SDCCH4:
		case GSM_PCHAN_CCCH_SDCCH4_CBCH:
			cbits = 0x04 + subCh;
			break;
		case GSM_PCHAN_SDCCH8_SACCH8C:
		case GSM_PCHAN_SDCCH8_SACCH8C_CBCH:
			cbits = 0x08 + subCh;
			break;
		default:
			LOGP(DL1C, LOGL_ERROR, "SACCH for pchan %d?\n", pchan);
			return 0;
		}
		break;
	case cOCTVC1_GSM_SAPI_ENUM_SDCCH:
		switch (pchan) {
		case GSM_PCHAN_CCCH_SDCCH4:
		case GSM_PCHAN_CCCH_SDCCH4_CBCH:
			cbits = 0x04 + subCh;
			break;
		case GSM_PCHAN_SDCCH8_SACCH8C:
		case GSM_PCHAN_SDCCH8_SACCH8C_CBCH:
			cbits = 0x08 + subCh;
			break;
		default:
			LOGP(DL1C, LOGL_ERROR, "SDCCH for pchan %d?\n", pchan);
			return 0;
		}
		break;
	case cOCTVC1_GSM_SAPI_ENUM_PCH_AGCH:
		cbits = 0x12;
		break;
	case cOCTVC1_GSM_SAPI_ENUM_TCHF:
		cbits = 0x01;
		break;
	case cOCTVC1_GSM_SAPI_ENUM_TCHH:
		cbits = 0x02 + subCh;
		break;
	case cOCTVC1_GSM_SAPI_ENUM_FACCHF:
		cbits = 0x01;
		break;
	case cOCTVC1_GSM_SAPI_ENUM_FACCHH:
		cbits = 0x02 + subCh;
		break;
	case cOCTVC1_GSM_SAPI_ENUM_PDTCH:
	case cOCTVC1_GSM_SAPI_ENUM_PACCH:
		switch (pchan) {
		case GSM_PCHAN_PDCH:
			cbits = 0x01;
			break;
		default:
			LOGP(DL1C, LOGL_ERROR, "PDTCH for pchan %d?\n", pchan);
			return 0;
		}
		break;
	case cOCTVC1_GSM_SAPI_ENUM_PTCCH:
		if (!L1SAP_IS_PTCCH(u32Fn)) {
			LOGP(DL1C, LOGL_FATAL, "Not expecting PTCCH at frame "
			     "number other than 12, got it at %u (%u). "
			     "Please fix!\n", u32Fn % 52, u32Fn);
			abort();
		}
		switch (pchan) {
		case GSM_PCHAN_PDCH:
			cbits = 0x01;
			break;
		default:
			LOGP(DL1C, LOGL_ERROR, "PTCCH for pchan %d?\n", pchan);
			return 0;
		}
		break;
	default:
		return 0;
	}
	return ((cbits << 3) | u8Tn);
}

static void data_req_from_rts_ind(tOCTVC1_GSM_MSG_TRX_REQUEST_LOGICAL_CHANNEL_DATA_CMD *data_req,
				const tOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_READY_TO_SEND_INDICATION_EVT *rts_ind)
{
	data_req->TrxId = rts_ind->TrxId;
	data_req->LchId = rts_ind->LchId;
	data_req->Data.ulFrameNumber = rts_ind->ulFrameNumber;
	data_req->Data.ulPayloadType = cOCTVC1_GSM_PAYLOAD_TYPE_ENUM_NONE;
}

#if 0
static void empty_req_from_rts_ind(tOCTVC1_GSM_MSG_TRX_REQUEST_LOGICAL_CHANNEL_EMPTY_FRAME_CMD * empty_req,
				const tOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_READY_TO_SEND_INDICATION_EVT *rts_ind)
{
	empty_req->TrxId = rts_ind->TrxId;
	empty_req->LchId = rts_ind->LchId;
	empty_req->ulFrameNumber = rts_ind->ulFrameNumber;
}
#endif

/***********************************************************************
 * handle messages coming down from generic part
 ***********************************************************************/


static int ph_data_req(struct gsm_bts_trx *trx, struct msgb *msg,
			struct osmo_phsap_prim *l1sap)
{
	struct phy_instance *pinst = trx_phy_instance(trx);
	struct octphy_hdl *fl1h = pinst->phy_link->u.octphy.hdl;
	struct msgb *l1msg = NULL;
	uint32_t u32Fn;
	uint8_t u8Tn, subCh, sapi = 0;
	uint8_t chan_nr, link_id;
	int len;
	int rc;

	if (!msg) {
		LOGPFN(DL1C, LOGL_FATAL, l1sap->u.data.fn, "L1SAP PH-DATA.req without msg. "
		     "Please fix!\n");
		abort();
	}

	len = msgb_l2len(msg);

	chan_nr = l1sap->u.data.chan_nr;
	link_id = l1sap->u.data.link_id;
	u32Fn = l1sap->u.data.fn;
	u8Tn = L1SAP_CHAN2TS(chan_nr);
	subCh = 0xf1;
	if (L1SAP_IS_LINK_SACCH(link_id)) {
		sapi = cOCTVC1_GSM_SAPI_ENUM_SACCH;
		if (!L1SAP_IS_CHAN_TCHF(chan_nr) && !L1SAP_IS_CHAN_PDCH(chan_nr))
			subCh = l1sap_chan2ss(chan_nr);
	} else if (L1SAP_IS_CHAN_TCHF(chan_nr) || L1SAP_IS_CHAN_PDCH(chan_nr)) {
		if (ts_is_pdch(&trx->ts[u8Tn])) {
			if (L1SAP_IS_PTCCH(u32Fn)) {
				sapi = cOCTVC1_GSM_SAPI_ENUM_PTCCH;
			} else {
				sapi = cOCTVC1_GSM_SAPI_ENUM_PDTCH;
			}
		} else {
			sapi = cOCTVC1_GSM_SAPI_ENUM_FACCHF;
		}
	} else if (L1SAP_IS_CHAN_TCHH(chan_nr)) {
		subCh = L1SAP_CHAN2SS_TCHH(chan_nr);
		sapi = cOCTVC1_GSM_SAPI_ENUM_FACCHH;
	} else if (L1SAP_IS_CHAN_SDCCH4(chan_nr)) {
		subCh = L1SAP_CHAN2SS_SDCCH4(chan_nr);
		sapi = cOCTVC1_GSM_SAPI_ENUM_SDCCH;
	} else if (L1SAP_IS_CHAN_SDCCH8(chan_nr)) {
		subCh = L1SAP_CHAN2SS_SDCCH8(chan_nr);
		sapi = cOCTVC1_GSM_SAPI_ENUM_SDCCH;
	} else if (L1SAP_IS_CHAN_BCCH(chan_nr)) {
		sapi = cOCTVC1_GSM_SAPI_ENUM_BCCH;
	} else if (L1SAP_IS_CHAN_AGCH_PCH(chan_nr)) {
		sapi = cOCTVC1_GSM_SAPI_ENUM_PCH_AGCH;
	} else {
		LOGPFN(DL1C, LOGL_NOTICE, u32Fn, "unknown prim %d op %d chan_nr %d link_id %d\n",
		     l1sap->oph.primitive, l1sap->oph.operation, chan_nr, link_id);
		rc = -EINVAL;
		goto done;
	}

	if (len) {
		/* create new PHY primitive in l1msg, copying payload */

		l1msg = l1p_msgb_alloc();
		if (!l1msg) {
			LOGPFN(DL1C, LOGL_FATAL, u32Fn, "L1SAP PH-DATA.req msg alloc failed\n");
			rc = -ENOMEM;
			goto done;
		}

		tOCTVC1_GSM_MSG_TRX_REQUEST_LOGICAL_CHANNEL_DATA_CMD *data_req =
			(tOCTVC1_GSM_MSG_TRX_REQUEST_LOGICAL_CHANNEL_DATA_CMD *)
				msgb_put(l1msg, sizeof(*data_req));

		l1if_fill_msg_hdr(&data_req->Header, l1msg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  	  cOCTVC1_GSM_MSG_TRX_REQUEST_LOGICAL_CHANNEL_DATA_CID);

		data_req->TrxId.byTrxId = pinst->u.octphy.trx_id;
		data_req->LchId.byTimeslotNb = u8Tn;
		data_req->LchId.bySAPI = sapi;
		data_req->LchId.bySubChannelNb = subCh;
		data_req->LchId.byDirection = cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS;
		data_req->Data.ulFrameNumber = u32Fn;
		data_req->Data.ulDataLength = msgb_l2len(msg);
		memcpy(data_req->Data.abyDataContent, msg->l2h, msgb_l2len(msg));

		mOCTVC1_GSM_MSG_TRX_REQUEST_LOGICAL_CHANNEL_DATA_CMD_SWAP(data_req);
	} else {
		/* No data available, Don't send Empty frame to PHY */
		rc = 0;
		goto done;
	}

	rc = l1if_req_compl(fl1h, l1msg, NULL, NULL);
done:
	return rc;
}


static int ph_tch_req(struct gsm_bts_trx *trx, struct msgb *msg,
		      struct osmo_phsap_prim *l1sap)
{
	struct phy_instance *pinst = trx_phy_instance(trx);
	struct octphy_hdl *fl1h = pinst->phy_link->u.octphy.hdl;
	struct gsm_lchan *lchan;
	uint32_t u32Fn;
	uint8_t u8Tn, subCh, sapi;
	uint8_t chan_nr;
	struct msgb *nmsg = NULL;

	chan_nr = l1sap->u.tch.chan_nr;
	u32Fn = l1sap->u.tch.fn;
	u8Tn = L1SAP_CHAN2TS(chan_nr);
	if (L1SAP_IS_CHAN_TCHH(chan_nr)) {
		subCh = L1SAP_CHAN2SS_TCHH(chan_nr);
		sapi = cOCTVC1_GSM_SAPI_ENUM_TCHH;
	} else {
		subCh = 0xf1;
		sapi = cOCTVC1_GSM_SAPI_ENUM_TCHF;
	}

	lchan = get_lchan_by_chan_nr(trx, chan_nr);

	/* create new message and fill data */
	if (msg) {
		nmsg = l1p_msgb_alloc();
		if (!nmsg) {
			LOGPFN(DL1C, LOGL_FATAL, u32Fn, "L1SAP PH-TCH.req msg alloc failed\n");
			return -ENOMEM;
		}

		msgb_pull(msg, sizeof(*l1sap));
		tOCTVC1_GSM_MSG_TRX_REQUEST_LOGICAL_CHANNEL_DATA_CMD *data_req =
			(tOCTVC1_GSM_MSG_TRX_REQUEST_LOGICAL_CHANNEL_DATA_CMD *)
			msgb_put(nmsg, sizeof(*data_req));

		mOCTVC1_GSM_MSG_TRX_REQUEST_LOGICAL_CHANNEL_DATA_CMD_DEF(data_req);

		l1if_fill_msg_hdr(&data_req->Header, nmsg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  	  cOCTVC1_GSM_MSG_TRX_REQUEST_LOGICAL_CHANNEL_DATA_CID);

		data_req->TrxId.byTrxId = pinst->u.octphy.trx_id;
		data_req->LchId.byTimeslotNb = u8Tn;
		data_req->LchId.bySAPI = sapi;
		data_req->LchId.bySubChannelNb = subCh;
		data_req->LchId.byDirection =
		    cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS;
		data_req->Data.ulFrameNumber = u32Fn;

		l1if_tch_encode(lchan,
				&data_req->Data.ulPayloadType,
				data_req->Data.abyDataContent,
				&data_req->Data.ulDataLength,
				msg->data, msg->len);

		mOCTVC1_GSM_MSG_TRX_REQUEST_LOGICAL_CHANNEL_DATA_CMD_SWAP(data_req);
	} else {
		/* No data available, Don't send Empty frame to PHY */
		return 0;
	}

	return l1if_req_compl(fl1h, nmsg, NULL, NULL);
}

static int mph_info_req(struct gsm_bts_trx *trx, struct msgb *msg,
			struct osmo_phsap_prim *l1sap)
{
	uint8_t chan_nr;
	struct gsm_lchan *lchan;
	int rc = 0;

	switch (l1sap->u.info.type) {
	case PRIM_INFO_ACT_CIPH:
		chan_nr = l1sap->u.info.u.ciph_req.chan_nr;
		lchan = get_lchan_by_chan_nr(trx, chan_nr);
		if (l1sap->u.info.u.ciph_req.uplink) {
			l1if_set_ciphering(lchan, 0);
			lchan->ciph_state = LCHAN_CIPH_RX_REQ;
		}
		if (l1sap->u.info.u.ciph_req.downlink) {
			l1if_set_ciphering(lchan, 1);
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
#pragma message ("Mode Modify is currently not supported for Octasic PHY (OS#3015)")
			/* l1if_rsl_mode_modify(lchan); */
		} else if (l1sap->u.info.u.act_req.sacch_only)
			l1if_rsl_deact_sacch(lchan);
		else
			l1if_rsl_chan_rel(lchan);
		break;
	default:
		LOGP(DL1C, LOGL_NOTICE, "unknown L1SAP MPH-INFO.req %d\n",
		     l1sap->u.info.type);
		rc = -EINVAL;
	}

	return rc;
}

/* primitive from common part. We are taking ownership of msgb */
int bts_model_l1sap_down(struct gsm_bts_trx *trx, struct osmo_phsap_prim *l1sap)
{
	struct msgb *msg = l1sap->oph.msg;
	int rc = 0;

	/* called functions MUST NOT take ownership of msgb, as it is
	 * free()d below */
	switch (OSMO_PRIM_HDR(&l1sap->oph)) {
	case OSMO_PRIM(PRIM_PH_DATA, PRIM_OP_REQUEST):
		rc = ph_data_req(trx, msg, l1sap);
		break;
	case OSMO_PRIM(PRIM_TCH, PRIM_OP_REQUEST):
		rc = ph_tch_req(trx, msg, l1sap);
		break;
	case OSMO_PRIM(PRIM_MPH_INFO, PRIM_OP_REQUEST):
		rc = mph_info_req(trx, msg, l1sap);
		break;
	default:
		LOGP(DL1C, LOGL_NOTICE, "L1SAP unknown prim %d op %d\n",
		     l1sap->oph.primitive, l1sap->oph.operation);
		rc = -EINVAL;
	}

	msgb_free(msg);

	return rc;
}

static int trx_close_all_cb(struct octphy_hdl *fl1, struct msgb *resp, void *data)
{
	tOCTVC1_GSM_MSG_TRX_CLOSE_ALL_RSP *car =
		(tOCTVC1_GSM_MSG_TRX_CLOSE_ALL_RSP *) resp->l2h;

	/* in a completion call-back, we take msgb ownership and must
	 * release it before returning */

	mOCTVC1_GSM_MSG_TRX_CLOSE_ALL_RSP_SWAP(car);

	/* we now know that the PHY link is connected */
	phy_link_state_set(fl1->phy_link, PHY_LINK_CONNECTED);

	msgb_free(resp);

	return 0;
}

static int phy_link_trx_close_all(struct phy_link *plink)
{
	struct octphy_hdl *fl1h = plink->u.octphy.hdl;
	struct msgb *msg = l1p_msgb_alloc();
	tOCTVC1_GSM_MSG_TRX_CLOSE_ALL_CMD *cac;

	cac = (tOCTVC1_GSM_MSG_TRX_CLOSE_ALL_CMD *)
				msgb_put(msg, sizeof(*cac));
	l1if_fill_msg_hdr(&cac->Header, msg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  cOCTVC1_GSM_MSG_TRX_CLOSE_ALL_CID);

	mOCTVC1_GSM_MSG_TRX_CLOSE_ALL_CMD_SWAP(cac);

	return l1if_req_compl(fl1h, msg, trx_close_all_cb, NULL);
}

int bts_model_phy_link_open(struct phy_link *plink)
{
	if (plink->u.octphy.hdl)
		l1if_close(plink->u.octphy.hdl);

	phy_link_state_set(plink, PHY_LINK_CONNECTING);

	plink->u.octphy.hdl = l1if_open(plink);
	if (!plink->u.octphy.hdl) {
		phy_link_state_set(plink, PHY_LINK_SHUTDOWN);
		return -1;
	}

	/* do we need to iterate over the list of instances and do some
	 * instance-specific initialization? */

	/* close all TRXs that might still exist in this link from
	 * previous execitions / sessions */
	phy_link_trx_close_all(plink);

	/* in the call-back to the above we will set the link state to
	 * connected */

	return 0;
}

int bts_model_init(struct gsm_bts *bts)
{
	LOGP(DL1C, LOGL_NOTICE, "model_init()\n");

	bts->variant = BTS_OSMO_OCTPHY;
	bts->support.ciphers = CIPHER_A5(1) | CIPHER_A5(2) | CIPHER_A5(3);

	/* FIXME: what is the nominal transmit power of the PHY/board? */
	bts->c0->nominal_power = 15;

	gsm_bts_set_feature(bts, BTS_FEAT_GPRS);
	gsm_bts_set_feature(bts, BTS_FEAT_OML_ALERTS);
#if defined(cOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM_FCCH_SCH_BCCH_CCCH_SDCCH4_CBCH_SACCHC4) && defined(cOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM_SDCCH8_CBCH_SACCHC8)
	gsm_bts_set_feature(bts, BTS_FEAT_CBCH);
#endif
	gsm_bts_set_feature(bts, BTS_FEAT_SPEECH_F_V1);
	gsm_bts_set_feature(bts, BTS_FEAT_SPEECH_H_V1);

	bts_model_vty_init(bts);

	return 0;
}

/***********************************************************************
 * handling of messages coming up from PHY
 ***********************************************************************/

/* When the measurement indication is received from the phy, the phy will
 * automatically stamp it with the frame number that matches the frame
 * number of the SACCH channel that marks the end of the measurement
 * period. (e.g. fn%104=90, on a TCH/H, TS0). However, the upper layers
 * expect the frame number to be aligned to the next SACCH frame after,
 * after the end of the measurement period that has just passed. (e.g.
 * (fn%104=10, on a TCH/H, TS0). The following function remaps the frame
 * number in order to match the higher layers expectations.
 * See also: 3GPP TS 05.02 Clause 7 Table 1 of 9  Mapping of logical channels
 * onto physical channels (see subclauses 6.3, 6.4, 6.5) */
static uint32_t translate_tch_meas_rep_fn104_reverse(uint32_t fn)
{
	uint8_t new_fn_mod;
	uint8_t fn_mod;

	fn_mod = fn % 104;

	switch (fn_mod) {
	case 103:
		new_fn_mod = 25;
		break;
	case 12:
		new_fn_mod = 38;
		break;
	case 25:
		new_fn_mod = 51;
		break;
	case 38:
		new_fn_mod = 64;
		break;
	case 51:
		new_fn_mod = 77;
		break;
	case 64:
		new_fn_mod = 90;
		break;
	case 77:
		new_fn_mod = 103;
		break;
	case 90:
		new_fn_mod = 12;
		break;
	default:
		/* No translation for frame numbers
		 * fall out of the raster */
		new_fn_mod = fn_mod;
	}

	return (fn - fn_mod) + new_fn_mod;
}

static unsigned int oct_meas2ber10k(const tOCTVC1_GSM_MEASUREMENT_INFO *m)
{
	if (m->usBERTotalBitCnt != 0) {
		return (unsigned int)((m->usBERCnt * BER_10K) / m->usBERTotalBitCnt);
	} else {
		return 0;
	}
}

static int oct_meas2rssi_dBm(const tOCTVC1_GSM_MEASUREMENT_INFO *m)
{
	/* rssi is in q8 format */
	return (m->sRSSIDbm >> 8);
}

static void process_meas_res(struct gsm_bts_trx *trx, uint8_t chan_nr,
			     uint32_t fn, uint32_t data_len,
			     tOCTVC1_GSM_MEASUREMENT_INFO * m)
{
	struct osmo_phsap_prim l1sap;

	memset(&l1sap, 0, sizeof(l1sap));
	osmo_prim_init(&l1sap.oph, SAP_GSM_PH, PRIM_MPH_INFO,
		       PRIM_OP_INDICATION, NULL);
	l1sap.u.info.type = PRIM_INFO_MEAS;
	l1sap.u.info.u.meas_ind.chan_nr = chan_nr;

	/* Update Timing offset for valid radio block */
	if (data_len != 0) {
		/* burst timing in 1x */
		l1sap.u.info.u.meas_ind.ta_offs_256bits = m->sBurstTiming4x*64;
	} else {
		/* FIXME, In current implementation, OCTPHY would send DATA_IND
		 * for all radio blocks (valid or invalid) But timing offset
		 * is only correct for valid block.  so we need different
		 * counter to accumulate Timing offset.. even we add zero for
		 * invalid block.. timing offset average calucation would not
		 * correct. */
		l1sap.u.info.u.meas_ind.ta_offs_256bits = 0;
	}

	l1sap.u.info.u.meas_ind.ber10k = oct_meas2ber10k(m);

	/* rssi is in q8 format */
	l1sap.u.info.u.meas_ind.inv_rssi = (uint8_t) oct_meas2rssi_dBm(m);

	/* copy logical frame number to MEAS IND data structure */
	l1sap.u.info.u.meas_ind.fn = translate_tch_meas_rep_fn104_reverse(fn);

	/* l1sap wants to take msgb ownership.  However, as there is no
	 * msg, it will msgb_free(l1sap.oph.msg == NULL) */
	l1sap_up(trx, &l1sap);
}

static void dump_meas_res(int ll, tOCTVC1_GSM_MEASUREMENT_INFO * m)
{
	LOGP(DMEAS, ll,
	     "Meas: RSSI %d dBm, Burst Timing %d Quarter of bits :%d, "
	     "BER Error Count %d , BER Toatal Bit count %d in last decoded frame\n",
	     m->sRSSIDbm, m->sBurstTiming, m->sBurstTiming4x, m->usBERCnt,
	     m->usBERTotalBitCnt);
}

static int handle_mph_time_ind(struct octphy_hdl *fl1, uint8_t trx_id, uint32_t fn)
{
	struct gsm_bts_trx *trx = trx_by_l1h(fl1, trx_id);
	struct osmo_phsap_prim l1sap;

	/* increment the primitive count for the alive timer */
	fl1->alive_prim_cnt++;

	/* ignore every time indication, except for c0 */
	if (trx != trx->bts->c0)
		return 0;

	memset(&l1sap, 0, sizeof(l1sap));
	osmo_prim_init(&l1sap.oph, SAP_GSM_PH, PRIM_MPH_INFO,
		       PRIM_OP_INDICATION, NULL);
	l1sap.u.info.type = PRIM_INFO_TIME;
	l1sap.u.info.u.time_ind.fn = fn;

	l1sap_up(trx, &l1sap);

	return 0;
}

static int handle_ph_readytosend_ind(struct octphy_hdl *fl1,
	tOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_READY_TO_SEND_INDICATION_EVT *evt,
	struct msgb *l1p_msg)
{
	struct gsm_bts_trx *trx = trx_by_l1h(fl1, evt->TrxId.byTrxId);
	struct gsm_bts *bts = trx->bts;
	struct osmo_phsap_prim *l1sap;
	struct gsm_time g_time;
	uint8_t chan_nr, link_id;
	uint32_t fn;
	int rc;
	uint32_t t3p;
	uint8_t ts_num, sc, sapi;

	struct msgb *resp_msg;
	tOCTVC1_GSM_MSG_TRX_REQUEST_LOGICAL_CHANNEL_DATA_CMD *data_req;

	/* Retrive the data */
	fn = evt->ulFrameNumber;
	ts_num = (uint8_t) evt->LchId.byTimeslotNb;
	sc = (uint8_t) evt->LchId.bySubChannelNb;
	sapi = (uint8_t) evt->LchId.bySAPI;

	gsm_fn2gsmtime(&g_time, fn);

	DEBUGPGT(DL1P, &g_time, "Rx PH-RTS.ind SAPI=%s\n",
	       get_value_string(octphy_l1sapi_names, sapi));

	/* in case we need to forward primitive to common part */
	chan_nr = chan_nr_by_sapi(&trx->ts[ts_num], sapi, sc, ts_num, fn);
	if (chan_nr) {
		if (sapi == cOCTVC1_GSM_SAPI_ENUM_SACCH)
			link_id = LID_SACCH;
		else
			link_id = LID_DEDIC;

		rc = msgb_trim(l1p_msg, sizeof(*l1sap));
		if (rc < 0)
			MSGB_ABORT(l1p_msg, "No room for primitive\n");
		l1sap = msgb_l1sap_prim(l1p_msg);
		if (sapi == cOCTVC1_GSM_SAPI_ENUM_TCHF
		    || sapi == cOCTVC1_GSM_SAPI_ENUM_TCHH) {
			osmo_prim_init(&l1sap->oph, SAP_GSM_PH, PRIM_TCH_RTS,
				       PRIM_OP_INDICATION, l1p_msg);
			l1sap->u.data.link_id = link_id;
			l1sap->u.tch.chan_nr = chan_nr;
			l1sap->u.tch.fn = fn;
		} else {
			osmo_prim_init(&l1sap->oph, SAP_GSM_PH, PRIM_PH_RTS,
				       PRIM_OP_INDICATION, l1p_msg);
			l1sap->u.data.link_id = link_id;
			l1sap->u.data.chan_nr = chan_nr;
			l1sap->u.data.fn = fn;
		}

		l1sap_up(trx, l1sap);

		/* return '1' to indicate l1sap_up has taken msgb ownership */
		return 1;
	}

	/* in all other cases, we need to allocate a new PH-DATA.ind
	 * primitive msgb and start to fill it */
	resp_msg = l1p_msgb_alloc();
	data_req = (tOCTVC1_GSM_MSG_TRX_REQUEST_LOGICAL_CHANNEL_DATA_CMD *)
		msgb_put(resp_msg, sizeof(*data_req));

	mOCTVC1_GSM_MSG_TRX_REQUEST_LOGICAL_CHANNEL_DATA_CMD_DEF(data_req);

	l1if_fill_msg_hdr(&data_req->Header, resp_msg, fl1, cOCTVC1_MSG_TYPE_COMMAND,
		  	  cOCTVC1_GSM_MSG_TRX_REQUEST_LOGICAL_CHANNEL_DATA_CID);

	data_req_from_rts_ind(data_req, evt);

	switch (sapi) {
		/* TODO: SCH via L1SAP */
	case cOCTVC1_GSM_SAPI_ENUM_SCH:
		/* compute T3prime */
		t3p = (g_time.t3 - 1) / 10;
		/* fill SCH burst with data */
		data_req->Data.ulDataLength = 4;
		data_req->Data.abyDataContent[0] =
		    	(bts->bsic << 2) | (g_time.t1 >> 9);
		data_req->Data.abyDataContent[1] = (g_time.t1 >> 1);
		data_req->Data.abyDataContent[2] =
		    	(g_time.t1 << 7) | (g_time.t2 << 2) | (t3p >> 1);
		data_req->Data.abyDataContent[3] = (t3p & 1);
		break;
	case cOCTVC1_GSM_SAPI_ENUM_CBCH:
		rc = bts_cbch_get(bts, data_req->Data.abyDataContent, &g_time);
		data_req->Data.ulDataLength = 23;   /* GSM MAX BLK SIZE */
		break;
	case cOCTVC1_GSM_SAPI_ENUM_PRACH:
#if 0
		/* in case we decide to send an empty frame... */

		tOCTVC1_GSM_MSG_TRX_REQUEST_LOGICAL_CHANNEL_EMPTY_FRAME_CMD
			    *empty_frame_req =
			    (tOCTVC1_GSM_MSG_TRX_REQUEST_LOGICAL_CHANNEL_EMPTY_FRAME_CMD
			     *) msgSendBuffer;

		empty_req_from_rts_ind(empty_frame_req, evt);

		/* send empty frame request */
		rc = Logical_Channel_Empty_Frame_Cmd(empty_frame_req);
		if (cOCTVC1_RC_OK != rc) {
			LOGPGT(DL1P, LOGL_ERROR, &g_time,
			     "Sending Empty Frame Request Failed! (%s)\n",
			     octvc1_rc2string(rc));
		}
		break;
#endif
	default:
		LOGPGT(DL1P, LOGL_ERROR, &g_time, "SAPI %s not handled via L1SAP!\n",
			get_value_string(octphy_l1sapi_names, sapi));
#if 0
		data_req->Data.ulDataLength = GSM_MACBLOCK_LEN;
		memcpy(data_req->Data.abyDataContent, fill_frame,
		       GSM_MACBLOCK_LEN);
#endif
		break;
	}

	mOCTVC1_GSM_MSG_TRX_REQUEST_LOGICAL_CHANNEL_DATA_CMD_SWAP(data_req);

	return l1if_req_compl(fl1, resp_msg, NULL, NULL);
}

static int handle_ph_data_ind(struct octphy_hdl *fl1,
		tOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_DATA_INDICATION_EVT *data_ind,
		struct msgb *l1p_msg)
{
	struct gsm_bts_trx *trx = trx_by_l1h(fl1, data_ind->TrxId.byTrxId);
	uint8_t chan_nr, link_id;
	struct osmo_phsap_prim *l1sap;
	uint32_t fn;
	uint8_t *data;
	uint16_t len;
	int16_t snr;
	int rc;

	uint8_t sapi = (uint8_t) data_ind->LchId.bySAPI;
	uint8_t ts_num = (uint8_t) data_ind->LchId.byTimeslotNb;
	uint8_t sc = (uint8_t) data_ind->LchId.bySubChannelNb;

	/* Need to combine two 16bit MSB and LSB to form 32bit FN */
	fn = data_ind->Data.ulFrameNumber;

	/* chan_nr and link_id */
	chan_nr = chan_nr_by_sapi(&trx->ts[ts_num], sapi, sc, ts_num, fn);
	if (!chan_nr) {
		LOGPFN(DL1C, LOGL_ERROR, fn, "Rx PH-DATA.ind for unknown L1 SAPI %s\n",
		     get_value_string(octphy_l1sapi_names, sapi));
		return ENOTSUP;
	}

	if (sapi == cOCTVC1_GSM_SAPI_ENUM_SACCH)
		link_id = LID_SACCH;
	else
		link_id = LID_DEDIC;

	memset(&l1sap, 0, sizeof(l1sap));

	/* uplink measurement */
	process_meas_res(trx, chan_nr, fn, data_ind->Data.ulDataLength,
			 &data_ind->MeasurementInfo);

	DEBUGPFN(DL1C, fn, "Rx PH-DATA.ind %s: %s data_len:%d \n",
		 get_value_string(octphy_l1sapi_names, sapi),
		 osmo_hexdump(data_ind->Data.abyDataContent, data_ind->Data.ulDataLength),
		 data_ind->Data.ulDataLength);

	/* check for TCH */
	if (sapi == cOCTVC1_GSM_SAPI_ENUM_TCHF ||
	    sapi == cOCTVC1_GSM_SAPI_ENUM_TCHH) {
		/* TCH speech frame handling */
		rc = l1if_tch_rx(trx, chan_nr, data_ind);
		return rc;
	}

	/* get data pointer and length */
	data = data_ind->Data.abyDataContent;
	len = data_ind->Data.ulDataLength;
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

#if (cOCTVC1_MAIN_VERSION_ID >= cOCTVC1_MAIN_VERSION_ID_FN_PARADIGM_CHG)
	if (sapi == cOCTVC1_GSM_SAPI_ENUM_PDTCH) {
		/* FIXME::PCU is expecting encode frame number*/
		l1sap->u.data.fn = fn - 3;
	} else
		l1sap->u.data.fn = fn;
#else
	l1sap->u.data.fn = fn;
#endif

	l1sap->u.data.rssi = oct_meas2rssi_dBm(&data_ind->MeasurementInfo);
	l1sap->u.data.ber10k = oct_meas2ber10k(&data_ind->MeasurementInfo);

	/* burst timing  in 1x but PCU is expecting 4X */
	l1sap->u.data.ta_offs_256bits = data_ind->MeasurementInfo.sBurstTiming4x*64;
	snr = data_ind->MeasurementInfo.sSNRDb;
	/* FIXME: better converion formulae for SnR -> C / I?
	l1sap->u.data.lqual_cb = (snr ? snr : (snr - 65536)) * 10 / 256;
	LOGP(DL1C, LOGL_ERROR, "SnR: raw %d, computed %d\n", snr, l1sap->u.data.lqual_cb);
	*/
	l1sap->u.data.lqual_cb = (snr ? snr : (snr - 65536)) * 100;
	l1sap->u.data.pdch_presence_info = PRES_INFO_BOTH; /* FIXME: consider EDGE support */

	l1sap_up(trx, l1sap);

	/* return '1' to indicate that l1sap_up has taken msgb ownership */
	return 1;
}

static int handle_ph_rach_ind(struct octphy_hdl *fl1,
		tOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_RACH_INDICATION_EVT *ra_ind,
		struct msgb *l1p_msg)
{
	struct gsm_bts_trx *trx = trx_by_l1h(fl1, ra_ind->TrxId.byTrxId);
	struct osmo_phsap_prim *l1sap;
	int rc;
	struct ph_rach_ind_param rach_ind_param;

	dump_meas_res(LOGL_DEBUG, &ra_ind->MeasurementInfo);

	if (ra_ind->ulMsgLength != 1) {
		LOGPFN(DL1C, LOGL_ERROR, ra_ind->ulFrameNumber,
			"Rx PH-RACH.ind has lenghth %d > 1\n", ra_ind->ulMsgLength);
		msgb_free(l1p_msg);
		return 0;
	}

	/* We need to evaluate ra_ind before below msgb_trim(), since that invalidates *ra_ind. */
	rach_ind_param = (struct ph_rach_ind_param) {
		/* .chan_nr set below */
		.ra = ra_ind->abyMsg[0],
		/* .acc_delay set below */
		.fn = ra_ind->ulFrameNumber,
		.is_11bit = 0,
		/* .burst_type remains unset */
		.rssi = oct_meas2rssi_dBm(&ra_ind->MeasurementInfo),
		.ber10k = oct_meas2ber10k(&ra_ind->MeasurementInfo),
		.acc_delay_256bits = ra_ind->MeasurementInfo.sBurstTiming4x * 64,
	};

	if (ra_ind->LchId.bySubChannelNb == cOCTVC1_GSM_ID_SUB_CHANNEL_NB_ENUM_ALL &&
	    ra_ind->LchId.bySAPI == cOCTVC1_GSM_SAPI_ENUM_RACH) {
		rach_ind_param.chan_nr = 0x88;
	} else {
		struct gsm_lchan *lchan = get_lchan_by_lchid(trx, &ra_ind->LchId);
		rach_ind_param.chan_nr = gsm_lchan2chan_nr(lchan);
	}

	/* check for under/overflow / sign */
	if (ra_ind->MeasurementInfo.sBurstTiming < 0)
		rach_ind_param.acc_delay = 0;
	else
		rach_ind_param.acc_delay = ra_ind->MeasurementInfo.sBurstTiming;

	/* msgb_trim() invalidates ra_ind, make that abundantly clear: */
	ra_ind = NULL;
	rc = msgb_trim(l1p_msg, sizeof(*l1sap));
	if (rc < 0)
		MSGB_ABORT(l1p_msg, "No room for primitive\n");
	l1sap = msgb_l1sap_prim(l1p_msg);
	osmo_prim_init(&l1sap->oph, SAP_GSM_PH, PRIM_PH_RACH, PRIM_OP_INDICATION,
			l1p_msg);
	l1sap->u.rach_ind = rach_ind_param;

	l1sap_up(trx, l1sap);

	/* return '1' to indicate l1sap_up has taken msgb ownership */
	return 1;
}

static int rx_gsm_trx_time_ind(struct msgb *msg)
{
	struct octphy_hdl *fl1h = msg->dst;
	tOCTVC1_GSM_MSG_TRX_TIME_INDICATION_EVT *tind =
		(tOCTVC1_GSM_MSG_TRX_TIME_INDICATION_EVT *) msg->l2h;

	mOCTVC1_GSM_MSG_TRX_TIME_INDICATION_EVT_SWAP(tind);

	return handle_mph_time_ind(fl1h, tind->TrxId.byTrxId, tind->ulFrameNumber);
}

/* mark this message as RETRANSMIT of a previous msg */
static void msg_set_retrans_flag(struct msgb *msg)
{
	tOCTVC1_MSG_HEADER *mh = (tOCTVC1_MSG_HEADER *) msg->l2h;
	uint32_t type_r_cmdid = ntohl(mh->ul_Type_R_CmdId);
	type_r_cmdid |= cOCTVC1_MSG_RETRANSMIT_FLAG;
	mh->ul_Type_R_CmdId = htonl(type_r_cmdid);
}

/* re-transmit all commands in the window that have a transaction ID lower than
 * trans_id */
static int retransmit_wlc_upto(struct octphy_hdl *fl1h, uint32_t trans_id)
{
	struct wait_l1_conf *wlc;
	int count = 0;

	LOGP(DL1C, LOGL_INFO, "Retransmitting up to trans_id=%u\n", trans_id);

	/* trans_id represents the trans_id of the just-received response, we
	 * therefore need to re-send any commands with a lower trans_id */
	llist_for_each_entry(wlc, &fl1h->wlc_list, list) {
		if (wlc->trans_id <= trans_id) {
			struct msgb *msg;
			if (wlc->num_retrans >= MAX_RETRANS) {
				LOGP(DL1C, LOGL_ERROR, "Command %s: maximum "
				     "number of retransmissions reached\n",
				     get_value_string(octphy_cid_vals,
						      wlc->prim_id));
				exit(24);
			}
			wlc->num_retrans++;
			msg = msgb_copy(wlc->cmd_msg, "PHY CMD Retrans");
			msg_set_retrans_flag(msg);
			osmo_wqueue_enqueue(&fl1h->phy_wq, msg);
			osmo_timer_schedule(&wlc->timer, CMD_TIMEOUT, 0);
			count++;
			LOGP(DL1C, LOGL_INFO, "Re-transmitting %s "
			     "(trans_id=%u, attempt %u)\n",
			     get_value_string(octphy_cid_vals, wlc->prim_id),
			     wlc->trans_id, wlc->num_retrans);
		}
	}

	return count;
}

/* Receive a response (to a prior command) from the PHY */
static int rx_octvc1_resp(struct msgb *msg, uint32_t msg_id, uint32_t trans_id)
{
	tOCTVC1_MSG_HEADER *mh = (tOCTVC1_MSG_HEADER *) msg->l2h;
	struct llist_head *first;
	uint32_t return_code = ntohl(mh->ulReturnCode);
	struct octphy_hdl *fl1h = msg->dst;
	struct wait_l1_conf *wlc = NULL;
	int rc;

	LOGP(DL1C, LOGL_DEBUG, "rx_octvc1_resp(msg_id=%s, trans_id=%u)\n",
		octvc1_rc2string(msg_id), trans_id);

	/* check if the response is for the oldest (first) entry in wlc_list */
	first = llist_first(&fl1h->wlc_list);
	if (first) {
		wlc = llist_entry(first, struct wait_l1_conf, list);
		if (wlc->trans_id == trans_id) {
			/* process the received response */
			llist_del(&wlc->list);
			fl1h->wlc_list_len--;
			if (wlc->cb) {
				/* call-back function must take msgb
				 * ownership. */
				rc = wlc->cb(fl1h, msg, wlc->cb_data);
			} else {
				rc = 0;
				msgb_free(msg);
			}
			release_wlc(wlc);
			/* check if there are postponed wlcs and re-fill the window */
			check_refill_window(fl1h, NULL);
			return rc;
		}
	}

	LOGP(DL1C, LOGL_NOTICE, "Sequence error: Rx response (cmd=%s, trans_id=%u) "
	     "for cmd != oldest entry in window (trans_id=%u)!!\n",
	     get_value_string(octphy_cid_vals, msg_id), trans_id,
	     wlc ? wlc->trans_id : 0);

	/* check if the response is for any of the other entries in wlc_list */
	llist_for_each_entry(wlc, &fl1h->wlc_list, list) {
		if (wlc->prim_id == msg_id && wlc->trans_id == trans_id) {
			/* it is assumed that all of the previous response
			 * message(s) have been lost, and we need to
			 * re-transmit older messages from the window */
			rc = retransmit_wlc_upto(fl1h, trans_id);
			fl1h->stats.retrans_cmds_trans_id += rc;
			/* do not process the received response, we rather wait
			 * for the in-order retransmissions to arrive */
			msgb_free(msg);
			return 0;
		}
	}

	/* ignore unhandled responses that went ok, but let the user know about
	 * failing ones.  */
	if (return_code != cOCTVC1_RC_OK) {
		LOGP(DL1C, LOGL_NOTICE, "Rx Unexpected response %s (trans_id=%u)\n",
		     get_value_string(octphy_cid_vals, msg_id), trans_id);
	}
	msgb_free(msg);
	return 0;

}

static int rx_gsm_clockmgr_status_ind(struct msgb *msg)
{
	struct octphy_hdl *fl1h = msg->dst;
	tOCTVC1_HW_MSG_CLOCK_SYNC_MGR_STATUS_CHANGE_EVT *evt =
		(tOCTVC1_HW_MSG_CLOCK_SYNC_MGR_STATUS_CHANGE_EVT *) msg->l2h;
	mOCTVC1_HW_MSG_CLOCK_SYNC_MGR_STATUS_CHANGE_EVT_SWAP(evt);

	LOGP(DL1C, LOGL_NOTICE, "Rx ClkMgr Status Change Event: "
		"%s -> %s\n",
		get_value_string(octphy_clkmgr_state_vals, evt->ulPreviousState),
		get_value_string(octphy_clkmgr_state_vals, evt->ulState));

	fl1h->clkmgr_state = evt->ulState;

	return 0;
}

static int rx_gsm_trx_status_ind(struct msgb *msg)
{
	tOCTVC1_GSM_MSG_TRX_STATUS_CHANGE_EVT *evt =
		(tOCTVC1_GSM_MSG_TRX_STATUS_CHANGE_EVT *) msg->l2h;

	mOCTVC1_GSM_MSG_TRX_STATUS_CHANGE_EVT_SWAP(evt);

	if (evt->ulStatus == cOCTVC1_GSM_TRX_STATUS_ENUM_RADIO_READY)
		LOGP(DL1C, LOGL_INFO, "Rx TRX Status Event: READY\n");
	else
		LOGP(DL1C, LOGL_ERROR, "Rx TRX Status Event: %u\n",
			evt->ulStatus);

	return 0;
}

/* DATA indication from PHY */
static int rx_gsm_trx_lchan_data_ind(struct msgb *msg)
{
	tOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_DATA_INDICATION_EVT *evt =
		(tOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_DATA_INDICATION_EVT *) msg->l2h;
	mOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_DATA_INDICATION_EVT_SWAP(evt);

	return handle_ph_data_ind(msg->dst, evt, msg);
}

/* Ready-to-Send indication from PHY */
static int rx_gsm_trx_rts_ind(struct msgb *msg)
{
	tOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_READY_TO_SEND_INDICATION_EVT *evt =
		(tOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_READY_TO_SEND_INDICATION_EVT *) msg->l2h;
	mOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_READY_TO_SEND_INDICATION_EVT_SWAP(evt);

	return handle_ph_readytosend_ind(msg->dst, evt, msg);
}

/* RACH receive indication from PHY */
static int rx_gsm_trx_rach_ind(struct msgb *msg)
{
	tOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_RACH_INDICATION_EVT *evt =
		(tOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_RACH_INDICATION_EVT *) msg->l2h;
	mOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_RACH_INDICATION_EVT_SWAP(evt);

	return handle_ph_rach_ind(msg->dst, evt, msg);
}

/* Receive a notification (indication) from PHY */
static int rx_octvc1_notif(struct msgb *msg, uint32_t msg_id)
{
	const char *evt_name = get_value_string(octphy_eid_vals, msg_id);
	struct octphy_hdl *fl1h = msg->dst;
	int rc = 0;

	if (!fl1h->opened) {
		LOGP(DL1P, LOGL_NOTICE, "Rx NOTIF %s: Ignoring as PHY TRX "
			"hasn't been re-opened yet\n", evt_name);
		msgb_free(msg);
		return 0;
	}

	LOGP(DL1P, LOGL_DEBUG, "Rx NOTIF %s\n", evt_name);

	/* called functions MUST NOT take ownership of the msgb,
	 * as it is free()d below - unless they return 1 */
	switch (msg_id) {
	case cOCTVC1_GSM_MSG_TRX_TIME_INDICATION_EID:
		rc = rx_gsm_trx_time_ind(msg);
		break;
	case cOCTVC1_HW_MSG_CLOCK_SYNC_MGR_STATUS_CHANGE_EID:
		rc = rx_gsm_clockmgr_status_ind(msg);
		break;
	case cOCTVC1_GSM_MSG_TRX_STATUS_CHANGE_EID:
		rc = rx_gsm_trx_status_ind(msg);
		break;
	case cOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_DATA_INDICATION_EID:
		rc = rx_gsm_trx_lchan_data_ind(msg);
		break;
	case cOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_READY_TO_SEND_INDICATION_EID:
		rc = rx_gsm_trx_rts_ind(msg);
		break;
	case cOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_RACH_INDICATION_EID:
		rc = rx_gsm_trx_rach_ind(msg);
		break;
	case cOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_RAW_DATA_INDICATION_EID:
		LOGP(DL1P, LOGL_NOTICE, "Rx Unhandled event %s (%u)\n",
			evt_name, msg_id);
		break;
	default:
		LOGP(DL1P, LOGL_NOTICE, "Rx Unknown event %s (%u)\n",
			evt_name, msg_id);
	}

	/* Special return value '1' means: do not free */
	if (rc != 1)
		msgb_free(msg);

	return rc;
}

static int rx_octvc1_event_msg(struct msgb *msg)
{
	tOCTVC1_EVENT_HEADER *eh = (tOCTVC1_EVENT_HEADER *) msg->l2h;
	uint32_t event_id = ntohl(eh->ulEventId);
	uint32_t length = ntohl(eh->ulLength);
	/* DO NOT YET SWAP HEADER HERE, as downstream functions want to
	 * swap it */

	/* OCTSDKAN5001 Chapter 6.1 */
	if (length < 12 || length > 1024) {
		LOGP(DL1C, LOGL_ERROR, "Rx EVENT length %u invalid\n", length);
		msgb_free(msg);
		return -1;
	}

	/* verify / ensure length */
	if (msgb_l2len(msg) < length) {
		LOGP(DL1C, LOGL_ERROR, "Rx EVENT msgb_l2len(%u) < "
		     "event_msg_length (%u)\n", msgb_l2len(msg), length);
		msgb_free(msg);
		return -1;
	}

	return rx_octvc1_notif(msg, event_id);
}

/* Receive a supervisory message from the PHY */
static int rx_octvc1_supv(struct msgb *msg, uint32_t msg_id, uint32_t trans_id)
{
	struct octphy_hdl *fl1h = msg->dst;
	tOCTVC1_MSG_HEADER *mh = (tOCTVC1_MSG_HEADER *) msg->l2h;
	tOCTVC1_CTRL_MSG_MODULE_REJECT_SPV *rej;
	uint32_t return_code = ntohl(mh->ulReturnCode);
	uint32_t rejected_msg_id;
	int rc;

	switch (msg_id) {
	case cOCTVC1_CTRL_MSG_MODULE_REJECT_SID:
		rej = (tOCTVC1_CTRL_MSG_MODULE_REJECT_SPV *) mh;
		mOCTVC1_CTRL_MSG_MODULE_REJECT_SPV_SWAP(rej);
		rejected_msg_id = (rej->ulRejectedCmdId >> cOCTVC1_MSG_ID_BIT_OFFSET) &
								cOCTVC1_MSG_ID_BIT_MASK;
		LOGP(DL1C, LOGL_NOTICE, "Rx REJECT_SID (TID=%u, "
		     "ExpectedTID=0x%08x, RejectedCmdID=%s)\n",
		     trans_id, rej->ulExpectedTransactionId,
		     get_value_string(octphy_cid_vals, rejected_msg_id));
		rc = retransmit_wlc_upto(fl1h, trans_id);
		fl1h->stats.retrans_cmds_supv += rc;
		break;
	default:
		LOGP(DL1C, LOGL_NOTICE, "Rx unhandled supervisory msg_id "
			"%u: ReturnCode:%u\n", msg_id, return_code);
		break;
	}

	return 0;
}

static int rx_octvc1_ctrl_msg(struct msgb *msg)
{
	tOCTVC1_MSG_HEADER *mh = (tOCTVC1_MSG_HEADER *) msg->l2h;
	uint32_t length = ntohl(mh->ulLength);
	uint32_t type_r_cmdid = ntohl(mh->ul_Type_R_CmdId);
	uint32_t msg_type = (type_r_cmdid >> cOCTVC1_MSG_TYPE_BIT_OFFSET) &
						cOCTVC1_MSG_TYPE_BIT_MASK;
	uint32_t msg_id = (type_r_cmdid >> cOCTVC1_MSG_ID_BIT_OFFSET) &
						cOCTVC1_MSG_ID_BIT_MASK;
	uint32_t return_code = ntohl(mh->ulReturnCode);
	const char *msg_name = get_value_string(octphy_cid_vals, msg_id);

	/* DO NOT YET SWAP HEADER HERE, as downstream functions want to
	 * swap it */

	/* FIXME: OCTSDKAN5001 Chapter 3.1 states max size is 1024, but we see
	 * larger messages in practise */
	if (length < 24 || length > 2048) {
		LOGP(DL1C, LOGL_ERROR, "Rx CTRL length %u invalid\n", length);
		msgb_free(msg);
		return -1;
	}

	/* verify / ensure length */
	if (msgb_l2len(msg) < length) {
		LOGP(DL1C, LOGL_ERROR, "Rx CTRL msgb_l2len(%u) < "
		     "ctrl_msg_length (%u)\n", msgb_l2len(msg), length);
		msgb_free(msg);
		return -1;
	}

	LOGP(DL1P, LOGL_DEBUG, "Rx %s.resp (rc=%s(%x))\n", msg_name,
			octvc1_rc2string(return_code), return_code);

	if (return_code != cOCTVC1_RC_OK) {
		LOGP(DL1P, LOGL_ERROR, "%s failed, rc=%s\n",
			msg_name, octvc1_rc2string(return_code));
	}

	/* called functions must take ownership of msgb */
	switch (msg_type) {
	case cOCTVC1_MSG_TYPE_RESPONSE:
		return rx_octvc1_resp(msg, msg_id, ntohl(mh->ulTransactionId));
	case cOCTVC1_MSG_TYPE_SUPERVISORY:
		return rx_octvc1_supv(msg, msg_id, ntohl(mh->ulTransactionId));
	case cOCTVC1_MSG_TYPE_NOTIFICATION:
	case cOCTVC1_MSG_TYPE_COMMAND:
		LOGP(DL1C, LOGL_NOTICE, "Rx unhandled msg_type %s (%u)\n",
			msg_name, msg_type);
		msgb_free(msg);
		break;
	default:
		LOGP(DL1P, LOGL_NOTICE, "Rx unknown msg_type %s (%u)\n",
			msg_name, msg_type);
		msgb_free(msg);
	}

	return 0;
}

static int rx_octvc1_data_f_msg(struct msgb *msg)
{
	tOCTVOCNET_PKT_DATA_F_HEADER *datafh =
		(tOCTVOCNET_PKT_DATA_F_HEADER *) msg->l2h;
	uint32_t log_obj_port = ntohl(datafh->VocNetHeader.ulLogicalObjPktPort);

	msg->l2h = (uint8_t *) datafh + sizeof(*datafh);

	if (log_obj_port ==
	    cOCTVOCNET_PKT_DATA_LOGICAL_OBJ_PKT_PORT_EVENT_SESSION) {
		uint32_t sub_type = ntohl(datafh->ulSubType) & 0xF;
		if (sub_type == cOCTVOCNET_PKT_SUBTYPE_API_EVENT) {
			/* called function must take msgb ownership */
			return rx_octvc1_event_msg(msg);
		} else {
			LOGP(DL1C, LOGL_ERROR, "Unknown DATA_F "
				"subtype 0x%x\n", sub_type);
			}
	} else {
		LOGP(DL1C, LOGL_ERROR, "Unknown logical object pkt port 0x%x\n",
			log_obj_port);
	}

	msgb_free(msg);
	return 0;
}

/* main receive routine for messages coming up from OCTPHY */
static int rx_octphy_msg(struct msgb *msg)
{
	tOCTVOCNET_PKT_CTL_HEADER *ctlh;
	int rc = 0;

	/* we assume that the packets start right with the OCTPKT header
	 * and that the ethernet hardware header has already been
	 * stripped before */
	msg->l1h = msg->data;

	uint32_t ch = ntohl(*(uint32_t *) msg->data);
	uint32_t format = (ch >> cOCTVOCNET_PKT_FORMAT_BIT_OFFSET)
				& cOCTVOCNET_PKT_FORMAT_BIT_MASK;
	uint32_t len = (ch >> cOCTVOCNET_PKT_LENGTH_BIT_OFFSET)
				& cOCTVOCNET_PKT_LENGTH_MASK;

	if (len > msgb_length(msg)) {
		LOGP(DL1C, LOGL_ERROR, "Received length (%u) < length "
			"as per packet header (%u): %s\n", msgb_length(msg),
			len, osmo_hexdump(msgb_data(msg), msgb_length(msg)));
		msgb_free(msg);
		return -1;
	}

	/* we first need to decode the common OCTPKT header and dispatch
	 * based on contrl (command/resp) or data (event=indication) */
	switch (format) {
	case cOCTVOCNET_PKT_FORMAT_CTRL:
		ctlh = (tOCTVOCNET_PKT_CTL_HEADER *) (msg->l1h + 4);
		/* FIXME: check src/dest fifo, socket ID */
		msg->l2h = (uint8_t *) ctlh + sizeof(*ctlh);
		/* called function must take msgb ownership */
		rc = rx_octvc1_ctrl_msg(msg);
		break;
	case cOCTVOCNET_PKT_FORMAT_F:
		msg->l2h = msg->l1h + 4;
		/* called function must take msgb ownership */
		rc = rx_octvc1_data_f_msg(msg);
		break;
	default:
		LOGP(DL1C, LOGL_ERROR, "Rx Unknown pkt_format 0x%x\n",
			format);
		msgb_free(msg);
		break;
	}

	return rc;
}

void bts_model_phy_link_set_defaults(struct phy_link *plink)
{
	/* configure some reasonable defaults, to be overridden by VTY */
	plink->u.octphy.rf_port_index = 0;
	plink->u.octphy.rx_gain_db = 70;
	plink->u.octphy.tx_atten_db = 0;
}

void bts_model_phy_instance_set_defaults(struct phy_instance *pinst)
{
	pinst->u.octphy.trx_id = pinst->num;
}

/***********************************************************************
 * octphy socket / main loop integration
 ***********************************************************************/

static int octphy_read_cb(struct osmo_fd *ofd)
{
	struct sockaddr_ll sll;
	socklen_t sll_len = sizeof(sll);
	int rc;
	struct msgb *msg = msgb_alloc_headroom(1500, 24, "PHY Rx");

	if (!msg)
		return -ENOMEM;

	/* this is the fl1h over which the message was received */
	msg->dst = ofd->data;

	rc = recvfrom(ofd->fd, msg->data, msgb_tailroom(msg), 0,
			(struct sockaddr *) &sll, &sll_len);
	if (rc < 0) {
		LOGP(DL1C, LOGL_ERROR, "Error in recvfrom(): %s\n",
			strerror(errno));
		msgb_free(msg);
		return rc;
	}
	msgb_put(msg, rc);

	return rx_octphy_msg(msg);
}

static int octphy_write_cb(struct osmo_fd *fd, struct msgb *msg)
{
	struct octphy_hdl *fl1h = fd->data;
	int rc;

	/* send the message down the socket */
	rc = sendto(fd->fd, msg->data, msgb_length(msg), 0,
		    (struct sockaddr *) &fl1h->phy_addr,
		    sizeof(fl1h->phy_addr));

	/* core write uqueue takes care of free() */
	if (rc < 0) {
		LOGP(DL1P, LOGL_ERROR, "Tx to PHY has failed: %s\n",
			strerror(errno));
	}

	return rc;
}

struct octphy_hdl *l1if_open(struct phy_link *plink)
{
	struct octphy_hdl *fl1h;
	struct ifreq ifr;
	int sfd, rc;
	char *phy_dev = plink->u.octphy.netdev_name;

	fl1h = talloc_zero(plink, struct octphy_hdl);
	if (!fl1h)
		return NULL;

	INIT_LLIST_HEAD(&fl1h->wlc_list);
	INIT_LLIST_HEAD(&fl1h->wlc_postponed);
	fl1h->phy_link = plink;

	if (!phy_dev) {
		LOGP(DL1C, LOGL_ERROR, "You have to specify a octphy net-device\n");
		talloc_free(fl1h);
		return NULL;
	}

	LOGP(DL1C, LOGL_NOTICE, "Opening L1 interface for OctPHY (%s)\n",
		phy_dev);

	sfd = osmo_sock_packet_init(SOCK_DGRAM, cOCTPKT_HDR_ETHERTYPE,
				    phy_dev, OSMO_SOCK_F_NONBLOCK);
	if (sfd < 0) {
		LOGP(DL1C, LOGL_FATAL, "Error opening PHY socket: %s\n",
			strerror(errno));
		talloc_free(fl1h);
		return NULL;
	}

	/* resolve the string device name to an ifindex */
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, phy_dev, sizeof(ifr.ifr_name));
	rc = ioctl(sfd, SIOCGIFINDEX, &ifr);
	if (rc < 0) {
		LOGP(DL1C, LOGL_FATAL, "Error using network device %s: %s\n",
			phy_dev, strerror(errno));
		close(sfd);
		talloc_free(fl1h);
		return NULL;
	}

	fl1h->session_id = rand();

	/* set fl1h->phy_addr, which we use as sendto() destination */
	fl1h->phy_addr.sll_family = AF_PACKET;
	fl1h->phy_addr.sll_protocol = htons(cOCTPKT_HDR_ETHERTYPE);
	fl1h->phy_addr.sll_ifindex = ifr.ifr_ifindex;
	fl1h->phy_addr.sll_hatype = ARPHRD_ETHER;
	fl1h->phy_addr.sll_halen = ETH_ALEN;
	/* plink->phy_addr.sll_addr is filled by bts_model_vty code */
	memcpy(fl1h->phy_addr.sll_addr, plink->u.octphy.phy_addr.sll_addr,
		ETH_ALEN);

	/* Write queue / osmo_fd registration */
	osmo_wqueue_init(&fl1h->phy_wq, 10);
	fl1h->phy_wq.write_cb = octphy_write_cb;
	fl1h->phy_wq.read_cb = octphy_read_cb;
	fl1h->phy_wq.bfd.fd = sfd;
	fl1h->phy_wq.bfd.when = BSC_FD_READ;
	fl1h->phy_wq.bfd.cb = osmo_wqueue_bfd_cb;
	fl1h->phy_wq.bfd.data = fl1h;
	rc = osmo_fd_register(&fl1h->phy_wq.bfd);
	if (rc < 0) {
		close(sfd);
		talloc_free(fl1h);
		return NULL;
	}

	return fl1h;
}

int l1if_close(struct octphy_hdl *fl1h)
{
	osmo_fd_unregister(&fl1h->phy_wq.bfd);
	close(fl1h->phy_wq.bfd.fd);
	talloc_free(fl1h);

	return 0;
}
