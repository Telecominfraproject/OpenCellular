/* GSM TS 12.21 O&M / OML, BTS side */

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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * Operation and Maintenance Messages
 */

#include "btsconfig.h"

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <osmocom/core/talloc.h>
#include <osmocom/core/msgb.h>
#include <osmocom/gsm/protocol/gsm_12_21.h>
#include <osmocom/gsm/abis_nm.h>
#include <osmocom/abis/e1_input.h>
#include <osmocom/abis/ipaccess.h>

#include <osmo-bts/logging.h>
#include <osmo-bts/gsm_data.h>
#include <osmo-bts/abis.h>
#include <osmo-bts/oml.h>
#include <osmo-bts/bts_model.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/signal.h>
#include <osmo-bts/phy_link.h>

static int oml_ipa_set_attr(struct gsm_bts *bts, struct msgb *msg);

static struct tlv_definition abis_nm_att_tlvdef_ipa_local = {};

/*
 * support
 */

static int oml_tlv_parse(struct tlv_parsed *tp, const uint8_t *buf, int len)
{
	return tlv_parse(tp, &abis_nm_att_tlvdef_ipa_local, buf, len, 0, 0);
}

struct msgb *oml_msgb_alloc(void)
{
	return msgb_alloc_headroom(1024, 128, "OML");
}

/* 3GPP TS 12.21 § 8.8.2 */
static int oml_tx_failure_event_rep(struct gsm_abis_mo *mo, uint16_t cause_value,
				    const char *fmt, ...)
{
	struct msgb *nmsg;
	va_list ap;

	LOGP(DOML, LOGL_NOTICE, "Sending %s to BSC: ", get_value_string(abis_mm_event_cause_names, cause_value));
	va_start(ap, fmt);
	osmo_vlogp(DOML, LOGL_NOTICE, __FILE__, __LINE__, 1, fmt, ap);
	nmsg = abis_nm_fail_evt_vrep(NM_EVT_PROC_FAIL, NM_SEVER_CRITICAL,
				     NM_PCAUSE_T_MANUF, cause_value, fmt, ap);
	va_end(ap);
	LOGPC(DOML, LOGL_NOTICE, "\n");

	if (!nmsg)
		return -ENOMEM;

	return oml_mo_send_msg(mo, nmsg, NM_MT_FAILURE_EVENT_REP);
}

void oml_fail_rep(uint16_t cause_value, const char *fmt, ...)
{
	va_list ap;
	char *rep;

	va_start(ap, fmt);
	rep = talloc_asprintf(tall_bts_ctx, fmt, ap);
	va_end(ap);

	osmo_signal_dispatch(SS_FAIL, cause_value, rep);
	/* signal dispatch is synchronous so all the signal handlers are
	   finished already: we're free to free */
	talloc_free(rep);
}

int oml_send_msg(struct msgb *msg, int is_manuf)
{
	struct abis_om_hdr *omh;

	if (is_manuf) {
		/* length byte, string + 0 termination */
		uint8_t *manuf = msgb_push(msg, 1 + sizeof(abis_nm_ipa_magic));
		manuf[0] = strlen(abis_nm_ipa_magic)+1;
		memcpy(manuf+1, abis_nm_ipa_magic, strlen(abis_nm_ipa_magic));
	}

	/* Push the main OML header and send it off */
	omh = (struct abis_om_hdr *) msgb_push(msg, sizeof(*omh));
	if (is_manuf)
		omh->mdisc = ABIS_OM_MDISC_MANUF;
	else
		omh->mdisc = ABIS_OM_MDISC_FOM;
	omh->placement = ABIS_OM_PLACEMENT_ONLY;
	omh->sequence = 0;
	omh->length = msgb_l3len(msg);

	msg->l2h = (uint8_t *)omh;

	return abis_oml_sendmsg(msg);
}

int oml_mo_send_msg(struct gsm_abis_mo *mo, struct msgb *msg, uint8_t msg_type)
{
	struct abis_om_fom_hdr *foh;

	msg->l3h = msgb_push(msg, sizeof(*foh));
	foh = (struct abis_om_fom_hdr *) msg->l3h;
	foh->msg_type = msg_type;
	foh->obj_class = mo->obj_class;
	memcpy(&foh->obj_inst, &mo->obj_inst, sizeof(foh->obj_inst));

	/* FIXME: This assumption may not always be correct */
	msg->trx = mo->bts->c0;

	return oml_send_msg(msg, 0);
}

/* FIXME: move to gsm_data_shared */
static char mo_buf[128];
char *gsm_abis_mo_name(const struct gsm_abis_mo *mo)
{
	snprintf(mo_buf, sizeof(mo_buf), "OC=%s INST=(%02x,%02x,%02x)",
		 get_value_string(abis_nm_obj_class_names, mo->obj_class),
		 mo->obj_inst.bts_nr, mo->obj_inst.trx_nr, mo->obj_inst.ts_nr);
	return mo_buf;
}

static inline void add_bts_attrs(struct msgb *msg, const struct gsm_bts *bts)
{
	abis_nm_put_sw_file(msg, "osmobts", PACKAGE_VERSION, true);
	abis_nm_put_sw_file(msg, btsatttr2str(BTS_TYPE_VARIANT), btsvariant2str(bts->variant), true);

	if (strlen(bts->sub_model))
		abis_nm_put_sw_file(msg, btsatttr2str(BTS_SUB_MODEL), bts->sub_model, true);
}

/* Add BTS features as 3GPP TS 52.021 §9.4.30 Manufacturer Id */
static inline void add_bts_feat(struct msgb *msg, const struct gsm_bts *bts)
{
	msgb_tl16v_put(msg, NM_ATT_MANUF_ID, _NUM_BTS_FEAT/8 + 1, bts->_features_data);
}

static inline void add_trx_attr(struct msgb *msg, struct gsm_bts_trx *trx)
{
	const struct phy_instance *pinst = trx_phy_instance(trx);

	abis_nm_put_sw_file(msg, btsatttr2str(TRX_PHY_VERSION), pinst && strlen(pinst->version) ? pinst->version : "Unknown",
			    true);
}

/* The number of attributes in §9.4.26 List of Required Attributes is 2 bytes,
   but the Count of not-reported attributes from §9.4.64 is 1 byte */
static inline uint8_t pack_num_unreported_attr(uint16_t attrs)
{
	if (attrs > 255) {
		LOGP(DOML, LOGL_ERROR, "O&M Get Attributes, Count of not-reported attributes is too big: %u\n",
		     attrs);
		return 255;
	}
	return attrs; /* Return number of unhandled attributes */
}

/* copy all the attributes accumulated in msg to out and return the total length of out buffer */
static inline int cleanup_attr_msg(uint8_t *out, int out_offset, struct msgb *msg)
{
	int len = 0;

	out[0] = pack_num_unreported_attr(out_offset - 1);

	if (msg) {
		memcpy(out + out_offset, msgb_data(msg), msg->len);
		len = msg->len;
		msgb_free(msg);
	}

	return len + out_offset + 1;
}

static inline int handle_attrs_trx(uint8_t *out, struct gsm_bts_trx *trx, const uint8_t *attr, uint16_t attr_len)
{
	uint16_t i, attr_out_index = 1; /* byte 0 is reserved for unsupported attributes counter */
	struct msgb *attr_buf = oml_msgb_alloc();

	if (!attr_buf)
		return -NM_NACK_CANT_PERFORM;

	for (i = 0; i < attr_len; i++) {
		bool processed = false;
		switch (attr[i]) {
		case NM_ATT_SW_CONFIG:
			if (trx) {
				add_trx_attr(attr_buf, trx);
				processed = true;
			} else
				LOGP(DOML, LOGL_ERROR, "O&M Get Attributes [%u], %s is unhandled due to missing TRX.\n",
				     i, get_value_string(abis_nm_att_names, attr[i]));
			break;
		default:
			LOGP(DOML, LOGL_ERROR, "O&M Get Attributes [%u], %s is unsupported by TRX.\n", i,
			     get_value_string(abis_nm_att_names, attr[i]));
		}
		/* assemble values of supported attributes and list of unsupported ones */
		if (!processed) {
			out[attr_out_index] = attr[i];
			attr_out_index++;
		}
	}

	return cleanup_attr_msg(out, attr_out_index, attr_buf);
}

