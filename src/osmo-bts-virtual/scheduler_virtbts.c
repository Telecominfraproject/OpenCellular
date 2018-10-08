/* Scheduler worker functiosn for Virtua OsmoBTS */

/* (C) 2015-2017 by Harald Welte <laforge@gnumonks.org>
 * (C) 2017 Sebastian Stumpf <sebastian.stumpf87@googlemail.com>
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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>

#include <osmocom/core/msgb.h>
#include <osmocom/core/talloc.h>
#include <osmocom/core/bits.h>
#include <osmocom/core/gsmtap_util.h>
#include <osmocom/core/gsmtap.h>
#include <osmocom/gsm/rsl.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/phy_link.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/rsl.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/l1sap.h>
#include <osmo-bts/amr.h>
#include <osmo-bts/scheduler.h>
#include <osmo-bts/scheduler_backend.h>
#include "virtual_um.h"
#include "l1_if.h"

#define MODULO_HYPERFRAME 0

static const char *gsmtap_hdr_stringify(const struct gsmtap_hdr *gh)
{
	static char buf[256];
	snprintf(buf, sizeof(buf), "(ARFCN=%u, ts=%u, ss=%u, type=%u/%u)",
		 gh->arfcn & GSMTAP_ARFCN_MASK, gh->timeslot, gh->sub_slot, gh->type, gh->sub_type);
	return buf;
}

/**
 * Send a message over the virtual um interface.
 * This will at first wrap the msg with a GSMTAP header and then write it to the declared multicast socket.
 * TODO: we might want to remove unused argument uint8_t tn
 */
static void tx_to_virt_um(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
			  enum trx_chan_type chan, struct msgb *msg)
{
	const struct trx_chan_desc *chdesc = &trx_chan_desc[chan];
	struct msgb *outmsg;			/* msg to send with gsmtap header prepended */
	uint16_t arfcn = l1t->trx->arfcn;	/* ARFCN of the tranceiver the message is send with */
	uint8_t signal_dbm = 63;		/* signal strength, 63 is best */
	uint8_t snr = 63;			/* signal noise ratio, 63 is best */
	uint8_t *data = msgb_l2(msg);		/* data to transmit (whole message without l1 header) */
	uint8_t data_len = msgb_l2len(msg);	/* length of data */
	uint8_t rsl_chantype;			/* RSL chan type (TS 08.58, 9.3.1) */
	uint8_t subslot;			/* multiframe subslot to send msg in (tch -> 0-26, bcch/ccch -> 0-51) */
	uint8_t timeslot;			/* TDMA timeslot to send in (0-7) */
	uint8_t gsmtap_chantype;		/* the GSMTAP channel */

	rsl_dec_chan_nr(chdesc->chan_nr, &rsl_chantype, &subslot, &timeslot);
	/* the timeslot is not encoded in the chan_nr of the chdesc, and so has to be overwritten */
	timeslot = tn;
	/* in Osmocom, AGCH is only sent on ccch block 0. no idea why. this seems to cause false GSMTAP channel
	 * types for agch and pch. */
	if (rsl_chantype == RSL_CHAN_PCH_AGCH &&
	    l1sap_fn2ccch_block(fn) >= num_agch(l1t->trx, "PH-DATA-REQ"))
		gsmtap_chantype = GSMTAP_CHANNEL_PCH;
	else
		gsmtap_chantype = chantype_rsl2gsmtap(rsl_chantype, chdesc->link_id); /* the logical channel type */

#if MODULO_HYPERFRAME
	/* Restart fn after every superframe (26 * 51 frames) to simulate hyperframe overflow each 6 seconds. */
	fn %= 26 * 51;
#endif

	outmsg = gsmtap_makemsg(arfcn, timeslot, gsmtap_chantype, subslot, fn, signal_dbm, snr, data, data_len);

