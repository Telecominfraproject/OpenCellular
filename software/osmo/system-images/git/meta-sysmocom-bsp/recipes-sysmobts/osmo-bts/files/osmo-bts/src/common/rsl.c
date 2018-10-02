/* GSM TS 08.58 RSL, BTS Side */

/* (C) 2011 by Andreas Eversberg <jolly@eversberg.eu>
 * (C) 2011-2017 by Harald Welte <laforge@gnumonks.org>
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

#include "btsconfig.h"	/* for PACKAGE_VERSION */

#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <stdbool.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <osmocom/core/msgb.h>
#include <osmocom/core/talloc.h>
#include <osmocom/gsm/rsl.h>
#include <osmocom/gsm/lapdm.h>
#include <osmocom/gsm/protocol/gsm_12_21.h>
#include <osmocom/gsm/protocol/gsm_08_58.h>
#include <osmocom/gsm/protocol/ipaccess.h>
#include <osmocom/trau/osmo_ortp.h>

#include <osmo-bts/logging.h>
#include <osmo-bts/gsm_data.h>
#include <osmo-bts/abis.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/rsl.h>
#include <osmo-bts/oml.h>
#include <osmo-bts/amr.h>
#include <osmo-bts/signal.h>
#include <osmo-bts/measurement.h>
#include <osmo-bts/pcu_if.h>
#include <osmo-bts/handover.h>
#include <osmo-bts/cbch.h>
#include <osmo-bts/l1sap.h>
#include <osmo-bts/bts_model.h>
#include <osmo-bts/pcuif_proto.h>

//#define FAKE_CIPH_MODE_COMPL


static int rsl_tx_error_report(struct gsm_bts_trx *trx, uint8_t cause, const uint8_t *chan_nr,
				const uint8_t *link_id, const struct msgb *orig_msg);

/* list of RSL SI types that can occur on the SACCH */
static const unsigned int rsl_sacch_sitypes[] = {
	RSL_SYSTEM_INFO_5,
	RSL_SYSTEM_INFO_6,
	RSL_SYSTEM_INFO_5bis,
	RSL_SYSTEM_INFO_5ter,
	RSL_EXT_MEAS_ORDER,
	RSL_MEAS_INFO,
};

/* FIXME: move this to libosmocore */
int osmo_in_array(unsigned int search, const unsigned int *arr, unsigned int size)
{
	unsigned int i;
	for (i = 0; i < size; i++) {
		if (arr[i] == search)
			return 1;
	}
	return 0;
}
#define OSMO_IN_ARRAY(search, arr) osmo_in_array(search, arr, ARRAY_SIZE(arr))

int msgb_queue_flush(struct llist_head *list)
{
	struct msgb *msg, *msg2;
	int count = 0;

	llist_for_each_entry_safe(msg, msg2, list, list) {
		msgb_free(msg);
		count++;
	}

	return count;
}

/* FIXME: move this to libosmocore */
void gsm48_gen_starting_time(uint8_t *out, struct gsm_time *gtime)
{
	uint8_t t1p = gtime->t1 % 32;
	out[0] = (t1p << 3) | (gtime->t3 >> 3);
	out[1] = (gtime->t3 << 5) | gtime->t2;
}

/* compute lchan->rsl_cmode and lchan->tch_mode from RSL CHAN MODE IE */
static void lchan_tchmode_from_cmode(struct gsm_lchan *lchan,
				     struct rsl_ie_chan_mode *cm)
{
	lchan->rsl_cmode = cm->spd_ind;
	lchan->ts->trx->bts->dtxd = (cm->dtx_dtu & RSL_CMOD_DTXd) ? true : false;

	switch (cm->chan_rate) {
	case RSL_CMOD_SP_GSM1:
		lchan->tch_mode = GSM48_CMODE_SPEECH_V1;
		break;
	case RSL_CMOD_SP_GSM2:
		lchan->tch_mode = GSM48_CMODE_SPEECH_EFR;
		break;
	case RSL_CMOD_SP_GSM3:
		lchan->tch_mode = GSM48_CMODE_SPEECH_AMR;
		break;
	case RSL_CMOD_SP_NT_14k5:
		lchan->tch_mode = GSM48_CMODE_DATA_14k5;
		break;
	case RSL_CMOD_SP_NT_12k0:
		lchan->tch_mode = GSM48_CMODE_DATA_12k0;
		break;
	case RSL_CMOD_SP_NT_6k0:
		lchan->tch_mode = GSM48_CMODE_DATA_6k0;
		break;
	}
}


/*
 * support
 */

/* Is this channel number for a dedicated channel (true) or not (false) */
static bool chan_nr_is_dchan(uint8_t chan_nr)
{
	/* See TS 48.058 9.3.1 + Osmocom extension for RSL_CHAN_OSMO_PDCH */
	if ((chan_nr & 0xc0) == 0x80)
		return false;
	else
		return true;
}

static struct gsm_lchan *lchan_lookup(struct gsm_bts_trx *trx, uint8_t chan_nr,
				      const char *log_name)
{
	int rc;
	struct gsm_lchan *lchan = rsl_lchan_lookup(trx, chan_nr, &rc);

	if (!lchan) {
		LOGP(DRSL, LOGL_ERROR, "%sunknown chan_nr=0x%02x\n", log_name,
		     chan_nr);
		return NULL;
	}

	if (rc < 0) {
		LOGP(DRSL, LOGL_ERROR, "%s %smismatching chan_nr=0x%02x\n",
		     gsm_ts_and_pchan_name(lchan->ts), log_name, chan_nr);
		return NULL;
	}
	return lchan;
}

static struct msgb *rsl_msgb_alloc(int hdr_size)
{
	struct msgb *nmsg;

	hdr_size += sizeof(struct ipaccess_head);

	nmsg = msgb_alloc_headroom(600+hdr_size, hdr_size, "RSL");
	if (!nmsg)
		return NULL;
	nmsg->l3h = nmsg->data;
	return nmsg;
}

static void rsl_trx_push_hdr(struct msgb *msg, uint8_t msg_type)
{
	struct abis_rsl_common_hdr *th;

	th = (struct abis_rsl_common_hdr *) msgb_push(msg, sizeof(*th));
	th->msg_discr = ABIS_RSL_MDISC_TRX;
	th->msg_type = msg_type;
}

static void rsl_cch_push_hdr(struct msgb *msg, uint8_t msg_type, uint8_t chan_nr)
{
	struct abis_rsl_cchan_hdr *cch;

	cch = (struct abis_rsl_cchan_hdr *) msgb_push(msg, sizeof(*cch));
	cch->c.msg_discr = ABIS_RSL_MDISC_COM_CHAN;
	cch->c.msg_type = msg_type;
	cch->ie_chan = RSL_IE_CHAN_NR;
	cch->chan_nr = chan_nr;
}

static void rsl_dch_push_hdr(struct msgb *msg, uint8_t msg_type, uint8_t chan_nr)
{
	struct abis_rsl_dchan_hdr *dch;

	dch = (struct abis_rsl_dchan_hdr *) msgb_push(msg, sizeof(*dch));
	dch->c.msg_discr = ABIS_RSL_MDISC_DED_CHAN;
	dch->c.msg_type = msg_type;
	dch->ie_chan = RSL_IE_CHAN_NR;
	dch->chan_nr = chan_nr;
}

static void rsl_ipa_push_hdr(struct msgb *msg, uint8_t msg_type, uint8_t chan_nr)
{
	struct abis_rsl_dchan_hdr *dch;

	dch = (struct abis_rsl_dchan_hdr *) msgb_push(msg, sizeof(*dch));
	dch->c.msg_discr = ABIS_RSL_MDISC_IPACCESS;
	dch->c.msg_type = msg_type;
	dch->ie_chan = RSL_IE_CHAN_NR;
	dch->chan_nr = chan_nr;
}

/*
 * TRX related messages
 */

/* 8.6.4 sending ERROR REPORT */
static int rsl_tx_error_report(struct gsm_bts_trx *trx, uint8_t cause, const uint8_t *chan_nr,
				const uint8_t *link_id, const struct msgb *orig_msg)
{
	unsigned int len = sizeof(struct abis_rsl_common_hdr);
	struct msgb *nmsg;

	LOGP(DRSL, LOGL_NOTICE, "Tx RSL Error Report: cause = 0x%02x\n", cause);

	if (orig_msg)
		len += 2 + 3+msgb_l2len(orig_msg); /* chan_nr + TLV(orig_msg) */
	if (chan_nr)
		len += 2;
	if (link_id)
		len += 2;

	nmsg = rsl_msgb_alloc(len);
	if (!nmsg)
		return -ENOMEM;
	msgb_tlv_put(nmsg, RSL_IE_CAUSE, 1, &cause);
	if (orig_msg && msgb_l2len(orig_msg) >= sizeof(struct abis_rsl_common_hdr)) {
		struct abis_rsl_common_hdr *ch = (struct abis_rsl_common_hdr *) msgb_l2(orig_msg);
		msgb_tv_put(nmsg, RSL_IE_MSG_ID, ch->msg_type);
	}
	if (chan_nr)
		msgb_tv_put(nmsg, RSL_IE_CHAN_NR, *chan_nr);
	if (link_id)
		msgb_tv_put(nmsg, RSL_IE_LINK_IDENT, *link_id);
	if (orig_msg)
		msgb_tlv_put(nmsg, RSL_IE_ERR_MSG, msgb_l2len(orig_msg), msgb_l2(orig_msg));

	rsl_trx_push_hdr(nmsg, RSL_MT_ERROR_REPORT);
	nmsg->trx = trx;

	return abis_bts_rsl_sendmsg(nmsg);
}

/* 8.6.1 sending RF RESOURCE INDICATION */
int rsl_tx_rf_res(struct gsm_bts_trx *trx)
{
	struct msgb *nmsg;

	LOGP(DRSL, LOGL_INFO, "Tx RSL RF RESource INDication\n");

	nmsg = rsl_msgb_alloc(sizeof(struct abis_rsl_common_hdr));
	if (!nmsg)
		return -ENOMEM;
	// FIXME: add interference levels of TRX
	msgb_tlv_put(nmsg, RSL_IE_RESOURCE_INFO, 0, NULL);
	rsl_trx_push_hdr(nmsg, RSL_MT_RF_RES_IND);
	nmsg->trx = trx;

	return abis_bts_rsl_sendmsg(nmsg);
}

/* 
 * common channel releated messages
 */

/* 8.5.1 BCCH INFOrmation is received */
static int rsl_rx_bcch_info(struct gsm_bts_trx *trx, struct msgb *msg)
{
	struct gsm_bts *bts = trx->bts;
	struct tlv_parsed tp;
	uint8_t rsl_si, count;
	enum osmo_sysinfo_type osmo_si;
	struct gsm48_system_information_type_2quater *si2q;
	struct bitvec bv;
	rsl_tlv_parse(&tp, msgb_l3(msg), msgb_l3len(msg));

	/* 9.3.30 System Info Type */
	if (!TLVP_PRESENT(&tp, RSL_IE_SYSINFO_TYPE))
		return rsl_tx_error_report(trx, RSL_ERR_MAND_IE_ERROR, NULL, NULL, msg);

	rsl_si = *TLVP_VAL(&tp, RSL_IE_SYSINFO_TYPE);
	if (OSMO_IN_ARRAY(rsl_si, rsl_sacch_sitypes))
		return rsl_tx_error_report(trx, RSL_ERR_IE_CONTENT, NULL, NULL, msg);

	osmo_si = osmo_rsl2sitype(rsl_si);
	if (osmo_si == SYSINFO_TYPE_NONE) {
		LOGP(DRSL, LOGL_NOTICE, " Rx RSL SI 0x%02x not supported.\n", rsl_si);
		return rsl_tx_error_report(trx, RSL_ERR_IE_CONTENT, NULL, NULL, msg);
	}
	/* 9.3.39 Full BCCH Information */
	if (TLVP_PRESENT(&tp, RSL_IE_FULL_BCCH_INFO)) {
		uint8_t len = TLVP_LEN(&tp, RSL_IE_FULL_BCCH_INFO);
		if (len > sizeof(sysinfo_buf_t)) {
			LOGP(DRSL, LOGL_ERROR, "Truncating received Full BCCH Info (%u -> %zu) for SI%s\n",
			     len, sizeof(sysinfo_buf_t), get_value_string(osmo_sitype_strs, osmo_si));
			len = sizeof(sysinfo_buf_t);
		}

		LOGP(DRSL, LOGL_INFO, " Rx RSL BCCH INFO (SI%s, %u bytes)\n",
		     get_value_string(osmo_sitype_strs, osmo_si), len);

		if (SYSINFO_TYPE_2quater == osmo_si) {
			si2q = (struct gsm48_system_information_type_2quater *) TLVP_VAL(&tp, RSL_IE_FULL_BCCH_INFO);
			bv.data = si2q->rest_octets;
			bv.data_len = GSM_MACBLOCK_LEN;
			bv.cur_bit = 3;
			bts->si2q_index = (uint8_t) bitvec_get_uint(&bv, 4);

			count = (uint8_t) bitvec_get_uint(&bv, 4);
			if (bts->si2q_count && bts->si2q_count != count) {
				LOGP(DRSL, LOGL_NOTICE, " Rx RSL SI2quater count updated: %u -> %d\n",
				     bts->si2q_count, count);
			}

			bts->si2q_count = count;
			if (bts->si2q_index > bts->si2q_count) {
				LOGP(DRSL, LOGL_ERROR, " Rx RSL SI2quater with index %u > count %u\n",
				     bts->si2q_index, bts->si2q_count);
				return rsl_tx_error_report(trx, RSL_ERR_IE_CONTENT, NULL, NULL, msg);
			}

			if (bts->si2q_index > SI2Q_MAX_NUM || bts->si2q_count > SI2Q_MAX_NUM) {
				LOGP(DRSL, LOGL_ERROR, " Rx RSL SI2quater with impossible parameters: index %u, count %u"
				     "should be <= %u\n", bts->si2q_index, bts->si2q_count, SI2Q_MAX_NUM);
				return rsl_tx_error_report(trx, RSL_ERR_IE_CONTENT, NULL, NULL, msg);
			}

			memset(GSM_BTS_SI2Q(bts, bts->si2q_index), GSM_MACBLOCK_PADDING, sizeof(sysinfo_buf_t));
			memcpy(GSM_BTS_SI2Q(bts, bts->si2q_index), TLVP_VAL(&tp, RSL_IE_FULL_BCCH_INFO), len);
		} else {
			memset(bts->si_buf[osmo_si], GSM_MACBLOCK_PADDING, sizeof(sysinfo_buf_t));
			memcpy(bts->si_buf[osmo_si], TLVP_VAL(&tp, RSL_IE_FULL_BCCH_INFO), len);
		}

		bts->si_valid |= (1 << osmo_si);

		if (SYSINFO_TYPE_3 == osmo_si && trx->nr == 0 &&
		    num_agch(trx, "RSL") != 1) {
			lchan_deactivate(&trx->bts->c0->ts[0].lchan[CCCH_LCHAN]);
			/* will be reactivated by sapi_deactivate_cb() */
			trx->bts->c0->ts[0].lchan[CCCH_LCHAN].rel_act_kind =
				LCHAN_REL_ACT_REACT;
		}

		if (SYSINFO_TYPE_13 == osmo_si)
			pcu_tx_si13(trx->bts, true);

	} else if (TLVP_PRESENT(&tp, RSL_IE_L3_INFO)) {
		uint16_t len = TLVP_LEN(&tp, RSL_IE_L3_INFO);
		if (len > sizeof(sysinfo_buf_t))
			len = sizeof(sysinfo_buf_t);
		bts->si_valid |= (1 << osmo_si);
		memset(bts->si_buf[osmo_si], 0x2b, sizeof(sysinfo_buf_t));
		memcpy(bts->si_buf[osmo_si],
			TLVP_VAL(&tp, RSL_IE_L3_INFO), len);
		LOGP(DRSL, LOGL_INFO, " Rx RSL BCCH INFO (SI%s)\n",
			get_value_string(osmo_sitype_strs, osmo_si));
	} else {
		bts->si_valid &= ~(1 << osmo_si);
		LOGP(DRSL, LOGL_INFO, " RX RSL Disabling BCCH INFO (SI%s)\n",
			get_value_string(osmo_sitype_strs, osmo_si));
		if (SYSINFO_TYPE_13 == osmo_si)
			pcu_tx_si13(trx->bts, false);
	}
	osmo_signal_dispatch(SS_GLOBAL, S_NEW_SYSINFO, bts);

	return 0;
}

