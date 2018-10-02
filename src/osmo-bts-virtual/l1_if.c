/* Virtual BTS layer 1 primitive handling and interface
 *
 * Copyright (C) 2015-2017 Harald Welte <laforge@gnumonks.org>
 * Copyright (C) 2017 Sebastian Stumpf <sebastian.stumpf87@googlemail.com>
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

#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/bits.h>
#include <osmocom/core/linuxlist.h>
#include <osmocom/core/gsmtap.h>
#include <osmocom/core/gsmtap_util.h>
#include <osmocom/gsm/protocol/gsm_08_58.h>
#include <osmocom/gsm/rsl.h>

#include <osmo-bts/logging.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/oml.h>
#include <osmo-bts/rsl.h>
#include <osmo-bts/l1sap.h>
#include <osmo-bts/bts_model.h>
#include <osmo-bts/phy_link.h>
#include <osmo-bts/amr.h>
#include <osmo-bts/abis.h>
#include <osmo-bts/scheduler.h>
#include "virtual_um.h"

extern int vbts_sched_start(struct gsm_bts *bts);

static struct phy_instance *phy_instance_by_arfcn(struct phy_link *plink, uint16_t arfcn)
{
	struct phy_instance *pinst;

	llist_for_each_entry(pinst, &plink->instances, list) {
		if (pinst->trx && pinst->trx->arfcn == arfcn)
			return pinst;
	}

	return NULL;
}

static int l1if_process_meas_res(struct gsm_bts_trx *trx, uint8_t tn, uint32_t fn, uint8_t chan_nr,
				 int n_errors, int n_bits_total, float rssi, float toa);
/**
 * Callback to handle incoming messages from the MS.
 * The incoming message should be GSM_TAP encapsulated.
 * TODO: implement all channels
 */