	if (outmsg) {
		struct phy_instance *pinst = trx_phy_instance(l1t->trx);
		struct gsmtap_hdr *gh = (struct gsmtap_hdr *)msgb_data(outmsg);
		int rc;

		rc = virt_um_write_msg(pinst->phy_link->u.virt.virt_um, outmsg);
		if (rc < 0)
			LOGL1S(DL1P, LOGL_ERROR, l1t, tn, chan, fn,
				"%s GSMTAP msg could not send to virtual Um\n", gsmtap_hdr_stringify(gh));
		else if (rc == 0)
			bts_shutdown(l1t->trx->bts, "VirtPHY write socket died\n");
		else
			LOGL1S(DL1P, LOGL_DEBUG, l1t, tn, chan, fn,
				"%s Sending GSMTAP message to virtual Um\n", gsmtap_hdr_stringify(gh));
	} else
		LOGL1S(DL1P, LOGL_ERROR, l1t, tn, chan, fn, "GSMTAP msg could not be created!\n");

	/* free incoming message */
	msgb_free(msg);
}

/*
 * TX on downlink
 */

/* an IDLE burst returns nothing. on C0 it is replaced by dummy burst */
ubit_t *tx_idle_fn(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan, uint8_t bid, uint16_t *nbits)
{
	return NULL;
}

ubit_t *tx_fcch_fn(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan, uint8_t bid, uint16_t *nbits)
{
	return NULL;
}

ubit_t *tx_sch_fn(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan, uint8_t bid, uint16_t *nbits)
{
	return NULL;
}

ubit_t *tx_data_fn(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan, uint8_t bid, uint16_t *nbits)
{
	struct msgb *msg;

	if (bid > 0)
		return NULL;

	/* get mac block from queue */
	msg = _sched_dequeue_prim(l1t, tn, fn, chan);
	if (!msg) {
		LOGL1S(DL1P, LOGL_INFO, l1t, tn, chan, fn, "has not been served !! No prim\n");
		return NULL;
	}

	/* check validity of message */
	if (msgb_l2len(msg) != GSM_MACBLOCK_LEN) {
		LOGL1S(DL1P, LOGL_FATAL, l1t, tn, chan, fn, "Prim not 23 bytes, please FIX! (len=%d)\n",
			msgb_l2len(msg));
		/* free message */
		msgb_free(msg);
		return NULL;
	}

	/* transmit the msg received on dl from bsc to layer1 (virt Um) */
	tx_to_virt_um(l1t, tn, fn, chan, msg);

	return NULL;
}

ubit_t *tx_pdtch_fn(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan, uint8_t bid, uint16_t *nbits)
{
	struct msgb *msg = NULL; /* make GCC happy */

	if (bid > 0)
		return NULL;

	/* get mac block from queue */
	msg = _sched_dequeue_prim(l1t, tn, fn, chan);
	if (!msg) {
		LOGL1S(DL1P, LOGL_INFO, l1t, tn, chan, fn, "has not been served !! No prim\n");
		return NULL;
	}

	tx_to_virt_um(l1t, tn, fn, chan, msg);

	return NULL;
}