static inline int handle_attrs_bts(uint8_t *out, const struct gsm_bts *bts, const uint8_t *attr, uint16_t attr_len)
{
	uint16_t i, attr_out_index = 1; /* byte 0 is reserved for unsupported attributes counter */
	struct msgb *attr_buf = oml_msgb_alloc();

	if (!attr_buf)
		return -NM_NACK_CANT_PERFORM;

	for (i = 0; i < attr_len; i++) {
		switch (attr[i]) {
		case NM_ATT_SW_CONFIG:
			add_bts_attrs(attr_buf, bts);
			break;
		case NM_ATT_MANUF_ID:
			add_bts_feat(attr_buf, bts);
			break;
		default:
			LOGP(DOML, LOGL_ERROR, "O&M Get Attributes [%u], %s is unsupported by BTS.\n", i,
			     get_value_string(abis_nm_att_names, attr[i]));
			out[attr_out_index] = attr[i]; /* assemble values of supported attributes and list of unsupported ones */
			attr_out_index++;
		}
	}

	return cleanup_attr_msg(out, attr_out_index, attr_buf);
}

/* send 3GPP TS 52.021 §8.11.2 Get Attribute Response */
static int oml_tx_attr_resp(struct gsm_bts *bts, const struct abis_om_fom_hdr *foh, const uint8_t *attr,
			    uint16_t attr_len)
{
	struct msgb *nmsg = oml_msgb_alloc();
	uint8_t resp[MAX_VERSION_LENGTH * attr_len * 2]; /* heuristic for Attribute Response Info space requirements */
	int len;

	LOGP(DOML, LOGL_INFO, "%s Tx Get Attribute Response\n",
	     get_value_string(abis_nm_obj_class_names, foh->obj_class));

	if (!nmsg)
		return -NM_NACK_CANT_PERFORM;

	switch (foh->obj_class) {
	case NM_OC_BTS:
		len = handle_attrs_bts(resp, bts, attr, attr_len);
		break;
	case NM_OC_BASEB_TRANSC:
		len = handle_attrs_trx(resp, gsm_bts_trx_num(bts, foh->obj_inst.trx_nr), attr, attr_len);
		break;
	default:
		LOGP(DOML, LOGL_ERROR, "Unsupported MO class %s in Get Attribute Response\n",
		     get_value_string(abis_nm_obj_class_names, foh->obj_class));
		len = -NM_NACK_RES_NOTIMPL;
	}

	if (len < 0) {
		LOGP(DOML, LOGL_ERROR, "Tx Get Attribute Response FAILED with %d\n", len);
		msgb_free(nmsg);
		return len;
	}

	/* §9.4.64 Get Attribute Response Info */
	msgb_tl16v_put(nmsg, NM_ATT_GET_ARI, len, resp);

	len = oml_mo_send_msg(&bts->mo, nmsg, NM_MT_GET_ATTR_RESP);
	return (len < 0) ? -NM_NACK_CANT_PERFORM : len;
}

/* 8.8.1 sending State Changed Event Report */
int oml_tx_state_changed(struct gsm_abis_mo *mo)
{
	struct msgb *nmsg;

	LOGP(DOML, LOGL_INFO, "%s Tx STATE CHG REP\n", gsm_abis_mo_name(mo));

	nmsg = oml_msgb_alloc();
	if (!nmsg)
		return -ENOMEM;

	/* 9.4.38 Operational State */
	msgb_tv_put(nmsg, NM_ATT_OPER_STATE, mo->nm_state.operational);

	/* 9.4.7 Availability Status */
	msgb_tl16v_put(nmsg, NM_ATT_AVAIL_STATUS, 1, &mo->nm_state.availability);

	/* 9.4.4 Administrative Status -- not in spec but also sent by nanobts */
	msgb_tv_put(nmsg, NM_ATT_ADM_STATE, mo->nm_state.administrative);

	return oml_mo_send_msg(mo, nmsg, NM_MT_STATECHG_EVENT_REP);
}

/* First initialization of MO, does _not_ generate state changes */
void oml_mo_state_init(struct gsm_abis_mo *mo, int op_state, int avail_state)
{
	mo->nm_state.availability = avail_state;
	mo->nm_state.operational = op_state;
}

int oml_mo_state_chg(struct gsm_abis_mo *mo, int op_state, int avail_state)
{
	int rc = 0;

	if ((op_state != -1 && mo->nm_state.operational != op_state) ||
	    (avail_state != -1 && mo->nm_state.availability != avail_state)) {
		if (avail_state != -1) {
			LOGP(DOML, LOGL_INFO, "%s AVAIL STATE %s -> %s\n",
				gsm_abis_mo_name(mo),
				abis_nm_avail_name(mo->nm_state.availability),
				abis_nm_avail_name(avail_state));
			mo->nm_state.availability = avail_state;
		}
		if (op_state != -1) {
			LOGP(DOML, LOGL_INFO, "%s OPER STATE %s -> %s\n",
				gsm_abis_mo_name(mo),
				abis_nm_opstate_name(mo->nm_state.operational),
				abis_nm_opstate_name(op_state));
			mo->nm_state.operational = op_state;
			osmo_signal_dispatch(SS_GLOBAL, S_NEW_OP_STATE, NULL);
		}

		/* send state change report */
		rc = oml_tx_state_changed(mo);
	}
	return rc;
}

int oml_mo_fom_ack_nack(struct gsm_abis_mo *mo, uint8_t orig_msg_type,
			uint8_t cause)
{
	struct msgb *msg;
	uint8_t new_msg_type;

	msg = oml_msgb_alloc();
	if (!msg)
		return -ENOMEM;

	if (cause) {
		new_msg_type = orig_msg_type + 2;
		msgb_tv_put(msg, NM_ATT_NACK_CAUSES, cause);
	} else {
		new_msg_type = orig_msg_type + 1;
	}

	return oml_mo_send_msg(mo, msg, new_msg_type);
}

int oml_mo_statechg_ack(struct gsm_abis_mo *mo)
{
	struct msgb *msg;
	int rc = 0;

	msg = oml_msgb_alloc();
	if (!msg)
		return -ENOMEM;

	msgb_tv_put(msg, NM_ATT_ADM_STATE, mo->nm_state.administrative);

	rc = oml_mo_send_msg(mo, msg, NM_MT_CHG_ADM_STATE_ACK);
	if (rc != 0)
		return rc;

	/* Emulate behaviour of ipaccess nanobts: Send a 'State Changed Event Report' as well. */
	return oml_tx_state_changed(mo);
}

int oml_mo_statechg_nack(struct gsm_abis_mo *mo, uint8_t nack_cause)
{
	return oml_mo_fom_ack_nack(mo, NM_MT_CHG_ADM_STATE, nack_cause);
}

int oml_mo_opstart_ack(struct gsm_abis_mo *mo)
{
	return oml_mo_fom_ack_nack(mo, NM_MT_OPSTART, 0);
}

int oml_mo_opstart_nack(struct gsm_abis_mo *mo, uint8_t nack_cause)
{
	return oml_mo_fom_ack_nack(mo, NM_MT_OPSTART, nack_cause);
}

