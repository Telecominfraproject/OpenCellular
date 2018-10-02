/* Test cases for tx_power.c Transmit Power Computation */

/* (C) 2017 by Harald Welte <laforge@gnumonks.org>
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

#include <stdint.h>

#include <osmocom/core/utils.h>
#include <osmocom/core/talloc.h>
#include <osmocom/core/application.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/tx_power.h>


static const struct trx_power_params tpp_1002 = {
	.trx_p_max_out_mdBm = to_mdB(23),
	.p_total_tgt_mdBm = to_mdB(23),
	.p_total_cur_mdBm = 0,
	.thermal_attenuation_mdB = 0,
	.user_gain_mdB = 0,
	.pa = {
		.nominal_gain_mdB = 0,
	},
	.user_pa = {
		.nominal_gain_mdB = 0,
	},
	.ramp = {
		.max_initial_pout_mdBm = to_mdB(23),
		.step_size_mdB = to_mdB(2),
		.step_interval_sec = 1,
	},
};

static const struct trx_power_params tpp_1020 = {
	.trx_p_max_out_mdBm = to_mdB(23),
	.p_total_tgt_mdBm = to_mdB(33),
	.p_total_cur_mdBm = 0,
	.thermal_attenuation_mdB = 0,
	.user_gain_mdB = 0,
	.pa = {
		.nominal_gain_mdB = to_mdB(10),
	},
	.user_pa = {
		.nominal_gain_mdB = 0,
	},
	.ramp = {
		.max_initial_pout_mdBm = to_mdB(0),
		.step_size_mdB = to_mdB(2),
		.step_interval_sec = 1,
	},
};

static const struct trx_power_params tpp_1100 = {
	.trx_p_max_out_mdBm = to_mdB(23),
	.p_total_tgt_mdBm = to_mdB(40),
	.p_total_cur_mdBm = 0,
	.thermal_attenuation_mdB = 0,
	.user_gain_mdB = 0,
	.pa = {
		.nominal_gain_mdB = to_mdB(17),
	},
	.user_pa = {
		.nominal_gain_mdB = 0,
	},
	.ramp = {
		.max_initial_pout_mdBm = to_mdB(0),
		.step_size_mdB = to_mdB(2),
		.step_interval_sec = 1,
	},
};

static const struct trx_power_params tpp_2050 = {
	.trx_p_max_out_mdBm = to_mdB(37),
	.p_total_tgt_mdBm = to_mdB(37),
	.p_total_cur_mdBm = 0,
	.thermal_attenuation_mdB = 0,
	.user_gain_mdB = 0,
	.pa = {
		.nominal_gain_mdB = 0,
	},
	.user_pa = {
		.nominal_gain_mdB = 0,
	},
	.ramp = {
		.max_initial_pout_mdBm = to_mdB(0),
		.step_size_mdB = to_mdB(2),
		.step_interval_sec = 1,
	},
};

static void test_sbts1002(struct gsm_bts_trx *trx)
{
	printf("Testing tx_power calculation for sysmoBTS 1002\n");
	trx->power_params = tpp_1002;
	trx->max_power_red = 0;
	OSMO_ASSERT(power_ramp_initial_power_mdBm(trx) == to_mdB(23));
	OSMO_ASSERT(get_p_max_out_mdBm(trx) == to_mdB(23));
	/* at max_power_red = 0, we expect full 23dBm */
	OSMO_ASSERT(get_p_nominal_mdBm(trx) == to_mdB(23));
	trx->max_power_red = 2;
	/* at max_power_red = 2, we expect 21dBm */
	OSMO_ASSERT(get_p_nominal_mdBm(trx) == to_mdB(21));
	/* at 1 step (of 2dB), we expect full 23-2-2=19 dBm */
	OSMO_ASSERT(get_p_target_mdBm(trx, 1) == to_mdB(19));
	/* at 2 steps (= 4dB), we expect 23-2-4=17*/
	OSMO_ASSERT(get_p_trxout_target_mdBm(trx, 2) == to_mdB(17));
}

static void test_sbts1020(struct gsm_bts_trx *trx)
{
	printf("Testing tx_power calculation for sysmoBTS 1020\n");
	trx->power_params = tpp_1020;
	trx->max_power_red = 0;
	OSMO_ASSERT(power_ramp_initial_power_mdBm(trx) == to_mdB(-10));
	OSMO_ASSERT(get_p_max_out_mdBm(trx) == to_mdB(33));
	/* at max_power_red = 0, we expect full 33dBm */
	OSMO_ASSERT(get_p_nominal_mdBm(trx) == to_mdB(33));
	trx->max_power_red = 2;
	/* at max_power_red = 2, we expect 31dBm */
	OSMO_ASSERT(get_p_nominal_mdBm(trx) == to_mdB(31));
	/* at 1 step (of 2dB), we expect full 33-2-2=29 dBm */
	OSMO_ASSERT(get_p_target_mdBm(trx, 1) == to_mdB(29));
	/* at 2 steps (= 4dB), we expect 33-2-4-10=17*/
	OSMO_ASSERT(get_p_trxout_target_mdBm(trx, 2) == to_mdB(17));
}