static void virt_um_rcv_cb(struct virt_um_inst *vui, struct msgb *msg)
{
	struct phy_link *plink = (struct phy_link *)vui->priv;
	struct phy_instance *pinst;
	if (!msg) {
		pinst = phy_instance_by_num(plink, 0);
		bts_shutdown(pinst->trx->bts, "VirtPHY read socket died\n");
		return;
	}

	struct gsmtap_hdr *gh = msgb_l1(msg);
	uint32_t fn = ntohl(gh->frame_number);	/* frame number of the rcv msg */
	uint16_t arfcn = ntohs(gh->arfcn); 	/* arfcn of the cell we currently camp on */
	uint8_t gsmtap_chantype = gh->sub_type; /* gsmtap channel type */
	uint8_t signal_dbm = gh->signal_dbm;	/* signal strength in dBm */
	//uint8_t snr = gh->snr_db;		/* signal noise ratio in dB */
	uint8_t subslot = gh->sub_slot;		/* multiframe subslot to send msg in (tch -> 0-26, bcch/ccch -> 0-51) */
	uint8_t timeslot = gh->timeslot;	/* tdma timeslot to send in (0-7) */
	uint8_t rsl_chantype;			/* rsl chan type (8.58, 9.3.1) */
	uint8_t link_id;			/* rsl link id tells if this is an ssociated or dedicated link */
	uint8_t chan_nr;			/* encoded rsl channel type, timeslot and mf subslot */
	struct osmo_phsap_prim l1sap;

	memset(&l1sap, 0, sizeof(l1sap));
	/* get rid of l1 gsmtap hdr */
	msg->l2h = msgb_pull(msg, sizeof(*gh));

	/* convert gsmtap chan to RSL chan and link id */
	chantype_gsmtap2rsl(gsmtap_chantype, &rsl_chantype, &link_id);
	chan_nr = rsl_enc_chan_nr(rsl_chantype, subslot, timeslot);

	/* ... or not uplink */
	if (!(arfcn & GSMTAP_ARFCN_F_UPLINK)) {
		LOGPFN(DL1P, LOGL_NOTICE, fn, "Ignoring incoming msg - no uplink flag\n");
		goto nomessage;
	}

	/* Generally ignore all msgs that are either not received with the right ARFCN... */
	pinst = phy_instance_by_arfcn(plink, arfcn & GSMTAP_ARFCN_MASK);
	if (!pinst)
		goto nomessage;

	/* switch case with removed ACCH flag */
	switch ((gsmtap_chantype & ~GSMTAP_CHANNEL_ACCH) & 0xff) {
	case GSMTAP_CHANNEL_RACH:
		/* generate primitive for upper layer
		 * see 04.08 - 3.3.1.3.1: the IMMEDIATE_ASSIGNMENT coming back from the network has to be
		 * sent with the same ra reference as in the CHANNEL_REQUEST that was received */
		osmo_prim_init(&l1sap.oph, SAP_GSM_PH, PRIM_PH_RACH, PRIM_OP_INDICATION, msg);

		l1sap.u.rach_ind.chan_nr = chan_nr;
		/* TODO: 11bit RACH */
		l1sap.u.rach_ind.ra = msgb_pull_u8(msg); /* directly after gh hdr comes ra */
		l1sap.u.rach_ind.acc_delay = 0; /* probably not used in virt um */
		l1sap.u.rach_ind.is_11bit = 0;
		l1sap.u.rach_ind.fn = fn;
		/* we don't rally know which RACH bursrt type the virtual MS is using, as this field is not
		 * part of information present in the GSMTAP header.  So we simply report all of them as 0 */
		l1sap.u.rach_ind.burst_type = GSM_L1_BURST_TYPE_ACCESS_0;
		break;
	case GSMTAP_CHANNEL_TCH_F:
	case GSMTAP_CHANNEL_TCH_H:
#if 0
		/* TODO: handle voice messages */
		if (!facch && ! tch_acch) {
			osmo_prim_init(&l1sap.oph, SAP_GSM_PH, PRIM_TCH, PRIM_OP_INDICATION, msg);
		}
#endif
	case GSMTAP_CHANNEL_SDCCH4:
	case GSMTAP_CHANNEL_SDCCH8:
	case GSMTAP_CHANNEL_PACCH:
	case GSMTAP_CHANNEL_PDCH:
	case GSMTAP_CHANNEL_PTCCH:
		osmo_prim_init(&l1sap.oph, SAP_GSM_PH, PRIM_PH_DATA,
		               PRIM_OP_INDICATION, msg);
		l1sap.u.data.chan_nr = chan_nr;
		l1sap.u.data.link_id = link_id;
		l1sap.u.data.fn = fn;
		l1sap.u.data.rssi = 0; /* Radio Signal Strength Indicator. Best -> 0 */
		l1sap.u.data.ber10k = 0; /* Bit Error Rate in 0.01%. Best -> 0 */
		l1sap.u.data.ta_offs_256bits = 0; /* Burst time of arrival in quarter bits. Probably used for Timing Advance calc. Best -> 0 */
		l1sap.u.data.lqual_cb = 10 * signal_dbm; /* Link quality in centiBel = 10 * dB. */
		l1sap.u.data.pdch_presence_info = PRES_INFO_BOTH;
		l1if_process_meas_res(pinst->trx, timeslot, fn, chan_nr, 0, 0, 0, 0);
		break;
	case GSMTAP_CHANNEL_AGCH:
	case GSMTAP_CHANNEL_PCH:
	case GSMTAP_CHANNEL_BCCH:
		LOGPFN(DL1P, LOGL_NOTICE, fn, "Ignore incoming msg - channel type downlink only!\n");
		goto nomessage;
	case GSMTAP_CHANNEL_SDCCH:
	case GSMTAP_CHANNEL_CCCH:
	case GSMTAP_CHANNEL_CBCH51:
	case GSMTAP_CHANNEL_CBCH52:
		LOGPFN(DL1P, LOGL_NOTICE, fn, "Ignore incoming msg - channel type not supported!\n");
		goto nomessage;
	default:
		LOGPFN(DL1P, LOGL_NOTICE, fn, "Ignore incoming msg - channel type unknown\n");
		goto nomessage;
	}

	/* forward primitive, lsap takes ownership of the msgb. */
	l1sap_up(pinst->trx, &l1sap);
	DEBUGPFN(DL1P, fn, "Message forwarded to layer 2.\n");
	return;

nomessage:
	talloc_free(msg);
}

/* called by common part once OML link is established */
int bts_model_oml_estab(struct gsm_bts *bts)
{
	return 0;
}