int oml_fom_ack_nack(struct msgb *old_msg, uint8_t cause)
{
	struct abis_om_hdr *old_oh = msgb_l2(old_msg);
	struct abis_om_fom_hdr *old_foh = msgb_l3(old_msg);
	struct msgb *msg;
	struct abis_om_fom_hdr *foh;
	int is_manuf = 0;

	msg = oml_msgb_alloc();
	if (!msg)
		return -ENOMEM;

	/* make sure to respond with MANUF if request was MANUF */
	if (old_oh->mdisc == ABIS_OM_MDISC_MANUF)
		is_manuf = 1;

	msg->trx = old_msg->trx;

	/* copy over old FOM-Header and later only change the msg_type */
	msg->l3h = msgb_push(msg, sizeof(*foh));
	foh = (struct abis_om_fom_hdr *) msg->l3h;
	memcpy(foh, old_foh, sizeof(*foh));

	/* alter message type */
	if (cause) {
		LOGP(DOML, LOGL_NOTICE, "Sending FOM NACK with cause %s.\n",
			abis_nm_nack_cause_name(cause));
		foh->msg_type += 2; /* nack */
		/* add cause */
		msgb_tv_put(msg, NM_ATT_NACK_CAUSES, cause);
	} else {
		LOGP(DOML, LOGL_DEBUG, "Sending FOM ACK.\n");
		foh->msg_type++; /* ack */
	}

	return oml_send_msg(msg, is_manuf);
}

/*
 * Formatted O&M messages
 */

/* 8.3.7 sending SW Activated Report */
int oml_mo_tx_sw_act_rep(struct gsm_abis_mo *mo)
{
	struct msgb *nmsg;

	LOGP(DOML, LOGL_INFO, "%s Tx SW ACT REP\n", gsm_abis_mo_name(mo));

	nmsg = oml_msgb_alloc();
	if (!nmsg)
		return -ENOMEM;

	msgb_put(nmsg, sizeof(struct abis_om_fom_hdr));
	return oml_mo_send_msg(mo, nmsg, NM_MT_SW_ACTIVATED_REP);
}

/* The defaults below correspond to the libosmocore default of 1s for
 * DCCH and 2s for ACCH. The BSC should override this via OML anyway. */
const unsigned int oml_default_t200_ms[7] = {
	[T200_SDCCH]		= 1000,
	[T200_FACCH_F]		= 1000,
	[T200_FACCH_H]		= 1000,
	[T200_SACCH_TCH_SAPI0]	= 2000,
	[T200_SACCH_SDCCH]	= 2000,
	[T200_SDCCH_SAPI3]	= 1000,
	[T200_SACCH_TCH_SAPI3]	= 2000,
};

static void dl_set_t200(struct lapdm_datalink *dl, unsigned int t200_msec)
{
	dl->dl.t200_sec = t200_msec / 1000;
	dl->dl.t200_usec = (t200_msec % 1000) * 1000;
}

/* Configure LAPDm T200 timers for this lchan according to OML */
int oml_set_lchan_t200(struct gsm_lchan *lchan)
{
	struct gsm_bts *bts = lchan->ts->trx->bts;
	struct lapdm_channel *lc = &lchan->lapdm_ch;
	unsigned int t200_dcch, t200_dcch_sapi3, t200_acch, t200_acch_sapi3;

	/* set T200 for main and associated channel */
	switch (lchan->type) {
	case GSM_LCHAN_SDCCH:
		t200_dcch = bts->t200_ms[T200_SDCCH];
		t200_dcch_sapi3 = bts->t200_ms[T200_SDCCH_SAPI3];
		t200_acch = bts->t200_ms[T200_SACCH_SDCCH];
		t200_acch_sapi3 = bts->t200_ms[T200_SACCH_SDCCH];
		break;
	case GSM_LCHAN_TCH_F:
		t200_dcch = bts->t200_ms[T200_FACCH_F];
		t200_dcch_sapi3 = bts->t200_ms[T200_FACCH_F];
		t200_acch = bts->t200_ms[T200_SACCH_TCH_SAPI0];
		t200_acch_sapi3 = bts->t200_ms[T200_SACCH_TCH_SAPI3];
		break;
	case GSM_LCHAN_TCH_H:
		t200_dcch = bts->t200_ms[T200_FACCH_H];
		t200_dcch_sapi3 = bts->t200_ms[T200_FACCH_H];
		t200_acch = bts->t200_ms[T200_SACCH_TCH_SAPI0];
		t200_acch_sapi3 = bts->t200_ms[T200_SACCH_TCH_SAPI3];
		break;
	default:
		return -1;
	}

	DEBUGP(DLLAPD, "%s: Setting T200 D0=%u, D3=%u, S0=%u, S3=%u"
		"(all in ms)\n", gsm_lchan_name(lchan), t200_dcch,
		t200_dcch_sapi3, t200_acch, t200_acch_sapi3);

	dl_set_t200(&lc->lapdm_dcch.datalink[DL_SAPI0], t200_dcch);
	dl_set_t200(&lc->lapdm_dcch.datalink[DL_SAPI3], t200_dcch_sapi3);
	dl_set_t200(&lc->lapdm_acch.datalink[DL_SAPI0], t200_acch);
	dl_set_t200(&lc->lapdm_acch.datalink[DL_SAPI3], t200_acch_sapi3);

	return 0;
}

/* 3GPP TS 52.021 §8.11.1 Get Attributes has been received */
static int oml_rx_get_attr(struct gsm_bts *bts, struct msgb *msg)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	struct tlv_parsed tp;
	int rc;

	if (!foh || !bts)
		return -EINVAL;

	abis_nm_debugp_foh(DOML, foh);
	DEBUGPC(DOML, "Rx GET ATTR\n");

	rc = oml_tlv_parse(&tp, foh->data, msgb_l3len(msg) - sizeof(*foh));
	if (rc < 0) {
		oml_tx_failure_event_rep(&bts->mo, OSMO_EVT_MAJ_UNSUP_ATTR, "Get Attribute parsing failure");
		return oml_fom_ack_nack(msg, NM_NACK_INCORR_STRUCT);
	}

	if (!TLVP_PRES_LEN(&tp, NM_ATT_LIST_REQ_ATTR, 1)) {
		LOGP(DOML, LOGL_ERROR, "O&M Get Attributes message without Attribute List?!\n");
		oml_tx_failure_event_rep(&bts->mo, OSMO_EVT_MAJ_UNSUP_ATTR, "Get Attribute without Attribute List");
		return oml_fom_ack_nack(msg, NM_NACK_INCORR_STRUCT);
	}

	rc = oml_tx_attr_resp(bts, foh, TLVP_VAL(&tp, NM_ATT_LIST_REQ_ATTR), TLVP_LEN(&tp, NM_ATT_LIST_REQ_ATTR));
	if (rc < 0) {
		LOGP(DOML, LOGL_ERROR, "responding to O&M Get Attributes message with NACK 0%x\n", -rc);
		return oml_fom_ack_nack(msg, -rc);
	}

	return 0;
}