static void tx_tch_common(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan, uint8_t bid, struct msgb **_msg_tch,
	struct msgb **_msg_facch, int codec_mode_request)
{
	struct l1sched_ts *l1ts = l1sched_trx_get_ts(l1t, tn);
	struct msgb *msg1, *msg2, *msg_tch = NULL, *msg_facch = NULL;
	struct l1sched_chan_state *chan_state = &l1ts->chan_state[chan];
	uint8_t rsl_cmode = chan_state->rsl_cmode;
	uint8_t tch_mode = chan_state->tch_mode;
	struct osmo_phsap_prim *l1sap;
#if 0
	/* handle loss detection of received TCH frames */
	if (rsl_cmode == RSL_CMOD_SPD_SPEECH
	 && ++(chan_state->lost_frames) > 5) {
		uint8_t tch_data[GSM_FR_BYTES];
		int len;

		LOGL1S(DL1P, LOGL_NOTICE, l1t, tn, chan, fn, "Missing TCH bursts detected, sending "
			"BFI for %s\n", trx_chan_desc[chan].name);

		/* indicate bad frame */
		switch (tch_mode) {
		case GSM48_CMODE_SPEECH_V1: /* FR / HR */
			if (chan != TRXC_TCHF) { /* HR */
				tch_data[0] = 0x70; /* F = 0, FT = 111 */
				memset(tch_data + 1, 0, 14);
				len = 15;
				break;
			}
			memset(tch_data, 0, GSM_FR_BYTES);
			len = GSM_FR_BYTES;
			break;
		case GSM48_CMODE_SPEECH_EFR: /* EFR */
			if (chan != TRXC_TCHF)
				goto inval_mode1;
			memset(tch_data, 0, GSM_EFR_BYTES);
			len = GSM_EFR_BYTES;
			break;
		case GSM48_CMODE_SPEECH_AMR: /* AMR */
			len = amr_compose_payload(tch_data,
				chan_state->codec[chan_state->dl_cmr],
				chan_state->codec[chan_state->dl_ft], 1);
			if (len < 2)
				break;
			memset(tch_data + 2, 0, len - 2);
			_sched_compose_tch_ind(l1t, tn, 0, chan, tch_data, len);
			break;
		default:
inval_mode1:
			LOGP(DL1P, LOGL_ERROR, "TCH mode invalid, please "
				"fix!\n");
			len = 0;
		}
		if (len)
			_sched_compose_tch_ind(l1t, tn, 0, chan, tch_data, len);
	}
#endif

	/* get frame and unlink from queue */
	msg1 = _sched_dequeue_prim(l1t, tn, fn, chan);
	msg2 = _sched_dequeue_prim(l1t, tn, fn, chan);
	if (msg1) {
		l1sap = msgb_l1sap_prim(msg1);
		if (l1sap->oph.primitive == PRIM_TCH) {
			msg_tch = msg1;
			if (msg2) {
				l1sap = msgb_l1sap_prim(msg2);
				if (l1sap->oph.primitive == PRIM_TCH) {
					LOGL1S(DL1P, LOGL_FATAL, l1t, tn, chan, fn,
						"TCH twice, please FIX! ");
					msgb_free(msg2);
				} else
					msg_facch = msg2;
			}
		} else {
			msg_facch = msg1;
			if (msg2) {
				l1sap = msgb_l1sap_prim(msg2);
				if (l1sap->oph.primitive != PRIM_TCH) {
					LOGL1S(DL1P, LOGL_FATAL, l1t, tn, chan, fn,
						"FACCH twice, please FIX! ");
					msgb_free(msg2);
				} else
					msg_tch = msg2;
			}
		}
	} else if (msg2) {
		l1sap = msgb_l1sap_prim(msg2);
		if (l1sap->oph.primitive == PRIM_TCH)
			msg_tch = msg2;
		else
			msg_facch = msg2;
	}

	/* check validity of message */
	if (msg_facch && msgb_l2len(msg_facch) != GSM_MACBLOCK_LEN) {
		LOGL1S(DL1P, LOGL_FATAL, l1t, tn, chan, fn, "Prim not 23 bytes, please FIX! (len=%d)\n",
			msgb_l2len(msg_facch));
		/* free message */
		msgb_free(msg_facch);
		msg_facch = NULL;
	}

