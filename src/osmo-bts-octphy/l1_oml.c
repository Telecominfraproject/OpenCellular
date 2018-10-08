/* Layer 1 (PHY) interface of osmo-bts OCTPHY integration */

/* Copyright (c) 2014 Octasic Inc. All rights reserved.
 * Copyright (c) 2015-2016 Harald Welte <laforge@gnumonks.org>
 *
 * based on a copy of osmo-bts-sysmo/l1_oml.c, which is
 * Copyright (C) 2011 by Harald Welte <laforge@gnumonks.org>
 * Copyright (C) 2013-2014 by Holger Hans Peter Freyther
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
#include <stdint.h>
#include <errno.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/utils.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/oml.h>
#include <osmo-bts/rsl.h>

#include <osmo-bts/amr.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/bts_model.h>
#include <osmo-bts/l1sap.h>

#include "l1_if.h"
#include "l1_oml.h"
#include "l1_utils.h"
#include "octphy_hw_api.h"
#include "btsconfig.h"

#include <octphy/octvc1/octvc1_rc2string.h>
#include <octphy/octvc1/gsm/octvc1_gsm_api_swap.h>
#include <octphy/octvc1/gsm/octvc1_gsm_default.h>
#include <octphy/octvc1/gsm/octvc1_gsm_id.h>
#include <octphy/octvc1/main/octvc1_main_default.h>
#include <octphy/octvc1/main/octvc1_main_version.h>

bool no_fw_check = 0;

#define LOGPTRX(byTrxId, level, fmt, args...) \
	LOGP(DL1C, level, "(byTrxId %u) " fmt, byTrxId, ## args)

/* Map OSMOCOM logical channel type to OctPHY Logical channel type */
static tOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM pchan_to_logChComb[_GSM_PCHAN_MAX] =
{
	[GSM_PCHAN_NONE]		= cOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM_EMPTY,
	[GSM_PCHAN_CCCH]		= cOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM_FCCH_SCH_BCCH_CCCH,
	[GSM_PCHAN_CCCH_SDCCH4] 	= cOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM_FCCH_SCH_BCCH_CCCH_SDCCH4_SACCHC4,
	[GSM_PCHAN_TCH_F]		= cOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM_TCHF_FACCHF_SACCHTF,
	[GSM_PCHAN_TCH_H]		= cOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM_TCHH_FACCHH_SACCHTH,
	[GSM_PCHAN_SDCCH8_SACCH8C]	= cOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM_SDCCH8_SACCHC8,
	// TODO - watch out below two!!!
	[GSM_PCHAN_PDCH]		= cOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM_PDTCHF_PACCHF_PTCCHF,
	[GSM_PCHAN_TCH_F_PDCH]		= cOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM_PDTCHF_PACCHF_PTCCHF,
#ifdef cOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM_FCCH_SCH_BCCH_CCCH_SDCCH4_CBCH_SACCHC4
	[GSM_PCHAN_CCCH_SDCCH4_CBCH]	= cOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM_FCCH_SCH_BCCH_CCCH_SDCCH4_CBCH_SACCHC4,
#endif
#ifdef cOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM_SDCCH8_CBCH_SACCHC8
	[GSM_PCHAN_SDCCH8_SACCH8C_CBCH] = cOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM_SDCCH8_CBCH_SACCHC8,
#endif
	[GSM_PCHAN_UNKNOWN]		= cOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM_EMPTY
};

enum sapi_cmd_type {
	SAPI_CMD_ACTIVATE,
	SAPI_CMD_CONFIG_CIPHERING,
	SAPI_CMD_CONFIG_LOGCH_PARAM,
	SAPI_CMD_SACCH_REL_MARKER,
	SAPI_CMD_REL_MARKER,
	SAPI_CMD_DEACTIVATE,
};

struct sapi_cmd {
	struct llist_head entry;
	tOCTVC1_GSM_SAPI_ENUM sapi;
	tOCTVC1_GSM_DIRECTION_ENUM dir;
	enum sapi_cmd_type type;
	int (*callback) (struct gsm_lchan * lchan, int status);
};

struct sapi_dir {
	tOCTVC1_GSM_SAPI_ENUM sapi;
	tOCTVC1_GSM_DIRECTION_ENUM dir;
};

static const struct sapi_dir ccch_sapis[] = {
	{cOCTVC1_GSM_SAPI_ENUM_FCCH, cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_SCH, cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_BCCH, cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_PCH_AGCH, cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_RACH, cOCTVC1_GSM_DIRECTION_ENUM_RX_BTS_MS},
};

static const struct sapi_dir tchf_sapis[] = {
	{cOCTVC1_GSM_SAPI_ENUM_TCHF, cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_TCHF, cOCTVC1_GSM_DIRECTION_ENUM_RX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_FACCHF, cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_FACCHF, cOCTVC1_GSM_DIRECTION_ENUM_RX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_SACCH, cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_SACCH, cOCTVC1_GSM_DIRECTION_ENUM_RX_BTS_MS},
};

static const struct sapi_dir tchh_sapis[] = {
	{cOCTVC1_GSM_SAPI_ENUM_TCHH, cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_TCHH, cOCTVC1_GSM_DIRECTION_ENUM_RX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_FACCHH, cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_FACCHH, cOCTVC1_GSM_DIRECTION_ENUM_RX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_SACCH, cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_SACCH, cOCTVC1_GSM_DIRECTION_ENUM_RX_BTS_MS},
};

static const struct sapi_dir sdcch_sapis[] = {
	{cOCTVC1_GSM_SAPI_ENUM_SDCCH, cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_SDCCH, cOCTVC1_GSM_DIRECTION_ENUM_RX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_SACCH, cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_SACCH, cOCTVC1_GSM_DIRECTION_ENUM_RX_BTS_MS},
};

static const struct sapi_dir cbch_sapis[] = {
	{cOCTVC1_GSM_SAPI_ENUM_CBCH, cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS},
	/* Does the CBCH really have a SACCH in Downlink */
	{cOCTVC1_GSM_SAPI_ENUM_SACCH, cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS},
};

static const struct sapi_dir pdtch_sapis[] = {
	{cOCTVC1_GSM_SAPI_ENUM_PDTCH, cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_PDTCH, cOCTVC1_GSM_DIRECTION_ENUM_RX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_PTCCH, cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS},
	{cOCTVC1_GSM_SAPI_ENUM_PTCCH, cOCTVC1_GSM_DIRECTION_ENUM_RX_BTS_MS},
};

struct lchan_sapis {
	const struct sapi_dir *sapis;
	uint32_t num_sapis;
};

static const struct lchan_sapis sapis_for_lchan[_GSM_LCHAN_MAX] = {
	[GSM_LCHAN_SDCCH] = {
		.sapis = sdcch_sapis,
		.num_sapis = ARRAY_SIZE(sdcch_sapis),
	},
	[GSM_LCHAN_TCH_F] = {
		.sapis = tchf_sapis,
		.num_sapis = ARRAY_SIZE(tchf_sapis),
	},
	[GSM_LCHAN_TCH_H] = {
		.sapis = tchh_sapis,
		.num_sapis = ARRAY_SIZE(tchh_sapis),
	},
	[GSM_LCHAN_CCCH] = {
		.sapis = ccch_sapis,
		.num_sapis = ARRAY_SIZE(ccch_sapis),
	},
	[GSM_LCHAN_PDTCH] = {
		.sapis = pdtch_sapis,
		.num_sapis = ARRAY_SIZE(pdtch_sapis),
	},
	[GSM_LCHAN_CBCH] = {
		.sapis = cbch_sapis,
		.num_sapis = ARRAY_SIZE(cbch_sapis),
	},
};

static const uint8_t trx_rqd_attr[] = { NM_ATT_RF_MAXPOWR_R };

extern uint8_t rach_detected_LA_g;
extern uint8_t rach_detected_Other_g;

static int opstart_compl(struct gsm_abis_mo *mo)
{
	/* TODO: Send NACK in case of error! */

	/* Set to Operational State: Enabled */
	oml_mo_state_chg(mo, NM_OPSTATE_ENABLED, NM_AVSTATE_OK);

	/* hack to auto-activate all SAPIs for the BCCH/CCCH on TS0 */
	if (mo->obj_class == NM_OC_CHANNEL && mo->obj_inst.trx_nr == 0 &&
	    mo->obj_inst.ts_nr == 7) {
		struct gsm_lchan *cbch = gsm_bts_get_cbch(mo->bts);
		mo->bts->c0->ts[0].lchan[CCCH_LCHAN].rel_act_kind =
			LCHAN_REL_ACT_OML;
		lchan_activate(&mo->bts->c0->ts[0].lchan[CCCH_LCHAN]);
		if (cbch) {
			cbch->rel_act_kind = LCHAN_REL_ACT_OML;
			lchan_activate(cbch);
		}
	}

	/* Send OPSTART ack */
	return oml_mo_opstart_ack(mo);
}

static
tOCTVC1_GSM_ID_SUB_CHANNEL_NB_ENUM lchan_to_GsmL1_SubCh_t(const struct gsm_lchan
							  * lchan)
{
	switch (lchan->ts->pchan) {
	case GSM_PCHAN_CCCH_SDCCH4:
	case GSM_PCHAN_CCCH_SDCCH4_CBCH:
		if (lchan->type == GSM_LCHAN_CCCH)
			return cOCTVC1_GSM_ID_SUB_CHANNEL_NB_ENUM_ALL;
		/* fall-through */
	case GSM_PCHAN_TCH_H:
	case GSM_PCHAN_SDCCH8_SACCH8C:
	case GSM_PCHAN_SDCCH8_SACCH8C_CBCH:
		return (tOCTVC1_GSM_ID_SUB_CHANNEL_NB_ENUM) lchan->nr;
	case GSM_PCHAN_NONE:
	case GSM_PCHAN_CCCH:
	case GSM_PCHAN_TCH_F:
	case GSM_PCHAN_PDCH:
	case GSM_PCHAN_UNKNOWN:
	default:
		return cOCTVC1_GSM_ID_SUB_CHANNEL_NB_ENUM_ALL;
	}
	return cOCTVC1_GSM_ID_SUB_CHANNEL_NB_ENUM_ALL;
}