/* 8.6.1 Set BTS Attributes has been received */
static int oml_rx_set_bts_attr(struct gsm_bts *bts, struct msgb *msg)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	struct tlv_parsed tp, *tp_merged;
	int rc, i;
	const uint8_t *payload;

	abis_nm_debugp_foh(DOML, foh);
	DEBUGPC(DOML, "Rx SET BTS ATTR\n");

	rc = oml_tlv_parse(&tp, foh->data, msgb_l3len(msg) - sizeof(*foh));
	if (rc < 0) {
		oml_tx_failure_event_rep(&bts->mo, OSMO_EVT_MAJ_UNSUP_ATTR,
					 "New value for Attribute not supported");
		return oml_fom_ack_nack(msg, NM_NACK_INCORR_STRUCT);
	}

	/* Test for globally unsupported stuff here */
	if (TLVP_PRES_LEN(&tp, NM_ATT_BCCH_ARFCN, 2)) {
		uint16_t arfcn = ntohs(tlvp_val16_unal(&tp, NM_ATT_BCCH_ARFCN));
		if (arfcn > 1024) {
			oml_tx_failure_event_rep(&bts->mo, OSMO_EVT_WARN_SW_WARN,
						 "Given ARFCN %u is not supported",
						 arfcn);
			LOGP(DOML, LOGL_NOTICE, "Given ARFCN %d is not supported.\n", arfcn);
			return oml_fom_ack_nack(msg, NM_NACK_FREQ_NOTAVAIL);
		}
	}
	/* 9.4.52 Starting Time */
	if (TLVP_PRESENT(&tp, NM_ATT_START_TIME)) {
		oml_tx_failure_event_rep(&bts->mo, OSMO_EVT_MAJ_UNSUP_ATTR,
					 "NM_ATT_START_TIME Attribute not "
					 "supported");
		return oml_fom_ack_nack(msg, NM_NACK_SPEC_IMPL_NOTSUPP);
	}

	/* merge existing BTS attributes with new attributes */
	tp_merged = osmo_tlvp_copy(bts->mo.nm_attr, bts);
	osmo_tlvp_merge(tp_merged, &tp);

	/* Ask BTS driver to validate new merged attributes */
	rc = bts_model_check_oml(bts, foh->msg_type, bts->mo.nm_attr, tp_merged, bts);
	if (rc < 0) {
		talloc_free(tp_merged);
		return oml_fom_ack_nack(msg, -rc);
	}

	/* Success: replace old BTS attributes with new */
	talloc_free(bts->mo.nm_attr);
	bts->mo.nm_attr = tp_merged;

	/* ... and actually still parse them */

	/* 9.4.25 Interference Level Boundaries */
	if (TLVP_PRES_LEN(&tp, NM_ATT_INTERF_BOUND, 6)) {
		payload = TLVP_VAL(&tp, NM_ATT_INTERF_BOUND);
		for (i = 0; i < 6; i++) {
			int16_t boundary = *payload;
			bts->interference.boundary[i] = -1 * boundary;
		}
	}
	/* 9.4.24 Intave Parameter */
	if (TLVP_PRES_LEN(&tp, NM_ATT_INTAVE_PARAM, 1))
		bts->interference.intave = *TLVP_VAL(&tp, NM_ATT_INTAVE_PARAM);

	/* 9.4.14 Connection Failure Criterion */
	if (TLVP_PRES_LEN(&tp, NM_ATT_CONN_FAIL_CRIT, 1)) {
		const uint8_t *val = TLVP_VAL(&tp, NM_ATT_CONN_FAIL_CRIT);

		switch (val[0]) {
		case 0xFF: /* Osmocom specific Extension of TS 12.21 */
			LOGP(DOML, LOGL_NOTICE, "WARNING: Radio Link Timeout "
			     "explicitly disabled, only use this for lab testing!\n");
			bts->radio_link_timeout = -1;
			break;
		case 0x01: /* Based on uplink SACCH (radio link timeout) */
			if (TLVP_LEN(&tp, NM_ATT_CONN_FAIL_CRIT) >= 2 &&
			    val[1] >= 4 && val[1] <= 64) {
				bts->radio_link_timeout = val[1];
				break;
			}
			/* fall-through */
		case 0x02: /* Based on RXLEV/RXQUAL measurements */
		default:
			LOGP(DOML, LOGL_NOTICE, "Given Conn. Failure Criterion "
				"not supported. Please use criterion 0x01 with "
				"RADIO_LINK_TIMEOUT value of 4..64\n");
			return oml_fom_ack_nack(msg, NM_NACK_PARAM_RANGE);
		}
	}

	/* 9.4.53 T200 */
	if (TLVP_PRES_LEN(&tp, NM_ATT_T200, ARRAY_SIZE(bts->t200_ms))) {
		payload = TLVP_VAL(&tp, NM_ATT_T200);
		for (i = 0; i < ARRAY_SIZE(bts->t200_ms); i++) {
			uint32_t t200_ms = payload[i] * abis_nm_t200_ms[i];
#if 0
			bts->t200_ms[i] = t200_ms;
			DEBUGP(DOML, "T200[%u]: OML=%u, mult=%u => %u ms\n",
				i, payload[i], abis_nm_t200_mult[i],
				bts->t200_ms[i]);
#else
			/* we'd rather use the 1s/2s (long) defaults by
			 * libosmocore, as we appear to have some bug(s)
			 * related to handling T200 expiration in
			 * libosmogsm lapd(m) code? */
			LOGP(DOML, LOGL_NOTICE, "Ignoring T200[%u] (%u ms) "
				"as sent by BSC due to suspected LAPDm bug!\n",
				i, t200_ms);
#endif
		}
	}

	/* 9.4.31 Maximum Timing Advance */
	if (TLVP_PRES_LEN(&tp, NM_ATT_MAX_TA, 1))
		bts->max_ta = *TLVP_VAL(&tp, NM_ATT_MAX_TA);

	/* 9.4.39 Overload Period */
	if (TLVP_PRES_LEN(&tp, NM_ATT_OVERL_PERIOD, 1))
		bts->load.overload_period = *TLVP_VAL(&tp, NM_ATT_OVERL_PERIOD);

	/* 9.4.12 CCCH Load Threshold */
	if (TLVP_PRES_LEN(&tp, NM_ATT_CCCH_L_T, 1))
		bts->load.ccch.load_ind_thresh = *TLVP_VAL(&tp, NM_ATT_CCCH_L_T);

	/* 9.4.11 CCCH Load Indication Period */
	if (TLVP_PRES_LEN(&tp, NM_ATT_CCCH_L_I_P, 1)) {
		bts->load.ccch.load_ind_period = *TLVP_VAL(&tp, NM_ATT_CCCH_L_I_P);
		load_timer_start(bts);
	}

	/* 9.4.44 RACH Busy Threshold */
	if (TLVP_PRES_LEN(&tp, NM_ATT_RACH_B_THRESH, 1)) {
		int16_t thresh = *TLVP_VAL(&tp, NM_ATT_RACH_B_THRESH);
		bts->load.rach.busy_thresh = -1 * thresh;
	}

	/* 9.4.45 RACH Load Averaging Slots */
	if (TLVP_PRES_LEN(&tp, NM_ATT_LDAVG_SLOTS, 2)) {
		bts->load.rach.averaging_slots =
			ntohs(tlvp_val16_unal(&tp, NM_ATT_LDAVG_SLOTS));
	}

	/* 9.4.10 BTS Air Timer */
	if (TLVP_PRES_LEN(&tp, NM_ATT_BTS_AIR_TIMER, 1)) {
		uint8_t t3105 = *TLVP_VAL(&tp, NM_ATT_BTS_AIR_TIMER);
		if (t3105 == 0) {
			LOGP(DOML, LOGL_NOTICE,
				"T3105 must have a value != 0.\n");
			return oml_fom_ack_nack(msg, NM_NACK_PARAM_RANGE);
		}
		bts->t3105_ms = t3105 * 10;
	}

	/* 9.4.37 NY1 */
	if (TLVP_PRES_LEN(&tp, NM_ATT_NY1, 1))
		bts->ny1 = *TLVP_VAL(&tp, NM_ATT_NY1);

	/* 9.4.8 BCCH ARFCN */
	if (TLVP_PRES_LEN(&tp, NM_ATT_BCCH_ARFCN, 2))
		bts->c0->arfcn = ntohs(tlvp_val16_unal(&tp, NM_ATT_BCCH_ARFCN));

	/* 9.4.9 BSIC */
	if (TLVP_PRES_LEN(&tp, NM_ATT_BSIC, 1))
		bts->bsic = *TLVP_VAL(&tp, NM_ATT_BSIC);

	/* call into BTS driver to apply new attributes to hardware */
	return bts_model_apply_oml(bts, msg, tp_merged, NM_OC_BTS, bts);
}

