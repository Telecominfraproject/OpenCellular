/*
 * (C) 2013,2014 by Holger Hans Peter Freyther
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
 */

#include <osmo-bts/bts.h>
#include <osmo-bts/l1sap.h>
#include <osmo-bts/power_control.h>

#include <stdio.h>

static inline void apply_power_test(struct gsm_lchan *lchan, int rxlev, int exp_ret, uint8_t exp_current)
{
	int ret = lchan_ms_pwr_ctrl(lchan, lchan->ms_power_ctrl.current, rxlev);

	printf("power control [%d]: MS current power %u\n", ret, lchan->ms_power_ctrl.current);
	OSMO_ASSERT(ret == exp_ret);
	OSMO_ASSERT(lchan->ms_power_ctrl.current == exp_current);
}

static void test_power_loop(void)
{
	struct gsm_bts bts;
	struct gsm_bts_trx trx;
	struct gsm_bts_trx_ts ts;
	struct gsm_lchan *lchan;

	memset(&bts, 0, sizeof(bts));
	memset(&trx, 0, sizeof(trx));
	memset(&ts, 0, sizeof(ts));

	lchan = &ts.lchan[0];
	lchan->ts = &ts;
	ts.trx = &trx;
	trx.bts = &bts;
	bts.band = GSM_BAND_1800;
	trx.ms_power_control = 1;
	bts.ul_power_target = -75;

	lchan->state = LCHAN_S_NONE;
	lchan->ms_power_ctrl.current = ms_pwr_ctl_lvl(GSM_BAND_1800, 0);
	OSMO_ASSERT(lchan->ms_power_ctrl.current == 15);

	/* Simply clamping */
	apply_power_test(lchan, -60, 0, 15);

	/*
	 * Now 15 dB too little and we should power it up. Could be a
	 * power level of 7 or 8 for 15 dBm
	 */
	apply_power_test(lchan, -90, 1, 7);

	/* It should be clamped to level 0 and 30 dBm */
	apply_power_test(lchan, -100, 1, 0);

	/* Fix it and jump down */
	lchan->ms_power_ctrl.fixed = 1;
	apply_power_test(lchan, -60, 0, 0);

	/* And leave it again */
	lchan->ms_power_ctrl.fixed = 0;
	apply_power_test(lchan, -40, 1, 15);
}

int main(int argc, char **argv)
{
	printf("Testing power loop...\n");

	test_power_loop();

	printf("Power loop test OK\n");

	return 0;
}