static void clear_amr_params(tOCTVC1_GSM_LOGICAL_CHANNEL_CONFIG * p_Config)
{
	/* common for the SIGN, V1 and EFR: */
	int i;
	p_Config->byCmiPhase = 0;
	p_Config->byInitRate = cOCTVC1_GSM_AMR_CODEC_MODE_ENUM_UNSET;
	/* 4 AMR active codec set */
	for (i = 0; i < cOCTVC1_GSM_RATE_LIST_SIZE; i++)
		p_Config->abyRate[i] = cOCTVC1_GSM_AMR_CODEC_MODE_ENUM_UNSET;
}

static void lchan2lch_par(struct gsm_lchan *lchan,
		   tOCTVC1_GSM_LOGICAL_CHANNEL_CONFIG * p_Config)
{
	struct amr_multirate_conf *amr_mrc = &lchan->tch.amr_mr;
	struct gsm48_multi_rate_conf *mr_conf =
	    (struct gsm48_multi_rate_conf *)amr_mrc->gsm48_ie;
	int j;

	LOGP(DL1C, LOGL_INFO, "%s: %s tch_mode=0x%02x\n",
	     gsm_lchan_name(lchan), __FUNCTION__, lchan->tch_mode);

	switch (lchan->tch_mode) {
	case GSM48_CMODE_SIGN:
		/* we have to set some TCH payload type even if we don't
		 * know yet what codec we will use later on */
		if (lchan->type == GSM_LCHAN_TCH_F) {
			clear_amr_params(p_Config);
		}
		break;

	case GSM48_CMODE_SPEECH_V1:
		clear_amr_params(p_Config);
		break;

	case GSM48_CMODE_SPEECH_EFR:
		clear_amr_params(p_Config);
		break;

	case GSM48_CMODE_SPEECH_AMR:
		p_Config->byCmiPhase = 1;	/* FIXME? */
		p_Config->byInitRate =
		    (tOCTVC1_GSM_AMR_CODEC_MODE_ENUM)
		    amr_get_initial_mode(lchan);

		/* initialize to clean state */
		for (j = 0; j < cOCTVC1_GSM_RATE_LIST_SIZE; j++)
			p_Config->abyRate[j] =
			    cOCTVC1_GSM_AMR_CODEC_MODE_ENUM_UNSET;

		j = 0;
		if (mr_conf->m4_75)
			p_Config->abyRate[j++] =
			    cOCTVC1_GSM_AMR_CODEC_MODE_ENUM_RATE_4_75;

		if (j >= cOCTVC1_GSM_RATE_LIST_SIZE)
			break;

		if (mr_conf->m5_15)
			p_Config->abyRate[j++] =
			    cOCTVC1_GSM_AMR_CODEC_MODE_ENUM_RATE_5_15;

		if (j >= cOCTVC1_GSM_RATE_LIST_SIZE)
			break;

		if (mr_conf->m5_90)
			p_Config->abyRate[j++] =
			    cOCTVC1_GSM_AMR_CODEC_MODE_ENUM_RATE_5_90;

		if (j >= cOCTVC1_GSM_RATE_LIST_SIZE)
			break;

		if (mr_conf->m6_70)
			p_Config->abyRate[j++] =
			    cOCTVC1_GSM_AMR_CODEC_MODE_ENUM_RATE_6_70;

		if (j >= cOCTVC1_GSM_RATE_LIST_SIZE)
			break;

		if (mr_conf->m7_40)
			p_Config->abyRate[j++] =
			    cOCTVC1_GSM_AMR_CODEC_MODE_ENUM_RATE_7_40;

		if (j >= cOCTVC1_GSM_RATE_LIST_SIZE)
			break;

		if (mr_conf->m7_95)
			p_Config->abyRate[j++] =
			    cOCTVC1_GSM_AMR_CODEC_MODE_ENUM_RATE_7_95;

		if (j >= cOCTVC1_GSM_RATE_LIST_SIZE)
			break;

		if (mr_conf->m10_2)
			p_Config->abyRate[j++] =
			    cOCTVC1_GSM_AMR_CODEC_MODE_ENUM_RATE_10_2;

		if (j >= cOCTVC1_GSM_RATE_LIST_SIZE)
			break;

		if (mr_conf->m12_2)
			p_Config->abyRate[j++] =
			    cOCTVC1_GSM_AMR_CODEC_MODE_ENUM_RATE_12_2;
		break;

	case GSM48_CMODE_DATA_14k5:
	case GSM48_CMODE_DATA_12k0:
	case GSM48_CMODE_DATA_6k0:
	case GSM48_CMODE_DATA_3k6:
		LOGP(DL1C, LOGL_ERROR, "%s: CSD not supported!\n",
		     gsm_lchan_name(lchan));
		break;

	}
}

/***********************************************************************
 * CORE SAPI QUEUE HANDLING
 ***********************************************************************/

static void sapi_queue_dispatch(struct gsm_lchan *lchan, int status);
static void sapi_queue_send(struct gsm_lchan *lchan);

static void sapi_clear_queue(struct llist_head *queue)
{
	struct sapi_cmd *next, *tmp;

	llist_for_each_entry_safe(next, tmp, queue, entry) {
		llist_del(&next->entry);
		talloc_free(next);
	}
}

static int lchan_act_compl_cb(struct octphy_hdl *fl1, struct msgb *resp, void *data)
{
	tOCTVC1_GSM_MSG_TRX_ACTIVATE_LOGICAL_CHANNEL_RSP *ar =
		(tOCTVC1_GSM_MSG_TRX_ACTIVATE_LOGICAL_CHANNEL_RSP *) resp->l2h;
	struct gsm_bts_trx *trx;
	struct gsm_lchan *lchan;
	uint8_t sapi;
	uint8_t direction;
	uint8_t status;

	/* in a completion call-back, we take msgb ownership and must
	 * release it before returning */

	mOCTVC1_GSM_MSG_TRX_ACTIVATE_LOGICAL_CHANNEL_RSP_SWAP(ar);
	trx = trx_by_l1h(fl1, ar->TrxId.byTrxId);
	if (!trx) {
		LOGPTRX(ar->TrxId.byTrxId, LOGL_ERROR, "response with unexpected physical transceiver-id during lchan activation\n");
		return -EINVAL;
	}

	lchan = get_lchan_by_lchid(trx, &ar->LchId);
	sapi = ar->LchId.bySAPI;
	direction = ar->LchId.byDirection;

	LOGP(DL1C, LOGL_INFO, "%s MPH-ACTIVATE.conf (%s ",
		gsm_lchan_name(lchan),
		get_value_string(octphy_l1sapi_names, sapi));
	LOGPC(DL1C, LOGL_INFO, "%s)\n",
		get_value_string(octphy_dir_names, direction));

	if (ar->Header.ulReturnCode != cOCTVC1_RC_OK) {
		LOGP(DL1C, LOGL_ERROR, "Error activating L1 SAPI %s\n",
			get_value_string(octphy_l1sapi_names, sapi));
		status = LCHAN_SAPI_S_ERROR;
	} else {
		status = LCHAN_SAPI_S_ASSIGNED;
	}

	switch (direction) {
	case cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS:
		lchan->sapis_dl[sapi] = status;
		break;
	case cOCTVC1_GSM_DIRECTION_ENUM_RX_BTS_MS:
		lchan->sapis_ul[sapi] = status;
		break;
	default:
		LOGP(DL1C, LOGL_ERROR, "Unknown direction %d\n",
			ar->LchId.byDirection);
		break;
	}

	if (llist_empty(&lchan->sapi_cmds)) {
		LOGP(DL1C, LOGL_ERROR,
		     "%s Got activation confirmation with empty queue\n",
		     gsm_lchan_name(lchan));
		goto err;
	}

	sapi_queue_dispatch(lchan, ar->Header.ulReturnCode);

err:
	msgb_free(resp);

	return 0;
}

static int mph_send_activate_req(struct gsm_lchan *lchan, struct sapi_cmd *cmd)
{
	struct phy_instance *pinst = trx_phy_instance(lchan->ts->trx);
	struct octphy_hdl *fl1h = pinst->phy_link->u.octphy.hdl;
	struct msgb *msg = l1p_msgb_alloc();
	tOCTVC1_GSM_MSG_TRX_ACTIVATE_LOGICAL_CHANNEL_CMD *lac;

	lac = (tOCTVC1_GSM_MSG_TRX_ACTIVATE_LOGICAL_CHANNEL_CMD *)
			msgb_put(msg, sizeof(*lac));
	l1if_fill_msg_hdr(&lac->Header, msg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  cOCTVC1_GSM_MSG_TRX_ACTIVATE_LOGICAL_CHANNEL_CID);

	lac->TrxId.byTrxId = pinst->u.octphy.trx_id;
	lac->LchId.byTimeslotNb = lchan->ts->nr;
	lac->LchId.bySubChannelNb = lchan_to_GsmL1_SubCh_t(lchan);
	lac->LchId.bySAPI = cmd->sapi;
	lac->LchId.byDirection = cmd->dir;

	lac->Config.byTimingAdvance = lchan->rqd_ta;
	lac->Config.byBSIC = lchan->ts->trx->bts->bsic;

	lchan2lch_par(lchan, &lac->Config);

	mOCTVC1_GSM_MSG_TRX_ACTIVATE_LOGICAL_CHANNEL_CMD_SWAP(lac);

	LOGP(DL1C, LOGL_INFO, "%s MPH-ACTIVATE.req (%s ",
		gsm_lchan_name(lchan),
		get_value_string(octphy_l1sapi_names, cmd->sapi));
	LOGPC(DL1C, LOGL_INFO, "%s)\n",
		get_value_string(octphy_dir_names, cmd->dir));

	return l1if_req_compl(fl1h, msg, lchan_act_compl_cb, NULL);
}