static void test_sbts1100(struct gsm_bts_trx *trx)
{
	printf("Testing tx_power calculation for sysmoBTS 1100\n");
	trx->power_params = tpp_1100;
	trx->max_power_red = 0;
	OSMO_ASSERT(power_ramp_initial_power_mdBm(trx) == to_mdB(-17));
	OSMO_ASSERT(get_p_max_out_mdBm(trx) == to_mdB(40));
	/* at max_power_red = 0, we expect full 33dBm */
	OSMO_ASSERT(get_p_nominal_mdBm(trx) == to_mdB(40));
	trx->max_power_red = 2;
	/* at max_power_red = 2, we expect 38dBm */
	OSMO_ASSERT(get_p_nominal_mdBm(trx) == to_mdB(38));
	/* at 1 step (of 2dB), we expect full 40-2-2=36 dBm */
	OSMO_ASSERT(get_p_target_mdBm(trx, 1) == to_mdB(36));
	/* at 2 steps (= 4dB), we expect 40-2-4-17=17*/
	OSMO_ASSERT(get_p_trxout_target_mdBm(trx, 2) == to_mdB(17));
}

static void test_sbts2050(struct gsm_bts_trx *trx)
{
	printf("Testing tx_power calculation for sysmoBTS 2050\n");
	trx->power_params = tpp_2050;
	trx->max_power_red = 0;
	OSMO_ASSERT(power_ramp_initial_power_mdBm(trx) == to_mdB(0));
	OSMO_ASSERT(get_p_max_out_mdBm(trx) == to_mdB(37));
	/* at max_power_red = 0, we expect full 37dBm */
	OSMO_ASSERT(get_p_nominal_mdBm(trx) == to_mdB(37));
	trx->max_power_red = 2;
	/* at max_power_red = 2, we expect 35dBm */
	OSMO_ASSERT(get_p_nominal_mdBm(trx) == to_mdB(35));
	/* at 1 step (of 2dB), we expect full 37-2-2=33 dBm */
	OSMO_ASSERT(get_p_target_mdBm(trx, 1) == to_mdB(33));
	/* at 2 steps (= 4dB), we expect 37-2-4=31dBm */
	OSMO_ASSERT(get_p_trxout_target_mdBm(trx, 2) == to_mdB(31));
}

int bts_model_change_power(struct gsm_bts_trx *trx, int p_trxout_mdBm)
{
	struct trx_power_params *tpp = &trx->power_params;

	printf("CHANGE_POWER(%d)\n", p_trxout_mdBm);

	if (tpp->ramp.attenuation_mdB == 0)
		exit(0);

	power_trx_change_compl(trx, p_trxout_mdBm);
	return 0;
}

static void test_power_ramp(struct gsm_bts_trx *trx, int dBm)
{
	printf("Testing tx_power ramping for sysmoBTS 1020\n");
	trx->power_params = tpp_1020;
	trx->max_power_red = 0;

	power_ramp_start(trx, to_mdB(dBm), 0);
}

int main(int argc, char **argv)
{
	static struct gsm_bts *bts;
	struct gsm_bts_trx *trx;
	void *tall_bts_ctx;

	tall_bts_ctx = talloc_named_const(NULL, 1, "OsmoBTS context");
	msgb_talloc_ctx_init(tall_bts_ctx, 0);

	osmo_init_logging2(tall_bts_ctx, &bts_log_info);
	osmo_stderr_target->categories[DL1C].loglevel = LOGL_DEBUG;
	log_set_print_filename(osmo_stderr_target, 0);

	bts = gsm_bts_alloc(tall_bts_ctx, 0);
	if (!bts) {
		fprintf(stderr, "Failed to create BTS structure\n");
		exit(1);
	}
	trx = gsm_bts_trx_alloc(bts);
	if (!trx) {
		fprintf(stderr, "Failed to TRX structure\n");
		exit(1);
	}

	if (bts_init(bts) < 0) {
		fprintf(stderr, "unable to to open bts\n");
		exit(1);
	}

	test_sbts1002(trx);
	test_sbts1020(trx);
	test_sbts1100(trx);
	test_sbts2050(trx);

	/* test error case / excess power (40 dBm is too much) */
	test_power_ramp(trx, 40);
	/* test actaul ramping to full 33 dBm */
	test_power_ramp(trx, 33);

	while (1) {
		osmo_select_main(0);
	}
}