/* 8.5.2 CCCH Load Indication (PCH) */
int rsl_tx_ccch_load_ind_pch(struct gsm_bts *bts, uint16_t paging_avail)
{
	struct msgb *msg;

	msg = rsl_msgb_alloc(sizeof(struct abis_rsl_cchan_hdr));
	if (!msg)
		return -ENOMEM;
	rsl_cch_push_hdr(msg, RSL_MT_CCCH_LOAD_IND, RSL_CHAN_PCH_AGCH);
	msgb_tv16_put(msg, RSL_IE_PAGING_LOAD, paging_avail);
	msg->trx = bts->c0;

	return abis_bts_rsl_sendmsg(msg);
}

/* 8.5.2 CCCH Load Indication (RACH) */
int rsl_tx_ccch_load_ind_rach(struct gsm_bts *bts, uint16_t total,
			      uint16_t busy, uint16_t access)
{
	struct msgb *msg;

	msg = rsl_msgb_alloc(sizeof(struct abis_rsl_cchan_hdr));
	if (!msg)
		return -ENOMEM;
	rsl_cch_push_hdr(msg, RSL_MT_CCCH_LOAD_IND, RSL_CHAN_RACH);
	/* tag and length */
	msgb_tv_put(msg, RSL_IE_RACH_LOAD, 6);
	/* content of the IE */
	msgb_put_u16(msg, total);
	msgb_put_u16(msg, busy);
	msgb_put_u16(msg, access);

	msg->trx = bts->c0;

	return abis_bts_rsl_sendmsg(msg);
}

/* 8.5.4 DELETE INDICATION */
static int rsl_tx_delete_ind(struct gsm_bts *bts, const uint8_t *ia, uint8_t ia_len)
{
	struct msgb *msg;

	msg = rsl_msgb_alloc(sizeof(struct abis_rsl_cchan_hdr));
	if (!msg)
		return -ENOMEM;
	rsl_cch_push_hdr(msg, RSL_MT_DELETE_IND, RSL_CHAN_PCH_AGCH);
	msgb_tlv_put(msg, RSL_IE_FULL_IMM_ASS_INFO, ia_len, ia);
	msg->trx = bts->c0;

	return abis_bts_rsl_sendmsg(msg);
}

/* 8.5.5 PAGING COMMAND */
static int rsl_rx_paging_cmd(struct gsm_bts_trx *trx, struct msgb *msg)
{
	struct tlv_parsed tp;
	struct gsm_bts *bts = trx->bts;
	uint8_t chan_needed = 0, paging_group;
	const uint8_t *identity_lv;
	int rc;

	rsl_tlv_parse(&tp, msgb_l3(msg), msgb_l3len(msg));

	if (!TLVP_PRESENT(&tp, RSL_IE_PAGING_GROUP) ||
	    !TLVP_PRESENT(&tp, RSL_IE_MS_IDENTITY))
		return rsl_tx_error_report(trx, RSL_ERR_MAND_IE_ERROR, NULL, NULL, msg);

	paging_group = *TLVP_VAL(&tp, RSL_IE_PAGING_GROUP);
	identity_lv = TLVP_VAL(&tp, RSL_IE_MS_IDENTITY)-1;

	if (TLVP_PRES_LEN(&tp, RSL_IE_CHAN_NEEDED, 1))
		chan_needed = *TLVP_VAL(&tp, RSL_IE_CHAN_NEEDED);

	rc = paging_add_identity(bts->paging_state, paging_group, identity_lv, chan_needed);
	if (rc < 0) {
		/* FIXME: notfiy the BSC on other errors? */
		if (rc == -ENOSPC)
			oml_fail_rep(OSMO_EVT_MIN_PAG_TAB_FULL,
				     "BTS paging table is full");
	}

	pcu_tx_pag_req(identity_lv, chan_needed);

	return 0;
}

/* 8.5.8 SMS BROADCAST COMMAND */
static int rsl_rx_sms_bcast_cmd(struct gsm_bts_trx *trx, struct msgb *msg)
{
	struct tlv_parsed tp;
	struct rsl_ie_cb_cmd_type *cb_cmd_type;

	rsl_tlv_parse(&tp, msgb_l3(msg), msgb_l3len(msg));

	if (!TLVP_PRESENT(&tp, RSL_IE_CB_CMD_TYPE) ||
	    !TLVP_PRESENT(&tp, RSL_IE_SMSCB_MSG))
		return rsl_tx_error_report(trx, RSL_ERR_MAND_IE_ERROR, NULL, NULL, msg);

	cb_cmd_type = (struct rsl_ie_cb_cmd_type *)
					TLVP_VAL(&tp, RSL_IE_CB_CMD_TYPE);

	return bts_process_smscb_cmd(trx->bts, *cb_cmd_type,
				     TLVP_LEN(&tp, RSL_IE_SMSCB_MSG),
				     TLVP_VAL(&tp, RSL_IE_SMSCB_MSG));
}

/*! Prefix a given SACCH frame with a L2/LAPDm UI header and store it in given output buffer.
 *  \param[out] buf Output buffer, must be caller-allocated and hold at least len + 2 or sizeof(sysinfo_buf_t) bytes
 *  \param[out] valid pointer to bit-mask of 'valid' System information types
 *  \param[in] current input data (L3 without L2/L1 header)
 *  \param[in] osmo_si Sytstem Infrormation Type (SYSINFO_TYPE_*)
 *  \param[in] len length of \a current in octets */
static inline void lapdm_ui_prefix(uint8_t *buf, uint32_t *valid, const uint8_t *current, uint8_t osmo_si, uint16_t len)
{
	/* We have to pre-fix with the two-byte LAPDM UI header */
	if (len > sizeof(sysinfo_buf_t) - 2) {
		LOGP(DRSL, LOGL_ERROR, "Truncating received SI%s (%u -> %zu) to prepend LAPDM UI header (2 bytes)\n",
		     get_value_string(osmo_sitype_strs, osmo_si), len, sizeof(sysinfo_buf_t) - 2);
		len = sizeof(sysinfo_buf_t) - 2;
	}

	(*valid) |= (1 << osmo_si);
	buf[0] = 0x03;	/* C/R + EA */
	buf[1] = 0x03;	/* UI frame */

	memset(buf + 2, GSM_MACBLOCK_PADDING, sizeof(sysinfo_buf_t) - 2);
	memcpy(buf + 2, current, len);
}

/*! Prefix a given SACCH frame with a L2/LAPDm UI header and store it in given BTS SACCH buffer
 *  \param[out] bts BTS in whose System Information State we shall store
 *  \param[in] current input data (L3 without L2/L1 header)
 *  \param[in] osmo_si Sytstem Infrormation Type (SYSINFO_TYPE_*)
 *  \param[in] len length of \a current in octets */
static inline void lapdm_ui_prefix_bts(struct gsm_bts *bts, const uint8_t *current, uint8_t osmo_si, uint16_t len)
{
	lapdm_ui_prefix(GSM_BTS_SI(bts, osmo_si), &bts->si_valid, current, osmo_si, len);
}

/*! Prefix a given SACCH frame with a L2/LAPDm UI header and store it in given lchan SACCH buffer
 *  \param[out] lchan Logical Channel in whose System Information State we shall store
 *  \param[in] current input data (L3 without L2/L1 header)
 *  \param[in] osmo_si Sytstem Infrormation Type (SYSINFO_TYPE_*)
 *  \param[in] len length of \a current in octets */
static inline void lapdm_ui_prefix_lchan(struct gsm_lchan *lchan, const uint8_t *current, uint8_t osmo_si, uint16_t len)
{
	lapdm_ui_prefix(GSM_LCHAN_SI(lchan, osmo_si), &lchan->si.valid, current, osmo_si, len);
}

/* 8.6.2 SACCH FILLING */
static int rsl_rx_sacch_fill(struct gsm_bts_trx *trx, struct msgb *msg)
{
	struct gsm_bts *bts = trx->bts;
	struct tlv_parsed tp;
	uint8_t rsl_si;
	enum osmo_sysinfo_type osmo_si;

	rsl_tlv_parse(&tp, msgb_l3(msg), msgb_l3len(msg));

	/* 9.3.30 System Info Type */
	if (!TLVP_PRESENT(&tp, RSL_IE_SYSINFO_TYPE))
		return rsl_tx_error_report(trx, RSL_ERR_MAND_IE_ERROR, NULL, NULL, msg);

	rsl_si = *TLVP_VAL(&tp, RSL_IE_SYSINFO_TYPE);
	if (!OSMO_IN_ARRAY(rsl_si, rsl_sacch_sitypes))
		return rsl_tx_error_report(trx, RSL_ERR_IE_CONTENT, NULL, NULL, msg);

	osmo_si = osmo_rsl2sitype(rsl_si);
	if (osmo_si == SYSINFO_TYPE_NONE) {
		LOGP(DRSL, LOGL_NOTICE, " Rx SACCH SI 0x%02x not supported.\n", rsl_si);
		return rsl_tx_error_report(trx, RSL_ERR_IE_CONTENT, NULL, NULL, msg);
	}
	if (TLVP_PRESENT(&tp, RSL_IE_L3_INFO)) {
		uint16_t len = TLVP_LEN(&tp, RSL_IE_L3_INFO);
		lapdm_ui_prefix_bts(bts, TLVP_VAL(&tp, RSL_IE_L3_INFO), osmo_si, len);

		LOGP(DRSL, LOGL_INFO, " Rx RSL SACCH FILLING (SI%s, %u bytes)\n",
		     get_value_string(osmo_sitype_strs, osmo_si), len);
	} else {
		bts->si_valid &= ~(1 << osmo_si);
		LOGP(DRSL, LOGL_INFO, " Rx RSL Disabling SACCH FILLING (SI%s)\n",
			get_value_string(osmo_sitype_strs, osmo_si));
	}
	osmo_signal_dispatch(SS_GLOBAL, S_NEW_SYSINFO, bts);

	return 0;

}

/* 8.5.6 IMMEDIATE ASSIGN COMMAND is received */
static int rsl_rx_imm_ass(struct gsm_bts_trx *trx, struct msgb *msg)
{
	struct tlv_parsed tp;

	rsl_tlv_parse(&tp, msgb_l3(msg), msgb_l3len(msg));

	if (!TLVP_PRESENT(&tp, RSL_IE_FULL_IMM_ASS_INFO))
		return rsl_tx_error_report(trx, RSL_ERR_MAND_IE_ERROR, NULL, NULL, msg);

	rate_ctr_inc2(trx->bts->ctrs, BTS_CTR_AGCH_RCVD);

	/* cut down msg to the 04.08 RR part */
	msg->l3h = (uint8_t *) TLVP_VAL(&tp, RSL_IE_FULL_IMM_ASS_INFO);
	msg->data = msg->l3h;
	msg->l2h = NULL;
	msg->len = TLVP_LEN(&tp, RSL_IE_FULL_IMM_ASS_INFO);

	/* put into the AGCH queue of the BTS */
	if (bts_agch_enqueue(trx->bts, msg) < 0) {
		/* if there is no space in the queue: send DELETE IND */
		rsl_tx_delete_ind(trx->bts, TLVP_VAL(&tp, RSL_IE_FULL_IMM_ASS_INFO),
				  TLVP_LEN(&tp, RSL_IE_FULL_IMM_ASS_INFO));
		rate_ctr_inc2(trx->bts->ctrs, BTS_CTR_AGCH_DELETED);
		msgb_free(msg);
	}

	/* return 1 means: don't msgb_free() the msg */
	return 1;
}

/*
 * dedicated channel related messages
 */

/* Send an RF CHANnel RELease ACKnowledge with the given chan_nr. This chan_nr may mismatch the current
 * lchan state, if we received a CHANnel RELease for an already released channel, and we're just acking
 * what we got without taking any action. */
static int tx_rf_rel_ack(struct gsm_lchan *lchan, uint8_t chan_nr)
{
	struct msgb *msg;

	msg = rsl_msgb_alloc(sizeof(struct abis_rsl_dchan_hdr));
	if (!msg)
		return -ENOMEM;

	rsl_dch_push_hdr(msg, RSL_MT_RF_CHAN_REL_ACK, chan_nr);
	msg->trx = lchan->ts->trx;

	return abis_bts_rsl_sendmsg(msg);
}

/* 8.4.19 sending RF CHANnel RELease ACKnowledge */
int rsl_tx_rf_rel_ack(struct gsm_lchan *lchan)
{
	uint8_t chan_nr = gsm_lchan2chan_nr(lchan);
	bool send_rel_ack;

	switch (lchan->rel_act_kind) {
	case LCHAN_REL_ACT_RSL:
		send_rel_ack = true;
		break;

	case LCHAN_REL_ACT_PCU:
		switch (lchan->ts->pchan) {
		case GSM_PCHAN_TCH_F_TCH_H_PDCH:
			if (lchan->ts->dyn.pchan_is != GSM_PCHAN_PDCH) {
				LOGP(DRSL, LOGL_ERROR,
				     "%s (ss=%d) PDCH release: not in PDCH mode\n",
				     gsm_ts_and_pchan_name(lchan->ts), lchan->nr);
				/* well, what to do about it ... carry on and hope it's fine. */
			}
			/* remember the fact that the TS is now released */
			lchan->ts->dyn.pchan_is = GSM_PCHAN_NONE;
			/* Continue to ack the release below. (This is a non-standard rel ack invented
			 * specifically for GSM_PCHAN_TCH_F_TCH_H_PDCH). */
			send_rel_ack = true;
			break;
		case GSM_PCHAN_TCH_F_PDCH:
			/* GSM_PCHAN_TCH_F_PDCH, does not require a rel ack. The caller
			 * l1sap_info_rel_cnf() will continue with bts_model_ts_disconnect(). */
			send_rel_ack = false;
			break;
		default:
			LOGP(DRSL, LOGL_ERROR, "%s PCU rel ack for unexpected lchan kind\n",
			     gsm_lchan_name(lchan));
			/* Release certainly was not requested by the BSC via RSL, so don't ack. */
			send_rel_ack = false;
			break;
		}
		break;

	default:
		/* A rel that was not requested by the BSC via RSL, hence not sending a rel ack to the
		 * BSC. */
		send_rel_ack = false;
		break;
	}

	if (!send_rel_ack) {
		LOGP(DRSL, LOGL_NOTICE, "%s not sending REL ACK\n",
			gsm_lchan_name(lchan));
		return 0;
	}

	LOGP(DRSL, LOGL_NOTICE, "%s (ss=%d) %s Tx CHAN REL ACK\n",
	     gsm_ts_and_pchan_name(lchan->ts), lchan->nr,
	     gsm_lchant_name(lchan->type));

	/*
	 * Free the LAPDm resources now that the BTS
	 * has released all the resources.
	 */
	lapdm_channel_exit(&lchan->lapdm_ch);

	return tx_rf_rel_ack(lchan, chan_nr);
}

/* 8.4.2 sending CHANnel ACTIVation ACKnowledge */
static int rsl_tx_chan_act_ack(struct gsm_lchan *lchan)
{
	struct gsm_time *gtime = get_time(lchan->ts->trx->bts);
	struct msgb *msg;
	uint8_t chan_nr = gsm_lchan2chan_nr(lchan);
	uint8_t ie[2];

	LOGP(DRSL, LOGL_NOTICE, "%s (ss=%d) %s Tx CHAN ACT ACK\n",
	     gsm_ts_and_pchan_name(lchan->ts), lchan->nr,
	     gsm_lchant_name(lchan->type));

	msg = rsl_msgb_alloc(sizeof(struct abis_rsl_dchan_hdr));
	if (!msg)
		return -ENOMEM;

	gsm48_gen_starting_time(ie, gtime);
	msgb_tv_fixed_put(msg, RSL_IE_FRAME_NUMBER, 2, ie);
	rsl_dch_push_hdr(msg, RSL_MT_CHAN_ACTIV_ACK, chan_nr);
	msg->trx = lchan->ts->trx;

	/* since activation was successful, do some lchan initialization */
	lchan->meas.res_nr = 0;

	return abis_bts_rsl_sendmsg(msg);
}