/* 8.6.2 Set Radio Attributes has been received */
static int oml_rx_set_radio_attr(struct gsm_bts_trx *trx, struct msgb *msg)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	struct tlv_parsed tp, *tp_merged;
	int rc;

	abis_nm_debugp_foh(DOML, foh);
	DEBUGPC(DOML, "Rx SET RADIO CARRIER ATTR\n");

	rc = oml_tlv_parse(&tp, foh->data, msgb_l3len(msg) - sizeof(*foh));
	if (rc < 0) {
		oml_tx_failure_event_rep(&trx->mo, OSMO_EVT_MAJ_UNSUP_ATTR,
					 "New value for Set Radio Attribute not"
					 " supported");
		return oml_fom_ack_nack(msg, NM_NACK_INCORR_STRUCT);
	}

	/* merge existing BTS attributes with new attributes */
	tp_merged = osmo_tlvp_copy(trx->mo.nm_attr, trx->bts);
	osmo_tlvp_merge(tp_merged, &tp);

	/* Ask BTS driver to validate new merged attributes */
	rc = bts_model_check_oml(trx->bts, foh->msg_type, trx->mo.nm_attr, tp_merged, trx);
	if (rc < 0) {
		talloc_free(tp_merged);
		return oml_fom_ack_nack(msg, -rc);
	}

	/* Success: replace old BTS attributes with new */
	talloc_free(trx->mo.nm_attr);
	trx->mo.nm_attr = tp_merged;

	/* ... and actually still parse them */

	/* 9.4.47 RF Max Power Reduction */
	if (TLVP_PRES_LEN(&tp, NM_ATT_RF_MAXPOWR_R, 1)) {
		trx->max_power_red = *TLVP_VAL(&tp, NM_ATT_RF_MAXPOWR_R) * 2;
		LOGP(DOML, LOGL_INFO, "Set RF Max Power Reduction = %d dBm\n",
		     trx->max_power_red);
	}
	/* 9.4.5 ARFCN List */
#if 0
	if (TLVP_PRESENT(&tp, NM_ATT_ARFCN_LIST)) {
		uint8_t *value = TLVP_VAL(&tp, NM_ATT_ARFCN_LIST);
		uint16_t _value;
		uint16_t length = TLVP_LEN(&tp, NM_ATT_ARFCN_LIST);
		uint16_t arfcn;
		int i;
		for (i = 0; i < length; i++) {
			memcpy(&_value, value, 2);
			arfcn = ntohs(_value);
			value += 2;
			if (arfcn > 1024)
				return oml_fom_ack_nack(msg, NM_NACK_FREQ_NOTAVAIL);
			trx->arfcn_list[i] = arfcn;
			LOGP(DOML, LOGL_INFO, " ARFCN list = %d\n", trx->arfcn_list[i]);
		}
		trx->arfcn_num = length;
	} else
		trx->arfcn_num = 0;
#else
	if (trx != trx->bts->c0 && TLVP_PRESENT(&tp, NM_ATT_ARFCN_LIST)) {
		const uint8_t *value = TLVP_VAL(&tp, NM_ATT_ARFCN_LIST);
		uint16_t _value;
		uint16_t length = TLVP_LEN(&tp, NM_ATT_ARFCN_LIST);
		uint16_t arfcn;
		if (length != 2) {
			LOGP(DOML, LOGL_ERROR, "Expecting only one ARFCN, "
				"because hopping not supported\n");
			return oml_fom_ack_nack(msg, NM_NACK_MSGINCONSIST_PHYSCFG);
		}
		memcpy(&_value, value, 2);
		arfcn = ntohs(_value);
		value += 2;
		if (arfcn > 1024) {
			oml_tx_failure_event_rep(&trx->bts->mo,
						 OSMO_EVT_WARN_SW_WARN,
						 "Given ARFCN %u is unsupported",
						 arfcn);
			LOGP(DOML, LOGL_NOTICE,
			     "Given ARFCN %u is unsupported.\n", arfcn);
			return oml_fom_ack_nack(msg, NM_NACK_FREQ_NOTAVAIL);
		}
		trx->arfcn = arfcn;
	}
#endif
	/* call into BTS driver to apply new attributes to hardware */
	return bts_model_apply_oml(trx->bts, msg, tp_merged, NM_OC_RADIO_CARRIER, trx);
}

static int conf_lchans(struct gsm_bts_trx_ts *ts)
{
	enum gsm_phys_chan_config pchan = ts->pchan;

	/* RSL_MT_IPAC_PDCH_ACT style dyn PDCH */
	if (pchan == GSM_PCHAN_TCH_F_PDCH)
		pchan = ts->flags & TS_F_PDCH_ACTIVE? GSM_PCHAN_PDCH
						    : GSM_PCHAN_TCH_F;

	/* Osmocom RSL CHAN ACT style dyn TS */
	if (pchan == GSM_PCHAN_TCH_F_TCH_H_PDCH) {
		pchan = ts->dyn.pchan_is;

		/* If the dyn TS doesn't have a pchan yet, do nothing. */
		if (pchan == GSM_PCHAN_NONE)
			return 0;
	}

	return conf_lchans_as_pchan(ts, pchan);
}

int conf_lchans_as_pchan(struct gsm_bts_trx_ts *ts,
			 enum gsm_phys_chan_config pchan)
{
	struct gsm_lchan *lchan;
	unsigned int i;

	switch (pchan) {
	case GSM_PCHAN_CCCH_SDCCH4_CBCH:
		/* fallthrough */
	case GSM_PCHAN_CCCH_SDCCH4:
		for (i = 0; i < 4; i++) {
			lchan = &ts->lchan[i];
			if (pchan == GSM_PCHAN_CCCH_SDCCH4_CBCH
			    && i == 2) {
				lchan->type = GSM_LCHAN_CBCH;
			} else {
				lchan->type = GSM_LCHAN_SDCCH;
			}
		}
		/* fallthrough */
	case GSM_PCHAN_CCCH:
		lchan = &ts->lchan[CCCH_LCHAN];
		lchan->type = GSM_LCHAN_CCCH;
		break;
	case GSM_PCHAN_TCH_F:
		lchan = &ts->lchan[0];
		lchan->type = GSM_LCHAN_TCH_F;
		break;
	case GSM_PCHAN_TCH_H:
		for (i = 0; i < 2; i++) {
			lchan = &ts->lchan[i];
			lchan->type = GSM_LCHAN_TCH_H;
		}
		break;
	case GSM_PCHAN_SDCCH8_SACCH8C_CBCH:
		/* fallthrough */
	case GSM_PCHAN_SDCCH8_SACCH8C:
		for (i = 0; i < 8; i++) {
			lchan = &ts->lchan[i];
			if (pchan == GSM_PCHAN_SDCCH8_SACCH8C_CBCH
			    && i == 2) {
				lchan->type = GSM_LCHAN_CBCH;
			} else {
				lchan->type = GSM_LCHAN_SDCCH;
			}
		}
		break;
	case GSM_PCHAN_PDCH:
		lchan = &ts->lchan[0];
		lchan->type = GSM_LCHAN_PDTCH;
		break;
	default:
		LOGP(DOML, LOGL_ERROR, "Unknown/unhandled PCHAN type: %u %s\n",
		     ts->pchan, gsm_pchan_name(ts->pchan));
		return -NM_NACK_PARAM_RANGE;
	}
	return 0;
}