static tOCTVC1_GSM_CIPHERING_ID_ENUM rsl2l1_ciph[] = {
	[0] = cOCTVC1_GSM_CIPHERING_ID_ENUM_UNUSED,
	[1] = cOCTVC1_GSM_CIPHERING_ID_ENUM_A5_0,
	[2] = cOCTVC1_GSM_CIPHERING_ID_ENUM_A5_1,
	[3] = cOCTVC1_GSM_CIPHERING_ID_ENUM_A5_2,
	[4] = cOCTVC1_GSM_CIPHERING_ID_ENUM_A5_3
};

static int set_ciph_compl_cb(struct octphy_hdl *fl1, struct msgb *resp, void *data)
{
	tOCTVC1_GSM_MSG_TRX_MODIFY_PHYSICAL_CHANNEL_CIPHERING_RSP *pcr =
		(tOCTVC1_GSM_MSG_TRX_MODIFY_PHYSICAL_CHANNEL_CIPHERING_RSP *) resp->l2h;
	struct gsm_bts_trx *trx;
	struct gsm_bts_trx_ts *ts;
	struct gsm_lchan *lchan;

	/* in a completion call-back, we take msgb ownership and must
	 * release it before returning */

	mOCTVC1_GSM_MSG_TRX_MODIFY_PHYSICAL_CHANNEL_CIPHERING_RSP_SWAP(pcr);

	if (pcr->Header.ulReturnCode != cOCTVC1_RC_OK) {
		LOGP(DL1C, LOGL_ERROR, "Error: Cipher Request Failed!\n\n");
		LOGP(DL1C, LOGL_ERROR, "Exiting... \n\n");
		msgb_free(resp);
		exit(-1);
	}

	trx = trx_by_l1h(fl1, pcr->TrxId.byTrxId);
	if (!trx) {
		LOGPTRX(pcr->TrxId.byTrxId, LOGL_ERROR, "response with unexpected physical transceiver-id during cipher mode activation\n");
		return -EINVAL;
	}

	OSMO_ASSERT(pcr->TrxId.byTrxId == trx->nr);
	ts = &trx->ts[pcr->PchId.byTimeslotNb];
	/* for some strange reason the response does not tell which
	 * sub-channel, only th request contains this information :( */
	lchan = &ts->lchan[(unsigned long) data];

	/* TODO: This state machine should be shared accross BTS models? */
	switch (lchan->ciph_state) {
	case LCHAN_CIPH_RX_REQ:	
		lchan->ciph_state = LCHAN_CIPH_RX_CONF;
		break;
	case LCHAN_CIPH_RX_CONF_TX_REQ:
		lchan->ciph_state = LCHAN_CIPH_RXTX_CONF;
		break;
	case LCHAN_CIPH_RXTX_REQ:
		lchan->ciph_state = LCHAN_CIPH_RX_CONF_TX_REQ;
		break;
	case LCHAN_CIPH_NONE:
		break;
	default:
		LOGPC(DL1C, LOGL_INFO, "unhandled state %u\n", lchan->ciph_state);
	}

	if (llist_empty(&lchan->sapi_cmds)) {
		LOGP(DL1C, LOGL_ERROR,
		     "%s Got ciphering conf with empty queue\n",
		     gsm_lchan_name(lchan));
		goto err;
	}
	sapi_queue_dispatch(lchan, pcr->Header.ulReturnCode);

err:
	msgb_free(resp);
	return 0;
}

static int mph_send_config_ciphering(struct gsm_lchan *lchan, struct sapi_cmd *cmd)
{
	struct phy_instance *pinst = trx_phy_instance(lchan->ts->trx);
	struct octphy_hdl *fl1h = pinst->phy_link->u.octphy.hdl;
	struct msgb *msg = l1p_msgb_alloc();
	tOCTVC1_GSM_MSG_TRX_MODIFY_PHYSICAL_CHANNEL_CIPHERING_CMD *pcc;

	pcc = (tOCTVC1_GSM_MSG_TRX_MODIFY_PHYSICAL_CHANNEL_CIPHERING_CMD *)
			msgb_put(msg, sizeof(*pcc));
	l1if_fill_msg_hdr(&pcc->Header, msg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  cOCTVC1_GSM_MSG_TRX_MODIFY_PHYSICAL_CHANNEL_CIPHERING_CID);

	pcc->TrxId.byTrxId = pinst->u.octphy.trx_id;
	pcc->PchId.byTimeslotNb = lchan->ts->nr;
	pcc->ulSubchannelNb = lchan_to_GsmL1_SubCh_t(lchan);
	pcc->ulDirection = cmd->dir;
	pcc->Config.ulCipherId = rsl2l1_ciph[lchan->encr.alg_id];
	memcpy(pcc->Config.abyKey, lchan->encr.key, lchan->encr.key_len);

	LOGP(DL1C, LOGL_INFO, "%s SET_CIPHERING (ALG=%u %s)\n",
		gsm_lchan_name(lchan), pcc->Config.ulCipherId,
		get_value_string(octphy_dir_names, pcc->ulDirection));

	mOCTVC1_GSM_MSG_TRX_MODIFY_PHYSICAL_CHANNEL_CIPHERING_CMD_SWAP(pcc);

	/* we have to save the lchan number in this strange way, as the
	 * PHY does not return the ulSubchannelNr in the response to
	 * this command */
	return l1if_req_compl(fl1h, msg, set_ciph_compl_cb, (void *)(unsigned long) lchan->nr);
}


/**
 * Queue and possible execute a SAPI command. Return 1 in case the command was
 * already executed and 0 in case if it was only put into the queue
 */
static int queue_sapi_command(struct gsm_lchan *lchan, struct sapi_cmd *cmd)
{
	int start = llist_empty(&lchan->sapi_cmds);
	llist_add_tail(&cmd->entry, &lchan->sapi_cmds);

	if (!start)
		return 0;

	sapi_queue_send(lchan);
	return 1;
}

static int mph_info_chan_confirm(struct gsm_lchan *lchan,
				 enum osmo_mph_info_type type, uint8_t cause)
{
	struct osmo_phsap_prim l1sap;

	memset(&l1sap, 0, sizeof(l1sap));
	osmo_prim_init(&l1sap.oph, SAP_GSM_PH, PRIM_MPH_INFO, PRIM_OP_CONFIRM,
		       NULL);
	l1sap.u.info.type = type;
	l1sap.u.info.u.act_cnf.chan_nr = gsm_lchan2chan_nr(lchan);
	l1sap.u.info.u.act_cnf.cause = cause;

	return l1sap_up(lchan->ts->trx, &l1sap);
}

static int sapi_deactivate_cb(struct gsm_lchan *lchan, int status)
{
	/* FIXME: Error handling. There is no NACK...  */
	if (status != cOCTVC1_RC_OK && lchan->state == LCHAN_S_REL_REQ) {
		LOGP(DL1C, LOGL_ERROR,
		     "%s is now broken. Stopping the release.\n",
		     gsm_lchan_name(lchan));
		lchan_set_state(lchan, LCHAN_S_BROKEN);
		sapi_clear_queue(&lchan->sapi_cmds);
		mph_info_chan_confirm(lchan, PRIM_INFO_DEACTIVATE, 0);
		return -1;
	}

	if (!llist_empty(&lchan->sapi_cmds))
		return 0;

	/* Don't send an REL ACK on SACCH deactivate */
	if (lchan->state != LCHAN_S_REL_REQ)
		return 0;

	lchan_set_state(lchan, LCHAN_S_NONE);
	mph_info_chan_confirm(lchan, PRIM_INFO_DEACTIVATE, 0);
	return 0;
}

static int enqueue_sapi_deact_cmd(struct gsm_lchan *lchan, int sapi, int dir)
{
	struct sapi_cmd *cmd = talloc_zero(lchan->ts->trx, struct sapi_cmd);

	cmd->sapi = sapi;
	cmd->dir = dir;
	cmd->type = SAPI_CMD_DEACTIVATE;
	cmd->callback = sapi_deactivate_cb;
	return queue_sapi_command(lchan, cmd);
}

/*
 * Release the SAPI if it was allocated. E.g. the SACCH might be already
 * deactivated or during a hand-over the TCH was not allocated yet.
 */
static int check_sapi_release(struct gsm_lchan *lchan, int sapi, int dir)
{
	/* check if we should schedule a release */
	if (dir == cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS) {
		if (lchan->sapis_dl[sapi] != LCHAN_SAPI_S_ASSIGNED)
			return 0;
		lchan->sapis_dl[sapi] = LCHAN_SAPI_S_REL;
	} else if (dir == cOCTVC1_GSM_DIRECTION_ENUM_RX_BTS_MS) {
		if (lchan->sapis_ul[sapi] != LCHAN_SAPI_S_ASSIGNED)
			return 0;
		lchan->sapis_ul[sapi] = LCHAN_SAPI_S_REL;
	}
	/* now schedule the command and maybe dispatch it */
	return enqueue_sapi_deact_cmd(lchan, sapi, dir);
}