/* 8.4.7 sending HANDOver DETection */
int rsl_tx_hando_det(struct gsm_lchan *lchan, uint8_t *ho_delay)
{
	struct msgb *msg;
	uint8_t chan_nr = gsm_lchan2chan_nr(lchan);

	LOGP(DRSL, LOGL_INFO, "Sending HANDOver DETect\n");

	msg = rsl_msgb_alloc(sizeof(struct abis_rsl_dchan_hdr));
	if (!msg)
		return -ENOMEM;

	/* 9.3.17 Access Delay */
	if (ho_delay)
		msgb_tv_put(msg, RSL_IE_ACCESS_DELAY, *ho_delay);

	rsl_dch_push_hdr(msg, RSL_MT_HANDO_DET, chan_nr);
	msg->trx = lchan->ts->trx;

	return abis_bts_rsl_sendmsg(msg);
}

/* 8.4.3 sending CHANnel ACTIVation Negative ACK */
static int _rsl_tx_chan_act_nack(struct gsm_bts_trx *trx, uint8_t chan_nr, uint8_t cause,
				 struct gsm_lchan *lchan)
{
	struct msgb *msg;

	if (lchan)
		LOGP(DRSL, LOGL_NOTICE, "%s: ", gsm_lchan_name(lchan));
	else
		LOGP(DRSL, LOGL_NOTICE, "0x%02x: ", chan_nr);
	LOGPC(DRSL, LOGL_NOTICE, "Sending Channel Activated NACK: cause = 0x%02x\n", cause);

	msg = rsl_msgb_alloc(sizeof(struct abis_rsl_dchan_hdr));
	if (!msg)
		return -ENOMEM;

	/* 9.3.26 Cause */
	msgb_tlv_put(msg, RSL_IE_CAUSE, 1, &cause);
	rsl_dch_push_hdr(msg, RSL_MT_CHAN_ACTIV_NACK, chan_nr);
	msg->trx = trx;

	return abis_bts_rsl_sendmsg(msg);
}
static int rsl_tx_chan_act_nack(struct gsm_lchan *lchan, uint8_t cause) {
	return _rsl_tx_chan_act_nack(lchan->ts->trx, gsm_lchan2chan_nr(lchan), cause, lchan);
}

/* Send an RSL Channel Activation Ack if cause is zero, a Nack otherwise. */
int rsl_tx_chan_act_acknack(struct gsm_lchan *lchan, uint8_t cause)
{
	if (lchan->rel_act_kind != LCHAN_REL_ACT_RSL) {
		LOGP(DRSL, LOGL_NOTICE, "%s not sending CHAN ACT %s\n",
			gsm_lchan_name(lchan), cause ? "NACK" : "ACK");
		return 0;
	}

	if (cause)
		return rsl_tx_chan_act_nack(lchan, cause);
	return rsl_tx_chan_act_ack(lchan);
}

/* 8.4.4 sending CONNection FAILure */
int rsl_tx_conn_fail(struct gsm_lchan *lchan, uint8_t cause)
{
	struct msgb *msg;
	uint8_t chan_nr = gsm_lchan2chan_nr(lchan);

	LOGP(DRSL, LOGL_NOTICE,
		"%s Sending Connection Failure: cause = 0x%02x\n",
		gsm_lchan_name(lchan), cause);

	msg = rsl_msgb_alloc(sizeof(struct abis_rsl_dchan_hdr));
	if (!msg)
		return -ENOMEM;

	/* 9.3.26 Cause */
	msgb_tlv_put(msg, RSL_IE_CAUSE, 1, &cause);
	rsl_dch_push_hdr(msg, RSL_MT_CONN_FAIL, chan_nr);
	msg->trx = lchan->ts->trx;

	return abis_bts_rsl_sendmsg(msg);
}

/* 8.5.3 sending CHANnel ReQuireD */
int rsl_tx_chan_rqd(struct gsm_bts_trx *trx, struct gsm_time *gtime,
		    uint8_t ra, uint8_t acc_delay)
{
	struct msgb *nmsg;
	uint8_t payload[3];

	LOGP(DRSL, LOGL_NOTICE, "Sending Channel Required\n");

	nmsg = rsl_msgb_alloc(sizeof(struct abis_rsl_cchan_hdr));
	if (!nmsg)
		return -ENOMEM;

	/* 9.3.19 Request Reference */
	payload[0] = ra;
	gsm48_gen_starting_time(payload+1, gtime);
	msgb_tv_fixed_put(nmsg, RSL_IE_REQ_REFERENCE, 3, payload);

	/* 9.3.17 Access Delay */
	msgb_tv_put(nmsg, RSL_IE_ACCESS_DELAY, acc_delay);

	rsl_cch_push_hdr(nmsg, RSL_MT_CHAN_RQD, 0x88); // FIXME
	nmsg->trx = trx;

	return abis_bts_rsl_sendmsg(nmsg);
}

/* copy the SACCH related sysinfo from BTS global buffer to lchan specific buffer */
static void copy_sacch_si_to_lchan(struct gsm_lchan *lchan)
{
	struct gsm_bts *bts = lchan->ts->trx->bts;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(rsl_sacch_sitypes); i++) {
		uint8_t rsl_si = rsl_sacch_sitypes[i];
		int osmo_si = osmo_rsl2sitype(rsl_si);
		uint32_t osmo_si_shifted = (1 << osmo_si);
		osmo_static_assert(_MAX_SYSINFO_TYPE <= sizeof(osmo_si_shifted) * 8,
				   si_enum_vals_fit_in_bit_mask);

		if (osmo_si == SYSINFO_TYPE_NONE)
			continue;
		if (!(bts->si_valid & osmo_si_shifted)) {
			lchan->si.valid &= ~osmo_si_shifted;
			continue;
		}
		lchan->si.valid |= osmo_si_shifted;
		memcpy(GSM_LCHAN_SI(lchan, osmo_si), GSM_BTS_SI(bts, osmo_si), sizeof(sysinfo_buf_t));
	}
}


static int encr_info2lchan(struct gsm_lchan *lchan,
			   const uint8_t *val, uint8_t len)
{
	int rc;
	struct gsm_bts *bts = lchan->ts->trx->bts;

	/* check if the encryption algorithm sent by BSC is supported! */
	rc = bts_supports_cipher(bts, *val);
	if (rc != 1) {
		LOGP(DRSL, LOGL_ERROR, "%s: BTS doesn't support cipher 0x%02x\n",
			gsm_lchan_name(lchan), *val);
		return -EINVAL;
	}

	/* length can be '1' in case of no ciphering */
	if (len < 1) {
		LOGP(DRSL, LOGL_ERROR, "%s: Encryption Info cannot have len=%d\n",
			gsm_lchan_name(lchan), len);
		return -EINVAL;
	}

	lchan->encr.alg_id = *val++;
	lchan->encr.key_len = len -1;
	if (lchan->encr.key_len > sizeof(lchan->encr.key))
		lchan->encr.key_len = sizeof(lchan->encr.key);
	memcpy(lchan->encr.key, val, lchan->encr.key_len);
	DEBUGP(DRSL, "%s: Setting lchan cipher algorithm 0x%02x\n",
		gsm_lchan_name(lchan), lchan->encr.alg_id);

	return 0;
}

/* Make sure no state from TCH use remains. */
static void clear_lchan_for_pdch_activ(struct gsm_lchan *lchan)
{
	/* These values don't apply to PDCH, just clear them. Particularly the encryption must be
	 * cleared, or we would enable encryption on PDCH with parameters remaining from the TCH. */
	lchan->ms_power = ms_pwr_ctl_lvl(lchan->ts->trx->bts->band, 0);
	lchan->ms_power_ctrl.current = lchan->ms_power;
	lchan->ms_power_ctrl.fixed = 0;
	lchan->rsl_cmode = 0;
	lchan->tch_mode = 0;
	memset(&lchan->encr, 0, sizeof(lchan->encr));
	memset(&lchan->ho, 0, sizeof(lchan->ho));
	lchan->bs_power = 0;
	lchan->ms_power = 0;
	memset(&lchan->ms_power_ctrl, 0, sizeof(lchan->ms_power_ctrl));
	lchan->rqd_ta = 0;
	copy_sacch_si_to_lchan(lchan);
	memset(&lchan->tch, 0, sizeof(lchan->tch));
}

/*!
 * Store the CHAN_ACTIV msg, connect the L1 timeslot in the proper type and
 * then invoke rsl_rx_chan_activ() with msg.
 */
static int dyn_ts_l1_reconnect(struct gsm_bts_trx_ts *ts, struct msgb *msg)
{
	DEBUGP(DRSL, "%s dyn_ts_l1_reconnect\n", gsm_ts_and_pchan_name(ts));

	switch (ts->dyn.pchan_want) {
	case GSM_PCHAN_TCH_F:
	case GSM_PCHAN_TCH_H:
		break;
	case GSM_PCHAN_PDCH:
		/* Only the first lchan matters for PDCH */
		clear_lchan_for_pdch_activ(ts->lchan);
		break;
	default:
		LOGP(DRSL, LOGL_ERROR,
		     "%s Cannot reconnect as pchan %s\n",
		     gsm_ts_and_pchan_name(ts),
		     gsm_pchan_name(ts->dyn.pchan_want));
		return -EINVAL;
	}

	/* We will feed this back to rsl_rx_chan_activ() later */
	ts->dyn.pending_chan_activ = msg;

	/* Disconnect, continue connecting from cb_ts_disconnected(). */
	DEBUGP(DRSL, "%s Disconnect\n", gsm_ts_and_pchan_name(ts));
	return bts_model_ts_disconnect(ts);
}

static enum gsm_phys_chan_config dyn_pchan_from_chan_nr(uint8_t chan_nr)
{
	uint8_t cbits = chan_nr & RSL_CHAN_NR_MASK;
	switch (cbits) {
	case RSL_CHAN_Bm_ACCHs:
		return GSM_PCHAN_TCH_F;
	case RSL_CHAN_Lm_ACCHs:
	case (RSL_CHAN_Lm_ACCHs + RSL_CHAN_NR_1):
		return GSM_PCHAN_TCH_H;
	case RSL_CHAN_OSMO_PDCH:
		return GSM_PCHAN_PDCH;
	default:
		LOGP(DRSL, LOGL_ERROR,
		     "chan nr 0x%x not covered by dyn_pchan_from_chan_nr()\n",
		     chan_nr);
		return GSM_PCHAN_UNKNOWN;
	}
}

