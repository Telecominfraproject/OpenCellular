/* MS Power Control Loop L1 */

/* (C) 2014 by Holger Hans Peter Freyther
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

#include <osmo-bts/logging.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/gsm_data.h>
#include <osmo-bts/measurement.h>
#include <osmo-bts/bts_model.h>
#include <osmo-bts/l1sap.h>

/*
 * Check if manual power control is needed
 * Check if fixed power was selected
 * Check if the MS is already using our level if not
 * the value is bogus..
 * TODO: Add a timeout.. e.g. if the ms is not capable of reaching
 * the value we have set.
 */
int lchan_ms_pwr_ctrl(struct gsm_lchan *lchan,
		      const uint8_t ms_power, const int rxLevel)
{
	int rx;
	int cur_dBm, new_dBm, new_pwr;
	struct gsm_bts *bts = lchan->ts->trx->bts;
	const enum gsm_band band = bts->band;

	if (!trx_ms_pwr_ctrl_is_osmo(lchan->ts->trx))
		return 0;
	if (lchan->ms_power_ctrl.fixed)
		return 0;

	/* The phone hasn't reached the power level yet */
	if (lchan->ms_power_ctrl.current != ms_power)
		return 0;

	/* What is the difference between what we want and received? */
	rx = bts->ul_power_target - rxLevel;

	cur_dBm = ms_pwr_dbm(band, ms_power);
	new_dBm = cur_dBm + rx;

	/* Clamp negative values and do it depending on the band */
	if (new_dBm < 0)
		new_dBm = 0;

	switch (band) {
	case GSM_BAND_1800:
		/* If MS_TX_PWR_MAX_CCH is set the values 29,
		 * 30, 31 are not used. Avoid specifying a dBm
		 * that would lead to these power levels. The
		 * phone might not be able to reach them. */
		if (new_dBm > 30)
			new_dBm = 30;
		break;
	default:
		break;
	}

	new_pwr = ms_pwr_ctl_lvl(band, new_dBm);
	if (lchan->ms_power_ctrl.current != new_pwr) {
		lchan->ms_power_ctrl.current = new_pwr;
		bts_model_adjst_ms_pwr(lchan);
		return 1;
	}

	return 0;
}
