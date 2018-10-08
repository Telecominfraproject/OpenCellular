/*
 * layer 1 primitive handling and interface
 *
 * Copyright (C) 2013  Andreas Eversberg <jolly@eversberg.eu>
 * Copyright (C) 2015  Alexander Chemeris <Alexander.Chemeris@fairwaves.co>
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
#include <stdbool.h>
#include <errno.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/bits.h>
#include <osmocom/gsm/abis_nm.h>

#include <osmo-bts/logging.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/oml.h>
#include <osmo-bts/rsl.h>
#include <osmo-bts/l1sap.h>
#include <osmo-bts/bts_model.h>
#include <osmo-bts/amr.h>
#include <osmo-bts/abis.h>
#include <osmo-bts/scheduler.h>

#include "l1_if.h"
#include "trx_if.h"


static const uint8_t transceiver_chan_types[_GSM_PCHAN_MAX] = {
	[GSM_PCHAN_NONE]                = 8,
	[GSM_PCHAN_CCCH]                = 4,
	[GSM_PCHAN_CCCH_SDCCH4]         = 5,
	[GSM_PCHAN_CCCH_SDCCH4_CBCH]    = 5,
	[GSM_PCHAN_TCH_F]               = 1,
	[GSM_PCHAN_TCH_H]               = 3,
	[GSM_PCHAN_SDCCH8_SACCH8C]      = 7,
	[GSM_PCHAN_SDCCH8_SACCH8C_CBCH] = 7,
	[GSM_PCHAN_PDCH]                = 13,
	/* [GSM_PCHAN_TCH_F_PDCH] not needed here, see trx_set_ts_as_pchan() */
	[GSM_PCHAN_UNKNOWN]             = 0,
};


static void check_transceiver_availability_trx(struct trx_l1h *l1h, int avail)
{
	struct phy_instance *pinst = l1h->phy_inst;
	struct gsm_bts_trx *trx = pinst->trx;
	uint8_t tn;

	/* HACK, we should change state when we receive first clock from
	 * transceiver */
	if (avail) {
		/* signal availability */
		oml_mo_state_chg(&trx->mo, NM_OPSTATE_DISABLED, NM_AVSTATE_OK);
		oml_mo_state_chg(&trx->bb_transc.mo, -1, NM_AVSTATE_OK);
		if (!pinst->u.osmotrx.sw_act_reported) {
			oml_mo_tx_sw_act_rep(&trx->mo);
			oml_mo_tx_sw_act_rep(&trx->bb_transc.mo);
			pinst->u.osmotrx.sw_act_reported = true;
		}

		for (tn = 0; tn < TRX_NR_TS; tn++)
			oml_mo_state_chg(&trx->ts[tn].mo, NM_OPSTATE_DISABLED,
				(l1h->config.slotmask & (1 << tn)) ?
					NM_AVSTATE_DEPENDENCY :
					NM_AVSTATE_NOT_INSTALLED);
	} else {
		oml_mo_state_chg(&trx->mo, NM_OPSTATE_DISABLED,
			NM_AVSTATE_OFF_LINE);
		oml_mo_state_chg(&trx->bb_transc.mo, NM_OPSTATE_DISABLED,
			NM_AVSTATE_OFF_LINE);

		for (tn = 0; tn < TRX_NR_TS; tn++)
			oml_mo_state_chg(&trx->ts[tn].mo, NM_OPSTATE_DISABLED,
				NM_AVSTATE_OFF_LINE);
	}
}

int check_transceiver_availability(struct gsm_bts *bts, int avail)
{
	struct gsm_bts_trx *trx;

	llist_for_each_entry(trx, &bts->trx_list, list) {
		struct phy_instance *pinst = trx_phy_instance(trx);
		struct trx_l1h *l1h = pinst->u.osmotrx.hdl;
		check_transceiver_availability_trx(l1h, avail);
	}
	return 0;
}