/* 8.6.3 Set Channel Attributes has been received */
static int oml_rx_set_chan_attr(struct gsm_bts_trx_ts *ts, struct msgb *msg)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	struct gsm_bts *bts = ts->trx->bts;
	struct tlv_parsed tp, *tp_merged;
	int rc;

	abis_nm_debugp_foh(DOML, foh);
	DEBUGPC(DOML, "Rx SET CHAN ATTR\n");

	rc = oml_tlv_parse(&tp, foh->data, msgb_l3len(msg) - sizeof(*foh));
	if (rc < 0) {
		oml_tx_failure_event_rep(&ts->mo, OSMO_EVT_MAJ_UNSUP_ATTR,
					 "New value for Set Channel Attribute "
					 "not supported");
		return oml_fom_ack_nack(msg, NM_NACK_INCORR_STRUCT);
	}

	/* 9.4.21 HSN... */
	/* 9.4.27 MAIO */
	if (TLVP_PRESENT(&tp, NM_ATT_HSN) || TLVP_PRESENT(&tp, NM_ATT_MAIO)) {
		LOGP(DOML, LOGL_NOTICE, "SET CHAN ATTR: Frequency hopping not supported.\n");
		return oml_fom_ack_nack(msg, NM_NACK_SPEC_IMPL_NOTSUPP);
	}

	/* 9.4.52 Starting Time */
	if (TLVP_PRESENT(&tp, NM_ATT_START_TIME)) {
		LOGP(DOML, LOGL_NOTICE, "SET CHAN ATTR: Starting time not supported.\n");
		return oml_fom_ack_nack(msg, NM_NACK_SPEC_IMPL_NOTSUPP);
	}

	/* merge existing BTS attributes with new attributes */
	tp_merged = osmo_tlvp_copy(ts->mo.nm_attr, bts);
	osmo_tlvp_merge(tp_merged, &tp);

	/* Call into BTS driver to check attribute values */
	rc = bts_model_check_oml(bts, foh->msg_type, ts->mo.nm_attr, tp_merged, ts);
	if (rc < 0) {
		LOGP(DOML, LOGL_ERROR, "SET CHAN ATTR: invalid attribute value, rc=%d\n", rc);
		talloc_free(tp_merged);
		/* Send NACK */
		return oml_fom_ack_nack(msg, -rc);
	}

	/* Success: replace old BTS attributes with new */
	talloc_free(ts->mo.nm_attr);
	ts->mo.nm_attr = tp_merged;

	/* 9.4.13 Channel Combination */
	if (TLVP_PRES_LEN(&tp, NM_ATT_CHAN_COMB, 1)) {
		uint8_t comb = *TLVP_VAL(&tp, NM_ATT_CHAN_COMB);
		ts->pchan = abis_nm_pchan4chcomb(comb);
		rc = conf_lchans(ts);
		if (rc < 0) {
			LOGP(DOML, LOGL_ERROR, "SET CHAN ATTR: invalid Chan Comb 0x%x"
			     " (pchan=%s, conf_lchans()->%d)\n",
			     comb, gsm_pchan_name(ts->pchan), rc);
			talloc_free(tp_merged);
			/* Send NACK */
			return oml_fom_ack_nack(msg, -rc);
		}
	}

	/* 9.4.5 ARFCN List */

	/* 9.4.60 TSC */
	if (TLVP_PRES_LEN(&tp, NM_ATT_TSC, 1)) {
		ts->tsc = *TLVP_VAL(&tp, NM_ATT_TSC);
	} else {
		/* If there is no TSC specified, use the BCC */
		ts->tsc = BSIC2BCC(bts->bsic);
	}
	LOGP(DOML, LOGL_INFO, "%s SET CHAN ATTR (TSC=%u pchan=%s)\n",
		gsm_abis_mo_name(&ts->mo), ts->tsc, gsm_pchan_name(ts->pchan));

	/* call into BTS driver to apply new attributes to hardware */
	return bts_model_apply_oml(bts, msg, tp_merged, NM_OC_CHANNEL, ts);
}

/* 8.9.2 Opstart has been received */
static int oml_rx_opstart(struct gsm_bts *bts, struct msgb *msg)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	struct gsm_abis_mo *mo;
	void *obj;

	abis_nm_debugp_foh(DOML, foh);
	DEBUGPC(DOML, "Rx OPSTART\n");

	/* Step 1: Resolve MO by obj_class/obj_inst */
	mo = gsm_objclass2mo(bts, foh->obj_class, &foh->obj_inst);
	obj = gsm_objclass2obj(bts, foh->obj_class, &foh->obj_inst);
	if (!mo || !obj)
		return oml_fom_ack_nack(msg, NM_NACK_OBJINST_UNKN);

	/* Step 2: Do some global dependency/consistency checking */
	if (mo->nm_state.operational == NM_OPSTATE_ENABLED) {
		DEBUGP(DOML, "... automatic ACK, OP state already was Enabled\n");
		return oml_mo_opstart_ack(mo);
	}

	/* Step 3: Ask BTS driver to apply the opstart */
	return bts_model_opstart(bts, mo, obj);
}

static int oml_rx_chg_adm_state(struct gsm_bts *bts, struct msgb *msg)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	struct tlv_parsed tp;
	struct gsm_abis_mo *mo;
	uint8_t adm_state;
	void *obj;
	int rc;

	abis_nm_debugp_foh(DOML, foh);
	DEBUGPC(DOML, "Rx CHG ADM STATE\n");

	rc = oml_tlv_parse(&tp, foh->data, msgb_l3len(msg) - sizeof(*foh));
	if (rc < 0) {
		LOGP(DOML, LOGL_ERROR, "Rx CHG ADM STATE: error during TLV parse\n");
		return oml_fom_ack_nack(msg, NM_NACK_INCORR_STRUCT);
	}

	if (!TLVP_PRESENT(&tp, NM_ATT_ADM_STATE)) {
		LOGP(DOML, LOGL_ERROR, "Rx CHG ADM STATE: no ADM state attribute\n");
		return oml_fom_ack_nack(msg, NM_NACK_INCORR_STRUCT);
	}

	adm_state = *TLVP_VAL(&tp, NM_ATT_ADM_STATE);

	/* Step 1: Resolve MO by obj_class/obj_inst */
	mo = gsm_objclass2mo(bts, foh->obj_class, &foh->obj_inst);
	obj = gsm_objclass2obj(bts, foh->obj_class, &foh->obj_inst);
	if (!mo || !obj)
		return oml_fom_ack_nack(msg, NM_NACK_OBJINST_UNKN);

	/* Step 2: Do some global dependency/consistency checking */
	if (mo->nm_state.administrative == adm_state)
		LOGP(DOML, LOGL_NOTICE,
		     "ADM state already was %s\n",
		     get_value_string(abis_nm_adm_state_names, adm_state));

	/* Step 3: Ask BTS driver to apply the state chg */
	return bts_model_chg_adm_state(bts, mo, obj, adm_state);
}

/* Check and report if the BTS number received via OML is incorrect:
   according to 3GPP TS 52.021 §9.3 BTS number is used to distinguish between different BTS of the same Site Manager.
   As we always have only single BTS per Site Manager (in case of Abis/IP with each BTS having dedicated OML connection
   to BSC), the only valid values are 0 and 0xFF (means all BTS' of a given Site Manager). */
static inline bool report_bts_number_incorrect(struct gsm_bts *bts, const struct abis_om_fom_hdr *foh, bool is_formatted)
{
	struct gsm_bts_trx *trx;
	struct gsm_abis_mo *mo = &bts->mo;
	const char *form = is_formatted ?
		"Unexpected BTS %d in formatted O&M %s (exp. 0 or 0xFF)" :
		"Unexpected BTS %d in manufacturer O&M %s (exp. 0 or 0xFF)";

	if (foh->obj_inst.bts_nr != 0 && foh->obj_inst.bts_nr != 0xff) {
		LOGP(DOML, LOGL_ERROR, form, foh->obj_inst.bts_nr, get_value_string(abis_nm_msgtype_names,
										    foh->msg_type));
		LOGPC(DOML, LOGL_ERROR, "\n");
		trx = gsm_bts_trx_num(bts, foh->obj_inst.trx_nr);
		if (trx) {
			trx->mo.obj_inst.bts_nr = 0;
			trx->mo.obj_inst.trx_nr = foh->obj_inst.trx_nr;
			trx->mo.obj_inst.ts_nr = 0xff;
			mo = &trx->mo;
		}
		oml_tx_failure_event_rep(mo, OSMO_EVT_MAJ_UKWN_MSG, form, foh->obj_inst.bts_nr,
					 get_value_string(abis_nm_msgtype_names, foh->msg_type));

		return true;
	}

	return false;
}