	/* check validity of message, get AMR ft and cmr */
	if (!msg_facch && msg_tch) {
		int len;
#if 0
		uint8_t bfi, cmr_codec, ft_codec;
		int cmr, ft, i;
#endif

		if (rsl_cmode != RSL_CMOD_SPD_SPEECH) {
			LOGL1S(DL1P, LOGL_NOTICE, l1t, tn, chan, fn, "Dropping speech frame, "
				"because we are not in speech mode\n");
			goto free_bad_msg;
		}

		switch (tch_mode) {
		case GSM48_CMODE_SPEECH_V1: /* FR / HR */
			if (chan != TRXC_TCHF) { /* HR */
				len = 15;
				if (msgb_l2len(msg_tch) >= 1
				 && (msg_tch->l2h[0] & 0xf0) != 0x00) {
					LOGL1S(DL1P, LOGL_NOTICE, l1t, tn, chan, fn,
						"Transmitting 'bad HR frame'\n");
					goto free_bad_msg;
				}
				break;
			}
			len = GSM_FR_BYTES;
			if (msgb_l2len(msg_tch) >= 1
			 && (msg_tch->l2h[0] >> 4) != 0xd) {
				LOGL1S(DL1P, LOGL_NOTICE, l1t, tn, chan, fn,
					"Transmitting 'bad FR frame'\n");
				goto free_bad_msg;
			}
			break;
		case GSM48_CMODE_SPEECH_EFR: /* EFR */
			if (chan != TRXC_TCHF)
				goto inval_mode2;
			len = GSM_EFR_BYTES;
			if (msgb_l2len(msg_tch) >= 1
			 && (msg_tch->l2h[0] >> 4) != 0xc) {
				LOGL1S(DL1P, LOGL_NOTICE, l1t, tn, chan, fn,
					"Transmitting 'bad EFR frame'\n");
				goto free_bad_msg;
			}
			break;
		case GSM48_CMODE_SPEECH_AMR: /* AMR */
#if 0
			len = amr_decompose_payload(msg_tch->l2h,
				msgb_l2len(msg_tch), &cmr_codec, &ft_codec,
				&bfi);
			cmr = -1;
			ft = -1;
			for (i = 0; i < chan_state->codecs; i++) {
				if (chan_state->codec[i] == cmr_codec)
					cmr = i;
				if (chan_state->codec[i] == ft_codec)
					ft = i;
			}
			if (cmr >= 0) { /* new request */
				chan_state->dl_cmr = cmr;
				/* disable AMR loop */
				trx_loop_amr_set(chan_state, 0);
			} else {
				/* enable AMR loop */
				trx_loop_amr_set(chan_state, 1);
			}
			if (ft < 0) {
				LOGL1S(DL1P, LOGL_ERROR, l1t, tn, chan, fn,
					"Codec (FT = %d) of RTP frame not in list. ", ft_codec);
				goto free_bad_msg;
			}
			if (codec_mode_request && chan_state->dl_ft != ft) {
				LOGL1S(DL1P, LOGL_NOTICE, l1t, tn, chan, fn,
					"Codec (FT = %d) of RTP cannot be changed now, but in "
					"next frame\n", ft_codec);
				goto free_bad_msg;
			}
			chan_state->dl_ft = ft;
			if (bfi) {
				LOGL1S(DL1P, LOGL_NOTICE, l1t, tn, chan, fn,
					"Transmitting 'bad AMR frame'\n");
				goto free_bad_msg;
			}
#else
			LOGL1S(DL1P, LOGL_ERROR, l1t, tn, chan, fn, "AMR not supported!\n");
			goto free_bad_msg;
#endif
			break;
		default:
inval_mode2:
			LOGL1S(DL1P, LOGL_ERROR, l1t, tn, chan, fn, "TCH mode invalid, please fix!\n");
			goto free_bad_msg;
		}
		if (len < 0) {
			LOGL1S(DL1P, LOGL_ERROR, l1t, tn, chan, fn, "Cannot send invalid AMR payload\n");
			goto free_bad_msg;
		}
		if (msgb_l2len(msg_tch) != len) {
			LOGL1S(DL1P, LOGL_ERROR, l1t, tn, chan, fn, "Cannot send payload with "
				"invalid length! (expecing %d, received %d)\n", len, msgb_l2len(msg_tch));
free_bad_msg:
			/* free message */
			msgb_free(msg_tch);
			msg_tch = NULL;
			goto send_frame;
		}
	}

send_frame:
	*_msg_tch = msg_tch;
	*_msg_facch = msg_facch;
}