static int lchan_deactivate_sapis(struct gsm_lchan *lchan)
{
	struct phy_instance *pinst = trx_phy_instance(lchan->ts->trx);
	struct octphy_hdl *fl1h = pinst->phy_link->u.octphy.hdl;
	const struct lchan_sapis *s4l = &sapis_for_lchan[lchan->type];
	int i, res;

	res = 0;

	/* The order matters.. the Facch needs to be released first */
	for (i = s4l->num_sapis - 1; i >= 0; i--) {
		/* Stop the alive timer once we deactivate the SCH */
		if (s4l->sapis[i].sapi == cOCTVC1_GSM_SAPI_ENUM_SCH)
			osmo_timer_del(&fl1h->alive_timer);

		/* Release if it was allocated */
		res |= check_sapi_release(lchan, s4l->sapis[i].sapi, s4l->sapis[i].dir);
	}

	/* nothing was queued */
	if (res == 0) {
		LOGP(DL1C, LOGL_ERROR, "%s all SAPIs already released?\n",
		     gsm_lchan_name(lchan));
		lchan_set_state(lchan, LCHAN_S_BROKEN);
		mph_info_chan_confirm(lchan, PRIM_INFO_DEACTIVATE, 0);
	}

	return res;
}

static int lchan_deact_compl_cb(struct octphy_hdl *fl1, struct msgb *resp, void *data)
{
	tOCTVC1_GSM_MSG_TRX_DEACTIVATE_LOGICAL_CHANNEL_RSP *ldr =
		(tOCTVC1_GSM_MSG_TRX_DEACTIVATE_LOGICAL_CHANNEL_RSP *) resp->l2h;
	struct gsm_bts_trx *trx;
	struct gsm_lchan *lchan;
	struct sapi_cmd *cmd;
	uint8_t status;

	/* in a completion call-back, we take msgb ownership and must
	 * release it before returning */

	mOCTVC1_GSM_MSG_TRX_DEACTIVATE_LOGICAL_CHANNEL_RSP_SWAP(ldr);
	trx = trx_by_l1h(fl1, ldr->TrxId.byTrxId);
	if (!trx) {
		LOGPTRX(ldr->TrxId.byTrxId, LOGL_ERROR, "response with unexpected physical transceiver-id during lchan deactivation\n");
		return -EINVAL;
	}

	lchan = get_lchan_by_lchid(trx, &ldr->LchId);

	LOGP(DL1C, LOGL_INFO, "%s MPH-DEACTIVATE.conf (%s ",
		gsm_lchan_name(lchan),
		get_value_string(octphy_l1sapi_names, ldr->LchId.bySAPI));
	LOGPC(DL1C, LOGL_INFO, "%s)\n",
		get_value_string(octphy_dir_names, ldr->LchId.byDirection));

	if (ldr->Header.ulReturnCode == cOCTVC1_RC_OK) {
		DEBUGP(DL1C, "Successful deactivation of L1 SAPI %s on TS %u\n",
			get_value_string(octphy_l1sapi_names, ldr->LchId.bySAPI),
			ldr->LchId.byTimeslotNb);
		status = LCHAN_SAPI_S_NONE;
	} else {
		LOGP(DL1C, LOGL_ERROR,
		     "Error deactivating L1 SAPI %s on TS %u\n",
		     get_value_string(octphy_l1sapi_names, ldr->LchId.bySAPI),
		     ldr->LchId.byTimeslotNb);
		status = LCHAN_SAPI_S_ERROR;
	}

	switch (ldr->LchId.byDirection) {
	case cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS:
		lchan->sapis_dl[ldr->LchId.bySAPI] = status;
		break;
	case cOCTVC1_GSM_DIRECTION_ENUM_RX_BTS_MS:
		lchan->sapis_ul[ldr->LchId.bySAPI] = status;
		break;
	}

	if (llist_empty(&lchan->sapi_cmds)) {
		LOGP(DL1C, LOGL_ERROR,
		     "%s Got de-activation confirmation with empty queue\n",
		     gsm_lchan_name(lchan));
		goto err;
	}

	cmd = llist_entry(lchan->sapi_cmds.next, struct sapi_cmd, entry);
	if (cmd->sapi != ldr->LchId.bySAPI ||
	    cmd->dir != ldr->LchId.byDirection ||
	    cmd->type != SAPI_CMD_DEACTIVATE) {
		LOGP(DL1C, LOGL_ERROR,
			"%s Confirmation mismatch (%d, %d) (%d, %d)\n",
			gsm_lchan_name(lchan), cmd->sapi, cmd->dir,
			ldr->LchId.bySAPI, ldr->LchId.byDirection);
		goto err;
	}

	sapi_queue_dispatch(lchan, status);

err:
	msgb_free(resp);
	return 0;
}

static int mph_send_deactivate_req(struct gsm_lchan *lchan, struct sapi_cmd *cmd)
{
	struct phy_instance *pinst = trx_phy_instance(lchan->ts->trx);
	struct octphy_hdl *fl1h = pinst->phy_link->u.octphy.hdl;
	struct msgb *msg = l1p_msgb_alloc();
	tOCTVC1_GSM_MSG_TRX_DEACTIVATE_LOGICAL_CHANNEL_CMD *ldc;

	ldc = (tOCTVC1_GSM_MSG_TRX_DEACTIVATE_LOGICAL_CHANNEL_CMD *)
			msgb_put(msg, sizeof(*ldc));
	l1if_fill_msg_hdr(&ldc->Header, msg, fl1h,cOCTVC1_MSG_TYPE_COMMAND,
			  cOCTVC1_GSM_MSG_TRX_DEACTIVATE_LOGICAL_CHANNEL_CID);

	ldc->TrxId.byTrxId = pinst->u.octphy.trx_id;
	ldc->LchId.byTimeslotNb = lchan->ts->nr;
	ldc->LchId.bySubChannelNb = lchan_to_GsmL1_SubCh_t(lchan);
	ldc->LchId.byDirection = cmd->dir;
	ldc->LchId.bySAPI = cmd->sapi;

	mOCTVC1_GSM_MSG_TRX_DEACTIVATE_LOGICAL_CHANNEL_CMD_SWAP(ldc);

	LOGP(DL1C, LOGL_INFO, "%s MPH-DEACTIVATE.req (%s ",
		gsm_lchan_name(lchan),
		get_value_string(octphy_l1sapi_names, cmd->sapi));
	LOGPC(DL1C, LOGL_INFO, "%s)\n",
		get_value_string(octphy_dir_names, cmd->dir));

	return l1if_req_compl(fl1h, msg, lchan_deact_compl_cb, NULL);

}

/**
 * Execute the first SAPI command of the queue. In case of the markers
 * this method is re-entrant so we need to make sure to remove a command
 * from the list before calling a function that will queue a command.
 *
 * \return 0 in case no Gsm Request was sent, 1 otherwise
 */

static int sapi_queue_exeute(struct gsm_lchan *lchan)
{
	int res = 0;
	struct sapi_cmd *cmd;

	cmd = llist_entry(lchan->sapi_cmds.next, struct sapi_cmd, entry);

	switch (cmd->type) {
	case SAPI_CMD_ACTIVATE:
		mph_send_activate_req(lchan, cmd);
		res = 1;
		break;
	case SAPI_CMD_CONFIG_CIPHERING:
		mph_send_config_ciphering(lchan, cmd);
		res = 1;
		break;
	case SAPI_CMD_CONFIG_LOGCH_PARAM:
		/* TODO: Mode modif not supported by OctPHY currently */
		/* mph_send_config_logchpar(lchan, cmd); */
		res = 1;
		break;
	case SAPI_CMD_SACCH_REL_MARKER:
		llist_del(&cmd->entry);
		talloc_free(cmd);
		res =
		    check_sapi_release(lchan, cOCTVC1_GSM_SAPI_ENUM_SACCH,
				       cOCTVC1_GSM_ID_DIRECTION_ENUM_TX_BTS_MS);
		res |=
		    check_sapi_release(lchan, cOCTVC1_GSM_SAPI_ENUM_SACCH,
				       cOCTVC1_GSM_ID_DIRECTION_ENUM_RX_BTS_MS);
		break;
	case SAPI_CMD_REL_MARKER:
		llist_del(&cmd->entry);
		talloc_free(cmd);
		res = lchan_deactivate_sapis(lchan);
		break;
	case SAPI_CMD_DEACTIVATE:
		res = mph_send_deactivate_req(lchan, cmd);
		res = 1;
		break;
	default:
		LOGP(DL1C, LOGL_NOTICE,
		     "Unimplemented command type %d\n", cmd->type);
		llist_del(&cmd->entry);
		talloc_free(cmd);
		res = 0;
		abort();
		break;
	}

	return res;
}

static void sapi_queue_send(struct gsm_lchan *lchan)
{
	int res;

	do {
		res = sapi_queue_exeute(lchan);
	} while (res == 0 && !llist_empty(&lchan->sapi_cmds));
}

static void sapi_queue_dispatch(struct gsm_lchan *lchan, int status)
{
	int end;
	struct sapi_cmd *cmd = llist_entry(lchan->sapi_cmds.next,
					   struct sapi_cmd, entry);
	llist_del(&cmd->entry);
	end = llist_empty(&lchan->sapi_cmds);

	if (cmd->callback)
		cmd->callback(lchan, status);
	talloc_free(cmd);

	if (end || llist_empty(&lchan->sapi_cmds)) {
		LOGP(DL1C, LOGL_NOTICE,
		     "%s End of queue encountered. Now empty? %d\n",
		     gsm_lchan_name(lchan), llist_empty(&lchan->sapi_cmds));
		return;
	}

	sapi_queue_send(lchan);
}

/* we regularly check if the L1 is still sending us primitives.
   if not, we simply stop the BTS program (and be re-spawned) */