/* called by bts_main to initialize physical link */
int bts_model_phy_link_open(struct phy_link *plink)
{
	struct phy_instance *pinst;

	//OSMO_ASSERT(plink->type == PHY_LINK_T_VIRTUAL);

	if (plink->u.virt.virt_um)
		virt_um_destroy(plink->u.virt.virt_um);

	phy_link_state_set(plink, PHY_LINK_CONNECTING);

	plink->u.virt.virt_um = virt_um_init(plink, plink->u.virt.ms_mcast_group, plink->u.virt.ms_mcast_port,
					     plink->u.virt.bts_mcast_group, plink->u.virt.bts_mcast_port,
					     virt_um_rcv_cb);
	if (!plink->u.virt.virt_um) {
		phy_link_state_set(plink, PHY_LINK_SHUTDOWN);
		return -1;
	}
	/* set back reference to plink */
	plink->u.virt.virt_um->priv = plink;

	/* iterate over list of PHY instances and initialize the scheduler */
	llist_for_each_entry(pinst, &plink->instances, list) {
		trx_sched_init(&pinst->u.virt.sched, pinst->trx);
		/* Only start the scheduler for the transceiver on C0.
		 * If we have multiple tranceivers, CCCH is always on C0
		 * and has to be auto active */
		/* Other TRX are activated via OML by a PRIM_INFO_MODIFY
		 * / PRIM_INFO_ACTIVATE */
		if (pinst->trx && pinst->trx == pinst->trx->bts->c0) {
			vbts_sched_start(pinst->trx->bts);
			/* init lapdm layer 3 callback for the trx on timeslot 0 == BCCH */
			lchan_init_lapdm(&pinst->trx->ts[0].lchan[CCCH_LCHAN]);
			/* FIXME: This is probably the wrong location to set the CCCH to active... the OML link def. needs to be reworked and fixed. */
			pinst->trx->ts[0].lchan[CCCH_LCHAN].rel_act_kind = LCHAN_REL_ACT_OML;
			lchan_set_state(&pinst->trx->ts[0].lchan[CCCH_LCHAN], LCHAN_S_ACTIVE);
		}
	}

	/* this will automatically update the MO state of all associated TRX objects */
	phy_link_state_set(plink, PHY_LINK_CONNECTED);

	return 0;
}


/*
 * primitive handling
 */

/* enable ciphering */
static int l1if_set_ciphering(struct gsm_lchan *lchan, uint8_t chan_nr, int downlink)
{
	struct gsm_bts_trx *trx = lchan->ts->trx;
	struct phy_instance *pinst = trx_phy_instance(trx);
	struct l1sched_trx *sched = &pinst->u.virt.sched;

	/* ciphering already enabled in both directions */
	if (lchan->ciph_state == LCHAN_CIPH_RXTX_CONF)
		return -EINVAL;

	if (!downlink) {
		/* set uplink */
		trx_sched_set_cipher(sched, chan_nr, 0, lchan->encr.alg_id - 1,
			lchan->encr.key, lchan->encr.key_len);
		lchan->ciph_state = LCHAN_CIPH_RX_CONF;
	} else {
		/* set downlink and also set uplink, if not already */
		if (lchan->ciph_state != LCHAN_CIPH_RX_CONF) {
			trx_sched_set_cipher(sched, chan_nr, 0,
				lchan->encr.alg_id - 1, lchan->encr.key,
				lchan->encr.key_len);
		}
		trx_sched_set_cipher(sched, chan_nr, 1, lchan->encr.alg_id - 1,
			lchan->encr.key, lchan->encr.key_len);
		lchan->ciph_state = LCHAN_CIPH_RXTX_CONF;
	}

	return 0;
}

static int mph_info_chan_confirm(struct gsm_bts_trx *trx, uint8_t chan_nr,
	enum osmo_mph_info_type type, uint8_t cause)
{
	struct osmo_phsap_prim l1sap;

	memset(&l1sap, 0, sizeof(l1sap));
	osmo_prim_init(&l1sap.oph, SAP_GSM_PH, PRIM_MPH_INFO, PRIM_OP_CONFIRM,
		NULL);
	l1sap.u.info.type = type;
	l1sap.u.info.u.act_cnf.chan_nr = chan_nr;
	l1sap.u.info.u.act_cnf.cause = cause;

	return l1sap_up(trx, &l1sap);
}