int bts_model_lchan_deactivate(struct gsm_lchan *lchan)
{
	struct phy_instance *pinst = trx_phy_instance(lchan->ts->trx);
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;

	if (lchan->rel_act_kind == LCHAN_REL_ACT_REACT) {
		lchan->rel_act_kind = LCHAN_REL_ACT_RSL;
		/* FIXME: perform whatever is needed (if any) to set proper PCH/AGCH allocation according to
		   3GPP TS 44.018 Table 10.5.2.11.1 using num_agch(lchan->ts->trx, "TRX L1"); function */
		return 0;
	}
	/* set lchan inactive */
	lchan_set_state(lchan, LCHAN_S_NONE);

	return trx_sched_set_lchan(&l1h->l1s, gsm_lchan2chan_nr(lchan),
				   LID_DEDIC, 0);
}

int bts_model_lchan_deactivate_sacch(struct gsm_lchan *lchan)
{
	struct phy_instance *pinst = trx_phy_instance(lchan->ts->trx);
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;
	return trx_sched_set_lchan(&l1h->l1s, gsm_lchan2chan_nr(lchan),
				   LID_SACCH, 0);
}

/*
 * transceiver provisioning
 */
int l1if_provision_transceiver_trx(struct trx_l1h *l1h)
{
	uint8_t tn;

	if (!transceiver_available)
		return -EIO;

	if (l1h->config.poweron
	 && l1h->config.tsc_valid
	 && l1h->config.bsic_valid
	 && l1h->config.arfcn_valid) {
	 	/* before power on */
		if (!l1h->config.arfcn_sent) {
			trx_if_cmd_rxtune(l1h, l1h->config.arfcn);
			trx_if_cmd_txtune(l1h, l1h->config.arfcn);
			l1h->config.arfcn_sent = 1;
		}
		if (!l1h->config.tsc_sent) {
			trx_if_cmd_settsc(l1h, l1h->config.tsc);
			l1h->config.tsc_sent = 1;
		}
		if (!l1h->config.bsic_sent) {
			trx_if_cmd_setbsic(l1h, l1h->config.bsic);
			l1h->config.bsic_sent = 1;
		}

		if (!l1h->config.poweron_sent) {
			trx_if_cmd_poweron(l1h);
			l1h->config.poweron_sent = 1;
		}

		/* after power on */
		if (l1h->config.rxgain_valid && !l1h->config.rxgain_sent) {
			trx_if_cmd_setrxgain(l1h, l1h->config.rxgain);
			l1h->config.rxgain_sent = 1;
		}
		if (l1h->config.power_valid && !l1h->config.power_sent) {
			trx_if_cmd_setpower(l1h, l1h->config.power);
			l1h->config.power_sent = 1;
		}
		if (l1h->config.maxdly_valid && !l1h->config.maxdly_sent) {
			trx_if_cmd_setmaxdly(l1h, l1h->config.maxdly);
			l1h->config.maxdly_sent = 1;
		}
		if (l1h->config.maxdlynb_valid && !l1h->config.maxdlynb_sent) {
			trx_if_cmd_setmaxdlynb(l1h, l1h->config.maxdlynb);
			l1h->config.maxdlynb_sent = 1;
		}

		for (tn = 0; tn < TRX_NR_TS; tn++) {
			if (l1h->config.slottype_valid[tn]
			 && !l1h->config.slottype_sent[tn]) {
				trx_if_cmd_setslot(l1h, tn,
					l1h->config.slottype[tn]);
				l1h->config.slottype_sent[tn] = 1;
			}
		}
		return 0;
	}

	if (!l1h->config.poweron && !l1h->config.poweron_sent) {
		trx_if_cmd_poweroff(l1h);
		l1h->config.poweron_sent = 1;
		l1h->config.rxgain_sent = 0;
		l1h->config.power_sent = 0;
		l1h->config.maxdly_sent = 0;
		l1h->config.maxdlynb_sent = 0;
		for (tn = 0; tn < TRX_NR_TS; tn++)
			l1h->config.slottype_sent[tn] = 0;
	}

	return 0;
}