/* 8.4.1 CHANnel ACTIVation is received */
static int rsl_rx_chan_activ(struct msgb *msg)
{
	struct abis_rsl_dchan_hdr *dch = msgb_l2(msg);
	struct gsm_lchan *lchan = msg->lchan;
	struct gsm_bts_trx_ts *ts = lchan->ts;
	struct rsl_ie_chan_mode *cm;
	struct tlv_parsed tp;
	uint8_t type;
	int rc;

	if (lchan->state != LCHAN_S_NONE) {
		LOGP(DRSL, LOGL_ERROR,
		     "%s: error: lchan is not available, but in state: %s.\n",
		     gsm_lchan_name(lchan), gsm_lchans_name(lchan->state));
		return rsl_tx_chan_act_nack(lchan, RSL_ERR_EQUIPMENT_FAIL);
	}

	if (ts->pchan == GSM_PCHAN_TCH_F_TCH_H_PDCH) {
		ts->dyn.pchan_want = dyn_pchan_from_chan_nr(dch->chan_nr);
		DEBUGP(DRSL, "%s rx chan activ\n", gsm_ts_and_pchan_name(ts));

		if (ts->dyn.pchan_is != ts->dyn.pchan_want) {
			/*
			 * The phy has the timeslot connected in a different
			 * mode than this activation needs it to be.
			 * Re-connect, then come back to rsl_rx_chan_activ().
			 */
			rc = dyn_ts_l1_reconnect(ts, msg);
			if (rc)
				return rsl_tx_chan_act_nack(lchan, RSL_ERR_NORMAL_UNSPEC);
			/* indicate that the msgb should not be freed. */
			return 1;
		}
	}

	LOGP(DRSL, LOGL_DEBUG, "%s: rx Channel Activation in state: %s.\n",
	     gsm_lchan_name(lchan), gsm_lchans_name(lchan->state));

	/* Initialize channel defaults */
	lchan->ms_power = ms_pwr_ctl_lvl(lchan->ts->trx->bts->band, 0);
	lchan->ms_power_ctrl.current = lchan->ms_power;
	lchan->ms_power_ctrl.fixed = 0;

	rsl_tlv_parse(&tp, msgb_l3(msg), msgb_l3len(msg));

	/* 9.3.3 Activation Type */
	if (!TLVP_PRESENT(&tp, RSL_IE_ACT_TYPE)) {
		LOGP(DRSL, LOGL_NOTICE, "missing Activation Type\n");
		return rsl_tx_chan_act_nack(lchan, RSL_ERR_MAND_IE_ERROR);
	}
	type = *TLVP_VAL(&tp, RSL_IE_ACT_TYPE);

	/* 9.3.6 Channel Mode */
	if (type != RSL_ACT_OSMO_PDCH) {
		if (!TLVP_PRESENT(&tp, RSL_IE_CHAN_MODE)) {
			LOGP(DRSL, LOGL_NOTICE, "missing Channel Mode\n");
			return rsl_tx_chan_act_nack(lchan, RSL_ERR_MAND_IE_ERROR);
		}
		cm = (struct rsl_ie_chan_mode *) TLVP_VAL(&tp, RSL_IE_CHAN_MODE);
		lchan_tchmode_from_cmode(lchan, cm);
	}

	/* 9.3.7 Encryption Information */
	if (TLVP_PRESENT(&tp, RSL_IE_ENCR_INFO)) {
		uint8_t len = TLVP_LEN(&tp, RSL_IE_ENCR_INFO);
		const uint8_t *val = TLVP_VAL(&tp, RSL_IE_ENCR_INFO);

		if (encr_info2lchan(lchan, val, len) < 0) {
			rsl_tx_error_report(msg->trx, RSL_ERR_IE_CONTENT, &dch->chan_nr, NULL, msg);
			return rsl_tx_chan_act_acknack(lchan, RSL_ERR_ENCR_UNIMPL);
		}
	} else
		memset(&lchan->encr, 0, sizeof(lchan->encr));

	/* 9.3.9 Handover Reference */
	if ((type == RSL_ACT_INTER_ASYNC ||
	     type == RSL_ACT_INTER_SYNC) &&
	    TLVP_PRES_LEN(&tp, RSL_IE_HANDO_REF, 1)) {
		lchan->ho.active = HANDOVER_ENABLED;
		lchan->ho.ref = *TLVP_VAL(&tp, RSL_IE_HANDO_REF);
	}

	/* 9.3.4 BS Power */
	if (TLVP_PRES_LEN(&tp, RSL_IE_BS_POWER, 1))
		lchan->bs_power = *TLVP_VAL(&tp, RSL_IE_BS_POWER);
	/* 9.3.13 MS Power */
	if (TLVP_PRES_LEN(&tp, RSL_IE_MS_POWER, 1)) {
		lchan->ms_power = *TLVP_VAL(&tp, RSL_IE_MS_POWER);
		lchan->ms_power_ctrl.current = lchan->ms_power;
		lchan->ms_power_ctrl.fixed = 0;
	}
	/* 9.3.24 Timing Advance */
	if (TLVP_PRES_LEN(&tp, RSL_IE_TIMING_ADVANCE, 1))
		lchan->rqd_ta = *TLVP_VAL(&tp, RSL_IE_TIMING_ADVANCE);

	/* 9.3.32 BS Power Parameters */
	/* 9.3.31 MS Power Parameters */
	/* 9.3.16 Physical Context */

	/* 9.3.29 SACCH Information */
	if (TLVP_PRESENT(&tp, RSL_IE_SACCH_INFO)) {
		uint8_t tot_len = TLVP_LEN(&tp, RSL_IE_SACCH_INFO);
		const uint8_t *val = TLVP_VAL(&tp, RSL_IE_SACCH_INFO);
		const uint8_t *cur = val;
		uint8_t num_msgs = *cur++;
		unsigned int i;
		for (i = 0; i < num_msgs; i++) {
			uint8_t rsl_si = *cur++;
			uint8_t si_len = *cur++;
			uint8_t osmo_si;

			if (!OSMO_IN_ARRAY(rsl_si, rsl_sacch_sitypes)) {
				rsl_tx_error_report(msg->trx, RSL_ERR_IE_CONTENT,
						    &dch->chan_nr, NULL, msg);
				return rsl_tx_chan_act_acknack(lchan, RSL_ERR_IE_CONTENT);
			}

			osmo_si = osmo_rsl2sitype(rsl_si);
			if (osmo_si == SYSINFO_TYPE_NONE) {
				LOGP(DRSL, LOGL_NOTICE, " Rx SACCH SI 0x%02x not supported.\n", rsl_si);
				rsl_tx_error_report(msg->trx, RSL_ERR_IE_CONTENT, &dch->chan_nr,
						    NULL, msg);
				return rsl_tx_chan_act_acknack(lchan, RSL_ERR_IE_CONTENT);
			}

			lapdm_ui_prefix_lchan(lchan, cur, osmo_si, si_len);

			cur += si_len;
			if (cur >= val + tot_len) {
				LOGP(DRSL, LOGL_ERROR, "Error parsing SACCH INFO IE\n");
				rsl_tx_error_report(msg->trx, RSL_ERR_IE_CONTENT, &dch->chan_nr,
						    NULL, msg);
				return rsl_tx_chan_act_acknack(lchan, RSL_ERR_IE_CONTENT);
			}
		}
	} else {
		/* use standard SACCH filling of the BTS */
		copy_sacch_si_to_lchan(lchan);
	}
	/* 9.3.52 MultiRate Configuration */
	if (TLVP_PRESENT(&tp, RSL_IE_MR_CONFIG)) {
		if (TLVP_LEN(&tp, RSL_IE_MR_CONFIG) > sizeof(lchan->mr_bts_lv) - 1) {
			LOGP(DRSL, LOGL_ERROR, "Error parsing MultiRate conf IE\n");
			rsl_tx_error_report(msg->trx, RSL_ERR_IE_CONTENT, &dch->chan_nr, NULL, msg);
			return rsl_tx_chan_act_acknack(lchan, RSL_ERR_IE_CONTENT);
		}
		memcpy(lchan->mr_bts_lv, TLVP_VAL(&tp, RSL_IE_MR_CONFIG) - 1,
		       TLVP_LEN(&tp, RSL_IE_MR_CONFIG) + 1);
		amr_parse_mr_conf(&lchan->tch.amr_mr, TLVP_VAL(&tp, RSL_IE_MR_CONFIG),
				  TLVP_LEN(&tp, RSL_IE_MR_CONFIG));
		amr_log_mr_conf(DRTP, LOGL_DEBUG, gsm_lchan_name(lchan),
				&lchan->tch.amr_mr);
		lchan->tch.last_cmr = AMR_CMR_NONE;
	}
	/* 9.3.53 MultiRate Control */
	/* 9.3.54 Supported Codec Types */

	LOGP(DRSL, LOGL_INFO, " chan_nr=0x%02x type=0x%02x mode=0x%02x\n",
		dch->chan_nr, type, lchan->tch_mode);

	/* Connecting PDCH on dyn TS goes via PCU instead. */
	if (ts->pchan == GSM_PCHAN_TCH_F_TCH_H_PDCH
	    && ts->dyn.pchan_want == GSM_PCHAN_PDCH) {
		/*
		 * We ack the activation to the BSC right away, regardless of
		 * the PCU succeeding or not; if a dynamic timeslot fails to go
		 * to PDCH mode for any reason, the BSC should still be able to
		 * switch it back to TCH modes and should not put the time slot
		 * in an error state. So for operating dynamic TS, the BSC
		 * would not take any action if the PDCH mode failed, e.g.
		 * because the PCU is not yet running. Even if alerting the
		 * core network of broken GPRS service is desired, this only
		 * makes sense when the PCU has not shown up for some time.
		 * It's easiest to not forward activation delays to the BSC: if
		 * the BSC tells us to do PDCH, we do our best, and keep the
		 * details on the BTS and PCU level. This is kind of analogous
		 * to how plain PDCH TS operate. Directly call
		 * rsl_tx_chan_act_ack() instead of rsl_tx_chan_act_acknack()
		 * because we don't want/need to decide whether to drop due to
		 * lchan->rel_act_kind.
		 */
		rc = rsl_tx_chan_act_ack(lchan);
		if (rc < 0)
			LOGP(DRSL, LOGL_ERROR, "%s Cannot send act ack: %d\n",
			     gsm_ts_and_pchan_name(ts), rc);

		/*
		 * pcu_tx_info_ind() will pick up the ts->dyn.pchan_want. If
		 * the PCU is not connected yet, ignore for now; the PCU will
		 * catch up (and send the RSL ack) once it connects.
		 */
		if (pcu_connected()) {
			DEBUGP(DRSL, "%s Activate via PCU\n", gsm_ts_and_pchan_name(ts));
			rc = pcu_tx_info_ind();
		}
		else {
			DEBUGP(DRSL, "%s Activate via PCU when PCU connects\n",
			       gsm_ts_and_pchan_name(ts));
			rc = 0;
		}
		if (rc) {
			rsl_tx_error_report(msg->trx, RSL_ERR_NORMAL_UNSPEC, &dch->chan_nr, NULL, msg);
			return rsl_tx_chan_act_acknack(lchan, RSL_ERR_NORMAL_UNSPEC);
		}
		return 0;
	}

	/* Remember to send an RSL ACK once the lchan is active */
	lchan->rel_act_kind = LCHAN_REL_ACT_RSL;

	/* actually activate the channel in the BTS */
	rc = l1sap_chan_act(lchan->ts->trx, dch->chan_nr, &tp);
	if (rc < 0)
		return rsl_tx_chan_act_acknack(lchan, -rc);

	return 0;
}

static int dyn_ts_pdch_release(struct gsm_lchan *lchan)
{
	struct gsm_bts_trx_ts *ts = lchan->ts;

	if (ts->dyn.pchan_is != ts->dyn.pchan_want) {
		LOGP(DRSL, LOGL_ERROR, "%s: PDCH release requested but already"
		     " in switchover\n", gsm_ts_and_pchan_name(ts));
		return -EINVAL;
	}

	/*
	 * Indicate PDCH Disconnect in dyn_pdch.want, let pcu_tx_info_ind()
	 * pick it up and wait for PCU to disable the channel.
	 */
	ts->dyn.pchan_want = GSM_PCHAN_NONE;

	if (!pcu_connected()) {
		/* PCU not connected yet. Just record the new type and done,
		 * the PCU will pick it up once connected. */
		ts->dyn.pchan_is = GSM_PCHAN_NONE;
		return 1;
	}

	return pcu_tx_info_ind();
}

/* 8.4.14 RF CHANnel RELease is received */
static int rsl_rx_rf_chan_rel(struct gsm_lchan *lchan, uint8_t chan_nr)
{
	int rc;

	if (lchan->state == LCHAN_S_NONE) {
		LOGP(DRSL, LOGL_ERROR,
		     "%s ss=%d state=%s Rx RSL RF Channel Release, but is already inactive;"
		     " just ACKing the release\n",
		     gsm_ts_and_pchan_name(lchan->ts), lchan->nr,
		     gsm_lchans_name(lchan->state));
		/* Just ack the release and ignore. Make sure to reflect the same chan_nr we received,
		 * not necessarily reflecting the current lchan state. */
		return tx_rf_rel_ack(lchan, chan_nr);
	}

	if (lchan->abis_ip.rtp_socket) {
		rsl_tx_ipac_dlcx_ind(lchan, RSL_ERR_NORMAL_UNSPEC);
		osmo_rtp_socket_log_stats(lchan->abis_ip.rtp_socket, DRTP, LOGL_INFO,
			"Closing RTP socket on Channel Release ");
		osmo_rtp_socket_free(lchan->abis_ip.rtp_socket);
		lchan->abis_ip.rtp_socket = NULL;
		msgb_queue_flush(&lchan->dl_tch_queue);
	}

	/* release handover state */
	handover_reset(lchan);

	lchan->rel_act_kind = LCHAN_REL_ACT_RSL;

	/* Dynamic channel in PDCH mode is released via PCU */
	if (lchan->ts->pchan == GSM_PCHAN_TCH_F_TCH_H_PDCH
	    && lchan->ts->dyn.pchan_is == GSM_PCHAN_PDCH) {
		rc = dyn_ts_pdch_release(lchan);
		if (rc == 1) {
			/* If the PCU is not connected, continue to rel ack right away. */
			lchan->rel_act_kind = LCHAN_REL_ACT_PCU;
			return rsl_tx_rf_rel_ack(lchan);
		}
		/* Waiting for PDCH release */
		return rc;
	}

	l1sap_chan_rel(lchan->ts->trx, chan_nr);

	lapdm_channel_exit(&lchan->lapdm_ch);

	return 0;
}

#ifdef FAKE_CIPH_MODE_COMPL
/* ugly hack to send a fake CIPH MODE COMPLETE back to the BSC */
#include <osmocom/gsm/protocol/gsm_04_08.h>
#include <osmocom/gsm/gsm48.h>
static int tx_ciph_mod_compl_hack(struct gsm_lchan *lchan, uint8_t link_id,
				  const char *imeisv)
{
	struct msgb *fake_msg;
	struct gsm48_hdr *g48h;
	uint8_t mid_buf[11];
	int rc;

	fake_msg = rsl_msgb_alloc(128);
	if (!fake_msg)
		return -ENOMEM;

	/* generate 04.08 RR message */
	g48h = (struct gsm48_hdr *) msgb_put(fake_msg, sizeof(*g48h));
	g48h->proto_discr = GSM48_PDISC_RR;
	g48h->msg_type = GSM48_MT_RR_CIPH_M_COMPL;

	/* add IMEISV, if requested */
	if (imeisv) {
		rc = gsm48_generate_mid_from_imsi(mid_buf, imeisv);
		if (rc > 0) {
			mid_buf[2] = (mid_buf[2] & 0xf8) | GSM_MI_TYPE_IMEISV;
			memcpy(msgb_put(fake_msg, rc), mid_buf, rc);
		}
	}

	rsl_rll_push_l3(fake_msg, RSL_MT_DATA_IND, gsm_lchan2chan_nr(lchan),
			link_id, 1);

	fake_msg->lchan = lchan;
	fake_msg->trx = lchan->ts->trx;

	/* send it back to the BTS */
	return abis_bts_rsl_sendmsg(fake_msg);
}

struct ciph_mod_compl {
	struct osmo_timer_list timer;
	struct gsm_lchan *lchan;
	int send_imeisv;
	uint8_t link_id;
};

static void cmc_timer_cb(void *data)
{
	struct ciph_mod_compl *cmc = data;
	const char *imeisv = NULL;

	LOGP(DRSL, LOGL_NOTICE,
	     "%s Sending FAKE CIPHERING MODE COMPLETE to BSC (Alg %u)\n",
	     gsm_lchan_name(cmc->lchan), cmc->lchan->encr.alg_id);

	if (cmc->send_imeisv)
		imeisv = "0123456789012345";

	/* We have no clue whatsoever that this lchan still exists! */
	tx_ciph_mod_compl_hack(cmc->lchan, cmc->link_id, imeisv);

	talloc_free(cmc);
}
#endif


/* 8.4.6 ENCRYPTION COMMAND */
static int rsl_rx_encr_cmd(struct msgb *msg)
{
	struct gsm_lchan *lchan = msg->lchan;
	struct abis_rsl_dchan_hdr *dch = msgb_l2(msg);
	struct tlv_parsed tp;
	uint8_t link_id;

	if (rsl_tlv_parse(&tp, msgb_l3(msg), msgb_l3len(msg)) < 0) {
		return rsl_tx_error_report(msg->trx, RSL_ERR_IE_CONTENT, &dch->chan_nr, NULL, msg);
	}

	if (!TLVP_PRESENT(&tp, RSL_IE_ENCR_INFO) ||
	    !TLVP_PRESENT(&tp, RSL_IE_L3_INFO) ||
	    !TLVP_PRESENT(&tp, RSL_IE_LINK_IDENT)) {
		return rsl_tx_error_report(msg->trx, RSL_ERR_MAND_IE_ERROR, &dch->chan_nr, NULL, msg);
	}

	/* 9.3.7 Encryption Information */
	if (TLVP_PRESENT(&tp, RSL_IE_ENCR_INFO)) {
		uint8_t len = TLVP_LEN(&tp, RSL_IE_ENCR_INFO);
		const uint8_t *val = TLVP_VAL(&tp, RSL_IE_ENCR_INFO);

		if (encr_info2lchan(lchan, val, len) < 0) {
			 return rsl_tx_error_report(msg->trx, RSL_ERR_IE_CONTENT, &dch->chan_nr,
					 	    NULL, msg);
		}
	}

	/* 9.3.2 Link Identifier */
	link_id = *TLVP_VAL(&tp, RSL_IE_LINK_IDENT);

	/* we have to set msg->l3h as rsl_rll_push_l3 will use it to
	 * determine the length field of the L3_INFO IE */
	msg->l3h = (uint8_t *) TLVP_VAL(&tp, RSL_IE_L3_INFO);

	/* pop the RSL dchan header, but keep L3 TLV */
	msgb_pull(msg, msg->l3h - msg->data);

	/* push a fake RLL DATA REQ header */
	rsl_rll_push_l3(msg, RSL_MT_DATA_REQ, dch->chan_nr, link_id, 1);


#ifdef FAKE_CIPH_MODE_COMPL
	if (lchan->encr.alg_id != RSL_ENC_ALG_A5(0)) {
		struct ciph_mod_compl *cmc;
		struct gsm48_hdr *g48h = (struct gsm48_hdr *) msg->l3h;

		cmc = talloc_zero(NULL, struct ciph_mod_compl);
		if (g48h->data[0] & 0x10)
			cmc->send_imeisv = 1;
		cmc->lchan = lchan;
		cmc->link_id = link_id;
		cmc->timer.cb = cmc_timer_cb;
		cmc->timer.data = cmc;
		osmo_timer_schedule(&cmc->timer, 1, 0);

		/* FIXME: send fake CM SERVICE ACCEPT to MS */

		return 0;
	} else
#endif
	{
	LOGP(DRSL, LOGL_INFO, "%s Fwd RSL ENCR CMD (Alg %u) to LAPDm\n",
		gsm_lchan_name(lchan), lchan->encr.alg_id);
	/* hand it into RSLms for transmission of L3_INFO to the MS */
	lapdm_rslms_recvmsg(msg, &lchan->lapdm_ch);
	/* return 1 to make sure the msgb is not free'd */
	return 1;
	}
}

/* 8.4.11 MODE MODIFY NEGATIVE ACKNOWLEDGE */
static int _rsl_tx_mode_modif_nack(struct gsm_bts_trx *trx, uint8_t chan_nr, uint8_t cause,
				  struct gsm_lchan *lchan)
{
	struct msgb *msg;

	if (lchan)
		LOGP(DRSL, LOGL_NOTICE, "%s: ", gsm_lchan_name(lchan));
	else
		LOGP(DRSL, LOGL_NOTICE, "0x%02x: ", chan_nr);
	LOGPC(DRSL, LOGL_NOTICE, "Tx MODE MODIFY NACK (cause = 0x%02x)\n", cause);

	msg = rsl_msgb_alloc(sizeof(struct abis_rsl_dchan_hdr));
	if (!msg)
		return -ENOMEM;

	msg->len = 0;
	msg->data = msg->tail = msg->l3h;

	/* 9.3.26 Cause */
	msgb_tlv_put(msg, RSL_IE_CAUSE, 1, &cause);
	rsl_dch_push_hdr(msg, RSL_MT_MODE_MODIFY_NACK, chan_nr);
	msg->trx = trx;

