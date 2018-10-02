/* Loop control for OsmoBTS-TRX */

/* (C) 2013 by Andreas Eversberg <jolly@eversberg.eu>
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
#include <stdlib.h>
#include <errno.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/l1sap.h>
#include <osmocom/core/bits.h>

#include "trx_if.h"
#include "l1_if.h"
#include "loops.h"

#define MS_PWR_DBM(arfcn, lvl) ms_pwr_dbm(gsm_arfcn2band(arfcn), lvl)

/*
 * MS Power loop
 */

static int ms_power_diff(struct gsm_lchan *lchan, uint8_t chan_nr, int8_t diff)
{
	struct gsm_bts_trx *trx = lchan->ts->trx;
	uint16_t arfcn = trx->arfcn;
	int8_t new_power;

	new_power = lchan->ms_power_ctrl.current - (diff >> 1);

	if (diff == 0)
		return 0;

	if (new_power < 0)
		new_power = 0;

	// FIXME: to go above 1W, we need to know classmark of MS
	if (arfcn >= 512 && arfcn <= 885) {
		if (new_power > 15)
			new_power = 15;
	} else {
		if (new_power > 19)
			new_power = 19;
	}

	/* a higher value means a lower level (and vice versa) */
	if (new_power > lchan->ms_power_ctrl.current + MS_LOWER_MAX)
		new_power = lchan->ms_power_ctrl.current + MS_LOWER_MAX;
	else if (new_power < lchan->ms_power_ctrl.current - MS_RAISE_MAX)
		new_power = lchan->ms_power_ctrl.current - MS_RAISE_MAX;

	if (lchan->ms_power_ctrl.current == new_power) {
		LOGP(DLOOP, LOGL_INFO, "Keeping MS new_power of trx=%u "
			"chan_nr=0x%02x at control level %d (%d dBm)\n",
			trx->nr, chan_nr, new_power,
			MS_PWR_DBM(arfcn, new_power));

		return 0;
	}

	LOGP(DLOOP, LOGL_INFO, "%s MS new_power of trx=%u chan_nr=0x%02x from "
		"control level %d (%d dBm) to %d (%d dBm)\n",
		(diff > 0) ? "Raising" : "Lowering",
		trx->nr, chan_nr, lchan->ms_power_ctrl.current,
		MS_PWR_DBM(arfcn, lchan->ms_power_ctrl.current), new_power,
		MS_PWR_DBM(arfcn, new_power));

	lchan->ms_power_ctrl.current = new_power;

	return 0;
}

static int ms_power_val(struct l1sched_chan_state *chan_state, int8_t rssi)
{
	/* ignore inserted dummy frames, treat as lost frames */
	if (rssi < -127)
		return 0;

	LOGP(DLOOP, LOGL_DEBUG, "Got RSSI value of %d\n", rssi);

	chan_state->meas.rssi_count++;

	chan_state->meas.rssi_got_burst = 1;

	/* store and process RSSI */
	if (chan_state->meas.rssi_valid_count
					== ARRAY_SIZE(chan_state->meas.rssi))
		return 0;
	chan_state->meas.rssi[chan_state->meas.rssi_valid_count++] = rssi;
	chan_state->meas.rssi_valid_count++;

	return 0;
}