int l1if_provision_transceiver(struct gsm_bts *bts)
{
	struct gsm_bts_trx *trx;
	uint8_t tn;

	llist_for_each_entry(trx, &bts->trx_list, list) {
		struct phy_instance *pinst = trx_phy_instance(trx);
		struct trx_l1h *l1h = pinst->u.osmotrx.hdl;
		l1h->config.arfcn_sent = 0;
		l1h->config.tsc_sent = 0;
		l1h->config.bsic_sent = 0;
		l1h->config.poweron_sent = 0;
		l1h->config.rxgain_sent = 0;
		l1h->config.power_sent = 0;
		l1h->config.maxdly_sent = 0;
		l1h->config.maxdlynb_sent = 0;
		for (tn = 0; tn < TRX_NR_TS; tn++)
			l1h->config.slottype_sent[tn] = 0;
		l1if_provision_transceiver_trx(l1h);
	}
	return 0;
}

/*
 * activation/configuration/deactivation of transceiver's TRX
 */

/* initialize the layer1 */
static int trx_init(struct gsm_bts_trx *trx)
{
	struct phy_instance *pinst = trx_phy_instance(trx);
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;

	/* power on transceiver, if not already */
	if (!l1h->config.poweron) {
		l1h->config.poweron = 1;
		l1h->config.poweron_sent = 0;
		l1if_provision_transceiver_trx(l1h);
	}

	if (trx == trx->bts->c0)
		lchan_init_lapdm(&trx->ts[0].lchan[CCCH_LCHAN]);

	/* Set to Operational State: Enabled */
	oml_mo_state_chg(&trx->mo, NM_OPSTATE_ENABLED, NM_AVSTATE_OK);

	/* Send OPSTART ack */
	return oml_mo_opstart_ack(&trx->mo);
}

/* deactivate transceiver */
int bts_model_trx_close(struct gsm_bts_trx *trx)
{
	struct phy_instance *pinst = trx_phy_instance(trx);
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;
	enum gsm_phys_chan_config pchan = trx->ts[0].pchan;

	/* close all logical channels and reset timeslots */
	trx_sched_reset(&l1h->l1s);

	/* deactivate lchan for CCCH */
	if (pchan == GSM_PCHAN_CCCH || pchan == GSM_PCHAN_CCCH_SDCCH4 ||
	    pchan == GSM_PCHAN_CCCH_SDCCH4_CBCH) {
		lchan_set_state(&trx->ts[0].lchan[CCCH_LCHAN], LCHAN_S_INACTIVE);
	}

	/* power off transceiver, if not already */
	if (l1h->config.poweron) {
		l1h->config.poweron = 0;
		l1h->config.poweron_sent = 0;
		l1if_provision_transceiver_trx(l1h);
	}

	/* Set to Operational State: Disabled */
	check_transceiver_availability_trx(l1h, 0);

	return 0;
}

/* on RSL failure, deactivate transceiver */
void bts_model_abis_close(struct gsm_bts *bts)
{
	bts_shutdown(bts, "Abis close");
}

int bts_model_adjst_ms_pwr(struct gsm_lchan *lchan)
{
	/* we always implement the power control loop in osmo-bts software, as
	 * there is no automatism in the underlying osmo-trx */
	return 0;
}

/* set bts attributes */
static uint8_t trx_set_bts(struct gsm_bts *bts, struct tlv_parsed *new_attr)
{
	struct gsm_bts_trx *trx;
	uint8_t bsic = bts->bsic;

	llist_for_each_entry(trx, &bts->trx_list, list) {
		struct phy_instance *pinst = trx_phy_instance(trx);
		struct trx_l1h *l1h = pinst->u.osmotrx.hdl;
		if (l1h->config.bsic != bsic || !l1h->config.bsic_valid) {
			l1h->config.bsic = bsic;
			l1h->config.bsic_valid = 1;
			l1h->config.bsic_sent = 0;
			l1if_provision_transceiver_trx(l1h);
		}
	}
	check_transceiver_availability(bts, transceiver_available);


	return 0;
}

/* set trx attributes */
static uint8_t trx_set_trx(struct gsm_bts_trx *trx)
{
	struct phy_instance *pinst = trx_phy_instance(trx);
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;
	uint16_t arfcn = trx->arfcn;

	if (l1h->config.arfcn != arfcn || !l1h->config.arfcn_valid) {
		l1h->config.arfcn = arfcn;
		l1h->config.arfcn_valid = 1;
		l1h->config.arfcn_sent = 0;
		l1if_provision_transceiver_trx(l1h);
	}

	if (l1h->config.power_oml) {
		l1h->config.power = trx->max_power_red;
		l1h->config.power_valid = 1;
		l1h->config.power_sent = 0;
		l1if_provision_transceiver_trx(l1h);
	}

	return 0;
}