	return abis_bts_rsl_sendmsg(msg);
}
static int rsl_tx_mode_modif_nack(struct gsm_lchan *lchan, uint8_t cause)
{
	return _rsl_tx_mode_modif_nack(lchan->ts->trx, gsm_lchan2chan_nr(lchan), cause, lchan);
}


/* 8.4.10 MODE MODIFY ACK */
static int rsl_tx_mode_modif_ack(struct gsm_lchan *lchan)
{
	struct msgb *msg;
	uint8_t chan_nr = gsm_lchan2chan_nr(lchan);

	LOGP(DRSL, LOGL_INFO, "%s Tx MODE MODIF ACK\n", gsm_lchan_name(lchan));

	msg = rsl_msgb_alloc(sizeof(struct abis_rsl_dchan_hdr));
	if (!msg)
		return -ENOMEM;

	rsl_dch_push_hdr(msg, RSL_MT_MODE_MODIFY_ACK, chan_nr);
	msg->trx = lchan->ts->trx;

	return abis_bts_rsl_sendmsg(msg);
}

/* 8.4.9 MODE MODIFY */
static int rsl_rx_mode_modif(struct msgb *msg)
{
	struct abis_rsl_dchan_hdr *dch = msgb_l2(msg);
	struct gsm_lchan *lchan = msg->lchan;
	struct rsl_ie_chan_mode *cm;
	struct tlv_parsed tp;

	rsl_tlv_parse(&tp, msgb_l3(msg), msgb_l3len(msg));

	/* 9.3.6 Channel Mode */
	if (!TLVP_PRESENT(&tp, RSL_IE_CHAN_MODE)) {
		LOGP(DRSL, LOGL_NOTICE, "missing Channel Mode\n");
		return rsl_tx_mode_modif_nack(lchan, RSL_ERR_MAND_IE_ERROR);
	}
	cm = (struct rsl_ie_chan_mode *) TLVP_VAL(&tp, RSL_IE_CHAN_MODE);
	lchan_tchmode_from_cmode(lchan, cm);

	if (bts_supports_cm(lchan->ts->trx->bts, lchan->ts->pchan, lchan->tch_mode) != 1) {
		LOGP(DRSL, LOGL_ERROR, "invalid mode/codec instructed by BSC, check BSC configuration.\n");
		return rsl_tx_mode_modif_nack(lchan, RSL_ERR_SERV_OPT_UNAVAIL);
	}

	/* 9.3.7 Encryption Information */
	if (TLVP_PRESENT(&tp, RSL_IE_ENCR_INFO)) {
		uint8_t len = TLVP_LEN(&tp, RSL_IE_ENCR_INFO);
		const uint8_t *val = TLVP_VAL(&tp, RSL_IE_ENCR_INFO);

		if (encr_info2lchan(lchan, val, len) < 0) {
			rsl_tx_error_report(msg->trx, RSL_ERR_IE_CONTENT, &dch->chan_nr, NULL, msg);
			return rsl_tx_mode_modif_nack(lchan, RSL_ERR_ENCR_UNIMPL);
		}
	}

	/* 9.3.45 Main channel reference */

	/* 9.3.52 MultiRate Configuration */
	if (TLVP_PRESENT(&tp, RSL_IE_MR_CONFIG)) {
		if (TLVP_LEN(&tp, RSL_IE_MR_CONFIG) > sizeof(lchan->mr_bts_lv) - 1) {
			LOGP(DRSL, LOGL_ERROR, "Error parsing MultiRate conf IE\n");
			rsl_tx_error_report(msg->trx, RSL_ERR_IE_CONTENT, &dch->chan_nr, NULL, msg);
			return rsl_tx_mode_modif_nack(lchan, RSL_ERR_IE_CONTENT);;
		}
		memcpy(lchan->mr_bts_lv, TLVP_VAL(&tp, RSL_IE_MR_CONFIG) - 1,
			TLVP_LEN(&tp, RSL_IE_MR_CONFIG) + 1);
		amr_parse_mr_conf(&lchan->tch.amr_mr, TLVP_VAL(&tp, RSL_IE_MR_CONFIG),
				  TLVP_LEN(&tp, RSL_IE_MR_CONFIG));
		amr_log_mr_conf(DRTP, LOGL_DEBUG, gsm_lchan_name(lchan),
				&lchan->tch.amr_mr);
		lchan->tch.last_cmr = AMR_CMR_NONE;
	}
	/* 9.3.53 MultiRate Control */
	/* 9.3.54 Supported Codec Types */

	l1sap_chan_modify(lchan->ts->trx, dch->chan_nr);

	/* FIXME: delay this until L1 says OK? */
	rsl_tx_mode_modif_ack(lchan);

	return 0;
}

/* 8.4.15 MS POWER CONTROL */
static int rsl_rx_ms_pwr_ctrl(struct msgb *msg)
{
	struct gsm_lchan *lchan = msg->lchan;
	struct tlv_parsed tp;

	rsl_tlv_parse(&tp, msgb_l3(msg), msgb_l3len(msg));
	if (TLVP_PRES_LEN(&tp, RSL_IE_MS_POWER, 1)) {
		uint8_t pwr = *TLVP_VAL(&tp, RSL_IE_MS_POWER) & 0x1F;
		lchan->ms_power_ctrl.fixed = 1;
		lchan->ms_power_ctrl.current = pwr;

		LOGP(DRSL, LOGL_NOTICE, "%s forcing power to %d\n",
			gsm_lchan_name(lchan), lchan->ms_power_ctrl.current);
		bts_model_adjst_ms_pwr(lchan);
	}

	return 0;
}

/* 8.4.20 SACCH INFO MODify */
static int rsl_rx_sacch_inf_mod(struct msgb *msg)
{
	struct abis_rsl_dchan_hdr *dch = msgb_l2(msg);
	struct gsm_lchan *lchan = msg->lchan;
	struct tlv_parsed tp;
	uint8_t rsl_si, osmo_si;

	rsl_tlv_parse(&tp, msgb_l3(msg), msgb_l3len(msg));

	if (TLVP_PRESENT(&tp, RSL_IE_STARTNG_TIME)) {
		LOGP(DRSL, LOGL_NOTICE, "Starting time not supported\n");
		return rsl_tx_error_report(msg->trx, RSL_ERR_SERV_OPT_UNIMPL, &dch->chan_nr, NULL, msg);
	}

	/* 9.3.30 System Info Type */
	if (!TLVP_PRESENT(&tp, RSL_IE_SYSINFO_TYPE))
		return rsl_tx_error_report(msg->trx, RSL_ERR_MAND_IE_ERROR, &dch->chan_nr, NULL, msg);

	rsl_si = *TLVP_VAL(&tp, RSL_IE_SYSINFO_TYPE);
	if (!OSMO_IN_ARRAY(rsl_si, rsl_sacch_sitypes))
		return rsl_tx_error_report(msg->trx, RSL_ERR_IE_CONTENT, &dch->chan_nr, NULL, msg);

	osmo_si = osmo_rsl2sitype(rsl_si);
	if (osmo_si == SYSINFO_TYPE_NONE) {
		LOGP(DRSL, LOGL_NOTICE, "%s Rx SACCH SI 0x%02x not supported.\n",
			gsm_lchan_name(lchan), rsl_si);
		return rsl_tx_error_report(msg->trx, RSL_ERR_IE_CONTENT, &dch->chan_nr, NULL, msg);
	}
	if (TLVP_PRESENT(&tp, RSL_IE_L3_INFO)) {
		uint16_t len = TLVP_LEN(&tp, RSL_IE_L3_INFO);
		lapdm_ui_prefix_lchan(lchan, TLVP_VAL(&tp, RSL_IE_L3_INFO), osmo_si, len);

		LOGP(DRSL, LOGL_INFO, "%s Rx RSL SACCH FILLING (SI%s)\n",
			gsm_lchan_name(lchan),
			get_value_string(osmo_sitype_strs, osmo_si));
	} else {
		lchan->si.valid &= ~(1 << osmo_si);
		LOGP(DRSL, LOGL_INFO, "%s Rx RSL Disabling SACCH FILLING (SI%s)\n",
			gsm_lchan_name(lchan),
			get_value_string(osmo_sitype_strs, osmo_si));
	}

	return 0;
}

/*
 * ip.access related messages
 */
static void rsl_add_rtp_stats(struct gsm_lchan *lchan, struct msgb *msg)
{
	struct ipa_stats {
		uint32_t packets_sent;
		uint32_t octets_sent;
		uint32_t packets_recv;
		uint32_t octets_recv;
		uint32_t packets_lost;
		uint32_t arrival_jitter;
		uint32_t avg_tx_delay;
	} __attribute__((packed));

	struct ipa_stats stats;

	memset(&stats, 0, sizeof(stats));

	if (lchan->abis_ip.rtp_socket)
		osmo_rtp_socket_stats(lchan->abis_ip.rtp_socket,
					&stats.packets_sent, &stats.octets_sent,
					&stats.packets_recv, &stats.octets_recv,
					&stats.packets_lost, &stats.arrival_jitter);
	/* convert to network byte order */
	stats.packets_sent = htonl(stats.packets_sent);
	stats.octets_sent = htonl(stats.octets_sent);
	stats.packets_recv = htonl(stats.packets_recv);
	stats.octets_recv = htonl(stats.octets_recv);
	stats.packets_lost = htonl(stats.packets_lost);

	msgb_tlv_put(msg, RSL_IE_IPAC_CONN_STAT, sizeof(stats), (uint8_t *) &stats);
}

int rsl_tx_ipac_dlcx_ind(struct gsm_lchan *lchan, uint8_t cause)
{
	struct msgb *nmsg;

	LOGP(DRSL, LOGL_NOTICE, "%s Sending RTP delete indication: cause = %s\n",
	     gsm_lchan_name(lchan), rsl_err_name(cause));

	nmsg = rsl_msgb_alloc(sizeof(struct abis_rsl_dchan_hdr));
	if (!nmsg)
		return -ENOMEM;

	msgb_tv16_put(nmsg, RSL_IE_IPAC_CONN_ID, htons(lchan->abis_ip.conn_id));
	rsl_add_rtp_stats(lchan, nmsg);
	msgb_tlv_put(nmsg, RSL_IE_CAUSE, 1, &cause);
	rsl_ipa_push_hdr(nmsg, RSL_MT_IPAC_DLCX_IND, gsm_lchan2chan_nr(lchan));

	nmsg->trx = lchan->ts->trx;

	return abis_bts_rsl_sendmsg(nmsg);
}

/* transmit an CRCX ACK for the lchan */
static int rsl_tx_ipac_XXcx_ack(struct gsm_lchan *lchan, int inc_pt2,
				  uint8_t orig_msgt)
{
	struct msgb *msg;
	uint8_t chan_nr = gsm_lchan2chan_nr(lchan);
	const char *name;
	struct in_addr ia;

	if (orig_msgt == RSL_MT_IPAC_CRCX)
		name = "CRCX";
	else
		name = "MDCX";

	ia.s_addr = htonl(lchan->abis_ip.bound_ip);
	LOGP(DRSL, LOGL_INFO, "%s RSL Tx IPAC_%s_ACK (local %s:%u, ",
	     gsm_lchan_name(lchan), name,
	     inet_ntoa(ia), lchan->abis_ip.bound_port);
	ia.s_addr = htonl(lchan->abis_ip.connect_ip);
	LOGPC(DRSL, LOGL_INFO, "remote %s:%u)\n",
		inet_ntoa(ia), lchan->abis_ip.connect_port);

	msg = rsl_msgb_alloc(sizeof(struct abis_rsl_dchan_hdr));
	if (!msg)
		return -ENOMEM;


	/* Connection ID */
	msgb_tv16_put(msg, RSL_IE_IPAC_CONN_ID, htons(lchan->abis_ip.conn_id));

	/* locally bound IP */
	msgb_v_put(msg, RSL_IE_IPAC_LOCAL_IP);
	msgb_put_u32(msg, lchan->abis_ip.bound_ip);

	/* locally bound port */
	msgb_tv16_put(msg, RSL_IE_IPAC_LOCAL_PORT,
		      lchan->abis_ip.bound_port);

	if (inc_pt2) {
		/* RTP Payload Type 2 */
		msgb_tv_put(msg, RSL_IE_IPAC_RTP_PAYLOAD2,
					lchan->abis_ip.rtp_payload2);
	}

	/* push the header in front */
	rsl_ipa_push_hdr(msg, orig_msgt + 1, chan_nr);
	msg->trx = lchan->ts->trx;

	return abis_bts_rsl_sendmsg(msg);
}

static int rsl_tx_ipac_dlcx_ack(struct gsm_lchan *lchan, int inc_conn_id)
{
	struct msgb *msg;
	uint8_t chan_nr = gsm_lchan2chan_nr(lchan);

	LOGP(DRSL, LOGL_INFO, "%s RSL Tx IPAC_DLCX_ACK\n",
		gsm_lchan_name(lchan));

	msg = rsl_msgb_alloc(sizeof(struct abis_rsl_dchan_hdr));
	if (!msg)
		return -ENOMEM;

	if (inc_conn_id) {
		msgb_tv16_put(msg, RSL_IE_IPAC_CONN_ID, lchan->abis_ip.conn_id);
		rsl_add_rtp_stats(lchan, msg);
	}

	rsl_ipa_push_hdr(msg, RSL_MT_IPAC_DLCX_ACK, chan_nr);
	msg->trx = lchan->ts->trx;

	return abis_bts_rsl_sendmsg(msg);
}

static int rsl_tx_ipac_dlcx_nack(struct gsm_lchan *lchan, int inc_conn_id,
				 uint8_t cause)
{
	struct msgb *msg;
	uint8_t chan_nr = gsm_lchan2chan_nr(lchan);

	LOGP(DRSL, LOGL_INFO, "%s RSL Tx IPAC_DLCX_NACK\n",
		gsm_lchan_name(lchan));

	msg = rsl_msgb_alloc(sizeof(struct abis_rsl_dchan_hdr));
	if (!msg)
		return -ENOMEM;

	if (inc_conn_id)
		msgb_tv_put(msg, RSL_IE_IPAC_CONN_ID, lchan->abis_ip.conn_id);

	msgb_tlv_put(msg, RSL_IE_CAUSE, 1, &cause);

	rsl_ipa_push_hdr(msg, RSL_MT_IPAC_DLCX_NACK, chan_nr);
	msg->trx = lchan->ts->trx;

	return abis_bts_rsl_sendmsg(msg);

}


/* transmit an CRCX NACK for the lchan */
static int tx_ipac_XXcx_nack(struct gsm_lchan *lchan, uint8_t cause,
			     int inc_ipport, uint8_t orig_msgtype)
{
	struct msgb *msg;
	uint8_t chan_nr = gsm_lchan2chan_nr(lchan);

	/* FIXME: allocate new msgb and copy old over */
	LOGP(DRSL, LOGL_NOTICE, "%s RSL Tx IPAC_BIND_NACK\n",
		gsm_lchan_name(lchan));

	msg = rsl_msgb_alloc(sizeof(struct abis_rsl_dchan_hdr));
	if (!msg)
		return -ENOMEM;

	if (inc_ipport) {
		/* remote IP */
		msgb_v_put(msg, RSL_IE_IPAC_REMOTE_IP);
		msgb_put_u32(msg, lchan->abis_ip.connect_ip);

		/* remote port */
		msgb_tv16_put(msg, RSL_IE_IPAC_REMOTE_PORT,
				htons(lchan->abis_ip.connect_port));
	}

	/* 9.3.26 Cause */
	msgb_tlv_put(msg, RSL_IE_CAUSE, 1, &cause);

	/* push the header in front */
	rsl_ipa_push_hdr(msg, orig_msgtype + 2, chan_nr);
	msg->trx = lchan->ts->trx;

	return abis_bts_rsl_sendmsg(msg);
}

static char *get_rsl_local_ip(struct gsm_bts_trx *trx)
{
	struct e1inp_ts *ts = trx->rsl_link->ts;
	struct sockaddr_storage ss;
	socklen_t sa_len = sizeof(ss);
	static char hostbuf[256];
	int rc;

	rc = getsockname(ts->driver.ipaccess.fd.fd, (struct sockaddr *) &ss,
			 &sa_len);
	if (rc < 0)
		return NULL;

	rc = getnameinfo((struct sockaddr *)&ss, sa_len,
			 hostbuf, sizeof(hostbuf), NULL, 0,
			 NI_NUMERICHOST);
	if (rc < 0)
		return NULL;

	return hostbuf;
}