static void alive_timer_cb(void *data)
{
	struct octphy_hdl *fl1h = data;

	if (fl1h->alive_prim_cnt == 0) {
		LOGP(DL1C, LOGL_FATAL, "L1 is no longer sending primitives!\n");
		exit(23);
	}
	fl1h->alive_prim_cnt = 0;
	osmo_timer_schedule(&fl1h->alive_timer, 5, 0);
}

/***********************************************************************
 * RSL DEACTIVATE SACCH
 ***********************************************************************/

static void enqueue_sacch_rel_marker(struct gsm_lchan *lchan)
{
	struct sapi_cmd *cmd;

	/* remember we need to check if the SACCH is allocated */
	cmd = talloc_zero(lchan->ts->trx, struct sapi_cmd);
	cmd->type = SAPI_CMD_SACCH_REL_MARKER;
	queue_sapi_command(lchan, cmd);
}

int bts_model_lchan_deactivate_sacch(struct gsm_lchan *lchan)
{
	enqueue_sacch_rel_marker(lchan);
	return 0;
}

int l1if_rsl_deact_sacch(struct gsm_lchan *lchan)
{
	/* Only de-activate the SACCH if the lchan is active */
	if (lchan->state != LCHAN_S_ACTIVE)
		return 0;
	return bts_model_lchan_deactivate_sacch(lchan);
}


/***********************************************************************
 * RSL CHANNEL RELEASE
 ***********************************************************************/

static void enqueue_rel_marker(struct gsm_lchan *lchan)
{
	struct sapi_cmd *cmd;

	/* remember we need to release all active SAPIs */
	cmd = talloc_zero(lchan->ts->trx, struct sapi_cmd);
	cmd->type = SAPI_CMD_REL_MARKER;
	queue_sapi_command(lchan, cmd);
}

int bts_model_lchan_deactivate(struct gsm_lchan *lchan)
{
	lchan_set_state(lchan, LCHAN_S_REL_REQ);
	enqueue_rel_marker(lchan);
	return 0;
}

int l1if_rsl_chan_rel(struct gsm_lchan *lchan)
{
	/* A duplicate RF Release Request, ignore it */
	if (lchan->state == LCHAN_S_REL_REQ)
		return 0;
	lchan_deactivate(lchan);
	return 0;
}


/***********************************************************************
 * SET CIPHERING
 ***********************************************************************/

static void enqueue_sapi_ciphering_cmd(struct gsm_lchan *lchan, int dir)
{
	struct sapi_cmd *cmd = talloc_zero(lchan->ts->trx, struct sapi_cmd);

	cmd->dir = dir;
	cmd->type = SAPI_CMD_CONFIG_CIPHERING;
	queue_sapi_command(lchan, cmd);
}

int l1if_set_ciphering(struct gsm_lchan *lchan, int dir_downlink)
{
	int dir;

	// ignore the request when the channel is not active
	if (lchan->state != LCHAN_S_ACTIVE)
		return -1;

	if (dir_downlink)
		dir = cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS;
	else
		dir = cOCTVC1_GSM_DIRECTION_ENUM_RX_BTS_MS;

	enqueue_sapi_ciphering_cmd(lchan, dir);

	return 0;
}


/***********************************************************************
 * RSL MODE MODIFY
 ***********************************************************************/

/* Mode modify is currently not supported by OctPHY */
static void enqueue_sapi_logchpar_cmd(struct gsm_lchan *lchan, int dir)
{
	struct sapi_cmd *cmd = talloc_zero(lchan->ts->trx, struct sapi_cmd);

	cmd->dir = dir;
	cmd->type = SAPI_CMD_CONFIG_LOGCH_PARAM;
	queue_sapi_command(lchan, cmd);
}


/* Mode modify is currently not supported by OctPHY */
static int tx_confreq_logchpar(struct gsm_lchan *lchan, uint8_t direction)
{
	enqueue_sapi_logchpar_cmd(lchan, direction);

	return 0;
}

/* Mode modify is currently not supported by OctPHY */
int l1if_rsl_mode_modify(struct gsm_lchan *lchan)
{
	if (lchan->state != LCHAN_S_ACTIVE)
		return -1;

	/* channel mode, encryption and/or multirate have changed */

	/* update multi-rate config */
	tx_confreq_logchpar(lchan, cOCTVC1_GSM_DIRECTION_ENUM_RX_BTS_MS);
	tx_confreq_logchpar(lchan, cOCTVC1_GSM_DIRECTION_ENUM_TX_BTS_MS);

	/* FIXME: update encryption */

	return 0;
}


/***********************************************************************
 * LCHAN / SAPI ACTIVATION
 ***********************************************************************/

static int sapi_activate_cb(struct gsm_lchan *lchan, int status)
{
	if (status != cOCTVC1_RC_OK) {
		lchan_set_state(lchan, LCHAN_S_BROKEN);
		sapi_clear_queue(&lchan->sapi_cmds);
		mph_info_chan_confirm(lchan, PRIM_INFO_ACTIVATE,
				      RSL_ERR_EQUIPMENT_FAIL);
		return -1;
	}

	if (!llist_empty(&lchan->sapi_cmds))
		return 0;

	if (lchan->state != LCHAN_S_ACT_REQ)
		return 0;

	lchan_set_state(lchan, LCHAN_S_ACTIVE);

	mph_info_chan_confirm(lchan, PRIM_INFO_ACTIVATE, 0);

	return 0;
}

static void enqueue_sapi_act_cmd(struct gsm_lchan *lchan, int sapi, int dir)
{
	struct sapi_cmd *cmd = talloc_zero(lchan->ts->trx, struct sapi_cmd);

	cmd->sapi = sapi;
	cmd->dir = dir;
	cmd->type = SAPI_CMD_ACTIVATE;
	cmd->callback = sapi_activate_cb;
	queue_sapi_command(lchan, cmd);
}

int lchan_activate(struct gsm_lchan *lchan)
{
	struct phy_instance *pinst = trx_phy_instance(lchan->ts->trx);
	struct octphy_hdl *fl1h = pinst->phy_link->u.octphy.hdl;
	const struct lchan_sapis *s4l = &sapis_for_lchan[lchan->type];
	unsigned int i;

	lchan_set_state(lchan, LCHAN_S_ACT_REQ);

	DEBUGP(DL1C, "lchan_act called\n");

	if (!llist_empty(&lchan->sapi_cmds))
		LOGP(DL1C, LOGL_ERROR,
		     "%s Trying to activate lchan, but commands in queue\n",
		     gsm_lchan_name(lchan));

	for (i = 0; i < s4l->num_sapis; i++) {
		int sapi = s4l->sapis[i].sapi;
		int dir = s4l->sapis[i].dir;

		if (sapi == cOCTVC1_GSM_SAPI_ENUM_SCH) {
			/* once we activate the SCH, we should get MPH-TIME.ind */
			fl1h->alive_timer.cb = alive_timer_cb;
			fl1h->alive_timer.data = fl1h;
			fl1h->alive_prim_cnt = 0;
			osmo_timer_schedule(&fl1h->alive_timer, 5, 0);
		}
		enqueue_sapi_act_cmd(lchan, sapi, dir);
	}

	lchan_init_lapdm(lchan);

	return 0;
}

int l1if_rsl_chan_act(struct gsm_lchan *lchan)
{
	lchan_activate(lchan);
	return 0;
}

#define talloc_replace(dst, ctx, src)			\
	do {						\
		if (dst)				\
			talloc_free(dst);		\
		dst = talloc_strdup(ctx, (const char *) src);	\
	} while (0)

static int app_info_sys_compl_cb(struct octphy_hdl *fl1h, struct msgb *resp, void *data)
{
	tOCTVC1_MAIN_MSG_APPLICATION_INFO_SYSTEM_RSP *aisr =
		(tOCTVC1_MAIN_MSG_APPLICATION_INFO_SYSTEM_RSP *) resp->l2h;

	/* in a completion call-back, we take msgb ownership and must
	 * release it before returning */

	mOCTVC1_MAIN_MSG_APPLICATION_INFO_SYSTEM_RSP_SWAP(aisr);

	LOGP(DL1C, LOGL_INFO, "Rx APP-INFO-SYSTEM.resp (platform='%s', version='%s')\n",
		aisr->szPlatform, aisr->szVersion);

#if OCTPHY_MULTI_TRX == 1
	LOGP(DL1C, LOGL_INFO, "Note: compiled with multi-trx support.\n");
#else
	LOGP(DL1C, LOGL_INFO, "Note: compiled without multi-trx support.\n");
#endif

	talloc_replace(fl1h->info.system.platform, fl1h, aisr->szPlatform);
	talloc_replace(fl1h->info.system.version, fl1h, aisr->szVersion);

	msgb_free(resp);

	return 0;
}

int l1if_check_app_sys_version(struct gsm_bts_trx *trx)
{
	struct phy_instance *pinst = trx_phy_instance(trx);
	struct octphy_hdl *fl1h = pinst->phy_link->u.octphy.hdl;
	struct msgb *msg = l1p_msgb_alloc();
	tOCTVC1_MAIN_MSG_APPLICATION_INFO_SYSTEM_CMD *ais;

	ais = (tOCTVC1_MAIN_MSG_APPLICATION_INFO_SYSTEM_CMD *)
						msgb_put(msg, sizeof(*ais));
	mOCTVC1_MAIN_MSG_APPLICATION_INFO_SYSTEM_CMD_DEF(ais);
	l1if_fill_msg_hdr(&ais->Header, msg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  cOCTVC1_MAIN_MSG_APPLICATION_INFO_SYSTEM_CID);

	mOCTVC1_MAIN_MSG_APPLICATION_INFO_SYSTEM_CMD_SWAP(ais);

	LOGP(DL1C, LOGL_INFO, "Tx APP-INFO-SYSTEM.req\n");

	return l1if_req_compl(fl1h, msg, app_info_sys_compl_cb, pinst);
}