/* set ts attributes */
static uint8_t trx_set_ts_as_pchan(struct gsm_bts_trx_ts *ts,
				   enum gsm_phys_chan_config pchan)
{
	struct phy_instance *pinst = trx_phy_instance(ts->trx);
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;
	uint8_t tn = ts->nr;
	uint16_t tsc = ts->tsc;
	uint8_t slottype;
	int rc;

	/* all TSC of all timeslots must be equal, because transceiver only
	 * supports one TSC per TRX */

	if (l1h->config.tsc != tsc || !l1h->config.tsc_valid) {
		l1h->config.tsc = tsc;
		l1h->config.tsc_valid = 1;
		l1h->config.tsc_sent = 0;
		l1if_provision_transceiver_trx(l1h);
	}

	/* ignore disabled slots */
	if (!(l1h->config.slotmask & (1 << tn)))
		return NM_NACK_RES_NOTAVAIL;

	/* set physical channel. For dynamic timeslots, the caller should have
	 * decided on a more specific PCHAN type already. */
	OSMO_ASSERT(pchan != GSM_PCHAN_TCH_F_PDCH);
	OSMO_ASSERT(pchan != GSM_PCHAN_TCH_F_TCH_H_PDCH);
	rc = trx_sched_set_pchan(&l1h->l1s, tn, pchan);
	if (rc)
		return NM_NACK_RES_NOTAVAIL;

	/* activate lchan for CCCH */
	if (pchan == GSM_PCHAN_CCCH || pchan == GSM_PCHAN_CCCH_SDCCH4 ||
	    pchan == GSM_PCHAN_CCCH_SDCCH4_CBCH) {
		ts->lchan[CCCH_LCHAN].rel_act_kind = LCHAN_REL_ACT_OML;
		lchan_set_state(&ts->lchan[CCCH_LCHAN], LCHAN_S_ACTIVE);
	}

	slottype = transceiver_chan_types[pchan];
	
	if (l1h->config.slottype[tn] != slottype
	 || !l1h->config.slottype_valid[tn]) {
		l1h->config.slottype[tn] = slottype;
		l1h->config.slottype_valid[tn] = 1;
		l1h->config.slottype_sent[tn] = 0;
		l1if_provision_transceiver_trx(l1h);
	}

	return 0;
}

static uint8_t trx_set_ts(struct gsm_bts_trx_ts *ts)
{
	enum gsm_phys_chan_config pchan;

	/* For dynamic timeslots, pick the pchan type that should currently be
	 * active. This should only be called during init, PDCH transitions
	 * will call trx_set_ts_as_pchan() directly. */
	switch (ts->pchan) {
	case GSM_PCHAN_TCH_F_PDCH:
		OSMO_ASSERT((ts->flags & TS_F_PDCH_PENDING_MASK) == 0);
		pchan = (ts->flags & TS_F_PDCH_ACTIVE)? GSM_PCHAN_PDCH
			                              : GSM_PCHAN_TCH_F;
		break;
	case GSM_PCHAN_TCH_F_TCH_H_PDCH:
		OSMO_ASSERT(ts->dyn.pchan_is == ts->dyn.pchan_want);
		pchan = ts->dyn.pchan_is;
		break;
	default:
		pchan = ts->pchan;
		break;
	}

	return trx_set_ts_as_pchan(ts, pchan);
}


/*
 * primitive handling
 */