ubit_t *tx_tchf_fn(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan, uint8_t bid, uint16_t *nbits)
{
	struct msgb *msg_tch = NULL, *msg_facch = NULL;

	if (bid > 0)
		return NULL;

	tx_tch_common(l1t, tn, fn, chan, bid, &msg_tch, &msg_facch,
		(((fn + 4) % 26) >> 2) & 1);

	/* no message at all */
	if (!msg_tch && !msg_facch) {
		LOGL1S(DL1P, LOGL_INFO, l1t, tn, chan, fn, "has not been served !! No prim\n");
		goto send_burst;
	}

	if (msg_facch) {
		tx_to_virt_um(l1t, tn, fn, chan, msg_facch);
		msgb_free(msg_tch);
	} else
		tx_to_virt_um(l1t, tn, fn, chan, msg_tch);

send_burst:

	return NULL;
}

ubit_t *tx_tchh_fn(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan, uint8_t bid, uint16_t *nbits)
{
	struct msgb *msg_tch = NULL, *msg_facch = NULL;
	struct l1sched_ts *l1ts = l1sched_trx_get_ts(l1t, tn);
	struct l1sched_chan_state *chan_state = &l1ts->chan_state[chan];
	//uint8_t tch_mode = chan_state->tch_mode;

	/* send burst, if we already got a frame */
	if (bid > 0)
		return NULL;

	/* get TCH and/or FACCH */
	tx_tch_common(l1t, tn, fn, chan, bid, &msg_tch, &msg_facch,
		(((fn + 4) % 26) >> 2) & 1);

	/* check for FACCH alignment */
	if (msg_facch && ((((fn + 4) % 26) >> 2) & 1)) {
		LOGL1S(DL1P, LOGL_ERROR, l1t, tn, chan, fn, "Cannot transmit FACCH starting on "
			"even frames, please fix RTS!\n");
		msgb_free(msg_facch);
		msg_facch = NULL;
	}

	/* no message at all */
	if (!msg_tch && !msg_facch && !chan_state->dl_ongoing_facch) {
		LOGL1S(DL1P, LOGL_INFO, l1t, tn, chan, fn, "has not been served !! No prim\n");
		goto send_burst;
	}

	if (msg_facch) {
		tx_to_virt_um(l1t, tn, fn, chan, msg_facch);
		msgb_free(msg_tch);
	} else
		tx_to_virt_um(l1t, tn, fn, chan, msg_tch);

send_burst:
	return NULL;
}


/***********************************************************************
 * RX on uplink (indication to upper layer)
 ***********************************************************************/

/* we don't use those functions, as we feed the MAC frames from GSMTAP
 * directly into the L1SAP, bypassing the TDMA multiplex logic oriented
 * towards receiving bursts */

int rx_rach_fn(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan, uint8_t bid, sbit_t *bits, uint16_t nbits,
	int8_t rssi, int16_t toa256)
{
	return 0;
}

/*! \brief a single burst was received by the PHY, process it */
int rx_data_fn(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan, uint8_t bid, sbit_t *bits, uint16_t nbits,
	int8_t rssi, int16_t toa256)
{
	return 0;
}

int rx_pdtch_fn(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan, uint8_t bid, sbit_t *bits, uint16_t nbits,
	int8_t rssi, int16_t toa256)
{
	return 0;
}

int rx_tchf_fn(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan, uint8_t bid, sbit_t *bits, uint16_t nbits,
	int8_t rssi, int16_t toa256)
{
	return 0;
}

int rx_tchh_fn(struct l1sched_trx *l1t, uint8_t tn, uint32_t fn,
	enum trx_chan_type chan, uint8_t bid, sbit_t *bits, uint16_t nbits,
	int8_t rssi, int16_t toa256)
{
	return 0;
}

void _sched_act_rach_det(struct l1sched_trx *l1t, uint8_t tn, uint8_t ss, int activate)
{
}

/***********************************************************************
 * main scheduler function
 ***********************************************************************/

#define RTS_ADVANCE		5	/* about 20ms */
#define FRAME_DURATION_uS	4615