static int app_info_compl_cb(struct octphy_hdl *fl1h, struct msgb *resp,
			     void *data)
{
	char ver_hdr[32];
	struct phy_instance *pinst = data;
	tOCTVC1_MAIN_MSG_APPLICATION_INFO_RSP *air =
		(tOCTVC1_MAIN_MSG_APPLICATION_INFO_RSP *) resp->l2h;

	snprintf(ver_hdr, sizeof(ver_hdr), "%02i.%02i.%02i-B%i",
		cOCTVC1_MAIN_VERSION_MAJOR, cOCTVC1_MAIN_VERSION_MINOR,
		cOCTVC1_MAIN_VERSION_MAINTENANCE, cOCTVC1_MAIN_VERSION_BUILD);

	mOCTVC1_MAIN_MSG_APPLICATION_INFO_RSP_SWAP(air);

	LOGP(DL1C, LOGL_INFO,
	     "Rx APP-INFO.resp (name='%s', desc='%s', ver='%s', ver_hdr='%s')\n",
	     air->szName, air->szDescription, air->szVersion, ver_hdr);

	/* Check if the firmware version of the DSP matches the header files
	 * that were used to compile osmo-bts */
	if (strcmp(air->szVersion, ver_hdr) != 0) {
		LOGP(DL1C, LOGL_ERROR,
		     "Invalid header-file-version / dsp-firmware-version combination\n");
		LOGP(DL1C, LOGL_ERROR,
		     "Expected firmware version: %s\n", ver_hdr);
		LOGP(DL1C, LOGL_ERROR,
		     "Actual firmware version:   %s\n", air->szVersion);

		if (!no_fw_check) {
			LOGP(DL1C, LOGL_ERROR,
			     "use option -I to override the check (not recommened)\n");
			LOGP(DL1C, LOGL_ERROR,
			     "exiting...\n");
			exit(1);
		}
	}

	talloc_replace(fl1h->info.app.name, fl1h, air->szName);
	talloc_replace(fl1h->info.app.description, fl1h, air->szDescription);
	talloc_replace(fl1h->info.app.version, fl1h, air->szVersion);
	OSMO_ASSERT(strlen(ver_hdr) < sizeof(pinst->version));
	osmo_strlcpy(pinst->version, ver_hdr, strlen(ver_hdr));

	/* in a completion call-back, we take msgb ownership and must
	 * release it before returning */
	msgb_free(resp);

	return 0;
}

int l1if_check_app_version(struct gsm_bts_trx *trx)
{
	struct phy_instance *pinst = trx_phy_instance(trx);
	struct octphy_hdl *fl1h = pinst->phy_link->u.octphy.hdl;
	struct msgb *msg = l1p_msgb_alloc();
	tOCTVC1_MAIN_MSG_APPLICATION_INFO_CMD *ai;

	ai = (tOCTVC1_MAIN_MSG_APPLICATION_INFO_CMD *) msgb_put(msg, sizeof(*ai));
	mOCTVC1_MAIN_MSG_APPLICATION_INFO_CMD_DEF(ai);
	l1if_fill_msg_hdr(&ai->Header, msg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  cOCTVC1_MAIN_MSG_APPLICATION_INFO_CID);

	mOCTVC1_MAIN_MSG_APPLICATION_INFO_CMD_SWAP(ai);

	LOGP(DL1C, LOGL_INFO, "Tx APP-INFO.req\n");

	return l1if_req_compl(fl1h, msg, app_info_compl_cb, pinst);
}

static int trx_close_cb(struct octphy_hdl *fl1, struct msgb *resp, void *data)
{
	tOCTVC1_GSM_MSG_TRX_CLOSE_RSP *car =
		(tOCTVC1_GSM_MSG_TRX_CLOSE_RSP *) resp->l2h;

	/* in a completion call-back, we take msgb ownership and must
	 * release it before returning */

	mOCTVC1_GSM_MSG_TRX_CLOSE_RSP_SWAP(car);

	LOGP(DL1C, LOGL_INFO, "Rx TRX-CLOSE.conf(%u)\n", car->TrxId.byTrxId);

	msgb_free(resp);

	return 0;
}

static int trx_close(struct gsm_bts_trx *trx)
{
	struct phy_instance *pinst = trx_phy_instance(trx);
	struct phy_link *plink = pinst->phy_link;
	struct octphy_hdl *fl1h = plink->u.octphy.hdl;
	struct msgb *msg = l1p_msgb_alloc();
	tOCTVC1_GSM_MSG_TRX_CLOSE_CMD *cac;

	cac = (tOCTVC1_GSM_MSG_TRX_CLOSE_CMD *)
				msgb_put(msg, sizeof(*cac));
	l1if_fill_msg_hdr(&cac->Header, msg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  cOCTVC1_GSM_MSG_TRX_CLOSE_CID);

	cac->TrxId.byTrxId = pinst->u.octphy.trx_id;

	LOGP(DL1C, LOGL_INFO, "Tx TRX-CLOSE.req(%u)\n", cac->TrxId.byTrxId);

	mOCTVC1_GSM_MSG_TRX_CLOSE_CMD_SWAP(cac);

	return l1if_req_compl(fl1h, msg, trx_close_cb, NULL);
}

/* call-back once the TRX_OPEN_CID response arrives */
static int trx_open_compl_cb(struct octphy_hdl *fl1h, struct msgb *resp, void *data)
{
	struct gsm_bts_trx *trx;

	tOCTVC1_GSM_MSG_TRX_OPEN_RSP *or =
		(tOCTVC1_GSM_MSG_TRX_OPEN_RSP *) resp->l2h;

	/* in a completion call-back, we take msgb ownership and must
	 * release it before returning */

	mOCTVC1_GSM_MSG_TRX_OPEN_RSP_SWAP(or);
	trx = trx_by_l1h(fl1h, or->TrxId.byTrxId);
	if (!trx) {
		LOGPTRX(or->TrxId.byTrxId, LOGL_ERROR, "response with unexpected physical transceiver-id during TRX opening procedure -- abort\n");
		exit(1);
	}

	LOGP(DL1C, LOGL_INFO, "TRX-OPEN.resp(trx=%u) = %s\n",
		trx->nr, octvc1_rc2string(or->Header.ulReturnCode));

	/* FIXME: check for ulReturnCode == OK */
	if (or->Header.ulReturnCode != cOCTVC1_RC_OK) {
		LOGP(DL1C, LOGL_ERROR, "TRX-OPEN failed: %s\n",
			octvc1_rc2string(or->Header.ulReturnCode));
		msgb_free(resp);
		exit(1);
	}

	msgb_free(resp);

	opstart_compl(&trx->mo);

	octphy_hw_get_pcb_info(fl1h);
	octphy_hw_get_rf_port_info(fl1h, 0);
	octphy_hw_get_rf_ant_rx_config(fl1h, 0, 0);
	octphy_hw_get_rf_ant_tx_config(fl1h, 0, 0);
	octphy_hw_get_rf_ant_rx_config(fl1h, 0, 1);
	octphy_hw_get_rf_ant_tx_config(fl1h, 0, 1);
	octphy_hw_get_clock_sync_info(fl1h);
	fl1h->opened = 1;

	return 0;
}

int l1if_trx_open(struct gsm_bts_trx *trx)
{
	/* putting it all together */
	struct phy_instance *pinst = trx_phy_instance(trx);
	struct phy_link *plink = pinst->phy_link;
	struct octphy_hdl *fl1h = pinst->phy_link->u.octphy.hdl;
	struct msgb *msg = l1p_msgb_alloc();
	tOCTVC1_GSM_MSG_TRX_OPEN_CMD *oc;

	oc = (tOCTVC1_GSM_MSG_TRX_OPEN_CMD *) msgb_put(msg, sizeof(*oc));
	l1if_fill_msg_hdr(&oc->Header, msg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  cOCTVC1_GSM_MSG_TRX_OPEN_CID);
	oc->ulRfPortIndex = plink->u.octphy.rf_port_index;
	oc->TrxId.byTrxId = pinst->u.octphy.trx_id;
	oc->Config.ulBand = osmocom_to_octphy_band(trx->bts->band, trx->arfcn);
	oc->Config.usArfcn = trx->arfcn;

#if OCTPHY_MULTI_TRX == 1
	if (pinst->u.octphy.trx_id)
		oc->Config.usCentreArfcn = plink->u.octphy.center_arfcn;
	else {
		oc->Config.usCentreArfcn = trx->arfcn;
		plink->u.octphy.center_arfcn = trx->arfcn;
	}
	oc->Config.usBcchArfcn = trx->bts->c0->arfcn;
#endif
	oc->Config.usTsc = trx->bts->bsic & 0x7;
	oc->RfConfig.ulRxGainDb = plink->u.octphy.rx_gain_db;
	/* FIXME: compute this based on nominal transmit power, etc. */
	if (plink->u.octphy.tx_atten_flag) {
		oc->RfConfig.ulTxAttndB = plink->u.octphy.tx_atten_db;
	} else {
		/* Take the Tx Attn received in set radio attribures
		 * x4 is for the value in db */
		oc->RfConfig.ulTxAttndB = (trx->max_power_red) << 2;
	}

#if OCTPHY_USE_ANTENNA_ID == 1
	oc->RfConfig.ulTxAntennaId = plink->u.octphy.tx_ant_id;
	oc->RfConfig.ulRxAntennaId = plink->u.octphy.rx_ant_id;
#endif

#if OCTPHY_MULTI_TRX == 1
	LOGP(DL1C, LOGL_INFO, "Tx TRX-OPEN.req(trx=%u, rf_port=%u, arfcn=%u, "
		"center=%u, tsc=%u, rx_gain=%u, tx_atten=%u)\n",
		oc->TrxId.byTrxId, oc->ulRfPortIndex, oc->Config.usArfcn,
		oc->Config.usCentreArfcn, oc->Config.usTsc, oc->RfConfig.ulRxGainDb,
		oc->RfConfig.ulTxAttndB);
#else
	LOGP(DL1C, LOGL_INFO, "Tx TRX-OPEN.req(trx=%u, rf_port=%u, arfcn=%u, "
		"tsc=%u, rx_gain=%u, tx_atten=%u)\n",
		oc->TrxId.byTrxId, oc->ulRfPortIndex, oc->Config.usArfcn,
		oc->Config.usTsc, oc->RfConfig.ulRxGainDb,
		oc->RfConfig.ulTxAttndB);
#endif

	mOCTVC1_GSM_MSG_TRX_OPEN_CMD_SWAP(oc);

	return l1if_req_compl(fl1h, msg, trx_open_compl_cb, NULL);
}

