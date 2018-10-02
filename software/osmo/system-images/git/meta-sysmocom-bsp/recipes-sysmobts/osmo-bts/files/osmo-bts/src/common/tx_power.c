/* Transmit Power computation */

/* (C) 2014 by Harald Welte <laforge@gnumonks.org>
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
#include <limits.h>
#include <errno.h>

#include <osmocom/core/utils.h>

#include <osmo-bts/logging.h>
#include <osmo-bts/gsm_data.h>
#include <osmo-bts/bts_model.h>
#include <osmo-bts/tx_power.h>

static int get_pa_drive_level_mdBm(const struct power_amp *pa,
		       int desired_p_out_mdBm, unsigned int arfcn)
{
	if (arfcn >= ARRAY_SIZE(pa->calib.delta_mdB))
		return INT_MIN;

	/* FIXME: temperature compensation */

	return desired_p_out_mdBm - pa->nominal_gain_mdB - pa->calib.delta_mdB[arfcn];
}

/* maximum output power of the system */
int get_p_max_out_mdBm(struct gsm_bts_trx *trx)
{
	struct trx_power_params *tpp = &trx->power_params;
	/* Add user gain, internal and external PA gain to TRX output power */
	return tpp->trx_p_max_out_mdBm + tpp->user_gain_mdB +
			tpp->pa.nominal_gain_mdB + tpp->user_pa.nominal_gain_mdB;
}

/* nominal output power, i.e. OML-reduced maximum output power */
int get_p_nominal_mdBm(struct gsm_bts_trx *trx)
{
	/* P_max_out subtracted by OML maximum power reduction IE */
	return get_p_max_out_mdBm(trx) - to_mdB(trx->max_power_red);
}

/* calculate the target total output power required, reduced by both
 * OML and RSL, but ignoring the attenuation required for power ramping and
 * thermal management */
int get_p_target_mdBm(struct gsm_bts_trx *trx, uint8_t bs_power_ie)
{
	/* Pn subtracted by RSL BS Power IE (in 2 dB steps) */
	return get_p_nominal_mdBm(trx) - to_mdB(bs_power_ie * 2);
}
int get_p_target_mdBm_lchan(struct gsm_lchan *lchan)
{
	return get_p_target_mdBm(lchan->ts->trx, lchan->bs_power);
}

/* calculate the actual total output power required, taking into account the
 * attenuation required for power ramping but not thermal management */
int get_p_actual_mdBm(struct gsm_bts_trx *trx, int p_target_mdBm)
{
	struct trx_power_params *tpp = &trx->power_params;

	/* P_target subtracted by ramp attenuation */
	return p_target_mdBm - tpp->ramp.attenuation_mdB;
}

/* calculate the effective total output power required, taking into account the
 * attenuation required for power ramping and thermal management */
int get_p_eff_mdBm(struct gsm_bts_trx *trx, int p_target_mdBm)
{
	struct trx_power_params *tpp = &trx->power_params;

	/* P_target subtracted by ramp attenuation */
	return p_target_mdBm - tpp->ramp.attenuation_mdB - tpp->thermal_attenuation_mdB;
}

/* calculate effect TRX output power required, taking into account the
 * attenuations required for power ramping and thermal management */
int get_p_trxout_eff_mdBm(struct gsm_bts_trx *trx, int p_target_mdBm)
{
	struct trx_power_params *tpp = &trx->power_params;
	int p_actual_mdBm, user_pa_drvlvl_mdBm, pa_drvlvl_mdBm;
	unsigned int arfcn = trx->arfcn;

	/* P_actual subtracted by any bulk gain added by the user */
	p_actual_mdBm = get_p_eff_mdBm(trx, p_target_mdBm) - tpp->user_gain_mdB;

	/* determine input drive level required at input to user PA */
	user_pa_drvlvl_mdBm = get_pa_drive_level_mdBm(&tpp->user_pa, p_actual_mdBm, arfcn);

	/* determine input drive level required at input to internal PA */
	pa_drvlvl_mdBm = get_pa_drive_level_mdBm(&tpp->pa, user_pa_drvlvl_mdBm, arfcn);

	/* internal PA input drive level is TRX output power */
	return pa_drvlvl_mdBm;
}