static int vbts_sched_fn(struct gsm_bts *bts, uint32_t fn)
{
	struct gsm_bts_trx *trx;

	/* send time indication */
	/* update model with new frame number, lot of stuff happening, measurements of timeslots */
	/* saving GSM time in BTS model, and more */
	l1if_mph_time_ind(bts, fn);

	/* advance the frame number? */
	llist_for_each_entry(trx, &bts->trx_list, list) {
		struct phy_instance *pinst = trx_phy_instance(trx);
		struct l1sched_trx *l1t = &pinst->u.virt.sched;
		int tn;
		uint16_t nbits;

		/* do for each of the 8 timeslots */
		for (tn = 0; tn < ARRAY_SIZE(l1t->ts); tn++) {
			/* Generate RTS indication to higher layers */
			/* This will basically do 2 things (check l1_if:bts_model_l1sap_down):
			 * 1) Get pending messages from layer 2 (from the lapdm queue)
			 * 2) Process the messages
			 *    --> Handle and process non-transparent RSL-Messages (activate channel, )
			 *    --> Forward transparent RSL-DATA-Messages to the ms by appending them to
			 *        the l1-dl-queue */
			_sched_rts(l1t, tn, (fn + RTS_ADVANCE) % GSM_HYPERFRAME);
			/* schedule transmit backend functions */
			/* Process data in the l1-dlqueue and forward it
			 * to MS */
			/* the returned bits are not used here, the routines called will directly forward their
			 * bits to the virt Um */
			_sched_dl_burst(l1t, tn, fn, &nbits);
		}
	}

	return 0;
}

static void vbts_fn_timer_cb(void *data)
{
	struct gsm_bts *bts = data;
	struct timeval tv_now;
	struct timeval *tv_clock = &bts->vbts.tv_clock;
	int32_t elapsed_us;

	gettimeofday(&tv_now, NULL);

	/* check how much time elapsed till the last timer callback call.
	 * this value should be about 4.615 ms (a bit greater) as this is the scheduling interval */
	elapsed_us = (tv_now.tv_sec - tv_clock->tv_sec) * 1000000
	                + (tv_now.tv_usec - tv_clock->tv_usec);

	/* not so good somehow a lot of time passed between two timer callbacks */
	if (elapsed_us > 2 *FRAME_DURATION_uS)
		LOGP(DL1P, LOGL_NOTICE, "vbts_fn_timer_cb after %d us\n", elapsed_us);

	/* schedule the current frame/s (fn = frame number)
	 * this loop will be called at least once, but can also be executed
	 * multiple times if more than one frame duration (4615us) passed till the last callback */
	while (elapsed_us > FRAME_DURATION_uS / 2) {
		const struct timeval tv_frame = {
			.tv_sec = 0,
			.tv_usec = FRAME_DURATION_uS,
		};
		timeradd(tv_clock, &tv_frame, tv_clock);
		/* increment the frame number in the BTS model instance */
		bts->vbts.last_fn = (bts->vbts.last_fn + 1) % GSM_HYPERFRAME;
		vbts_sched_fn(bts, bts->vbts.last_fn);
		elapsed_us -= FRAME_DURATION_uS;
	}

	/* re-schedule the timer */
	/* timer is set to frame duration - elapsed time to guarantee that this cb method will be
	 * periodically executed every 4.615ms */
	osmo_timer_schedule(&bts->vbts.fn_timer, 0, FRAME_DURATION_uS - elapsed_us);
}

int vbts_sched_start(struct gsm_bts *bts)
{
	LOGP(DL1P, LOGL_NOTICE, "starting VBTS scheduler\n");

	memset(&bts->vbts.fn_timer, 0, sizeof(bts->vbts.fn_timer));
	bts->vbts.fn_timer.cb = vbts_fn_timer_cb;
	bts->vbts.fn_timer.data = bts;

	gettimeofday(&bts->vbts.tv_clock, NULL);
	/* trigger the first timer after 4615us (a frame duration) */
	osmo_timer_schedule(&bts->vbts.fn_timer, 0, FRAME_DURATION_uS);

	return 0;
}
