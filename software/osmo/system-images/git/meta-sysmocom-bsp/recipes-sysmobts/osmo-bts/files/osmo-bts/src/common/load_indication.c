/* Support for generating RSL Load Indication */

/* (C) 2011 by Harald Welte <laforge@gnumonks.org>
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

#include <osmocom/core/timer.h>
#include <osmocom/core/msgb.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/rsl.h>
#include <osmo-bts/paging.h>

static void reset_load_counters(struct gsm_bts *bts)
{
	/* re-set the counters */
	bts->load.ccch.pch_used = bts->load.ccch.pch_total = 0;
}

static void load_timer_cb(void *data)
{
	struct gsm_bts *bts = data;
	unsigned int pch_percent, rach_percent;

	/* compute percentages */
	if (bts->load.ccch.pch_total == 0)
		pch_percent = 0;
	else
		pch_percent = (bts->load.ccch.pch_used * 100) /
					bts->load.ccch.pch_total;

	if (pch_percent >= bts->load.ccch.load_ind_thresh) {
		/* send RSL load indication message to BSC */
		uint16_t buffer_space = paging_buffer_space(bts->paging_state);
		rsl_tx_ccch_load_ind_pch(bts, buffer_space);
	} else {
		/* This is an extenstion of TS 08.58.  We don't only
		 * send load indications if the load is above threshold,
		 * but we also explicitly indicate that we are below
		 * threshold by using the magic value 0xffff */
		rsl_tx_ccch_load_ind_pch(bts, 0xffff);
	}

	if (bts->load.rach.total == 0)
		rach_percent = 0;
	else
		rach_percent = (bts->load.rach.busy * 100) /
					bts->load.rach.total;

	if (rach_percent >= bts->load.ccch.load_ind_thresh) {
		/* send RSL load indication message to BSC */
		rsl_tx_ccch_load_ind_rach(bts, bts->load.rach.total,
					  bts->load.rach.busy,
					  bts->load.rach.access);
	}

	reset_load_counters(bts);

	/* re-schedule the timer */
	osmo_timer_schedule(&bts->load.ccch.timer,
			    bts->load.ccch.load_ind_period, 0);
}

void load_timer_start(struct gsm_bts *bts)
{
	if (!bts->load.ccch.timer.data) {
		bts->load.ccch.timer.data = bts;
		bts->load.ccch.timer.cb = load_timer_cb;
		reset_load_counters(bts);
	}
	osmo_timer_schedule(&bts->load.ccch.timer, bts->load.ccch.load_ind_period, 0);
}

void load_timer_stop(struct gsm_bts *bts)
{
	osmo_timer_del(&bts->load.ccch.timer);
}