static int rsl_rx_ipac_XXcx(struct msgb *msg)
{
	struct abis_rsl_dchan_hdr *dch = msgb_l2(msg);
	struct tlv_parsed tp;
	struct gsm_lchan *lchan = msg->lchan;
	struct gsm_bts *bts = lchan->ts->trx->bts;
	const uint8_t *payload_type, *speech_mode, *payload_type2;
	uint32_t connect_ip = 0;
	uint16_t connect_port = 0;
	int rc, inc_ip_port = 0, port;
	char *name;
	struct in_addr ia;
	struct in_addr addr;

	if (dch->c.msg_type == RSL_MT_IPAC_CRCX)
		name = "CRCX";
	else
		name = "MDCX";

	/* check the kind of channel and reject */
	if (lchan->type != GSM_LCHAN_TCH_F && lchan->type != GSM_LCHAN_TCH_H)
		return tx_ipac_XXcx_nack(lchan, 0x52,
					 0, dch->c.msg_type);

	rc = rsl_tlv_parse(&tp, msgb_l3(msg), msgb_l3len(msg));
	if (rc < 0)
		return tx_ipac_XXcx_nack(lchan, RSL_ERR_MAND_IE_ERROR,
					 0, dch->c.msg_type);

	LOGP(DRSL, LOGL_DEBUG, "%s IPAC_%s: ", gsm_lchan_name(lchan), name);
	if (TLVP_PRES_LEN(&tp, RSL_IE_IPAC_REMOTE_IP, 4)) {
		connect_ip = tlvp_val32_unal(&tp, RSL_IE_IPAC_REMOTE_IP);
		addr.s_addr = connect_ip;
		LOGPC(DRSL, LOGL_DEBUG, "connect_ip=%s ", inet_ntoa(addr));
	}

	if (TLVP_PRES_LEN(&tp, RSL_IE_IPAC_REMOTE_PORT, 2)) {
		connect_port = tlvp_val16_unal(&tp, RSL_IE_IPAC_REMOTE_PORT);
		LOGPC(DRSL, LOGL_DEBUG, "connect_port=%u ",
		      ntohs(connect_port));
	}

	speech_mode = TLVP_VAL(&tp, RSL_IE_IPAC_SPEECH_MODE);
	if (speech_mode)
		LOGPC(DRSL, LOGL_DEBUG, "speech_mode=%u ", *speech_mode);

	payload_type = TLVP_VAL(&tp, RSL_IE_IPAC_RTP_PAYLOAD);
	if (payload_type)
		LOGPC(DRSL, LOGL_DEBUG, "payload_type=%u ", *payload_type);

	LOGPC(DRSL, LOGL_DEBUG, "\n");

	payload_type2 = TLVP_VAL(&tp, RSL_IE_IPAC_RTP_PAYLOAD2);

	if (dch->c.msg_type == RSL_MT_IPAC_CRCX && connect_ip && connect_port)
		inc_ip_port = 1;

	if (payload_type && payload_type2) {
		LOGP(DRSL, LOGL_ERROR, "%s Rx RSL IPAC %s, "
			"RTP_PT and RTP_PT2 in same msg !?!\n",
			gsm_lchan_name(lchan), name);
		return tx_ipac_XXcx_nack(lchan, RSL_ERR_MAND_IE_ERROR,
					 inc_ip_port, dch->c.msg_type);
	}

	if (dch->c.msg_type == RSL_MT_IPAC_CRCX) {
		char cname[32];
		char *ipstr = NULL;
		if (lchan->abis_ip.rtp_socket) {
			LOGP(DRSL, LOGL_ERROR, "%s Rx RSL IPAC CRCX, "
				"but we already have socket!\n",
				gsm_lchan_name(lchan));
			return tx_ipac_XXcx_nack(lchan, RSL_ERR_RES_UNAVAIL,
						 inc_ip_port, dch->c.msg_type);
		}
		/* FIXME: select default value depending on speech_mode */
		//if (!payload_type)
		lchan->tch.last_fn = LCHAN_FN_DUMMY;
		lchan->abis_ip.rtp_socket = osmo_rtp_socket_create(lchan->ts->trx,
								OSMO_RTP_F_POLL);
		if (!lchan->abis_ip.rtp_socket) {
			LOGP(DRTP, LOGL_ERROR,
			     "%s IPAC Failed to create RTP/RTCP sockets\n",
			     gsm_lchan_name(lchan));
			oml_fail_rep(OSMO_EVT_CRIT_RTP_TOUT,
				     "%s IPAC Failed to create RTP/RTCP sockets",
				     gsm_lchan_name(lchan));
			return tx_ipac_XXcx_nack(lchan, RSL_ERR_RES_UNAVAIL,
						 inc_ip_port, dch->c.msg_type);
		}
		rc = osmo_rtp_socket_set_param(lchan->abis_ip.rtp_socket,
					       bts->rtp_jitter_adaptive ?
					       OSMO_RTP_P_JIT_ADAP :
					       OSMO_RTP_P_JITBUF,
					       bts->rtp_jitter_buf_ms);
		if (rc < 0)
			LOGP(DRTP, LOGL_ERROR,
			     "%s IPAC Failed to set RTP socket parameters: %s\n",
			     gsm_lchan_name(lchan), strerror(-rc));
		else
			LOGP(DRTP, LOGL_INFO,
			     "%s IPAC set RTP socket parameters: %d\n",
			     gsm_lchan_name(lchan), rc);
		lchan->abis_ip.rtp_socket->priv = lchan;
		lchan->abis_ip.rtp_socket->rx_cb = &l1sap_rtp_rx_cb;

		if (connect_ip && connect_port) {
			/* if CRCX specifies a remote IP, we can bind()
			 * here to 0.0.0.0 and wait for the connect()
			 * below, after which the kernel will have
			 * selected the local IP address.  */
			ipstr = "0.0.0.0";
		} else {
			/* if CRCX does not specify a remote IP, we will
			 * not do any connect() below, and thus the
			 * local socket will remain bound to 0.0.0.0 -
			 * which however we cannot legitimately report
			 * back to the BSC in the CRCX_ACK */
			ipstr = get_rsl_local_ip(lchan->ts->trx);
		}
		rc = osmo_rtp_socket_bind(lchan->abis_ip.rtp_socket,
					  ipstr, -1);
		if (rc < 0) {
			LOGP(DRTP, LOGL_ERROR,
			     "%s IPAC Failed to bind RTP/RTCP sockets\n",
			     gsm_lchan_name(lchan));
			oml_fail_rep(OSMO_EVT_CRIT_RTP_TOUT,
				     "%s IPAC Failed to bind RTP/RTCP sockets",
				     gsm_lchan_name(lchan));
			osmo_rtp_socket_free(lchan->abis_ip.rtp_socket);
			lchan->abis_ip.rtp_socket = NULL;
			msgb_queue_flush(&lchan->dl_tch_queue);
			return tx_ipac_XXcx_nack(lchan, RSL_ERR_RES_UNAVAIL,
						 inc_ip_port, dch->c.msg_type);
		}
		/* Ensure RTCP SDES contains some useful information */
		snprintf(cname, sizeof(cname), "bts@%s", ipstr);
		osmo_rtp_set_source_desc(lchan->abis_ip.rtp_socket, cname,
					 gsm_lchan_name(lchan), NULL, NULL,
					 gsm_trx_unit_id(lchan->ts->trx),
					 "OsmoBTS-" PACKAGE_VERSION, NULL);
		/* FIXME: multiplex connection, BSC proxy */
	} else {
		/* MDCX */
		if (!lchan->abis_ip.rtp_socket) {
			LOGP(DRSL, LOGL_ERROR, "%s Rx RSL IPAC MDCX, "
				"but we have no RTP socket!\n",
				gsm_lchan_name(lchan));
			return tx_ipac_XXcx_nack(lchan, RSL_ERR_RES_UNAVAIL,
						 inc_ip_port, dch->c.msg_type);
		}
	}


	/* Special rule: If connect_ip == 0.0.0.0, use RSL IP
	 * address */
	if (connect_ip == 0) {
		struct e1inp_sign_link *sign_link =
					lchan->ts->trx->rsl_link;

		ia.s_addr = htonl(get_signlink_remote_ip(sign_link));
	} else
		ia.s_addr = connect_ip;
	rc = osmo_rtp_socket_connect(lchan->abis_ip.rtp_socket,
				     inet_ntoa(ia), ntohs(connect_port));
	if (rc < 0) {
		LOGP(DRTP, LOGL_ERROR,
		     "%s Failed to connect RTP/RTCP sockets\n",
		     gsm_lchan_name(lchan));
		osmo_rtp_socket_free(lchan->abis_ip.rtp_socket);
		lchan->abis_ip.rtp_socket = NULL;
		msgb_queue_flush(&lchan->dl_tch_queue);
		return tx_ipac_XXcx_nack(lchan, RSL_ERR_RES_UNAVAIL,
					 inc_ip_port, dch->c.msg_type);
	}
	/* save IP address and port number */
	lchan->abis_ip.connect_ip = ntohl(ia.s_addr);
	lchan->abis_ip.connect_port = ntohs(connect_port);

	rc = osmo_rtp_get_bound_ip_port(lchan->abis_ip.rtp_socket,
					&lchan->abis_ip.bound_ip,
					&port);
	if (rc < 0)
		LOGP(DRTP, LOGL_ERROR, "%s IPAC cannot obtain "
		     "locally bound IP/port: %d\n",
		     gsm_lchan_name(lchan), rc);
	lchan->abis_ip.bound_port = port;

	/* Everything has succeeded, we can store new values in lchan */
	if (payload_type) {
		lchan->abis_ip.rtp_payload = *payload_type;
		if (lchan->abis_ip.rtp_socket)
			osmo_rtp_socket_set_pt(lchan->abis_ip.rtp_socket,
						*payload_type);
	}
	if (payload_type2) {
		lchan->abis_ip.rtp_payload2 = *payload_type2;
		if (lchan->abis_ip.rtp_socket)
			osmo_rtp_socket_set_pt(lchan->abis_ip.rtp_socket,
						*payload_type2);
	}
	if (speech_mode)
		lchan->abis_ip.speech_mode = *speech_mode;

	/* FIXME: CSD, jitterbuffer, compression */

	return rsl_tx_ipac_XXcx_ack(lchan, payload_type2 ? 1 : 0,
				    dch->c.msg_type);
}

static int rsl_rx_ipac_dlcx(struct msgb *msg)
{
	struct tlv_parsed tp;
	struct gsm_lchan *lchan = msg->lchan;
	int rc, inc_conn_id = 0;

	rc = rsl_tlv_parse(&tp, msgb_l3(msg), msgb_l3len(msg));
	if (rc < 0)
		return rsl_tx_ipac_dlcx_nack(lchan, 0, RSL_ERR_MAND_IE_ERROR);

	if (TLVP_PRESENT(&tp, RSL_IE_IPAC_CONN_ID))
		inc_conn_id = 1;

	rc = rsl_tx_ipac_dlcx_ack(lchan, inc_conn_id);
	if (lchan->abis_ip.rtp_socket) {
		osmo_rtp_socket_log_stats(lchan->abis_ip.rtp_socket, DRTP, LOGL_INFO,
					  "Closing RTP socket on DLCX ");
		osmo_rtp_socket_free(lchan->abis_ip.rtp_socket);
		lchan->abis_ip.rtp_socket = NULL;
		msgb_queue_flush(&lchan->dl_tch_queue);
	}
	return rc;
}

/*
 * dynamic TCH/F_PDCH related messages, originally ip.access specific but
 * reused for other BTS models (sysmo-bts, ...)
 */

/* PDCH ACT/DEACT ACKNOWLEDGE */
static int rsl_tx_dyn_pdch_ack(struct gsm_lchan *lchan, bool pdch_act)
{
	struct gsm_time *gtime = get_time(lchan->ts->trx->bts);
	uint8_t chan_nr = gsm_lchan2chan_nr(lchan);
	struct msgb *msg;
	uint8_t ie[2];

	LOGP(DRSL, LOGL_NOTICE, "%s Tx PDCH %s ACK\n",
	     gsm_lchan_name(lchan), pdch_act? "ACT" : "DEACT");

	msg = rsl_msgb_alloc(sizeof(struct abis_rsl_dchan_hdr));
	if (!msg)
		return -ENOMEM;

	if (pdch_act) {
		gsm48_gen_starting_time(ie, gtime);
		msgb_tv_fixed_put(msg, RSL_IE_FRAME_NUMBER, 2, ie);
	}
	rsl_dch_push_hdr(msg,
			 pdch_act? RSL_MT_IPAC_PDCH_ACT_ACK
				 : RSL_MT_IPAC_PDCH_DEACT_ACK,
			 chan_nr);
	msg->lchan = lchan;
	msg->trx = lchan->ts->trx;

	return abis_bts_rsl_sendmsg(msg);
}

/* PDCH ACT/DEACT NEGATIVE ACKNOWLEDGE */
static int rsl_tx_dyn_pdch_nack(struct gsm_lchan *lchan, bool pdch_act,
				uint8_t cause)
{
	struct msgb *msg;
	uint8_t chan_nr = gsm_lchan2chan_nr(lchan);

	LOGP(DRSL, LOGL_NOTICE, "%s Tx PDCH %s NACK (cause = 0x%02x)\n",
	     gsm_lchan_name(lchan), pdch_act? "ACT" : "DEACT", cause);

	msg = rsl_msgb_alloc(sizeof(struct abis_rsl_dchan_hdr));
	if (!msg)
		return -ENOMEM;

	msg->len = 0;
	msg->data = msg->tail = msg->l3h;

	/* 9.3.26 Cause */
	msgb_tlv_put(msg, RSL_IE_CAUSE, 1, &cause);
	rsl_dch_push_hdr(msg,
			 pdch_act? RSL_MT_IPAC_PDCH_ACT_NACK
				 : RSL_MT_IPAC_PDCH_DEACT_NACK,
			 chan_nr);
	msg->lchan = lchan;
	msg->trx = lchan->ts->trx;

	return abis_bts_rsl_sendmsg(msg);
}

/*
 * Starting point for dynamic PDCH switching. See osmo-gsm-manuals.git for a
 * diagram of what will happen here. The implementation is as follows:
 *
 * PDCH ACT == TCH/F -> PDCH:
 * 1. call bts_model_ts_disconnect() to disconnect TCH/F;
 * 2. cb_ts_disconnected() is called when done;
 * 3. call bts_model_ts_connect() to connect as PDTCH;
 * 4. cb_ts_connected() is called when done;
 * 5. instruct the PCU to enable PDTCH;
 * 6. the PCU will call back with an activation request;
 * 7. l1sap_info_act_cnf() will call ipacc_dyn_pdch_complete() when SAPI
 *    activations are done;
 * 8. send a PDCH ACT ACK.
 *
 * PDCH DEACT == PDCH -> TCH/F:
 * 1. instruct the PCU to disable PDTCH;
 * 2. the PCU will call back with a deactivation request;
 * 3. l1sap_info_rel_cnf() will call bts_model_ts_disconnect() when SAPI
 *    deactivations are done;
 * 4. cb_ts_disconnected() is called when done;
 * 5. call bts_model_ts_connect() to connect as TCH/F;
 * 6. cb_ts_connected() is called when done;
 * 7. directly call ipacc_dyn_pdch_complete(), since no further action required
 *    for TCH/F;
 * 8. send a PDCH DEACT ACK.
 *
 * When an error happens along the way, a PDCH DE/ACT NACK is sent.
 * TODO: may need to be made more waterproof in all stages, to send a NACK and
 * clear the PDCH pending flags from ts->flags.
 */