static int ms_power_clock(struct gsm_lchan *lchan,
	uint8_t chan_nr, struct l1sched_chan_state *chan_state)
{
	struct gsm_bts_trx *trx = lchan->ts->trx;
	struct phy_instance *pinst = trx_phy_instance(trx);
	int rssi;
	int i;

	/* skip every second clock, to prevent oscillating due to roundtrip
	 * delay */
	if (!(chan_state->meas.clock & 1))
		return 0;

	LOGP(DLOOP, LOGL_DEBUG, "Got SACCH master clock at RSSI count %d\n",
		chan_state->meas.rssi_count);

	/* wait for initial burst */
	if (!chan_state->meas.rssi_got_burst)
		return 0;

	/* if no burst was received from MS at clock */
	if (chan_state->meas.rssi_count == 0) {
		LOGP(DLOOP, LOGL_NOTICE, "LOST SACCH frame of trx=%u "
			"chan_nr=0x%02x, so we raise MS power\n",
			trx->nr, chan_nr);
		return ms_power_diff(lchan, chan_nr, MS_RAISE_MAX);
	}

	/* reset total counter */
	chan_state->meas.rssi_count = 0;

	/* check the minimum level received after MS acknowledged the ordered
	 * power level */
	if (chan_state->meas.rssi_valid_count == 0)
		return 0;
	for (rssi = 999, i = 0; i < chan_state->meas.rssi_valid_count; i++) {
		if (rssi > chan_state->meas.rssi[i])
			rssi = chan_state->meas.rssi[i];
	}

	/* reset valid counter */
	chan_state->meas.rssi_valid_count = 0;

	/* change RSSI */
	LOGP(DLOOP, LOGL_DEBUG, "Lowest RSSI: %d Target RSSI: %d Current "
		"MS power: %d (%d dBm) of trx=%u chan_nr=0x%02x\n", rssi,
		pinst->phy_link->u.osmotrx.trx_target_rssi, lchan->ms_power_ctrl.current,
		MS_PWR_DBM(trx->arfcn, lchan->ms_power_ctrl.current),
		trx->nr, chan_nr);
	ms_power_diff(lchan, chan_nr, pinst->phy_link->u.osmotrx.trx_target_rssi - rssi);

	return 0;
}


/* 90% of one bit duration in 1/256 symbols: 256*0.9 */
#define TOA256_9OPERCENT	230

/*
 * Timing Advance loop
 */

int ta_val(struct gsm_lchan *lchan, uint8_t chan_nr,
	struct l1sched_chan_state *chan_state, int16_t toa256)
{
	struct gsm_bts_trx *trx = lchan->ts->trx;

	/* check if the current L1 header acks to the current ordered TA */
	if (lchan->meas.l1_info[1] != lchan->rqd_ta)
		return 0;

	/* sum measurement */
	chan_state->meas.toa256_sum += toa256;
	if (++(chan_state->meas.toa_num) < 16)
		return 0;

	/* complete set */
	toa256 = chan_state->meas.toa256_sum / chan_state->meas.toa_num;

	/* check for change of TOA */
	if (toa256 < -TOA256_9OPERCENT && lchan->rqd_ta > 0) {
		LOGP(DLOOP, LOGL_INFO, "TOA of trx=%u chan_nr=0x%02x is too "
			"early (%d), now lowering TA from %d to %d\n",
			trx->nr, chan_nr, toa256, lchan->rqd_ta,
			lchan->rqd_ta - 1);
		lchan->rqd_ta--;
	} else if (toa256 > TOA256_9OPERCENT && lchan->rqd_ta < 63) {
		LOGP(DLOOP, LOGL_INFO, "TOA of trx=%u chan_nr=0x%02x is too "
			"late (%d), now raising TA from %d to %d\n",
			trx->nr, chan_nr, toa256, lchan->rqd_ta,
			lchan->rqd_ta + 1);
		lchan->rqd_ta++;
	} else
		LOGP(DLOOP, LOGL_INFO, "TOA of trx=%u chan_nr=0x%02x is "
			"correct (%d), keeping current TA of %d\n",
			trx->nr, chan_nr, toa256, lchan->rqd_ta);

	chan_state->meas.toa_num = 0;
	chan_state->meas.toa256_sum = 0;

	return 0;
}

int trx_loop_sacch_input(struct l1sched_trx *l1t, uint8_t chan_nr,
	struct l1sched_chan_state *chan_state, int8_t rssi, int16_t toa256)
{
	struct gsm_lchan *lchan = &l1t->trx->ts[L1SAP_CHAN2TS(chan_nr)]
					.lchan[l1sap_chan2ss(chan_nr)];
	struct phy_instance *pinst = trx_phy_instance(l1t->trx);

	if (pinst->phy_link->u.osmotrx.trx_ms_power_loop)
		ms_power_val(chan_state, rssi);