#if OCTPHY_USE_16X_OVERSAMPLING == 1
static int over_sample_16x_modif_compl_cb(struct octphy_hdl *fl1,
					  struct msgb *resp, void *data)
{
	tOCTVC1_GSM_MSG_OVERSAMPLE_SELECT_16X_MODIFY_RSP *mcr =
	    (tOCTVC1_GSM_MSG_OVERSAMPLE_SELECT_16X_MODIFY_RSP*) resp->l2h;

	/* in a completion call-back, we take msgb ownership and must
	 * release it before returning */

	mOCTVC1_GSM_MSG_OVERSAMPLE_SELECT_16X_MODIFY_RSP_SWAP(mcr);

	LOGP(DL1C, LOGL_INFO, "Rx OVER-SAMPLE-16x-MODIFY.conf\n");

	msgb_free(resp);

	return 0;
}

static int l1if_over_sample_16x_modif(struct gsm_bts_trx *trx)
{
	/* NOTE: The 16x oversampling mode should always be enabled. Single-
	 * TRX operation will work with standard 4x oversampling, but multi-
	 * TRX requires 16x oversampling */

	struct phy_instance *pinst = trx_phy_instance(trx);
	struct phy_link *plink = pinst->phy_link;
	struct octphy_hdl *fl1h = pinst->phy_link->u.octphy.hdl;
	struct msgb *msg = l1p_msgb_alloc();
	tOCTVC1_GSM_MSG_OVERSAMPLE_SELECT_16X_MODIFY_CMD *mc;

	mc = (tOCTVC1_GSM_MSG_OVERSAMPLE_SELECT_16X_MODIFY_CMD*) msgb_put(msg,
									  sizeof
									  (*mc));
	mOCTVC1_GSM_MSG_OVERSAMPLE_SELECT_16X_MODIFY_CMD_DEF(mc);
	l1if_fill_msg_hdr(&mc->Header, msg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  cOCTVC1_GSM_MSG_OVERSAMPLE_SELECT_16X_MODIFY_CID);

	if (plink->u.octphy.over_sample_16x == true)
		mc->ulOversample16xEnableFlag = 1;
	else
		mc->ulOversample16xEnableFlag = 0;

	mOCTVC1_GSM_MSG_OVERSAMPLE_SELECT_16X_MODIFY_CMD_SWAP(mc);

	LOGP(DL1C, LOGL_INFO, "Tx OVER-SAMPLE-16x-MODIF.req\n");

	return l1if_req_compl(fl1h, msg, over_sample_16x_modif_compl_cb, 0);
}
#endif

uint32_t trx_get_hlayer1(struct gsm_bts_trx * trx)
{
	return 0;
}

static int trx_init(struct gsm_bts_trx *trx)
{
	if (!gsm_abis_mo_check_attr(&trx->mo, trx_rqd_attr,
				    ARRAY_SIZE(trx_rqd_attr))) {
		/* HACK: spec says we need to decline, but openbsc
		 * doesn't deal with this very well */
		return oml_mo_opstart_ack(&trx->mo);
		/* return oml_mo_opstart_nack(&trx->mo, NM_NACK_CANT_PERFORM); */
	}

	l1if_check_app_version(trx);
	l1if_check_app_sys_version(trx);

#if OCTPHY_USE_16X_OVERSAMPLING == 1
	l1if_over_sample_16x_modif(trx);
#endif

	return l1if_trx_open(trx);
}

/***********************************************************************
 * PHYSICAL CHANNE ACTIVATION
 ***********************************************************************/

static int pchan_act_compl_cb(struct octphy_hdl *fl1, struct msgb *resp, void *data)
{
	tOCTVC1_GSM_MSG_TRX_ACTIVATE_PHYSICAL_CHANNEL_RSP *ar =
		(tOCTVC1_GSM_MSG_TRX_ACTIVATE_PHYSICAL_CHANNEL_RSP *) resp->l2h;
	uint8_t ts_nr;
	struct gsm_bts_trx *trx;
	struct gsm_bts_trx_ts *ts;
	struct gsm_abis_mo *mo;

	/* in a completion call-back, we take msgb ownership and must
	 * release it before returning */

	mOCTVC1_GSM_MSG_TRX_ACTIVATE_PHYSICAL_CHANNEL_RSP_SWAP(ar);
	trx = trx_by_l1h(fl1, ar->TrxId.byTrxId);
	if (!trx) {
		LOGPTRX(ar->TrxId.byTrxId, LOGL_ERROR, "response with unexpected physical transceiver-id during physical channel activation -- abort\n");
		exit(1);
	}

	ts_nr = ar->PchId.byTimeslotNb;
	OSMO_ASSERT(ts_nr <= ARRAY_SIZE(trx->ts));

	ts = &trx->ts[ts_nr];

	LOGP(DL1C, LOGL_INFO, "PCHAN-ACT.conf(trx=%u, ts=%u, chcomb=%u) = %s\n",
		ts->trx->nr, ts->nr, ts->pchan,
		octvc1_rc2string(ar->Header.ulReturnCode));

	if (ar->Header.ulReturnCode != cOCTVC1_RC_OK) {
		LOGP(DL1C, LOGL_ERROR,
		     "PCHAN-ACT failed: %s\n\n",
		     octvc1_rc2string(ar->Header.ulReturnCode));
		LOGP(DL1C, LOGL_ERROR, "Exiting... \n\n");
		msgb_free(resp);
		exit(-1);
	}

	trx = ts->trx;
	mo = &trx->ts[ar->PchId.byTimeslotNb].mo;

	msgb_free(resp);

	return opstart_compl(mo);
}

static int ts_connect_as(struct gsm_bts_trx_ts *ts,
			 enum gsm_phys_chan_config pchan,
			 l1if_compl_cb * cb, void *data)
{
	struct phy_instance *pinst = trx_phy_instance(ts->trx);
	struct octphy_hdl *fl1h = pinst->phy_link->u.octphy.hdl;
	struct msgb *msg = l1p_msgb_alloc();
	tOCTVC1_GSM_MSG_TRX_ACTIVATE_PHYSICAL_CHANNEL_CMD *oc =
	    (tOCTVC1_GSM_MSG_TRX_ACTIVATE_PHYSICAL_CHANNEL_CMD *) oc;

	oc = (tOCTVC1_GSM_MSG_TRX_ACTIVATE_PHYSICAL_CHANNEL_CMD*)
		msgb_put(msg, sizeof(*oc));
	l1if_fill_msg_hdr(&oc->Header, msg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  cOCTVC1_GSM_MSG_TRX_ACTIVATE_PHYSICAL_CHANNEL_CID);

	oc->TrxId.byTrxId = pinst->u.octphy.trx_id;
	oc->PchId.byTimeslotNb = ts->nr;
	oc->ulChannelType = pchan_to_logChComb[pchan];

	/* TODO: how should we know the payload type here?  Also, why
	 * would the payload type have to be the same for both halves of
	 * a TCH/H ? */
	switch (oc->ulChannelType) {
	case cOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM_TCHF_FACCHF_SACCHTF:
	case cOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM_PDTCHF_PACCHF_PTCCHF:
		oc->ulPayloadType = cOCTVC1_GSM_PAYLOAD_TYPE_ENUM_FULL_RATE;
		break;
	case cOCTVC1_GSM_LOGICAL_CHANNEL_COMBINATION_ENUM_TCHH_FACCHH_SACCHTH:
		oc->ulPayloadType = cOCTVC1_GSM_PAYLOAD_TYPE_ENUM_HALF_RATE;
		break;
	}

	LOGP(DL1C, LOGL_INFO, "PCHAN-ACT.req(trx=%u, ts=%u, chcomb=%u)\n",
	     ts->trx->nr, ts->nr, pchan);

	mOCTVC1_GSM_MSG_TRX_ACTIVATE_PHYSICAL_CHANNEL_CMD_SWAP(oc);

	return l1if_req_compl(fl1h, msg, cb, data);
}

/* Dynamic timeslots: Disconnect callback, reports completed disconnection
 * to higher layers */
static int ts_disconnect_cb(struct octphy_hdl *fl1, struct msgb *resp,
			    void *data)
{
	tOCTVC1_GSM_MSG_TRX_DEACTIVATE_PHYSICAL_CHANNEL_RSP *ar =
	    (tOCTVC1_GSM_MSG_TRX_DEACTIVATE_PHYSICAL_CHANNEL_RSP *) resp->l2h;
	uint8_t ts_nr;
	struct gsm_bts_trx *trx;
	struct gsm_bts_trx_ts *ts;

	trx = trx_by_l1h(fl1, ar->TrxId.byTrxId);
	if (!trx) {
		LOGPTRX(ar->TrxId.byTrxId, LOGL_ERROR, "response with unexpected physical transceiver-id during ts disconnection\n");
		return -EINVAL;
	}

	ts_nr = ar->PchId.byTimeslotNb;
	ts = &trx->ts[ts_nr];

	cb_ts_disconnected(ts);

	return 0;
}