static void rsl_rx_dyn_pdch(struct msgb *msg, bool pdch_act)
{
	int rc;
	struct gsm_lchan *lchan = msg->lchan;
	struct gsm_bts_trx_ts *ts = lchan->ts;
	bool is_pdch_act = (ts->flags & TS_F_PDCH_ACTIVE);

	if (ts->flags & TS_F_PDCH_PENDING_MASK) {
		/* Only one of the pending flags should ever be set at the same
		 * time, but just log both in case both should be set. */
		LOGP(DRSL, LOGL_ERROR,
		     "%s Request to PDCH %s, but PDCH%s%s is still pending\n",
		     gsm_lchan_name(lchan), pdch_act? "ACT" : "DEACT",
		     (ts->flags & TS_F_PDCH_ACT_PENDING)? " ACT" : "",
		     (ts->flags & TS_F_PDCH_DEACT_PENDING)? " DEACT" : "");
		rsl_tx_dyn_pdch_nack(lchan, pdch_act, RSL_ERR_NORMAL_UNSPEC);
		return;
	}

	if (lchan->state != LCHAN_S_NONE) {
		LOGP(DRSL, LOGL_ERROR,
		     "%s Request to PDCH %s, but lchan is still active\n",
		     gsm_ts_and_pchan_name(ts), pdch_act? "ACT" : "DEACT");
		rsl_tx_dyn_pdch_nack(lchan, pdch_act, RSL_ERR_NORMAL_UNSPEC);
	}

	ts->flags |= pdch_act? TS_F_PDCH_ACT_PENDING
			     : TS_F_PDCH_DEACT_PENDING;

	/* ensure that this is indeed a dynamic-PDCH channel */
	if (ts->pchan != GSM_PCHAN_TCH_F_PDCH) {
		LOGP(DRSL, LOGL_ERROR,
		     "%s Attempt to PDCH %s on TS that is not a TCH/F_PDCH (is %s)\n",
		     gsm_lchan_name(lchan), pdch_act? "ACT" : "DEACT",
		     gsm_pchan_name(ts->pchan));
		ipacc_dyn_pdch_complete(ts, -EINVAL);
		return;
	}

	if (is_pdch_act == pdch_act) {
		LOGP(DL1C, LOGL_NOTICE,
		     "%s Request to PDCH %s, but is already so\n",
		     gsm_lchan_name(lchan), pdch_act? "ACT" : "DEACT");
		ipacc_dyn_pdch_complete(ts, 0);
		return;
	}

	if (pdch_act) {
		/* Clear TCH state. Only first lchan matters for PDCH */
		clear_lchan_for_pdch_activ(ts->lchan);

		/* First, disconnect the TCH channel, to connect PDTCH later */
		rc = bts_model_ts_disconnect(ts);
	} else {
		/* First, deactivate PDTCH through the PCU, to connect TCH
		 * later.
		 * pcu_tx_info_ind() will pick up TS_F_PDCH_DEACT_PENDING and
		 * trigger a deactivation.
		 * Except when the PCU is not connected yet, then trigger
		 * disconnect immediately from here. The PCU will catch up when
		 * it connects. */
		/* TODO: timeout on channel connect / disconnect request from PCU? */
		if (pcu_connected())
			rc = pcu_tx_info_ind();
		else
			rc = bts_model_ts_disconnect(ts);
	}

	/* Error? then NACK right now. */
	if (rc)
		ipacc_dyn_pdch_complete(ts, rc);
}

static void ipacc_dyn_pdch_ts_disconnected(struct gsm_bts_trx_ts *ts)
{
	int rc;
	enum gsm_phys_chan_config as_pchan;

	if (ts->flags & TS_F_PDCH_DEACT_PENDING) {
		LOGP(DRSL, LOGL_DEBUG,
		     "%s PDCH DEACT operation: channel disconnected, will reconnect as TCH\n",
		     gsm_lchan_name(ts->lchan));
		as_pchan = GSM_PCHAN_TCH_F;
	} else if (ts->flags & TS_F_PDCH_ACT_PENDING) {
		LOGP(DRSL, LOGL_DEBUG,
		     "%s PDCH ACT operation: channel disconnected, will reconnect as PDTCH\n",
		     gsm_lchan_name(ts->lchan));
		as_pchan = GSM_PCHAN_PDCH;
	} else
		/* No reconnect pending. */
		return;

	rc = conf_lchans_as_pchan(ts, as_pchan);
	if (rc)
		goto error_nack;

	rc = bts_model_ts_connect(ts, as_pchan);

error_nack:
	/* Error? then NACK right now. */
	if (rc)
		ipacc_dyn_pdch_complete(ts, rc);
}

static void osmo_dyn_ts_disconnected(struct gsm_bts_trx_ts *ts)
{
	DEBUGP(DRSL, "%s Disconnected\n", gsm_ts_and_pchan_name(ts));
	ts->dyn.pchan_is = GSM_PCHAN_NONE;

	switch (ts->dyn.pchan_want) {
	case GSM_PCHAN_TCH_F:
	case GSM_PCHAN_TCH_H:
	case GSM_PCHAN_PDCH:
		break;
	default:
		LOGP(DRSL, LOGL_ERROR,
		     "%s Dyn TS disconnected, but invalid desired pchan: %s\n",
		     gsm_ts_and_pchan_name(ts), gsm_pchan_name(ts->dyn.pchan_want));
		ts->dyn.pchan_want = GSM_PCHAN_NONE;
		/* TODO: how would this recover? */
		return;
	}

	conf_lchans_as_pchan(ts, ts->dyn.pchan_want);
	DEBUGP(DRSL, "%s Connect\n", gsm_ts_and_pchan_name(ts));
	bts_model_ts_connect(ts, ts->dyn.pchan_want);
}

void cb_ts_disconnected(struct gsm_bts_trx_ts *ts)
{
	OSMO_ASSERT(ts);

	switch (ts->pchan) {
	case GSM_PCHAN_TCH_F_PDCH:
		return ipacc_dyn_pdch_ts_disconnected(ts);
	case GSM_PCHAN_TCH_F_TCH_H_PDCH:
		return osmo_dyn_ts_disconnected(ts);
	default:
		return;
	}
}

static void ipacc_dyn_pdch_ts_connected(struct gsm_bts_trx_ts *ts)
{
	int rc;

	if (ts->flags & TS_F_PDCH_DEACT_PENDING) {
		if (ts->lchan[0].type != GSM_LCHAN_TCH_F)
			LOGP(DRSL, LOGL_ERROR, "%s PDCH DEACT error:"
			     " timeslot connected, so expecting"
			     " lchan type TCH/F, but is %s\n",
			     gsm_lchan_name(ts->lchan),
			     gsm_lchant_name(ts->lchan[0].type));

		LOGP(DRSL, LOGL_DEBUG, "%s PDCH DEACT operation:"
		     " timeslot connected as TCH/F\n",
		     gsm_lchan_name(ts->lchan));

		/* During PDCH DEACT, we're done right after the TCH/F came
		 * back up. */
		ipacc_dyn_pdch_complete(ts, 0);

	} else if (ts->flags & TS_F_PDCH_ACT_PENDING) {
		if (ts->lchan[0].type != GSM_LCHAN_PDTCH)
			LOGP(DRSL, LOGL_ERROR, "%s PDCH ACT error:"
			     " timeslot connected, so expecting"
			     " lchan type PDTCH, but is %s\n",
			     gsm_lchan_name(ts->lchan),
			     gsm_lchant_name(ts->lchan[0].type));

		LOGP(DRSL, LOGL_DEBUG, "%s PDCH ACT operation:"
		     " timeslot connected as PDTCH\n",
		     gsm_lchan_name(ts->lchan));

		/* The PDTCH is connected, now tell the PCU about it. Except
		 * when the PCU is not connected (yet), then there's nothing
		 * left to do now. The PCU will catch up when it connects. */
		if (!pcu_connected()) {
			ipacc_dyn_pdch_complete(ts, 0);
			return;
		}

		/* The PCU will request to activate the PDTCH SAPIs, which,
		 * when done, will call back to ipacc_dyn_pdch_complete(). */
		/* TODO: timeout on channel connect / disconnect request from PCU? */
		rc = pcu_tx_info_ind();

		/* Error? then NACK right now. */
		if (rc)
			ipacc_dyn_pdch_complete(ts, rc);
	}
}

static void osmo_dyn_ts_connected(struct gsm_bts_trx_ts *ts)
{
	int rc;
	struct msgb *msg = ts->dyn.pending_chan_activ;
	ts->dyn.pending_chan_activ = NULL;

	if (!msg) {
		LOGP(DRSL, LOGL_ERROR,
		     "%s TS re-connected, but no chan activ msg pending\n",
		     gsm_ts_and_pchan_name(ts));
		return;
	}

	ts->dyn.pchan_is = ts->dyn.pchan_want;
	DEBUGP(DRSL, "%s Connected\n", gsm_ts_and_pchan_name(ts));

	/* continue where we left off before re-connecting the TS. */
	rc = rsl_rx_chan_activ(msg);
	if (rc != 1)
		msgb_free(msg);
}

void cb_ts_connected(struct gsm_bts_trx_ts *ts)
{
	OSMO_ASSERT(ts);

	switch (ts->pchan) {
	case GSM_PCHAN_TCH_F_PDCH:
		return ipacc_dyn_pdch_ts_connected(ts);
	case GSM_PCHAN_TCH_F_TCH_H_PDCH:
		return osmo_dyn_ts_connected(ts);
	default:
		return;
	}
}

void ipacc_dyn_pdch_complete(struct gsm_bts_trx_ts *ts, int rc)
{
	bool pdch_act;
	OSMO_ASSERT(ts);

	pdch_act = ts->flags & TS_F_PDCH_ACT_PENDING;

	if ((ts->flags & TS_F_PDCH_PENDING_MASK) == TS_F_PDCH_PENDING_MASK)
		LOGP(DRSL, LOGL_ERROR,
		     "%s Internal Error: both PDCH ACT and PDCH DEACT pending\n",
		     gsm_lchan_name(ts->lchan));

	ts->flags &= ~TS_F_PDCH_PENDING_MASK;

	if (rc != 0) {
		LOGP(DRSL, LOGL_ERROR,
		     "PDCH %s on dynamic TCH/F_PDCH returned error %d\n",
		     pdch_act? "ACT" : "DEACT", rc);
		rsl_tx_dyn_pdch_nack(ts->lchan, pdch_act, RSL_ERR_NORMAL_UNSPEC);
		return;
	}

	if (pdch_act)
		ts->flags |= TS_F_PDCH_ACTIVE;
	else
		ts->flags &= ~TS_F_PDCH_ACTIVE;
	DEBUGP(DRSL, "%s %s switched to %s mode (ts->flags == %x)\n",
	       gsm_lchan_name(ts->lchan), gsm_pchan_name(ts->pchan),
	       pdch_act? "PDCH" : "TCH/F", ts->flags);

	rc = rsl_tx_dyn_pdch_ack(ts->lchan, pdch_act);
	if (rc)
		LOGP(DRSL, LOGL_ERROR,
		     "Failed to transmit PDCH %s ACK, rc %d\n",
		     pdch_act? "ACT" : "DEACT", rc);
}

/* handle a message with an RSL CHAN_NR that is incompatible/unknown */
static int rsl_reject_unknown_lchan(struct msgb *msg)
{
	struct abis_rsl_common_hdr *rh = msgb_l2(msg);
	struct abis_rsl_dchan_hdr *dch;
	int rc;

	/* Handle GSM 08.58 7 Error Handling for the given input. This method will
	 * send either a CHANNEL ACTIVATION NACK, MODE MODIFY NACK or ERROR REPORT
	 * depending on the input of the method. */

	/* TS 48.058 Section 7 explains how to do error handling */
	switch (rh->msg_discr & 0xfe) {
	case ABIS_RSL_MDISC_DED_CHAN:
		dch = msgb_l2(msg);
		switch (dch->c.msg_type) {
		case RSL_MT_CHAN_ACTIV:
			rc = _rsl_tx_chan_act_nack(msg->trx, dch->chan_nr,
						   RSL_ERR_MAND_IE_ERROR, NULL);
			break;
		case RSL_MT_MODE_MODIFY_REQ:
			rc = _rsl_tx_mode_modif_nack(msg->trx, dch->chan_nr,
						     RSL_ERR_MAND_IE_ERROR, NULL);
			break;
		default:
			rc = rsl_tx_error_report(msg->trx, RSL_ERR_MAND_IE_ERROR, NULL, NULL, msg);
			break;
		}
		break;
	case ABIS_RSL_MDISC_RLL:
		/* fall-through */
	case ABIS_RSL_MDISC_COM_CHAN:
		/* fall-through */
	case ABIS_RSL_MDISC_TRX:
		/* fall-through */
	case ABIS_RSL_MDISC_IPACCESS:
		/* fall-through */
	default:
		/* ERROR REPORT */
		rc = rsl_tx_error_report(msg->trx, RSL_ERR_MAND_IE_ERROR, NULL, NULL, msg);
	}

	msgb_free(msg);
	return rc;
}

/*
 * selecting message
 */

static int rsl_rx_rll(struct gsm_bts_trx *trx, struct msgb *msg)
{
	struct abis_rsl_rll_hdr *rh = msgb_l2(msg);
	struct gsm_lchan *lchan;

	if (msgb_l2len(msg) < sizeof(*rh)) {
		LOGP(DRSL, LOGL_NOTICE, "RSL Radio Link Layer message too short\n");
		rsl_tx_error_report(trx, RSL_ERR_PROTO, &rh->chan_nr, &rh->link_id, msg);
		msgb_free(msg);
		return -EIO;
	}
	msg->l3h = (unsigned char *)rh + sizeof(*rh);

	if (!chan_nr_is_dchan(rh->chan_nr))
		return rsl_reject_unknown_lchan(msg);

	lchan = lchan_lookup(trx, rh->chan_nr, "RSL rx RLL: ");
	if (!lchan) {
		LOGP(DRLL, LOGL_NOTICE, "Rx RLL %s for unknown lchan\n",
			rsl_msg_name(rh->c.msg_type));
		return rsl_reject_unknown_lchan(msg);
	}

	DEBUGP(DRLL, "%s Rx RLL %s Abis -> LAPDm\n", gsm_lchan_name(lchan),
		rsl_msg_name(rh->c.msg_type));

	/* exception: RLL messages are _NOT_ freed as they are now
	 * owned by LAPDm which might have queued them */
	return lapdm_rslms_recvmsg(msg, &lchan->lapdm_ch);
}

static inline int rsl_link_id_is_sacch(uint8_t link_id)
{
	if (link_id >> 6 == 1)
		return 1;
	else
		return 0;
}

static int rslms_is_meas_rep(struct msgb *msg)
{
	struct abis_rsl_common_hdr *rh = msgb_l2(msg);
	struct abis_rsl_rll_hdr *rllh;
	struct gsm48_hdr *gh;

	if ((rh->msg_discr & 0xfe) != ABIS_RSL_MDISC_RLL)
		return 0;

	if (rh->msg_type != RSL_MT_UNIT_DATA_IND)
		return 0;

	rllh = msgb_l2(msg);
	if (rsl_link_id_is_sacch(rllh->link_id) == 0)
		return 0;

	gh = msgb_l3(msg);
	if (gh->proto_discr != GSM48_PDISC_RR)
		return 0;

	switch (gh->msg_type) {
	case GSM48_MT_RR_MEAS_REP:
	case GSM48_MT_RR_EXT_MEAS_REP:
		return 1;
	default:
		break;
	}

	/* FIXME: this does not cover the Bter frame format and the associated
	 * short RR protocol descriptor for ENHANCED MEASUREMENT REPORT */

	return 0;
}

static inline uint8_t ms_to2rsl(const struct gsm_lchan *lchan, const struct lapdm_entity *le)
{
	return (lchan->ms_t_offs >= 0) ? lchan->ms_t_offs : (lchan->p_offs - le->ta);
}

static inline bool ms_to_valid(const struct gsm_lchan *lchan)
{
	return (lchan->ms_t_offs >= 0) || (lchan->p_offs >= 0);
}