/* calculate target TRX output power required, ignoring the
 * attenuations required for power ramping but not thermal management */
int get_p_trxout_target_mdBm(struct gsm_bts_trx *trx, uint8_t bs_power_ie)
{
	struct trx_power_params *tpp = &trx->power_params;
	int p_target_mdBm, user_pa_drvlvl_mdBm, pa_drvlvl_mdBm;
	unsigned int arfcn = trx->arfcn;

	/* P_target subtracted by any bulk gain added by the user */
	p_target_mdBm = get_p_target_mdBm(trx, bs_power_ie) - tpp->user_gain_mdB;

	/* determine input drive level required at input to user PA */
	user_pa_drvlvl_mdBm = get_pa_drive_level_mdBm(&tpp->user_pa, p_target_mdBm, arfcn);

	/* determine input drive level required at input to internal PA */
	pa_drvlvl_mdBm = get_pa_drive_level_mdBm(&tpp->pa, user_pa_drvlvl_mdBm, arfcn);

	/* internal PA input drive level is TRX output power */
	return pa_drvlvl_mdBm;
}
int get_p_trxout_target_mdBm_lchan(struct gsm_lchan *lchan)
{
	return get_p_trxout_target_mdBm(lchan->ts->trx, lchan->bs_power);
}


/* output power ramping code */

/* The idea here is to avoid a hard switch from 0 to 100, but to actually
 * slowly and gradually ramp up or down the power.  This is needed on the
 * one hand side to avoid very fast dynamic load changes towards the PA power
 * supply, but is also needed in order to avoid a DoS by too many subscriber
 * attempting to register at the same time.  Rather, grow the cell slowly in
 * radius than start with the full radius at once.  */

static int we_are_ramping_up(struct gsm_bts_trx *trx)
{
	struct trx_power_params *tpp = &trx->power_params;

	if (tpp->p_total_tgt_mdBm > tpp->p_total_cur_mdBm)
		return 1;
	else
		return 0;
}

static void power_ramp_do_step(struct gsm_bts_trx *trx, int first);

/* timer call-back for the ramp timer */
static void power_ramp_timer_cb(void *_trx)
{
	struct gsm_bts_trx *trx = _trx;
	struct trx_power_params *tpp = &trx->power_params;
	int p_trxout_eff_mdBm;

	/* compute new actual total output power (= minus ramp attenuation) */
	tpp->p_total_cur_mdBm = get_p_actual_mdBm(trx, tpp->p_total_tgt_mdBm);

	/* compute new effective (= minus ramp and thermal attenuation) TRX output required */
	p_trxout_eff_mdBm = get_p_trxout_eff_mdBm(trx, tpp->p_total_tgt_mdBm);

	LOGP(DL1C, LOGL_DEBUG, "ramp_timer_cb(cur_pout=%d, tgt_pout=%d, "
		"ramp_att=%d, therm_att=%d, user_gain=%d)\n",
		tpp->p_total_cur_mdBm, tpp->p_total_tgt_mdBm,
		tpp->ramp.attenuation_mdB, tpp->thermal_attenuation_mdB,
		tpp->user_gain_mdB);

	LOGP(DL1C, LOGL_INFO,
		"ramping TRX board output power to %d mdBm.\n", p_trxout_eff_mdBm);

	/* Instruct L1 to apply new effective TRX output power required */
	bts_model_change_power(trx, p_trxout_eff_mdBm);
}

/* BTS model call-back once one a call to bts_model_change_power()
 * completes, indicating actual L1 transmit power */
void power_trx_change_compl(struct gsm_bts_trx *trx, int p_trxout_cur_mdBm)
{
	struct trx_power_params *tpp = &trx->power_params;
	int p_trxout_should_mdBm;

	p_trxout_should_mdBm = get_p_trxout_eff_mdBm(trx, tpp->p_total_tgt_mdBm);

	/* for now we simply write an error message, but in the future
	 * we might use the value (again) as part of our math? */
	if (p_trxout_cur_mdBm != p_trxout_should_mdBm) {
		LOGP(DL1C, LOGL_ERROR, "bts_model notifies us of %u mdBm TRX "
		     "output power.  However, it should be %u mdBm!\n",
		     p_trxout_cur_mdBm, p_trxout_should_mdBm);
	}

	/* and do another step... */
	power_ramp_do_step(trx, 0);
}