static int down_fom(struct gsm_bts *bts, struct msgb *msg)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	struct gsm_bts_trx *trx;
	int ret;

	if (msgb_l2len(msg) < sizeof(*foh)) {
		LOGP(DOML, LOGL_NOTICE, "Formatted O&M message too short\n");
		trx = gsm_bts_trx_num(bts, foh->obj_inst.trx_nr);
		if (trx) {
			trx->mo.obj_inst.bts_nr = 0;
			trx->mo.obj_inst.trx_nr = foh->obj_inst.trx_nr;
			trx->mo.obj_inst.ts_nr = 0xff;
			oml_tx_failure_event_rep(&trx->mo, OSMO_EVT_MAJ_UKWN_MSG,
						 "Formatted O&M message too short");
		}
		return -EIO;
	}

	if (report_bts_number_incorrect(bts, foh, true))
		return oml_fom_ack_nack(msg, NM_NACK_BTSNR_UNKN);

	switch (foh->msg_type) {
	case NM_MT_SET_BTS_ATTR:
		ret = oml_rx_set_bts_attr(bts, msg);
		break;
	case NM_MT_SET_RADIO_ATTR:
		trx = gsm_bts_trx_num(bts, foh->obj_inst.trx_nr);
		if (!trx)
			return oml_fom_ack_nack(msg, NM_NACK_TRXNR_UNKN);
		ret = oml_rx_set_radio_attr(trx, msg);
		break;
	case NM_MT_SET_CHAN_ATTR:
		trx = gsm_bts_trx_num(bts, foh->obj_inst.trx_nr);
		if (!trx)
			return oml_fom_ack_nack(msg, NM_NACK_TRXNR_UNKN);
		if (foh->obj_inst.ts_nr >= ARRAY_SIZE(trx->ts))
			return oml_fom_ack_nack(msg, NM_NACK_OBJINST_UNKN);
		ret = oml_rx_set_chan_attr(&trx->ts[foh->obj_inst.ts_nr], msg);
		break;
	case NM_MT_OPSTART:
		ret = oml_rx_opstart(bts, msg);
		break;
	case NM_MT_CHG_ADM_STATE:
		ret = oml_rx_chg_adm_state(bts, msg);
		break;
	case NM_MT_IPACC_SET_ATTR:
		ret = oml_ipa_set_attr(bts, msg);
		break;
	case NM_MT_GET_ATTR:
		ret = oml_rx_get_attr(bts, msg);
		break;
	default:
		LOGP(DOML, LOGL_INFO, "unknown Formatted O&M msg_type 0x%02x\n",
			foh->msg_type);
		trx = gsm_bts_trx_num(bts, foh->obj_inst.trx_nr);
		if (trx) {
			trx->mo.obj_inst.bts_nr = 0;
			trx->mo.obj_inst.trx_nr = foh->obj_inst.trx_nr;
			trx->mo.obj_inst.ts_nr = 0xff;
			oml_tx_failure_event_rep(&trx->mo, OSMO_EVT_MAJ_UKWN_MSG,
						 "unknown Formatted O&M "
						 "msg_type 0x%02x",
						 foh->msg_type);
		} else
			oml_tx_failure_event_rep(&bts->mo, OSMO_EVT_MAJ_UKWN_MSG,
						 "unknown Formatted O&M "
						 "msg_type 0x%02x",
						 foh->msg_type);
		ret = oml_fom_ack_nack(msg, NM_NACK_MSGTYPE_INVAL);
	}

	return ret;
}

/*
 * manufacturer related messages
 */

static int oml_ipa_mo_set_attr_nse(void *obj, struct tlv_parsed *tp)
{
	struct gsm_bts *bts = container_of(obj, struct gsm_bts, gprs.nse);

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_NSEI, 2))
		bts->gprs.nse.nsei =
			ntohs(tlvp_val16_unal(tp, NM_ATT_IPACC_NSEI));

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_NS_CFG, 7)) {
		memcpy(&bts->gprs.nse.timer,
		       TLVP_VAL(tp, NM_ATT_IPACC_NS_CFG), 7);
	}

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_BSSGP_CFG, 11)) {
		memcpy(&bts->gprs.cell.timer,
		       TLVP_VAL(tp, NM_ATT_IPACC_BSSGP_CFG), 11);
	}

	osmo_signal_dispatch(SS_GLOBAL, S_NEW_NSE_ATTR, bts);

	return 0;
}

static int oml_ipa_mo_set_attr_cell(void *obj, struct tlv_parsed *tp)
{
	struct gsm_bts *bts = container_of(obj, struct gsm_bts, gprs.cell);
	struct gprs_rlc_cfg *rlcc = &bts->gprs.cell.rlc_cfg;
	const uint8_t *cur;
	uint16_t _cur_s;

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_RAC, 1))
		bts->gprs.rac = *TLVP_VAL(tp, NM_ATT_IPACC_RAC);

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_GPRS_PAGING_CFG, 2)) {
		cur = TLVP_VAL(tp, NM_ATT_IPACC_GPRS_PAGING_CFG);
		rlcc->paging.repeat_time = cur[0] * 50;
		rlcc->paging.repeat_count = cur[1];
	}

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_BVCI, 2))
		bts->gprs.cell.bvci =
			ntohs(tlvp_val16_unal(tp, NM_ATT_IPACC_BVCI));

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_RLC_CFG, 9)) {
		cur = TLVP_VAL(tp, NM_ATT_IPACC_RLC_CFG);
		rlcc->parameter[RLC_T3142] = cur[0];
		rlcc->parameter[RLC_T3169] = cur[1];
		rlcc->parameter[RLC_T3191] = cur[2];
		rlcc->parameter[RLC_T3193] = cur[3];
		rlcc->parameter[RLC_T3195] = cur[4];
		rlcc->parameter[RLC_N3101] = cur[5];
		rlcc->parameter[RLC_N3103] = cur[6];
		rlcc->parameter[RLC_N3105] = cur[7];
		rlcc->parameter[CV_COUNTDOWN] = cur[8];
	}

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_CODING_SCHEMES, 2)) {
		int i;
		rlcc->cs_mask = 0;
		cur = TLVP_VAL(tp, NM_ATT_IPACC_CODING_SCHEMES);

		for (i = 0; i < 4; i++) {
			if (cur[0] & (1 << i))
				rlcc->cs_mask |= (1 << (GPRS_CS1+i));
		}
		if (cur[0] & 0x80)
			rlcc->cs_mask |= (1 << GPRS_MCS9);
		for (i = 0; i < 8; i++) {
			if (cur[1] & (1 << i))
				rlcc->cs_mask |= (1 << (GPRS_MCS1+i));
		}
	}

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_RLC_CFG_2, 5)) {
		cur = TLVP_VAL(tp, NM_ATT_IPACC_RLC_CFG_2);
		memcpy(&_cur_s, cur, 2);
		rlcc->parameter[T_DL_TBF_EXT] = ntohs(_cur_s) * 10;
		cur += 2;
		memcpy(&_cur_s, cur, 2);
		rlcc->parameter[T_UL_TBF_EXT] = ntohs(_cur_s) * 10;
		cur += 2;
		rlcc->initial_cs = *cur;
	}

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_RLC_CFG_3, 1)) {
		rlcc->initial_mcs = *TLVP_VAL(tp, NM_ATT_IPACC_RLC_CFG_3);
	}

	osmo_signal_dispatch(SS_GLOBAL, S_NEW_CELL_ATTR, bts);

	return 0;
}

static int oml_ipa_mo_set_attr_nsvc(struct gsm_bts_gprs_nsvc *nsvc,
				    struct tlv_parsed *tp)
{
	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_NSVCI, 2))
		nsvc->nsvci = ntohs(tlvp_val16_unal(tp, NM_ATT_IPACC_NSVCI));

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_NS_LINK_CFG, 8)) {
		const uint8_t *cur = TLVP_VAL(tp, NM_ATT_IPACC_NS_LINK_CFG);
		uint16_t _cur_s;
		uint32_t _cur_l;

		memcpy(&_cur_s, cur, 2);
		nsvc->remote_port = ntohs(_cur_s);
		cur += 2;
		memcpy(&_cur_l, cur, 4);
		nsvc->remote_ip = ntohl(_cur_l);
		cur += 4;
		memcpy(&_cur_s, cur, 2);
		nsvc->local_port = ntohs(_cur_s);
	}

	osmo_signal_dispatch(SS_GLOBAL, S_NEW_NSVC_ATTR, nsvc);

	return 0;
}