/* 8.4.8 MEASUREMENT RESult */
static int rsl_tx_meas_res(struct gsm_lchan *lchan, uint8_t *l3, int l3_len, const struct lapdm_entity *le)
{
	struct msgb *msg;
	uint8_t meas_res[16];
	uint8_t chan_nr = gsm_lchan2chan_nr(lchan);
	int res_valid = lchan->meas.flags & LC_UL_M_F_RES_VALID;
	struct gsm_bts *bts = lchan->ts->trx->bts;

	LOGP(DRSL, LOGL_DEBUG,
	     "%s chan_num:%u Tx MEAS RES valid(%d), flags(%02x)\n",
	     gsm_lchan_name(lchan), chan_nr, res_valid, lchan->meas.flags);

	if (!res_valid)
		return -EINPROGRESS;

	msg = rsl_msgb_alloc(sizeof(struct abis_rsl_dchan_hdr));
	if (!msg)
		return -ENOMEM;

	LOGP(DRSL, LOGL_DEBUG,
	     "%s Send Meas RES: NUM:%u, RXLEV_FULL:%u, RXLEV_SUB:%u, RXQUAL_FULL:%u, RXQUAL_SUB:%u, MS_PWR:%u, UL_TA:%u, L3_LEN:%d, TimingOff:%u\n",
	     gsm_lchan_name(lchan),
	     lchan->meas.res_nr,
	     lchan->meas.ul_res.full.rx_lev,
	     lchan->meas.ul_res.sub.rx_lev,
	     lchan->meas.ul_res.full.rx_qual,
	     lchan->meas.ul_res.sub.rx_qual,
	     lchan->meas.l1_info[0],
	     lchan->meas.l1_info[1], l3_len, ms_to2rsl(lchan, le) - MEAS_MAX_TIMING_ADVANCE);

	msgb_tv_put(msg, RSL_IE_MEAS_RES_NR, lchan->meas.res_nr++);
	size_t ie_len = gsm0858_rsl_ul_meas_enc(&lchan->meas.ul_res,
						lchan->tch.dtx.dl_active,
						meas_res);
	lchan->tch.dtx.dl_active = false;
	if (ie_len >= 3) {
		if (bts->supp_meas_toa256) {
			/* append signed 16bit value containing MS timing offset in 1/256th symbols
			 * in the vendor-specific "Supplementary Measurement Information" part of
			 * the uplink measurements IE.  This is the current offset *relative* to the
			 * TA which the MS has already applied.  So if you want to know the total
			 * propagation time between MS and BTS, you need to add the actual TA value
			 * used (from L1_INFO below, in full symbols) plus the ms_toa256 value
			 * in 1/256 symbol periods. */
			meas_res[ie_len++] = lchan->meas.ms_toa256 >> 8;
			meas_res[ie_len++] = lchan->meas.ms_toa256 & 0xff;
		}
		msgb_tlv_put(msg, RSL_IE_UPLINK_MEAS, ie_len, meas_res);
		lchan->meas.flags &= ~LC_UL_M_F_RES_VALID;
	}
	msgb_tv_put(msg, RSL_IE_BS_POWER, lchan->meas.bts_tx_pwr);
	if (lchan->meas.flags & LC_UL_M_F_L1_VALID) {
		msgb_tv_fixed_put(msg, RSL_IE_L1_INFO, 2, lchan->meas.l1_info);
		lchan->meas.flags &= ~LC_UL_M_F_L1_VALID;
	}
	msgb_tl16v_put(msg, RSL_IE_L3_INFO, l3_len, l3);
	if (ms_to_valid(lchan)) {
		msgb_tv_put(msg, RSL_IE_MS_TIMING_OFFSET, ms_to2rsl(lchan, le));
		lchan->ms_t_offs = -1;
		lchan->p_offs = -1;
	}

	rsl_dch_push_hdr(msg, RSL_MT_MEAS_RES, chan_nr);
	msg->trx = lchan->ts->trx;

	return abis_bts_rsl_sendmsg(msg);
}

/* call-back for LAPDm code, called when it wants to send msgs UP */
int lapdm_rll_tx_cb(struct msgb *msg, struct lapdm_entity *le, void *ctx)
{
	struct gsm_lchan *lchan = ctx;
	struct abis_rsl_common_hdr *rh;

	OSMO_ASSERT(msg);
	rh = msgb_l2(msg);

	if (lchan->state != LCHAN_S_ACTIVE) {
		LOGP(DRSL, LOGL_INFO, "%s(%s) is not active . Dropping message.\n",
			gsm_lchan_name(lchan), gsm_lchans_name(lchan->state));
		msgb_free(msg);
		return 0;
	}

	msg->trx = lchan->ts->trx;
	msg->lchan = lchan;

	/* check if this is a measurement report from SACCH which needs special
	 * processing before forwarding */
	if (rslms_is_meas_rep(msg)) {
		int rc;

		LOGP(DRSL, LOGL_INFO, "%s Handing RLL msg %s from LAPDm to MEAS REP\n",
			gsm_lchan_name(lchan), rsl_msg_name(rh->msg_type));

		/* REL_IND handling */
		if (rh->msg_type == RSL_MT_REL_IND &&
			(lchan->type == GSM_LCHAN_TCH_F || lchan->type == GSM_LCHAN_TCH_H)) {
			LOGP(DRSL, LOGL_INFO, "%s Scheduling %s to L3 in next associated TCH-RTS.ind\n",
				gsm_lchan_name(lchan),
				rsl_msg_name(rh->msg_type));

			if(lchan->pending_rel_ind_msg) {
				LOGP(DRSL, LOGL_INFO, "Dropping pending release indication message\n");
					msgb_free(lchan->pending_rel_ind_msg);
			}

			lchan->pending_rel_ind_msg = msg;
			return 0;
		}

		rc = rsl_tx_meas_res(lchan, msgb_l3(msg), msgb_l3len(msg), le);
		msgb_free(msg);
		return rc;
	} else {
		LOGP(DRSL, LOGL_INFO, "%s Fwd RLL msg %s from LAPDm to A-bis\n",
			gsm_lchan_name(lchan), rsl_msg_name(rh->msg_type));

		return abis_bts_rsl_sendmsg(msg);
	}
}

static int rsl_rx_cchan(struct gsm_bts_trx *trx, struct msgb *msg)
{
	struct abis_rsl_cchan_hdr *cch = msgb_l2(msg);
	int ret = 0;

	if (msgb_l2len(msg) < sizeof(*cch)) {
		LOGP(DRSL, LOGL_NOTICE, "RSL Common Channel Management message too short\n");
		rsl_tx_error_report(trx, RSL_ERR_PROTO, NULL, NULL, msg);
		msgb_free(msg);
		return -EIO;
	}
	msg->l3h = (unsigned char *)cch + sizeof(*cch);

	if (chan_nr_is_dchan(cch->chan_nr))
		return rsl_reject_unknown_lchan(msg);

	msg->lchan = lchan_lookup(trx, cch->chan_nr, "RSL rx CCHAN: ");
	if (!msg->lchan) {
		LOGP(DRSL, LOGL_ERROR, "Rx RSL %s for unknown lchan\n",
			rsl_msg_name(cch->c.msg_type));
		return rsl_reject_unknown_lchan(msg);
	}

	LOGP(DRSL, LOGL_INFO, "%s Rx RSL %s\n", gsm_lchan_name(msg->lchan),
		rsl_msg_name(cch->c.msg_type));

	switch (cch->c.msg_type) {
	case RSL_MT_BCCH_INFO:
		ret = rsl_rx_bcch_info(trx, msg);
		break;
	case RSL_MT_IMMEDIATE_ASSIGN_CMD:
		ret = rsl_rx_imm_ass(trx, msg);
		break;
	case RSL_MT_PAGING_CMD:
		ret = rsl_rx_paging_cmd(trx, msg);
		break;
	case RSL_MT_SMS_BC_CMD:
		ret = rsl_rx_sms_bcast_cmd(trx, msg);
		break;
	case RSL_MT_SMS_BC_REQ:
	case RSL_MT_NOT_CMD:
		LOGP(DRSL, LOGL_NOTICE, "unimplemented RSL cchan msg_type %s\n",
			rsl_msg_name(cch->c.msg_type));
		break;
	default:
		LOGP(DRSL, LOGL_NOTICE, "undefined RSL cchan msg_type 0x%02x\n",
			cch->c.msg_type);
		ret = -EINVAL;
		break;
	}

	if (ret != 1)
		msgb_free(msg);

	return ret;
}

static int rsl_rx_dchan(struct gsm_bts_trx *trx, struct msgb *msg)
{
	struct abis_rsl_dchan_hdr *dch = msgb_l2(msg);
	int ret = 0;

	if (msgb_l2len(msg) < sizeof(*dch)) {
		LOGP(DRSL, LOGL_NOTICE, "RSL Dedicated Channel Management message too short\n");
		msgb_free(msg);
		return -EIO;
	}
	msg->l3h = (unsigned char *)dch + sizeof(*dch);

	if (!chan_nr_is_dchan(dch->chan_nr))
		return rsl_reject_unknown_lchan(msg);

	msg->lchan = lchan_lookup(trx, dch->chan_nr, "RSL rx DCHAN: ");
	if (!msg->lchan) {
		LOGP(DRSL, LOGL_ERROR, "Rx RSL %s for unknown lchan\n",
			rsl_or_ipac_msg_name(dch->c.msg_type));
		return rsl_reject_unknown_lchan(msg);
	}

	LOGP(DRSL, LOGL_INFO, "%s ss=%d Rx RSL %s\n",
	     gsm_ts_and_pchan_name(msg->lchan->ts), msg->lchan->nr,
	     rsl_or_ipac_msg_name(dch->c.msg_type));

	switch (dch->c.msg_type) {
	case RSL_MT_CHAN_ACTIV:
		ret = rsl_rx_chan_activ(msg);
		break;
	case RSL_MT_RF_CHAN_REL:
		ret = rsl_rx_rf_chan_rel(msg->lchan, dch->chan_nr);
		break;
	case RSL_MT_SACCH_INFO_MODIFY:
		ret = rsl_rx_sacch_inf_mod(msg);
		break;
	case RSL_MT_DEACTIVATE_SACCH:
		ret = l1sap_chan_deact_sacch(trx, dch->chan_nr);
		break;
	case RSL_MT_ENCR_CMD:
		ret = rsl_rx_encr_cmd(msg);
		break;
	case RSL_MT_MODE_MODIFY_REQ:
		ret = rsl_rx_mode_modif(msg);
		break;
	case RSL_MT_MS_POWER_CONTROL:
		ret = rsl_rx_ms_pwr_ctrl(msg);
		break;
	case RSL_MT_IPAC_PDCH_ACT:
	case RSL_MT_IPAC_PDCH_DEACT:
		rsl_rx_dyn_pdch(msg, dch->c.msg_type == RSL_MT_IPAC_PDCH_ACT);
		ret = 0;
		break;
	case RSL_MT_PHY_CONTEXT_REQ:
	case RSL_MT_PREPROC_CONFIG:
	case RSL_MT_RTD_REP:
	case RSL_MT_PRE_HANDO_NOTIF:
	case RSL_MT_MR_CODEC_MOD_REQ:
	case RSL_MT_TFO_MOD_REQ:
		LOGP(DRSL, LOGL_NOTICE, "unimplemented RSL dchan msg_type %s\n",
			rsl_msg_name(dch->c.msg_type));
		break;
	default:
		LOGP(DRSL, LOGL_NOTICE, "undefined RSL dchan msg_type 0x%02x\n",
			dch->c.msg_type);
		ret = -EINVAL;
	}

	if (ret != 1)
		msgb_free(msg);

	return ret;
}

static int rsl_rx_trx(struct gsm_bts_trx *trx, struct msgb *msg)
{
	struct abis_rsl_common_hdr *th = msgb_l2(msg);
	int ret = 0;

	if (msgb_l2len(msg) < sizeof(*th)) {
		LOGP(DRSL, LOGL_NOTICE, "RSL TRX message too short\n");
		rsl_tx_error_report(trx, RSL_ERR_PROTO, NULL, NULL, msg);
		msgb_free(msg);
		return -EIO;
	}
	msg->l3h = (unsigned char *)th + sizeof(*th);

	switch (th->msg_type) {
	case RSL_MT_SACCH_FILL:
		ret = rsl_rx_sacch_fill(trx, msg);
		break;
	default:
		LOGP(DRSL, LOGL_NOTICE, "undefined RSL TRX msg_type 0x%02x\n",
			th->msg_type);
		ret = -EINVAL;
	}

	if (ret != 1)
		msgb_free(msg);

	return ret;
}

static int rsl_rx_ipaccess(struct gsm_bts_trx *trx, struct msgb *msg)
{
	struct abis_rsl_dchan_hdr *dch = msgb_l2(msg);
	int ret = 0;

	if (msgb_l2len(msg) < sizeof(*dch)) {
		LOGP(DRSL, LOGL_NOTICE, "RSL ip.access message too short\n");
		rsl_tx_error_report(trx, RSL_ERR_PROTO, NULL, NULL, msg);
		msgb_free(msg);
		return -EIO;
	}
	msg->l3h = (unsigned char *)dch + sizeof(*dch);

	if (!chan_nr_is_dchan(dch->chan_nr))
		return rsl_reject_unknown_lchan(msg);

	msg->lchan = lchan_lookup(trx, dch->chan_nr, "RSL rx IPACC: ");
	if (!msg->lchan) {
		LOGP(DRSL, LOGL_ERROR, "Rx RSL %s for unknow lchan\n",
			rsl_msg_name(dch->c.msg_type));
		return rsl_reject_unknown_lchan(msg);
	}

	LOGP(DRSL, LOGL_INFO, "%s Rx RSL %s\n", gsm_lchan_name(msg->lchan),
		rsl_ipac_msg_name(dch->c.msg_type));

	switch (dch->c.msg_type) {
	case RSL_MT_IPAC_CRCX:
	case RSL_MT_IPAC_MDCX:
		ret = rsl_rx_ipac_XXcx(msg);
		break;
	case RSL_MT_IPAC_DLCX:
		ret = rsl_rx_ipac_dlcx(msg);
		break;
	default:
		LOGP(DRSL, LOGL_NOTICE, "unsupported RSL ip.access msg_type 0x%02x\n",
			dch->c.msg_type);
		ret = -EINVAL;
	}

	if (ret != 1)
		msgb_free(msg);
	return ret;
}

int lchan_deactivate(struct gsm_lchan *lchan)
{
	OSMO_ASSERT(lchan);

	lchan->ciph_state = 0;
	return bts_model_lchan_deactivate(lchan);
}

int down_rsl(struct gsm_bts_trx *trx, struct msgb *msg)
{
	struct abis_rsl_common_hdr *rslh;
	int ret = 0;

	OSMO_ASSERT(trx);
	OSMO_ASSERT(msg);

	rslh = msgb_l2(msg);

	if (msgb_l2len(msg) < sizeof(*rslh)) {
		LOGP(DRSL, LOGL_NOTICE, "RSL message too short\n");
		rsl_tx_error_report(trx, RSL_ERR_PROTO, NULL, NULL, msg);
		msgb_free(msg);
		return -EIO;
	}

	switch (rslh->msg_discr & 0xfe) {
	case ABIS_RSL_MDISC_RLL:
		ret = rsl_rx_rll(trx, msg);
		/* exception: RLL messages are _NOT_ freed as they are now
		 * owned by LAPDm which might have queued them */
		break;
	case ABIS_RSL_MDISC_COM_CHAN:
		ret = rsl_rx_cchan(trx, msg);
		break;
	case ABIS_RSL_MDISC_DED_CHAN:
		ret = rsl_rx_dchan(trx, msg);
		break;
	case ABIS_RSL_MDISC_TRX:
		ret = rsl_rx_trx(trx, msg);
		break;
	case ABIS_RSL_MDISC_IPACCESS:
		ret = rsl_rx_ipaccess(trx, msg);
		break;
	default:
		LOGP(DRSL, LOGL_NOTICE, "unknown RSL msg_discr 0x%02x\n",
			rslh->msg_discr);
		rsl_tx_error_report(trx, RSL_ERR_MSG_DISCR, NULL, NULL, msg);
		msgb_free(msg);
		ret = -EINVAL;
	}

	/* we don't free here, as rsl_rx{cchan,dchan,trx,ipaccess,rll} are
	 * responsible for owning the msg */

	return ret;
}