int l1if_mph_time_ind(struct gsm_bts *bts, uint32_t fn)
{
	struct osmo_phsap_prim l1sap;

	memset(&l1sap, 0, sizeof(l1sap));
	osmo_prim_init(&l1sap.oph, SAP_GSM_PH, PRIM_MPH_INFO,
		PRIM_OP_INDICATION, NULL);
	l1sap.u.info.type = PRIM_INFO_TIME;
	l1sap.u.info.u.time_ind.fn = fn;

	if (!bts->c0)
		return -EINVAL;

	return l1sap_up(bts->c0, &l1sap);
}


static void l1if_fill_meas_res(struct osmo_phsap_prim *l1sap, uint8_t chan_nr, float ta,
				float ber, float rssi, uint32_t fn)
{
	memset(l1sap, 0, sizeof(*l1sap));
	osmo_prim_init(&l1sap->oph, SAP_GSM_PH, PRIM_MPH_INFO,
		PRIM_OP_INDICATION, NULL);
	l1sap->u.info.type = PRIM_INFO_MEAS;
	l1sap->u.info.u.meas_ind.chan_nr = chan_nr;
	l1sap->u.info.u.meas_ind.ta_offs_256bits = (int16_t)(ta*4);
	l1sap->u.info.u.meas_ind.ber10k = (unsigned int) (ber * 10000);
	l1sap->u.info.u.meas_ind.inv_rssi = (uint8_t) (rssi * -1);
	l1sap->u.info.u.meas_ind.fn = fn;
}

static int l1if_process_meas_res(struct gsm_bts_trx *trx, uint8_t tn, uint32_t fn, uint8_t chan_nr,
				 int n_errors, int n_bits_total, float rssi, float toa)
{
	struct gsm_lchan *lchan = &trx->ts[tn].lchan[l1sap_chan2ss(chan_nr)];
	struct osmo_phsap_prim l1sap;
	/* 100% BER is n_bits_total is 0 */
	float ber = n_bits_total==0 ? 1.0 : (float)n_errors / (float)n_bits_total;

	DEBUGPFN(DMEAS, fn, "RX L1 frame %s chan_nr=0x%02x MS pwr=%ddBm rssi=%.1f dBFS "
		"ber=%.2f%% (%d/%d bits) L1_ta=%d rqd_ta=%d toa=%.2f\n",
		gsm_lchan_name(lchan), chan_nr, ms_pwr_dbm(lchan->ts->trx->bts->band, lchan->ms_power),
		rssi, ber*100, n_errors, n_bits_total, lchan->meas.l1_info[1], lchan->rqd_ta, toa);

	l1if_fill_meas_res(&l1sap, chan_nr, lchan->rqd_ta + toa, ber, rssi, fn);

	return l1sap_up(trx, &l1sap);
}



