/* Traffic Channel (TCH) part of osmo-bts OCTPHY integration */

/* Copyright (c) 2014 Octasic Inc. All rights reserved.
 * Copyright (c) 2015 Harald Welte <laforge@gnumonks.org>
 *
 * based on a copy of osmo-bts-sysmo/l1_tch.c, which is
 * Copyright (C) 2011-2013 by Harald Welte <laforge@gnumonks.org>
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
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <osmocom/core/bits.h>

#include <osmo-bts/logging.h>
#include <osmo-bts/gsm_data.h>
#include <osmo-bts/l1sap.h>

#include "l1_if.h"

struct msgb *l1_to_rtppayload_fr(uint8_t *l1_payload, uint8_t payload_len)
{
	struct msgb *msg;
	uint8_t *cur;

	msg = msgb_alloc_headroom(1024, 128, "L1P-to-RTP");
	if (!msg)
		return NULL;

	/* step1: reverse the bit-order of each payload byte */
	osmo_revbytebits_buf(l1_payload, payload_len);

	cur = msgb_put(msg, GSM_FR_BYTES);

	/* step2: we need to shift the entire L1 payload by 4 bits right */
	osmo_nibble_shift_right(cur, l1_payload, GSM_FR_BITS / 4);

	cur[0] |= 0xD0;

	return msg;
}

/*! \brief convert GSM-FR from RTP payload to L1 format
 *  \param[out] l1_payload payload part of L1 buffer
 *  \param[in] rtp_payload pointer to RTP payload data
 *  \param[in] payload_len length of \a rtp_payload
 *  \returns number of \a l1_payload bytes filled
 */
int rtppayload_to_l1_fr(uint8_t *l1_payload, const uint8_t *rtp_payload,
			unsigned int payload_len)
{
	/* step2: we need to shift the RTP payload left by one nibble */
	osmo_nibble_shift_left_unal(l1_payload, rtp_payload, GSM_FR_BITS / 4);

	/* step1: reverse the bit-order of each payload byte */
	osmo_revbytebits_buf(l1_payload, payload_len);
	return GSM_FR_BYTES;
}

static struct msgb *l1_to_rtppayload_hr(uint8_t *l1_payload, uint8_t payload_len)
{
	struct msgb *msg;
	uint8_t *cur;

	msg = msgb_alloc_headroom(1024, 128, "L1P-to-RTP");
	if (!msg)
		return NULL;

	if (payload_len != GSM_HR_BYTES) {
		LOGP(DL1P, LOGL_ERROR, "L1 HR frame length %u != expected %u\n",
			payload_len, GSM_HR_BYTES);
		return NULL;
	}

	cur = msgb_put(msg, GSM_HR_BYTES);
	memcpy(cur, l1_payload, GSM_HR_BYTES);

	/* reverse the bit-order of each payload byte */
	osmo_revbytebits_buf(cur, GSM_HR_BYTES);

	return msg;
}

/*! \brief convert GSM-FR from RTP payload to L1 format
 *  \param[out] l1_payload payload part of L1 buffer
 *  \param[in] rtp_payload pointer to RTP payload data
 *  \param[in] payload_len length of \a rtp_payload
 *  \returns number of \a l1_payload bytes filled
 */
static int rtppayload_to_l1_hr(uint8_t *l1_payload, const uint8_t *rtp_payload,
				unsigned int payload_len)
{

	if (payload_len != GSM_HR_BYTES) {
		LOGP(DL1P, LOGL_ERROR, "RTP HR frame length %u != expected %u\n",
			payload_len, GSM_HR_BYTES);
		return 0;
	}

	memcpy(l1_payload, rtp_payload, GSM_HR_BYTES);

	/* reverse the bit-order of each payload byte */
	osmo_revbytebits_buf(l1_payload, GSM_HR_BYTES);

	return GSM_HR_BYTES;
}