/* enable ciphering */
static int l1if_set_ciphering(struct trx_l1h *l1h, struct gsm_lchan *lchan,
	uint8_t chan_nr, int downlink)
{
	/* ciphering already enabled in both directions */
	if (lchan->ciph_state == LCHAN_CIPH_RXTX_CONF)
		return -EINVAL;

	if (!downlink) {
		/* set uplink */
		trx_sched_set_cipher(&l1h->l1s, chan_nr, 0, lchan->encr.alg_id - 1,
			lchan->encr.key, lchan->encr.key_len);
		lchan->ciph_state = LCHAN_CIPH_RX_CONF;
	} else {
		/* set downlink and also set uplink, if not already */
		if (lchan->ciph_state != LCHAN_CIPH_RX_CONF) {
			trx_sched_set_cipher(&l1h->l1s, chan_nr, 0,
				lchan->encr.alg_id - 1, lchan->encr.key,
				lchan->encr.key_len);
		}
		trx_sched_set_cipher(&l1h->l1s, chan_nr, 1, lchan->encr.alg_id - 1,
			lchan->encr.key, lchan->encr.key_len);
		lchan->ciph_state = LCHAN_CIPH_RXTX_CONF;
	}

	return 0;
}

static int mph_info_chan_confirm(struct trx_l1h *l1h, uint8_t chan_nr,
	enum osmo_mph_info_type type, uint8_t cause)
{
	struct phy_instance *pinst = l1h->phy_inst;
	struct osmo_phsap_prim l1sap;

	memset(&l1sap, 0, sizeof(l1sap));
	osmo_prim_init(&l1sap.oph, SAP_GSM_PH, PRIM_MPH_INFO, PRIM_OP_CONFIRM,
		NULL);
	l1sap.u.info.type = type;
	l1sap.u.info.u.act_cnf.chan_nr = chan_nr;
	l1sap.u.info.u.act_cnf.cause = cause;