static void power_ramp_do_step(struct gsm_bts_trx *trx, int first)
{
	struct trx_power_params *tpp = &trx->power_params;

	/* we had finished in last loop iteration */
	if (!first && tpp->ramp.attenuation_mdB == 0)
		return;

	if (we_are_ramping_up(trx)) {
		/* ramp up power -> ramp down attenuation */
		tpp->ramp.attenuation_mdB -= tpp->ramp.step_size_mdB;
		if (tpp->ramp.attenuation_mdB <= 0) {
			/* we are done */
			tpp->ramp.attenuation_mdB = 0;
		}
	} else {
		/* ramp down power -> ramp up attenuation */
		tpp->ramp.attenuation_mdB += tpp->ramp.step_size_mdB;
		if (tpp->ramp.attenuation_mdB >= 0) {
			/* we are done */
			tpp->ramp.attenuation_mdB = 0;
		}
	}

	/* schedule timer for the next step */
	tpp->ramp.step_timer.data = trx;
	tpp->ramp.step_timer.cb = power_ramp_timer_cb;
	osmo_timer_schedule(&tpp->ramp.step_timer, tpp->ramp.step_interval_sec, 0);
}


int power_ramp_start(struct gsm_bts_trx *trx, int p_total_tgt_mdBm, int bypass)
{
	struct trx_power_params *tpp = &trx->power_params;

	/* The input to this function is the actual desired output power, i.e.
	 * the maximum total system power subtracted by OML as well as RSL
	 * reductions */

	LOGP(DL1C, LOGL_INFO, "power_ramp_start(cur=%d, tgt=%d)\n",
		tpp->p_total_cur_mdBm, p_total_tgt_mdBm);

	if (!bypass && (p_total_tgt_mdBm > get_p_nominal_mdBm(trx))) {
		LOGP(DL1C, LOGL_ERROR, "Asked to ramp power up to "
		     "%d mdBm, which exceeds P_max_out (%d)\n",
		     p_total_tgt_mdBm, get_p_nominal_mdBm(trx));
		return -ERANGE;
	}

	/* Cancel any pending request */
	osmo_timer_del(&tpp->ramp.step_timer);

	/* set the new target */
	tpp->p_total_tgt_mdBm = p_total_tgt_mdBm;

	if (we_are_ramping_up(trx)) {
		if (tpp->p_total_tgt_mdBm <= tpp->ramp.max_initial_pout_mdBm) {
			LOGP(DL1C, LOGL_INFO,
				"target_power(%d) is below max.initial power\n",
				tpp->p_total_tgt_mdBm);
			/* new setting is below the maximum initial output
			 * power, so we can directly jump to this level */
			tpp->p_total_cur_mdBm = tpp->p_total_tgt_mdBm;
			tpp->ramp.attenuation_mdB = 0;
			power_ramp_timer_cb(trx);
		} else {
			/* We need to step it up. Start from the current value */
			/* Set attenuation to cause no power change right now */
			tpp->ramp.attenuation_mdB = tpp->p_total_tgt_mdBm - tpp->p_total_cur_mdBm;

			/* start with the first step */
			power_ramp_do_step(trx, 1);
		}
	} else {
		/* Set ramp attenuation to negative value, and increase that by
		 * steps until it reaches 0 */
		tpp->ramp.attenuation_mdB = tpp->p_total_tgt_mdBm - tpp->p_total_cur_mdBm;

		/* start with the first step */
		power_ramp_do_step(trx, 1);
	}

	return 0;
}

/* determine the initial transceiver output power at start-up time */
int power_ramp_initial_power_mdBm(struct gsm_bts_trx *trx)
{
	struct trx_power_params *tpp = &trx->power_params;
	int pout_mdBm;

	/* this is the maximum initial output on the antenna connector
	 * towards the antenna */
	pout_mdBm = tpp->ramp.max_initial_pout_mdBm;

	/* use this as input to compute transceiver board power
	 * (reflecting gains in internal/external amplifiers */
	return get_p_trxout_eff_mdBm(trx, pout_mdBm);
}