/* Dynamic timeslots: Connect  callback, reports completed disconnection to
 * higher layers */
static int ts_connect_cb(struct octphy_hdl *fl1, struct msgb *resp, void *data)
{
	tOCTVC1_GSM_MSG_TRX_ACTIVATE_PHYSICAL_CHANNEL_RSP *ar =
	    (tOCTVC1_GSM_MSG_TRX_ACTIVATE_PHYSICAL_CHANNEL_RSP *) resp->l2h;
	uint8_t ts_nr;
	struct gsm_bts_trx *trx;
	struct gsm_bts_trx_ts *ts;

	/* in a completion call-back, we take msgb ownership and must
	 * release it before returning */

	mOCTVC1_GSM_MSG_TRX_ACTIVATE_PHYSICAL_CHANNEL_RSP_SWAP(ar);
	trx = trx_by_l1h(fl1, ar->TrxId.byTrxId);
	if (!trx) {
		LOGPTRX(ar->TrxId.byTrxId, LOGL_ERROR, "response with unexpected physical transceiver-id while connecting ts\n");
		return -EINVAL;
	}

	ts_nr = ar->PchId.byTimeslotNb;
	OSMO_ASSERT(ts_nr <= ARRAY_SIZE(trx->ts));

	ts = &trx->ts[ts_nr];

	LOGP(DL1C, LOGL_INFO, "PCHAN-ACT.conf(trx=%u, ts=%u, chcomb=%u) = %s\n",
	     ts->trx->nr, ts->nr, ts->pchan,
	     octvc1_rc2string(ar->Header.ulReturnCode));

	if (ar->Header.ulReturnCode != cOCTVC1_RC_OK) {
		LOGP(DL1C, LOGL_ERROR,
		     "PCHAN-ACT failed: %s\n\n",
		     octvc1_rc2string(ar->Header.ulReturnCode));
		LOGP(DL1C, LOGL_ERROR, "Exiting... \n\n");
		msgb_free(resp);
		exit(-1);
	}

	msgb_free(resp);

	cb_ts_connected(ts);

	return 0;
}

/***********************************************************************
 * BTS MODEL CALLBACKS
 ***********************************************************************/

int bts_model_adjst_ms_pwr(struct gsm_lchan *lchan)
{
	/* TODO: How to do this ? */
	return 0;
}

int gsm_abis_mo_check_attr(const struct gsm_abis_mo *mo,
			   const uint8_t * attr_ids, unsigned int num_attr_ids)
{
	unsigned int i;

	if (!mo->nm_attr)
		return 0;

	for (i = 0; i < num_attr_ids; i++) {
		if (!TLVP_PRESENT(mo->nm_attr, attr_ids[i]))
			return 0;
	}
	return 1;
}

int bts_model_oml_estab(struct gsm_bts *bts)
{
	int i;
	for (i = 0; i < bts->num_trx; i++) {
		struct gsm_bts_trx *trx = gsm_bts_trx_num(bts, i);
		l1if_activate_rf(trx, 1);
	}
	return 0;
}

int bts_model_chg_adm_state(struct gsm_bts *bts, struct gsm_abis_mo *mo,
			    void *obj, uint8_t adm_state)
{
	int rc;

	struct gsm_bts_trx *trx;
	struct phy_instance *pinst;
	struct octphy_hdl *fl1h;

	switch (mo->obj_class) {
	case NM_OC_RADIO_CARRIER:

		trx = ((struct gsm_bts_trx *)obj);
		pinst = trx_phy_instance(trx);
		fl1h = pinst->phy_link->u.octphy.hdl;

		if (mo->procedure_pending) {
			LOGP(DL1C, LOGL_ERROR, "Discarding adm change command: "
			     "pending procedure on TRX %d\n", trx->nr);
			return 0;
		}
		mo->procedure_pending = 1;
		switch (adm_state) {
		case NM_STATE_LOCKED:

			pinst->u.octphy.trx_locked = 1;

			/* Stop heartbeat check */
			osmo_timer_del(&fl1h->alive_timer);

			bts_model_trx_deact_rf(trx);

			/* Close TRX */
			rc = bts_model_trx_close(trx);
			if (rc != 0) {
				LOGP(DL1C, LOGL_ERROR,
				     "Cannot close TRX %d, it is already closed.\n",
				     trx->nr);
			}
			break;

		case NM_STATE_UNLOCKED:

			if (pinst->u.octphy.trx_locked) {
				pinst->u.octphy.trx_locked = 0;
				l1if_activate_rf(trx, 1);
			}

			break;

		default:
			break;
		}

		mo->procedure_pending = 0;
		break;

	default:
		/* blindly accept all state changes */
		break;
	}

	mo->nm_state.administrative = adm_state;
	return oml_mo_statechg_ack(mo);
}

int bts_model_trx_deact_rf(struct gsm_bts_trx *trx)
{
	return l1if_activate_rf(trx, 0);
}

int bts_model_trx_close(struct gsm_bts_trx *trx)
{
	/* FIXME: close only one TRX */
	return trx_close(trx);
}


/* callback from OML */
int bts_model_check_oml(struct gsm_bts *bts, uint8_t msg_type,
			struct tlv_parsed *old_attr,
			struct tlv_parsed *new_attr, void *obj)
{
	/* FIXME: check if the attributes are valid */
	return 0;
}

/* callback from OML */
int bts_model_apply_oml(struct gsm_bts *bts, struct msgb *msg,
			struct tlv_parsed *new_attr, int kind, void *obj)
{
	if (kind == NM_OC_RADIO_CARRIER) {
		struct gsm_bts_trx *trx = obj;
		/*struct octphy_hdl *fl1h = trx_octphy_hdl(trx); */

		power_ramp_start(trx, get_p_target_mdBm(trx, 0), 0);
	}
	return oml_fom_ack_nack(msg, 0);
}


/* callback from OML */
int bts_model_opstart(struct gsm_bts *bts, struct gsm_abis_mo *mo, void *obj)
{
	int rc = -1;
	struct gsm_bts_trx_ts *ts;

	switch (mo->obj_class) {
	case NM_OC_RADIO_CARRIER:
		rc = trx_init(obj);
		break;
	case NM_OC_CHANNEL:
		ts = (struct gsm_bts_trx_ts*) obj;
		rc = ts_connect_as(ts, ts->pchan, pchan_act_compl_cb, NULL);
		break;
	case NM_OC_BTS:
	case NM_OC_SITE_MANAGER:
	case NM_OC_BASEB_TRANSC:
	case NM_OC_GPRS_NSE:
	case NM_OC_GPRS_CELL:
	case NM_OC_GPRS_NSVC:
		oml_mo_state_chg(mo, NM_OPSTATE_ENABLED, -1);
		rc = oml_mo_opstart_ack(mo);
		break;
	default:
		rc = oml_mo_opstart_nack(mo, NM_NACK_OBJCLASS_NOTSUPP);
	}
	return rc;
}

int bts_model_change_power(struct gsm_bts_trx *trx, int p_trxout_mdBm)
{
#pragma message ("Implement bts_model_change_power based on TRX_MODIFY_RF_CID (OS#3016)")
	return 0;
}

int bts_model_ts_disconnect(struct gsm_bts_trx_ts *ts)
{
	struct phy_instance *pinst = trx_phy_instance(ts->trx);
	struct octphy_hdl *fl1h = pinst->phy_link->u.octphy.hdl;
	struct msgb *msg = l1p_msgb_alloc();
	tOCTVC1_GSM_MSG_TRX_DEACTIVATE_PHYSICAL_CHANNEL_CMD *oc =
	    (tOCTVC1_GSM_MSG_TRX_DEACTIVATE_PHYSICAL_CHANNEL_CMD *) oc;

	oc = (tOCTVC1_GSM_MSG_TRX_DEACTIVATE_PHYSICAL_CHANNEL_CMD *)
	    msgb_put(msg, sizeof(*oc));
	l1if_fill_msg_hdr(&oc->Header, msg, fl1h, cOCTVC1_MSG_TYPE_COMMAND,
			  cOCTVC1_GSM_MSG_TRX_DEACTIVATE_PHYSICAL_CHANNEL_CID);

	oc->TrxId.byTrxId = pinst->u.octphy.trx_id;
	oc->PchId.byTimeslotNb = ts->nr;

	LOGP(DL1C, LOGL_INFO, "PCHAN-DEACT.req(trx=%u, ts=%u, chcomb=%u)\n",
	     ts->trx->nr, ts->nr, ts->pchan);

	mOCTVC1_GSM_MSG_TRX_DEACTIVATE_PHYSICAL_CHANNEL_CMD_SWAP(oc);

	return l1if_req_compl(fl1h, msg, ts_disconnect_cb, NULL);
}

int bts_model_ts_connect(struct gsm_bts_trx_ts *ts,
			 enum gsm_phys_chan_config as_pchan)
{
	if (as_pchan == GSM_PCHAN_TCH_F_PDCH
	    || as_pchan == GSM_PCHAN_TCH_F_TCH_H_PDCH) {
		LOGP(DL1C, LOGL_ERROR,
		     "%s Requested TS connect as %s,"
		     " expected a specific pchan instead\n",
		     gsm_ts_and_pchan_name(ts), gsm_pchan_name(as_pchan));
		exit(1);
		return -EINVAL;
	}

	return ts_connect_as(ts, as_pchan, ts_connect_cb, NULL);
}