	return l1sap_up(pinst->trx, &l1sap);
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


static void l1if_fill_meas_res(struct osmo_phsap_prim *l1sap, uint8_t chan_nr, int16_t toa256,
	float ber, float rssi, uint32_t fn)
{
	memset(l1sap, 0, sizeof(*l1sap));
	osmo_prim_init(&l1sap->oph, SAP_GSM_PH, PRIM_MPH_INFO,
		PRIM_OP_INDICATION, NULL);
	l1sap->u.info.type = PRIM_INFO_MEAS;
	l1sap->u.info.u.meas_ind.chan_nr = chan_nr;
	l1sap->u.info.u.meas_ind.ta_offs_256bits = toa256;
	l1sap->u.info.u.meas_ind.ber10k = (unsigned int) (ber * 10000);
	l1sap->u.info.u.meas_ind.inv_rssi = (uint8_t) (rssi * -1);
	l1sap->u.info.u.meas_ind.fn = fn;
}

int l1if_process_meas_res(struct gsm_bts_trx *trx, uint8_t tn, uint32_t fn, uint8_t chan_nr,
	int n_errors, int n_bits_total, float rssi, int16_t toa256)
{
	struct gsm_lchan *lchan = &trx->ts[tn].lchan[l1sap_chan2ss(chan_nr)];
	struct osmo_phsap_prim l1sap;
	/* 100% BER is n_bits_total is 0 */
	float ber = n_bits_total==0 ? 1.0 : (float)n_errors / (float)n_bits_total;

	LOGPFN(DMEAS, LOGL_DEBUG, fn, "RX UL measurement for %s fn=%u chan_nr=0x%02x MS pwr=%ddBm rssi=%.1f dBFS "
		"ber=%.2f%% (%d/%d bits) L1_ta=%d rqd_ta=%d toa256=%d\n",
		gsm_lchan_name(lchan), fn, chan_nr, ms_pwr_dbm(lchan->ts->trx->bts->band, lchan->ms_power_ctrl.current),
		rssi, ber*100, n_errors, n_bits_total, lchan->meas.l1_info[1], lchan->rqd_ta, toa256);

	l1if_fill_meas_res(&l1sap, chan_nr, toa256, ber, rssi, fn);

	return l1sap_up(trx, &l1sap);
}


/* primitive from common part */
int bts_model_l1sap_down(struct gsm_bts_trx *trx, struct osmo_phsap_prim *l1sap)
{
	struct phy_instance *pinst = trx_phy_instance(trx);
	struct trx_l1h *l1h = pinst->u.osmotrx.hdl;
	struct msgb *msg = l1sap->oph.msg;
	uint8_t chan_nr;
	int rc = 0;
	struct gsm_lchan *lchan;

	switch (OSMO_PRIM_HDR(&l1sap->oph)) {
	case OSMO_PRIM(PRIM_PH_DATA, PRIM_OP_REQUEST):
		if (!msg)
			break;
		/* put data into scheduler's queue */
		return trx_sched_ph_data_req(&l1h->l1s, l1sap);
	case OSMO_PRIM(PRIM_TCH, PRIM_OP_REQUEST):
		if (!msg)
			break;
		/* put data into scheduler's queue */
		return trx_sched_tch_req(&l1h->l1s, l1sap);
	case OSMO_PRIM(PRIM_MPH_INFO, PRIM_OP_REQUEST):
		switch (l1sap->u.info.type) {
		case PRIM_INFO_ACT_CIPH:
			chan_nr = l1sap->u.info.u.ciph_req.chan_nr;
			lchan = get_lchan_by_chan_nr(trx, chan_nr);
			if (l1sap->u.info.u.ciph_req.uplink)
				l1if_set_ciphering(l1h, lchan, chan_nr, 0);
			if (l1sap->u.info.u.ciph_req.downlink)
				l1if_set_ciphering(l1h, lchan, chan_nr, 1);
			break;
		case PRIM_INFO_ACTIVATE:
		case PRIM_INFO_DEACTIVATE:
		case PRIM_INFO_MODIFY:
			chan_nr = l1sap->u.info.u.act_req.chan_nr;
			lchan = get_lchan_by_chan_nr(trx, chan_nr);
			if (l1sap->u.info.type == PRIM_INFO_ACTIVATE) {
				if ((chan_nr & 0xE0) == 0x80) {
					LOGP(DL1C, LOGL_ERROR, "Cannot activate"
						" chan_nr 0x%02x\n", chan_nr);
					break;
				}

				/* trx_chan_desc[] in scheduler.c uses the RSL_CHAN_OSMO_PDCH cbits
				 * (0xc0) to indicate the need for PDTCH and PTCCH SAPI activation.
				 * However, 0xc0 is a cbits pattern exclusively used for Osmocom style
				 * dyn TS (a non-standard RSL Chan Activ mod); hence, for IPA style dyn
				 * TS, the chan_nr will never reflect 0xc0 and we would omit the
				 * PDTCH,PTTCH SAPIs. To properly de-/activate the PDTCH SAPIs in
				 * scheduler.c, make sure the 0xc0 cbits are set for de-/activating PDTCH
				 * lchans, i.e. both Osmocom and IPA style dyn TS. (For Osmocom style dyn
				 * TS, the chan_nr typically already reflects 0xc0, while it doesn't for
				 * IPA style.) */
				if (lchan->type == GSM_LCHAN_PDTCH)
					chan_nr = RSL_CHAN_OSMO_PDCH | (chan_nr & ~RSL_CHAN_NR_MASK);

				/* activate dedicated channel */
				trx_sched_set_lchan(&l1h->l1s, chan_nr, LID_DEDIC, 1);
				/* activate associated channel */
				trx_sched_set_lchan(&l1h->l1s, chan_nr, LID_SACCH, 1);
				/* set mode */
				trx_sched_set_mode(&l1h->l1s, chan_nr,
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
				l1if_set_ciphering(l1h, lchan, chan_nr, 0);
				l1if_set_ciphering(l1h, lchan, chan_nr, 1);
				if (lchan->encr.alg_id)
					lchan->ciph_state = LCHAN_CIPH_RXTX_CONF;
				else
					lchan->ciph_state = LCHAN_CIPH_NONE;

				/* confirm */
				mph_info_chan_confirm(l1h, chan_nr,
					PRIM_INFO_ACTIVATE, 0);
				break;
			}
			if (l1sap->u.info.type == PRIM_INFO_MODIFY) {
				/* change mode */
				trx_sched_set_mode(&l1h->l1s, chan_nr,
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
			/* here, type == PRIM_INFO_DEACTIVATE */
			if ((chan_nr & 0xE0) == 0x80) {
				LOGP(DL1C, LOGL_ERROR, "Cannot deactivate "
					"chan_nr 0x%02x\n", chan_nr);
				break;
			}
			/* deactivate associated channel */
			bts_model_lchan_deactivate_sacch(lchan);
			if (!l1sap->u.info.u.act_req.sacch_only) {
				/* deactivate dedicated channel */
				lchan_deactivate(lchan);
				/* confirm only on dedicated channel */
				mph_info_chan_confirm(l1h, chan_nr,
					PRIM_INFO_DEACTIVATE, 0);
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


/*
 * oml handling
 */

/* callback from OML */
int bts_model_check_oml(struct gsm_bts *bts, uint8_t msg_type,
			struct tlv_parsed *old_attr, struct tlv_parsed *new_attr,
			void *obj)
{
	/* FIXME: check if the attributes are valid */
	return 0;
}

/* callback from OML */
int bts_model_apply_oml(struct gsm_bts *bts, struct msgb *msg,
			struct tlv_parsed *new_attr, int kind, void *obj)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	int cause = 0;

	switch (foh->msg_type) {
	case NM_MT_SET_BTS_ATTR:
		cause = trx_set_bts(obj, new_attr);
		break;
	case NM_MT_SET_RADIO_ATTR:
		cause = trx_set_trx(obj);
		break;
	case NM_MT_SET_CHAN_ATTR:
		cause = trx_set_ts(obj);
		break;
	}

	return oml_fom_ack_nack(msg, cause);
}

/* callback from OML */
int bts_model_opstart(struct gsm_bts *bts, struct gsm_abis_mo *mo,
		      void *obj)
{
	int rc;
	LOGP(DOML, LOGL_DEBUG, "bts_model_opstart: %s received\n",
	     get_value_string(abis_nm_obj_class_names, mo->obj_class));
	switch (mo->obj_class) {
	case NM_OC_RADIO_CARRIER:
		/* activate transceiver */
		rc = trx_init(obj);
		break;
	case NM_OC_CHANNEL:
	case NM_OC_BTS:
	case NM_OC_SITE_MANAGER:
	case NM_OC_BASEB_TRANSC:
	case NM_OC_GPRS_NSE:
	case NM_OC_GPRS_CELL:
	case NM_OC_GPRS_NSVC:
		oml_mo_state_chg(mo, NM_OPSTATE_ENABLED, NM_AVSTATE_OK);
		rc = oml_mo_opstart_ack(mo);
		break;
	default:
		rc = oml_mo_opstart_nack(mo, NM_NACK_OBJCLASS_NOTSUPP);
	}
	return rc;
}

int bts_model_chg_adm_state(struct gsm_bts *bts, struct gsm_abis_mo *mo,
			    void *obj, uint8_t adm_state)
{
	/* blindly accept all state changes */
	mo->nm_state.administrative = adm_state;
	return oml_mo_statechg_ack(mo);
}

int bts_model_trx_deact_rf(struct gsm_bts_trx *trx)
{
	return 0;
}

int bts_model_oml_estab(struct gsm_bts *bts)
{
	return 0;
}

int bts_model_change_power(struct gsm_bts_trx *trx, int p_trxout_mdBm)
{
#warning "implement bts_model_change_power\n"
	LOGP(DL1C, LOGL_NOTICE, "Setting TRX output power not supported!\n");
	return 0;
}

int bts_model_ts_disconnect(struct gsm_bts_trx_ts *ts)
{
	/* no action required, signal completion right away. */
	cb_ts_disconnected(ts);
	return 0;
}

int bts_model_ts_connect(struct gsm_bts_trx_ts *ts,
			 enum gsm_phys_chan_config as_pchan)
{
	int rc;
	LOGP(DL1C, LOGL_DEBUG, "%s bts_model_ts_connect(as_pchan=%s)\n",
	     gsm_ts_name(ts), gsm_pchan_name(as_pchan));

	rc = trx_set_ts_as_pchan(ts, as_pchan);
	if (rc)
		return rc;

	LOGP(DL1C, LOGL_NOTICE, "%s bts_model_ts_connect(as_pchan=%s) success,"
	     " calling cb_ts_connected()\n",
	     gsm_ts_name(ts), gsm_pchan_name(as_pchan));

	cb_ts_connected(ts);
	return 0;
}