/* brief receive a traffic L1 primitive for a given lchan */
int l1if_tch_rx(struct gsm_bts_trx *trx, uint8_t chan_nr,
		tOCTVC1_GSM_MSG_TRX_LOGICAL_CHANNEL_DATA_INDICATION_EVT *
		data_ind)
{
	uint32_t payload_type = data_ind->Data.ulPayloadType;
	uint8_t *payload = data_ind->Data.abyDataContent;

	uint32_t fn = data_ind->Data.ulFrameNumber;
	uint16_t b_total = data_ind->MeasurementInfo.usBERTotalBitCnt;
	uint16_t b_error = data_ind->MeasurementInfo.usBERCnt;
	uint16_t ber10k = b_total ? BER_10K * b_error / b_total : 0;
	int16_t lqual_cb = 0; /* FIXME: check min_qual_norm! */

	uint8_t payload_len;
	struct msgb *rmsg = NULL;
	struct gsm_lchan *lchan =
	    &trx->ts[L1SAP_CHAN2TS(chan_nr)].lchan[l1sap_chan2ss(chan_nr)];

	if (data_ind->Data.ulDataLength < 1) {
		LOGPFN(DL1P, LOGL_DEBUG, fn, "chan_nr %d Rx Payload size 0\n", chan_nr);
		/* Push empty payload to upper layers */
		rmsg = msgb_alloc_headroom(256, 128, "L1P-to-RTP");
		return add_l1sap_header(trx, rmsg, lchan, chan_nr,
					data_ind->Data.ulFrameNumber,
					ber10k, lqual_cb);
	}

	payload_len = data_ind->Data.ulDataLength;

	switch (payload_type) {
	case cOCTVC1_GSM_PAYLOAD_TYPE_ENUM_FULL_RATE:
	case cOCTVC1_GSM_PAYLOAD_TYPE_ENUM_ENH_FULL_RATE:
		if (lchan->type != GSM_LCHAN_TCH_F)
			goto err_payload_match;
		break;
	case cOCTVC1_GSM_PAYLOAD_TYPE_ENUM_HALF_RATE:
		if (lchan->type != GSM_LCHAN_TCH_H)
			goto err_payload_match;
		break;
	case cOCTVC1_GSM_PAYLOAD_TYPE_ENUM_AMR_FULL_RATE:
	case cOCTVC1_GSM_PAYLOAD_TYPE_ENUM_AMR_HALF_RATE:
		if (lchan->type != GSM_LCHAN_TCH_H &&
		    lchan->type != GSM_LCHAN_TCH_F)
			goto err_payload_match;
		break;
	default:
		LOGPFN(DL1P, LOGL_NOTICE, fn, "%s Rx Payload Type %d is unsupported\n",
		     gsm_lchan_name(lchan), payload_type);
		break;
	}

	LOGPFN(DL1P, LOGL_DEBUG, fn, "%s Rx codec frame (%u): %s\n", gsm_lchan_name(lchan),
		payload_len, osmo_hexdump(payload, payload_len));

	switch (payload_type) {
	case cOCTVC1_GSM_PAYLOAD_TYPE_ENUM_FULL_RATE:
		rmsg = l1_to_rtppayload_fr(payload, payload_len);
		break;
	case cOCTVC1_GSM_PAYLOAD_TYPE_ENUM_HALF_RATE:
		rmsg = l1_to_rtppayload_hr(payload, payload_len);
		break;
	case cOCTVC1_GSM_PAYLOAD_TYPE_ENUM_ENH_FULL_RATE:
		/* Currently not supported */
#if 0
		rmsg = l1_to_rtppayload_efr(payload, payload_len);
		break;
#endif
	case cOCTVC1_GSM_PAYLOAD_TYPE_ENUM_AMR_FULL_RATE:
	case cOCTVC1_GSM_PAYLOAD_TYPE_ENUM_AMR_HALF_RATE:
		/* Currently not supported */
#if 0
		rmsg = l1_to_rtppayload_amr(payload, payload_len,
				&lchan->tch.amr_mr);
#else
		LOGPFN(DL1P, LOGL_ERROR, fn, "OctPHY only supports FR!\n");
		return -1;
#endif
		break;
	}

	if (rmsg)
		return add_l1sap_header(trx, rmsg, lchan, chan_nr,
					data_ind->Data.ulFrameNumber,
					ber10k, lqual_cb);

	return 0;

err_payload_match:
	LOGPFN(DL1P, LOGL_ERROR, fn, "%s Rx Payload Type %d incompatible with lchan\n",
	     gsm_lchan_name(lchan), payload_type);
	return -EINVAL;
}

#define RTP_MSGB_ALLOC_SIZE	512

/*! \brief function for incoming RTP via TCH.req
 *  \param rs RTP Socket
 *  \param[in] rtp_pl buffer containing RTP payload
 *  \param[in] rtp_pl_len length of \a rtp_pl
 *
 * This function prepares a msgb with a L1 PH-DATA.req primitive and
 * queues it into lchan->dl_tch_queue.
 *
 * Note that the actual L1 primitive header is not fully initialized
 * yet, as things like the frame number, etc. are unknown at the time we
 * pre-fill the primtive.
 */
void l1if_tch_encode(struct gsm_lchan *lchan, uint32_t *payload_type,
		     uint8_t *data, uint32_t *len, const uint8_t *rtp_pl,
		     unsigned int rtp_pl_len)
{
	uint8_t *l1_payload;
	int rc = -1;

	DEBUGP(DRTP, "%s RTP IN: %s\n", gsm_lchan_name(lchan),
	       osmo_hexdump(rtp_pl, rtp_pl_len));

	l1_payload = &data[0];

	switch (lchan->tch_mode) {
	case GSM48_CMODE_SPEECH_V1:
		if (lchan->type == GSM_LCHAN_TCH_F) {
			*payload_type = cOCTVC1_GSM_PAYLOAD_TYPE_ENUM_FULL_RATE;
			rc = rtppayload_to_l1_fr(l1_payload,
						 rtp_pl, rtp_pl_len);
		} else {
			*payload_type = cOCTVC1_GSM_PAYLOAD_TYPE_ENUM_HALF_RATE;
			/* Not supported currently */
			rc = rtppayload_to_l1_hr(l1_payload,
						 rtp_pl, rtp_pl_len);
		}
		break;
	case GSM48_CMODE_SPEECH_EFR:
		/* Not supported currently */
	case GSM48_CMODE_SPEECH_AMR:
		/* Not supported currently */
		LOGP(DRTP, LOGL_ERROR, "OctPHY only supports FR!\n");
	default:
		/* we don't support CSD modes */
		rc = -1;
		break;
	}

	if (rc < 0) {
		LOGP(DRTP, LOGL_ERROR, "%s unable to parse RTP payload\n",
		     gsm_lchan_name(lchan));
		return;
	}

	*len = rc;

	DEBUGP(DRTP, "%s RTP->L1: %s\n", gsm_lchan_name(lchan),
	       osmo_hexdump(data, *len));
}