/* primitive from common part */
int bts_model_l1sap_down(struct gsm_bts_trx *trx, struct osmo_phsap_prim *l1sap)
{
	struct phy_instance *pinst = trx_phy_instance(trx);
	struct l1sched_trx *sched = &pinst->u.virt.sched;
	struct msgb *msg = l1sap->oph.msg;
	uint8_t chan_nr;
	uint8_t tn, ss;
	int rc = 0;
	struct gsm_lchan *lchan;

	switch (OSMO_PRIM_HDR(&l1sap->oph)) {
	case OSMO_PRIM(PRIM_PH_DATA, PRIM_OP_REQUEST):
		if (!msg)
			break;
		/* put data into scheduler's queue */
		return trx_sched_ph_data_req(sched, l1sap);
	case OSMO_PRIM(PRIM_TCH, PRIM_OP_REQUEST):
		if (!msg)
			break;
		/* put data into scheduler's queue */
		return trx_sched_tch_req(sched, l1sap);
	case OSMO_PRIM(PRIM_MPH_INFO, PRIM_OP_REQUEST):
		switch (l1sap->u.info.type) {
		case PRIM_INFO_ACT_CIPH:
			chan_nr = l1sap->u.info.u.ciph_req.chan_nr;
			tn = L1SAP_CHAN2TS(chan_nr);
			ss = l1sap_chan2ss(chan_nr);
			lchan = &trx->ts[tn].lchan[ss];
			if (l1sap->u.info.u.ciph_req.uplink)
				l1if_set_ciphering(lchan, chan_nr, 0);
			if (l1sap->u.info.u.ciph_req.downlink)
				l1if_set_ciphering(lchan, chan_nr, 1);
			break;
		case PRIM_INFO_ACTIVATE:
		case PRIM_INFO_DEACTIVATE:
		case PRIM_INFO_MODIFY:
			chan_nr = l1sap->u.info.u.act_req.chan_nr;
			tn = L1SAP_CHAN2TS(chan_nr);
			ss = l1sap_chan2ss(chan_nr);
			lchan = &trx->ts[tn].lchan[ss];
			/* we receive a channel activation request from the BSC,
			 * e.g. as a response to a channel req on RACH */
			if (l1sap->u.info.type == PRIM_INFO_ACTIVATE) {
				if ((chan_nr & 0xE0) == 0x80) {
					LOGP(DL1C, LOGL_ERROR, "Cannot activate"
						" chan_nr 0x%02x\n", chan_nr);
					break;
				}
				/* activate dedicated channel */
				trx_sched_set_lchan(sched, chan_nr, LID_DEDIC, 1);
				/* activate associated channel */
				trx_sched_set_lchan(sched, chan_nr, LID_SACCH, 1);
				/* set mode */
				trx_sched_set_mode(sched, chan_nr,
					lchan->rsl_cmode, lchan->tch_mode,
					lchan->tch.amr_mr.num_modes,
					lchan->tch.amr_mr.bts_mode[0].mode,
					lchan->tch.amr_mr.bts_mode[1].mode,
					lchan->tch.amr_mr.bts_mode[2].mode,
					lchan->tch.amr_mr.bts_mode[3].mode,
					amr_get_initial_mode(lchan),
					(lchan->ho.active == 1));
				/* init lapdm */
				lchan_init_lapdm(lchan);
				/* set lchan active */
				lchan_set_state(lchan, LCHAN_S_ACTIVE);
				/* set initial ciphering */
				l1if_set_ciphering(lchan, chan_nr, 0);
				l1if_set_ciphering(lchan, chan_nr, 1);
				if (lchan->encr.alg_id)
					lchan->ciph_state = LCHAN_CIPH_RXTX_CONF;
				else
					lchan->ciph_state = LCHAN_CIPH_NONE;

				/* confirm */
				mph_info_chan_confirm(trx, chan_nr,
					PRIM_INFO_ACTIVATE, 0);
				break;
			}
			if (l1sap->u.info.type == PRIM_INFO_MODIFY) {
				/* change mode */
				trx_sched_set_mode(sched, chan_nr,
					lchan->rsl_cmode, lchan->tch_mode,
					lchan->tch.amr_mr.num_modes,
					lchan->tch.amr_mr.bts_mode[0].mode,
					lchan->tch.amr_mr.bts_mode[1].mode,
					lchan->tch.amr_mr.bts_mode[2].mode,
					lchan->tch.amr_mr.bts_mode[3].mode,
					amr_get_initial_mode(lchan),
					0);
				break;
			}
			if ((chan_nr & 0xE0) == 0x80) {
				LOGP(DL1C, LOGL_ERROR, "Cannot deactivate "
					"chan_nr 0x%02x\n", chan_nr);
				break;
			}
			/* deactivate associated channel */
			trx_sched_set_lchan(sched, chan_nr, 0x40, 0);
			if (!l1sap->u.info.u.act_req.sacch_only) {
				/* set lchan inactive */
				lchan_set_state(lchan, LCHAN_S_NONE);
				/* deactivate dedicated channel */
				trx_sched_set_lchan(sched, chan_nr, 0x00, 0);
				/* confirm only on dedicated channel */
				mph_info_chan_confirm(trx, chan_nr,
					PRIM_INFO_DEACTIVATE, 0);
				lchan->ciph_state = 0; /* FIXME: do this in common/\*.c */
			}
			break;
		default:
			LOGP(DL1C, LOGL_NOTICE, "unknown MPH-INFO.req %d\n",
				l1sap->u.info.type);
			rc = -EINVAL;
			goto done;
		}
		break;
	default:
		LOGP(DL1C, LOGL_NOTICE, "unknown prim %d op %d\n",
			l1sap->oph.primitive, l1sap->oph.operation);
		rc = -EINVAL;
		goto done;
	}

done:
	if (msg)
		msgb_free(msg);
	return rc;
}