static int oml_ipa_mo_set_attr(struct gsm_bts *bts, struct gsm_abis_mo *mo,
				void *obj, struct tlv_parsed *tp)
{
	int rc;

	switch (mo->obj_class) {
	case NM_OC_GPRS_NSE:
		rc = oml_ipa_mo_set_attr_nse(obj, tp);
		break;
	case NM_OC_GPRS_CELL:
		rc = oml_ipa_mo_set_attr_cell(obj, tp);
		break;
	case NM_OC_GPRS_NSVC:
		rc = oml_ipa_mo_set_attr_nsvc(obj, tp);
		break;
	default:
		rc = NM_NACK_OBJINST_UNKN;
	}

	return rc;
}

static int oml_ipa_set_attr(struct gsm_bts *bts, struct msgb *msg)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	struct gsm_abis_mo *mo;
	struct tlv_parsed tp;
	void *obj;
	int rc;

	abis_nm_debugp_foh(DOML, foh);
	DEBUGPC(DOML, "Rx IPA SET ATTR\n");

	rc = oml_tlv_parse(&tp, foh->data, msgb_l3len(msg) - sizeof(*foh));
	if (rc < 0) {
		mo = gsm_objclass2mo(bts, foh->obj_class, &foh->obj_inst);
		if (!mo)
			return oml_fom_ack_nack(msg, NM_NACK_OBJINST_UNKN);
		oml_tx_failure_event_rep(mo, OSMO_EVT_MAJ_UNSUP_ATTR,
					 "New value for IPAC Set Attribute not "
					 "supported\n");
		return oml_fom_ack_nack(msg, NM_NACK_INCORR_STRUCT);
	}

	/* Resolve MO by obj_class/obj_inst */
	mo = gsm_objclass2mo(bts, foh->obj_class, &foh->obj_inst);
	obj = gsm_objclass2obj(bts, foh->obj_class, &foh->obj_inst);
	if (!mo || !obj)
		return oml_fom_ack_nack(msg, NM_NACK_OBJINST_UNKN);

	rc = oml_ipa_mo_set_attr(bts, mo, obj, &tp);

	return oml_fom_ack_nack(msg, rc);
}

static int rx_oml_ipa_rsl_connect(struct gsm_bts_trx *trx, struct msgb *msg,
				  struct tlv_parsed *tp)
{
	struct e1inp_sign_link *oml_link = trx->bts->oml_link;
	uint16_t port = IPA_TCP_PORT_RSL;
	uint32_t ip = get_signlink_remote_ip(oml_link);
	struct in_addr in;
	int rc;

	uint8_t stream_id = 0;

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_DST_IP, 4)) {
		ip = ntohl(tlvp_val32_unal(tp, NM_ATT_IPACC_DST_IP));
	}
	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_DST_IP_PORT, 2)) {
		port = ntohs(tlvp_val16_unal(tp, NM_ATT_IPACC_DST_IP_PORT));
	}
	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_STREAM_ID, 1)) {
		stream_id = *TLVP_VAL(tp, NM_ATT_IPACC_STREAM_ID);
	}

	in.s_addr = htonl(ip);
	LOGP(DOML, LOGL_INFO, "Rx IPA RSL CONNECT IP=%s PORT=%u STREAM=0x%02x\n", 
		inet_ntoa(in), port, stream_id);

	if (trx->bts->variant == BTS_OSMO_OMLDUMMY) {
		rc = 0;
		LOGP(DOML, LOGL_NOTICE, "Not connecting RSL in OML-DUMMY!\n");
	} else
		rc = e1inp_ipa_bts_rsl_connect_n(oml_link->ts->line, inet_ntoa(in), port, trx->nr);
	if (rc < 0) {
		LOGP(DOML, LOGL_ERROR, "Error in abis_open(RSL): %d\n", rc);
		return oml_fom_ack_nack(msg, NM_NACK_CANT_PERFORM);
	}

	return oml_fom_ack_nack(msg, 0);
}

static int down_mom(struct gsm_bts *bts, struct msgb *msg)
{
	struct abis_om_hdr *oh = msgb_l2(msg);
	struct abis_om_fom_hdr *foh;
	struct gsm_bts_trx *trx;
	uint8_t idstrlen = oh->data[0];
	struct tlv_parsed tp;
	int ret;

	if (msgb_l2len(msg) < sizeof(*foh)) {
		LOGP(DOML, LOGL_NOTICE, "Manufacturer O&M message too short\n");
		return -EIO;
	}

	if (strncmp((char *)&oh->data[1], abis_nm_ipa_magic, idstrlen)) {
		LOGP(DOML, LOGL_ERROR, "Manufacturer OML message != ipaccess not supported\n");
		return -EINVAL;
	}

	msg->l3h = oh->data + 1 + idstrlen;
	foh = (struct abis_om_fom_hdr *) msg->l3h;

	if (report_bts_number_incorrect(bts, foh, false))
		return oml_fom_ack_nack(msg, NM_NACK_BTSNR_UNKN);

	ret = oml_tlv_parse(&tp, foh->data, oh->length - sizeof(*foh));
	if (ret < 0) {
		LOGP(DOML, LOGL_ERROR, "TLV parse error %d\n", ret);
		return oml_fom_ack_nack(msg, NM_NACK_BTSNR_UNKN);
	}

	abis_nm_debugp_foh(DOML, foh);
	DEBUGPC(DOML, "Rx IPACCESS(0x%02x): ", foh->msg_type);

	switch (foh->msg_type) {
	case NM_MT_IPACC_RSL_CONNECT:
		trx = gsm_bts_trx_num(bts, foh->obj_inst.trx_nr);
		ret = rx_oml_ipa_rsl_connect(trx, msg, &tp);
		break;
	case NM_MT_IPACC_SET_ATTR:
		ret = oml_ipa_set_attr(bts, msg);
		break;
	default:
		LOGP(DOML, LOGL_INFO, "Manufacturer Formatted O&M msg_type 0x%02x\n",
			foh->msg_type);
		ret = oml_fom_ack_nack(msg, NM_NACK_MSGTYPE_INVAL);
	}

	return ret;
}

/* incoming OML message from BSC */
int down_oml(struct gsm_bts *bts, struct msgb *msg)
{
	struct abis_om_hdr *oh = msgb_l2(msg);
	int ret = 0;

	if (msgb_l2len(msg) < 1) {
		LOGP(DOML, LOGL_NOTICE, "OML message too short\n");
		msgb_free(msg);
		return -EIO;
	}
	msg->l3h = (unsigned char *)oh + sizeof(*oh);

	switch (oh->mdisc) {
	case ABIS_OM_MDISC_FOM:
		if (msgb_l2len(msg) < sizeof(*oh)) {
			LOGP(DOML, LOGL_NOTICE, "Formatted O&M message too short\n");
			ret = -EIO;
			break;
		}
		ret = down_fom(bts, msg);
		break;
	case ABIS_OM_MDISC_MANUF:
		if (msgb_l2len(msg) < sizeof(*oh)) {
			LOGP(DOML, LOGL_NOTICE, "Manufacturer O&M message too short\n");
			ret = -EIO;
			break;
		}
		ret = down_mom(bts, msg);
		break;
	default:
		LOGP(DOML, LOGL_NOTICE, "unknown OML msg_discr 0x%02x\n",
			oh->mdisc);
		ret = -EINVAL;
	}

	msgb_free(msg);

	return ret;
}

static int handle_fail_sig(unsigned int subsys, unsigned int signal, void *handle,
			   void *signal_data)
{
	if (signal_data)
		oml_tx_failure_event_rep(handle, signal, "%s", signal_data);
	else
		oml_tx_failure_event_rep(handle, signal, "");

	return 0;
}

int oml_init(struct gsm_abis_mo *mo)
{
	DEBUGP(DOML, "Initializing OML attribute definitions\n");
	tlv_def_patch(&abis_nm_att_tlvdef_ipa_local, &abis_nm_att_tlvdef_ipa);
	tlv_def_patch(&abis_nm_att_tlvdef_ipa_local, &abis_nm_att_tlvdef);
	osmo_signal_register_handler(SS_FAIL, handle_fail_sig, mo);

	return 0;
}