	if (pinst->phy_link->u.osmotrx.trx_ta_loop)
		ta_val(lchan, chan_nr, chan_state, toa256);

	return 0;
}

int trx_loop_sacch_clock(struct l1sched_trx *l1t, uint8_t chan_nr,
	struct l1sched_chan_state *chan_state)
{
	struct gsm_lchan *lchan = &l1t->trx->ts[L1SAP_CHAN2TS(chan_nr)]
					.lchan[l1sap_chan2ss(chan_nr)];
	struct phy_instance *pinst = trx_phy_instance(l1t->trx);

	if (pinst->phy_link->u.osmotrx.trx_ms_power_loop)
		ms_power_clock(lchan, chan_nr, chan_state);

	/* count the number of SACCH clocks */
	chan_state->meas.clock++;

	return 0;
}

int trx_loop_amr_input(struct l1sched_trx *l1t, uint8_t chan_nr,
	struct l1sched_chan_state *chan_state, float ber)
{
	struct gsm_bts_trx *trx = l1t->trx;
	struct gsm_lchan *lchan = &trx->ts[L1SAP_CHAN2TS(chan_nr)]
					.lchan[l1sap_chan2ss(chan_nr)];

	/* check if loop is enabled */
	if (!chan_state->amr_loop)
		return 0;

	/* wait for MS to use the requested codec */
	if (chan_state->ul_ft != chan_state->dl_cmr)
		return 0;

	/* count bit errors */
	if (L1SAP_IS_CHAN_TCHH(chan_nr)) {
		chan_state->ber_num += 2;
		chan_state->ber_sum += (ber + ber);
	} else {
		chan_state->ber_num++;
		chan_state->ber_sum += ber;
	}

	/* count frames */
	if (chan_state->ber_num < 48)
		return 0;

	/* calculate average (reuse ber variable) */
	ber = chan_state->ber_sum / chan_state->ber_num;

	/* reset bit errors */
	chan_state->ber_num = 0;
	chan_state->ber_sum = 0;

	LOGP(DLOOP, LOGL_DEBUG, "Current bit error rate (BER) %.6f "
		"codec id %d of trx=%u chan_nr=0x%02x\n", ber,
		chan_state->ul_ft, trx->nr, chan_nr);

	/* degrade */
	if (chan_state->dl_cmr > 0) {
		/* degrade, if ber is above threshold FIXME: C/I */
		if (ber >
		   lchan->tch.amr_mr.bts_mode[chan_state->dl_cmr-1].threshold) {
			LOGP(DLOOP, LOGL_DEBUG, "Degrading due to BER %.6f "
				"from codec id %d to %d of trx=%u "
				"chan_nr=0x%02x\n", ber, chan_state->dl_cmr,
				chan_state->dl_cmr - 1, trx->nr, chan_nr);
			chan_state->dl_cmr--;
		}

		return 0;
	}

	/* upgrade */
	if (chan_state->dl_cmr < chan_state->codecs - 1) {
		/* degrade, if ber is above threshold  FIXME: C/I*/
		if (ber <
		    lchan->tch.amr_mr.bts_mode[chan_state->dl_cmr].threshold
		  - lchan->tch.amr_mr.bts_mode[chan_state->dl_cmr].hysteresis) {
			LOGP(DLOOP, LOGL_DEBUG, "Upgrading due to BER %.6f "
				"from codec id %d to %d of trx=%u "
				"chan_nr=0x%02x\n", ber, chan_state->dl_cmr,
				chan_state->dl_cmr + 1, trx->nr, chan_nr);
			chan_state->dl_cmr++;
		}

		return 0;
	}

	return 0;
}

int trx_loop_amr_set(struct l1sched_chan_state *chan_state, int loop)
{
	if (chan_state->amr_loop && !loop) {
		chan_state->amr_loop = 0;

		return 0;
	}

	if (!chan_state->amr_loop && loop) {
		chan_state->amr_loop = 1;

		/* reset bit errors */
		chan_state->ber_num = 0;
		chan_state->ber_sum = 0;

		return 0;
	}

	return 0;
}
