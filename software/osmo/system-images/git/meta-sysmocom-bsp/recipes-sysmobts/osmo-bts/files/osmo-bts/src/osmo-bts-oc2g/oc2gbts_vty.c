/* VTY interface for NuRAN Wireless OC-2G */

/* Copyright (C) 2015 by Yves Godin <support@nuranwireless.com>
 * Copyright (C) 2016 by Harald Welte <laforge@gnumonks.org>
 * 
 * Based on sysmoBTS:
 *     (C) 2011 by Harald Welte <laforge@gnumonks.org>
 *     (C) 2012,2013 by Holger Hans Peter Freyther
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

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>

#include <arpa/inet.h>

#include <osmocom/core/msgb.h>
#include <osmocom/core/talloc.h>
#include <osmocom/core/select.h>
#include <osmocom/core/rate_ctr.h>

#include <osmocom/gsm/tlv.h>

#include <osmocom/vty/vty.h>
#include <osmocom/vty/command.h>
#include <osmocom/vty/misc.h>

#include <osmocom/ctrl/control_cmd.h>
#include <osmo-bts/signal.h>
#include <osmo-bts/oml.h>
#include <osmo-bts/bts.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/phy_link.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/bts_model.h>
#include <osmo-bts/vty.h>

#include "oc2gbts.h"
#include "l1_if.h"
#include "utils.h"

extern int lchan_activate(struct gsm_lchan *lchan);
extern int rsl_tx_preproc_meas_res(struct gsm_lchan *lchan);

#define TRX_STR "Transceiver related commands\n" "TRX number\n"

#define SHOW_TRX_STR				\
	SHOW_STR				\
	TRX_STR
#define DSP_TRACE_F_STR		"DSP Trace Flag\n"

static struct gsm_bts *vty_bts;

static const struct value_string oc2g_pedestal_mode_strs[] = {
	{ OC2G_PEDESTAL_OFF, "off" },
	{ OC2G_PEDESTAL_ON, "on" },
	{ 0, NULL }
};

static const struct value_string oc2g_led_mode_strs[] = {
	{ OC2G_LED_CONTROL_BTS, "bts" },
	{ OC2G_LED_CONTROL_EXT, "external" },
	{ 0, NULL }
};

static const struct value_string oc2g_auto_adj_pwr_strs[] = {
	{ OC2G_TX_PWR_ADJ_NONE, "none" },
	{ OC2G_TX_PWR_ADJ_AUTO, "auto" },
	{ 0, NULL }
};

/* configuration */

DEFUN(cfg_phy_cal_path, cfg_phy_cal_path_cmd,
	"trx-calibration-path PATH",
	"Set the path name to TRX calibration data\n" "Path name\n")
{
	struct phy_instance *pinst = vty->index;

	if (pinst->u.oc2g.calib_path)
		talloc_free(pinst->u.oc2g.calib_path);

	pinst->u.oc2g.calib_path = talloc_strdup(pinst, argv[0]);

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_dsp_trace_f, cfg_phy_dsp_trace_f_cmd,
	"HIDDEN", TRX_STR)
{
	struct phy_instance *pinst = vty->index;
	unsigned int flag;

	flag = get_string_value(oc2gbts_tracef_names, argv[1]);
	pinst->u.oc2g.dsp_trace_f |= ~flag;

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_no_dsp_trace_f, cfg_phy_no_dsp_trace_f_cmd,
	"HIDDEN", NO_STR TRX_STR)
{
	struct phy_instance *pinst = vty->index;
	unsigned int flag;

	flag = get_string_value(oc2gbts_tracef_names, argv[1]);
	pinst->u.oc2g.dsp_trace_f &= ~flag;

	return CMD_SUCCESS;
}


/* runtime */

DEFUN(show_dsp_trace_f, show_dsp_trace_f_cmd,
	"show trx <0-0> dsp-trace-flags",
	SHOW_TRX_STR "Display the current setting of the DSP trace flags")
{
	int trx_nr = atoi(argv[0]);
	struct gsm_bts_trx *trx = gsm_bts_trx_num(vty_bts, trx_nr);
	struct oc2gl1_hdl *fl1h;
	int i;

	if (!trx)
		return CMD_WARNING;

	fl1h = trx_oc2gl1_hdl(trx);

	vty_out(vty, "Oc2g L1 DSP trace flags:%s", VTY_NEWLINE);
	for (i = 0; i < ARRAY_SIZE(oc2gbts_tracef_names); i++) {
		const char *endis;

		if (oc2gbts_tracef_names[i].value == 0 &&
		    oc2gbts_tracef_names[i].str == NULL)
			break;

		if (fl1h->dsp_trace_f & oc2gbts_tracef_names[i].value)
			endis = "enabled";
		else
			endis = "disabled";

		vty_out(vty, "DSP Trace %-15s %s%s",
			oc2gbts_tracef_names[i].str, endis,
			VTY_NEWLINE);
	}

	return CMD_SUCCESS;

}

DEFUN(dsp_trace_f, dsp_trace_f_cmd, "HIDDEN", TRX_STR)
{
	int phy_nr = atoi(argv[0]);
	struct phy_instance *pinst;
	struct oc2gl1_hdl *fl1h;
	unsigned int flag ;

	pinst = vty_get_phy_instance(vty, phy_nr, 0);
	if (!pinst)
		return CMD_WARNING;

	fl1h = pinst->u.oc2g.hdl;
	flag = get_string_value(oc2gbts_tracef_names, argv[1]);
	l1if_set_trace_flags(fl1h, fl1h->dsp_trace_f | flag);

	return CMD_SUCCESS;
}

DEFUN(no_dsp_trace_f, no_dsp_trace_f_cmd, "HIDDEN", NO_STR TRX_STR)
{
	int phy_nr = atoi(argv[0]);
	struct phy_instance *pinst;
	struct oc2gl1_hdl *fl1h;
	unsigned int flag ;

	pinst = vty_get_phy_instance(vty, phy_nr, 0);
	if (!pinst)
		return CMD_WARNING;

	fl1h = pinst->u.oc2g.hdl;
	flag = get_string_value(oc2gbts_tracef_names, argv[1]);
	l1if_set_trace_flags(fl1h, fl1h->dsp_trace_f & ~flag);

	return CMD_SUCCESS;
}

DEFUN(show_sys_info, show_sys_info_cmd,
	"show phy <0-1> instance <0-0> system-information",
	SHOW_TRX_STR "Display information about system\n")
{
	int phy_nr = atoi(argv[0]);
	int inst_nr = atoi(argv[1]);
	struct phy_link *plink = phy_link_by_num(phy_nr);
	struct phy_instance *pinst;
	struct oc2gl1_hdl *fl1h;
	int i;

	if (!plink) {
		vty_out(vty, "Cannot find PHY link %u%s",
			phy_nr, VTY_NEWLINE);
		return CMD_WARNING;
	}
	pinst = phy_instance_by_num(plink, inst_nr);
	if (!plink) {
		vty_out(vty, "Cannot find PHY instance %u%s",
			phy_nr, VTY_NEWLINE);
		return CMD_WARNING;
	}
	fl1h = pinst->u.oc2g.hdl;

	vty_out(vty, "DSP Version: %u.%u.%u, FPGA Version: %u.%u.%u%s",
		fl1h->hw_info.dsp_version[0],
		fl1h->hw_info.dsp_version[1],
		fl1h->hw_info.dsp_version[2],
		fl1h->hw_info.fpga_version[0],
		fl1h->hw_info.fpga_version[1],
		fl1h->hw_info.fpga_version[2], VTY_NEWLINE);

	vty_out(vty, "GSM Band Support: ");
	for (i = 0; i < sizeof(fl1h->hw_info.band_support); i++) {
		if (fl1h->hw_info.band_support & (1 << i))
			vty_out(vty, "%s ",  gsm_band_name(1 << i));
	}
	vty_out(vty, "%s", VTY_NEWLINE);
	vty_out(vty, "Min Tx Power: %d dBm%s", fl1h->phy_inst->u.oc2g.minTxPower, VTY_NEWLINE);
	vty_out(vty, "Max Tx Power: %d dBm%s", fl1h->phy_inst->u.oc2g.maxTxPower, VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(activate_lchan, activate_lchan_cmd,
	"trx <0-0> <0-7> (activate|deactivate) <0-7>",
	TRX_STR
	"Timeslot number\n"
	"Activate Logical Channel\n"
	"Deactivate Logical Channel\n"
	"Logical Channel Number\n" )
{
	int trx_nr = atoi(argv[0]);
	int ts_nr = atoi(argv[1]);
	int lchan_nr = atoi(argv[3]);
	struct gsm_bts_trx *trx = gsm_bts_trx_num(vty_bts, trx_nr);
	struct gsm_bts_trx_ts *ts = &trx->ts[ts_nr];
	struct gsm_lchan *lchan = &ts->lchan[lchan_nr];

	if (!strcmp(argv[2], "activate"))
		lchan_activate(lchan);
	else
		lchan_deactivate(lchan);

	return CMD_SUCCESS;
}

DEFUN(set_tx_power, set_tx_power_cmd,
	"trx nr <0-1> tx-power <-110-100>",
	TRX_STR
	"TRX number \n"
	"Set transmit power (override BSC)\n"
	"Transmit power in dBm\n")
{
	int trx_nr = atoi(argv[0]);
	int power = atoi(argv[1]);
	struct gsm_bts_trx *trx = gsm_bts_trx_num(vty_bts, trx_nr);

	power_ramp_start(trx, to_mdB(power), 1);

	return CMD_SUCCESS;
}

DEFUN(loopback, loopback_cmd,
	"trx <0-0> <0-7> loopback <0-1>",
	TRX_STR
	"Timeslot number\n"
	"Set TCH loopback\n"
	"Logical Channel Number\n")
{
	int trx_nr = atoi(argv[0]);
	int ts_nr = atoi(argv[1]);
	int lchan_nr = atoi(argv[2]);
	struct gsm_bts_trx *trx = gsm_bts_trx_num(vty_bts, trx_nr);
	struct gsm_bts_trx_ts *ts = &trx->ts[ts_nr];
	struct gsm_lchan *lchan = &ts->lchan[lchan_nr];

	lchan->loopback = 1;

	return CMD_SUCCESS;
}

DEFUN(no_loopback, no_loopback_cmd,
	"no trx <0-0> <0-7> loopback <0-1>",
	NO_STR TRX_STR
	"Timeslot number\n"
	"Set TCH loopback\n"
	"Logical Channel Number\n")
{
	int trx_nr = atoi(argv[0]);
	int ts_nr = atoi(argv[1]);
	int lchan_nr = atoi(argv[2]);
	struct gsm_bts_trx *trx = gsm_bts_trx_num(vty_bts, trx_nr);
	struct gsm_bts_trx_ts *ts = &trx->ts[ts_nr];
	struct gsm_lchan *lchan = &ts->lchan[lchan_nr];

	lchan->loopback = 0;

	return CMD_SUCCESS;
}

DEFUN(cfg_trx_nominal_power, cfg_trx_nominal_power_cmd,
       "nominal-tx-power <0-40>",
       "Set the nominal transmit output power in dBm\n"
       "Nominal transmit output power level in dBm\n")
{
	int nominal_power = atoi(argv[0]);
	struct gsm_bts_trx *trx = vty->index;

	if (( nominal_power >  40 ) ||  ( nominal_power < 0 )) {
		vty_out(vty, "Nominal Tx power level must be between 0 and 40 dBm (%d) %s",
		nominal_power, VTY_NEWLINE);
		return CMD_WARNING;
	}

	trx->nominal_power = nominal_power;
	trx->power_params.trx_p_max_out_mdBm = to_mdB(nominal_power);

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_max_cell_size, cfg_phy_max_cell_size_cmd,
       "max-cell-size <0-166>",
       "Set the maximum cell size  in qbits\n")
{
	struct phy_instance *pinst = vty->index;
	int cell_size = (uint8_t)atoi(argv[0]);

	if (( cell_size >  166 ) || ( cell_size < 0 )) {
		vty_out(vty, "Max cell size must be between 0 and 166 qbits (%d) %s",
				cell_size, VTY_NEWLINE);
		return CMD_WARNING;
	}

	pinst->u.oc2g.max_cell_size = (uint8_t)cell_size;
	return CMD_SUCCESS;
}

DEFUN(cfg_phy_pedestal_mode, cfg_phy_pedestal_mode_cmd,
		"pedestal-mode (on|off)",
		"Set unused time-slot transmission in pedestal mode\n"
		"Transmission pedestal mode can be (off, on)\n")
{
	struct phy_instance *pinst = vty->index;
	int val = get_string_value(oc2g_pedestal_mode_strs, argv[0]);

	if((val < OC2G_PEDESTAL_OFF)  || (val > OC2G_PEDESTAL_ON)) {
			vty_out(vty, "Invalid unused time-slot transmission mode %d%s", val, VTY_NEWLINE);
			return CMD_WARNING;
	}

	pinst->u.oc2g.pedestal_mode = (uint8_t)val;
	return CMD_SUCCESS;
}

/* TODO(oramadan) MERGE 
DEFUN(cfg_bts_led_mode, cfg_bts_led_mode_cmd,
		"led-control-mode (bts|external)",
		"Set LED controlled by BTS or external software\n"
		"LED can be controlled by (bts, external)\n")
{
	struct gsm_bts *bts = vty->index;
	struct gsm_bts_role_bts *btsb = bts_role_bts(bts);
	int val = get_string_value(oc2g_led_mode_strs, argv[0]);

	if((val < OC2G_LED_CONTROL_BTS)  || (val > OC2G_LED_CONTROL_EXT)) {
			vty_out(vty, "Invalid LED control mode %d%s", val, VTY_NEWLINE);
			return CMD_WARNING;
	}

	btsb->oc2g.led_ctrl_mode = (uint8_t)val;
	return CMD_SUCCESS;
} */

DEFUN(cfg_phy_dsp_alive_timer, cfg_phy_dsp_alive_timer_cmd,
	"dsp-alive-period <0-60>",
	"Set DSP alive timer period in second\n")
{
	struct phy_instance *pinst = vty->index;
	uint8_t period = (uint8_t)atoi(argv[0]);

	if (( period >  60 ) || ( period < 0 )) {
		vty_out(vty, "DSP heart beat alive timer period must be between 0 and 60 seconds (%d) %s",
				period, VTY_NEWLINE);
		return CMD_WARNING;
	}

	pinst->u.oc2g.dsp_alive_period = period;
	return CMD_SUCCESS;
}

DEFUN(cfg_phy_auto_tx_pwr_adj, cfg_phy_auto_tx_pwr_adj_cmd,
	"pwr-adj-mode (none|auto)",
	"Set output power adjustment mode\n")
{
	struct phy_instance *pinst = vty->index;
	int val = get_string_value(oc2g_auto_adj_pwr_strs, argv[0]);

	if((val < OC2G_TX_PWR_ADJ_NONE) || (val > OC2G_TX_PWR_ADJ_AUTO)) {
		vty_out(vty, "Invalid output power adjustment mode %d%s", val, VTY_NEWLINE);
		return CMD_WARNING;
	}

	pinst->u.oc2g.tx_pwr_adj_mode = (uint8_t)val;
	return CMD_SUCCESS;
}

DEFUN(cfg_phy_tx_red_pwr_8psk, cfg_phy_tx_red_pwr_8psk_cmd,
	"tx-red-pwr-8psk <0-40>",
	"Set reduction output power for 8-PSK scheme in dB unit\n")
{
	struct phy_instance *pinst = vty->index;
	int val = atoi(argv[0]);

	if ((val > 40) || (val < 0)) {
		vty_out(vty, "Reduction Tx power level must be between 0 and 40 dB (%d) %s",
		val, VTY_NEWLINE);
		return CMD_WARNING;
	}

	pinst->u.oc2g.tx_pwr_red_8psk = (uint8_t)val;
	return CMD_SUCCESS;
}

DEFUN(cfg_phy_c0_idle_red_pwr, cfg_phy_c0_idle_red_pwr_cmd,
	"c0-idle-red-pwr <0-40>",
	"Set reduction output power for C0 idle slot in dB unit\n")
{
	struct phy_instance *pinst = vty->index;
	int val = atoi(argv[0]);

	if ((val > 40) || (val < 0)) {
		vty_out(vty, "Reduction Tx power level must be between 0 and 40 dB (%d) %s",
		val, VTY_NEWLINE);
		return CMD_WARNING;
	}

	pinst->u.oc2g.tx_c0_idle_pwr_red = (uint8_t)val;
	return CMD_SUCCESS;
}

DEFUN(trigger_ho_cause, trigger_ho_cause_cmd, "HIDDEN", TRX_STR)
{
	struct gsm_network *net = gsmnet_from_vty(vty);
	struct gsm_bts *bts;
	struct gsm_bts_trx *trx;
	struct gsm_bts_trx_ts *ts;
	struct gsm_lchan *lchan;
	int trx_nr, ts_nr, lchan_nr;
	uint8_t ho_cause;
	uint8_t old_ho_cause;

	/* get BTS pointer */
	bts = gsm_bts_num(net, 0);
	if (!bts) {
		vty_out(vty, "Can not get BTS node %s", VTY_NEWLINE);
		return CMD_WARNING;
	}
	/* get TRX pointer */
	if (argc >= 1) {
		trx_nr = atoi(argv[0]);
		if (trx_nr >= bts->num_trx) {
			vty_out(vty, "%% can't find TRX %s%s", argv[0],
				VTY_NEWLINE);
			return CMD_WARNING;
		}
		trx = gsm_bts_trx_num(bts, trx_nr);
	}
	/* get TS pointer */
	if (argc >= 2) {
		ts_nr = atoi(argv[1]);
		if (ts_nr >= TRX_NR_TS) {
			vty_out(vty, "%% can't find TS %s%s", argv[1],
				VTY_NEWLINE);
			return CMD_WARNING;
		}
		ts = &trx->ts[ts_nr];
	}
	/* get lchan pointer */
	if (argc >= 3) {
		lchan_nr = atoi(argv[2]);
		if (lchan_nr >= TS_MAX_LCHAN) {
			vty_out(vty, "%% can't find LCHAN %s%s", argv[2],
				VTY_NEWLINE);
			return CMD_WARNING;
		}
		lchan = &ts->lchan[lchan_nr];
	}

	/* get HO cause */
	if (argc >= 4) {
		ho_cause = get_string_value(oc2gbts_rsl_ho_causes, argv[3]);
		if (ho_cause >= IPAC_HO_RQD_CAUSE_MAX) {
			vty_out(vty, "%% can't find valid HO cause %s%s", argv[3],
				VTY_NEWLINE);
			return CMD_WARNING;
		}
	} else {
		vty_out(vty, "%% HO cause is not provided %s", VTY_NEWLINE);
		return CMD_WARNING;
	}
	/* TODO(oramadan) MERGE 
	/* store recorded HO causes * /
	old_ho_cause = lchan->meas_preproc.rec_ho_causes;

	/* Apply new HO causes * /
	lchan->meas_preproc.rec_ho_causes = 1 << (ho_cause - 1);

	/* Send measuremnt report to BSC * /
	rsl_tx_preproc_meas_res(lchan);

	/* restore HO cause * /
	lchan->meas_preproc.rec_ho_causes = old_ho_cause;
	*/

	return CMD_SUCCESS;
}

/* TODO(oramadan) MERGE 
DEFUN(cfg_bts_rtp_drift_threshold, cfg_bts_rtp_drift_threshold_cmd,
	"rtp-drift-threshold <0-10000>",
	"RTP parameters\n"
	"RTP timestamp drift threshold in ms\n")
{
	struct gsm_bts *bts = vty->index;
	struct gsm_bts_role_bts *btsb = bts_role_bts(bts);

	btsb->oc2g.rtp_drift_thres_ms = (unsigned int) atoi(argv[0]);

	return CMD_SUCCESS;
}
*/

void bts_model_config_write_bts(struct vty *vty, struct gsm_bts *bts)
{
	/* TODO(oramadan) MERGE 
	struct gsm_bts_role_bts *btsb = bts_role_bts(bts);

	vty_out(vty, " led-control-mode %s%s",
			get_value_string(oc2g_led_mode_strs, btsb->oc2g.led_ctrl_mode), VTY_NEWLINE);

	vty_out(vty, " rtp-drift-threshold %d%s",
			btsb->oc2g.rtp_drift_thres_ms, VTY_NEWLINE);
	*/

}

void bts_model_config_write_trx(struct vty *vty, struct gsm_bts_trx *trx)
{
	vty_out(vty, "  nominal-tx-power %d%s", trx->nominal_power,VTY_NEWLINE);
}

void bts_model_config_write_phy(struct vty *vty, struct phy_link *plink)
{
}

void bts_model_config_write_phy_inst(struct vty *vty, struct phy_instance *pinst)
{
	int i;

	for (i = 0; i < 32; i++) {
		if (pinst->u.oc2g.dsp_trace_f & (1 << i)) {
			const char *name;
			name = get_value_string(oc2gbts_tracef_names, (1 << i));
			vty_out(vty, "  dsp-trace-flag %s%s", name,
				VTY_NEWLINE);
		}
	}
	if (pinst->u.oc2g.calib_path)
		vty_out(vty, "  trx-calibration-path %s%s",
			pinst->u.oc2g.calib_path, VTY_NEWLINE);

	vty_out(vty, "  max-cell-size %d%s",
			pinst->u.oc2g.max_cell_size, VTY_NEWLINE);

	vty_out(vty, "  pedestal-mode %s%s",
			get_value_string(oc2g_pedestal_mode_strs, pinst->u.oc2g.pedestal_mode) , VTY_NEWLINE);

	vty_out(vty, "  dsp-alive-period %d%s",
			pinst->u.oc2g.dsp_alive_period, VTY_NEWLINE);

	vty_out(vty, "  pwr-adj-mode %s%s",
			get_value_string(oc2g_auto_adj_pwr_strs, pinst->u.oc2g.tx_pwr_adj_mode), VTY_NEWLINE);

	vty_out(vty, "  tx-red-pwr-8psk %d%s",
			pinst->u.oc2g.tx_pwr_red_8psk, VTY_NEWLINE);

	vty_out(vty, "  c0-idle-red-pwr %d%s",
			pinst->u.oc2g.tx_c0_idle_pwr_red, VTY_NEWLINE);
}

int bts_model_vty_init(struct gsm_bts *bts)
{
	vty_bts = bts;

	/* runtime-patch the command strings with debug levels */
	dsp_trace_f_cmd.string = vty_cmd_string_from_valstr(bts, oc2gbts_tracef_names,
						"phy <0-1> dsp-trace-flag (",
						"|",")", VTY_DO_LOWER);
	dsp_trace_f_cmd.doc = vty_cmd_string_from_valstr(bts, oc2gbts_tracef_docs,
						TRX_STR DSP_TRACE_F_STR,
						"\n", "", 0);

	no_dsp_trace_f_cmd.string = vty_cmd_string_from_valstr(bts, oc2gbts_tracef_names,
						"no phy <0-1> dsp-trace-flag (",
						"|",")", VTY_DO_LOWER);
	no_dsp_trace_f_cmd.doc = vty_cmd_string_from_valstr(bts, oc2gbts_tracef_docs,
						NO_STR TRX_STR DSP_TRACE_F_STR,
						"\n", "", 0);

	cfg_phy_dsp_trace_f_cmd.string = vty_cmd_string_from_valstr(bts,
						oc2gbts_tracef_names,
						"dsp-trace-flag (",
						"|",")", VTY_DO_LOWER);
	cfg_phy_dsp_trace_f_cmd.doc = vty_cmd_string_from_valstr(bts,
						oc2gbts_tracef_docs,
						DSP_TRACE_F_STR,
						"\n", "", 0);

	cfg_phy_no_dsp_trace_f_cmd.string = vty_cmd_string_from_valstr(bts,
						oc2gbts_tracef_names,
						"no dsp-trace-flag (",
						"|",")", VTY_DO_LOWER);
	cfg_phy_no_dsp_trace_f_cmd.doc = vty_cmd_string_from_valstr(bts,
						oc2gbts_tracef_docs,
						NO_STR DSP_TRACE_F_STR,
						"\n", "", 0);

	trigger_ho_cause_cmd.string = vty_cmd_string_from_valstr(bts,
						oc2gbts_rsl_ho_causes,
						"trigger-ho-cause trx <0-1> ts <0-7> lchan <0-1> cause (",
						"|",")", VTY_DO_LOWER);

	install_element_ve(&show_dsp_trace_f_cmd);
	install_element_ve(&show_sys_info_cmd);
	install_element_ve(&dsp_trace_f_cmd);
	install_element_ve(&no_dsp_trace_f_cmd);

	install_element(ENABLE_NODE, &activate_lchan_cmd);
	install_element(ENABLE_NODE, &set_tx_power_cmd);

	install_element(ENABLE_NODE, &loopback_cmd);
	install_element(ENABLE_NODE, &no_loopback_cmd);

	install_element(ENABLE_NODE, &trigger_ho_cause_cmd);

	install_element(BTS_NODE, &cfg_bts_auto_band_cmd);
	install_element(BTS_NODE, &cfg_bts_no_auto_band_cmd);
	/* TODO(oramadan) MERGE 
	install_element(BTS_NODE, &cfg_bts_led_mode_cmd);
	install_element(BTS_NODE, &cfg_bts_rtp_drift_threshold_cmd);
	*/

	install_element(TRX_NODE, &cfg_trx_nominal_power_cmd);

	install_element(PHY_INST_NODE, &cfg_phy_dsp_trace_f_cmd);
	install_element(PHY_INST_NODE, &cfg_phy_no_dsp_trace_f_cmd);
	install_element(PHY_INST_NODE, &cfg_phy_cal_path_cmd);
	install_element(PHY_INST_NODE, &cfg_phy_pedestal_mode_cmd);
	install_element(PHY_INST_NODE, &cfg_phy_max_cell_size_cmd);
	install_element(PHY_INST_NODE, &cfg_phy_dsp_alive_timer_cmd);
	install_element(PHY_INST_NODE, &cfg_phy_auto_tx_pwr_adj_cmd);
	install_element(PHY_INST_NODE, &cfg_phy_tx_red_pwr_8psk_cmd);
	install_element(PHY_INST_NODE, &cfg_phy_c0_idle_red_pwr_cmd);

	return 0;
}

/* TODO(oramadan) MERGE 
/* OC2G BTS control interface * /
CTRL_CMD_DEFINE_WO_NOVRF(oc2g_oml_alert, "oc2g-oml-alert");
static int set_oc2g_oml_alert(struct ctrl_cmd *cmd, void *data)
{
	struct gsm_bts *bts = cmd->node;
	int cause = atoi(cmd->value);
	char *saveptr = NULL;

	alarm_sig_data.mo = &bts->mo;
	cause = atoi(strtok_r(cmd->value, ",",  &saveptr));
	alarm_sig_data.event_serverity = (cause >> 8) & 0x0F;
	alarm_sig_data.add_text = strtok_r(NULL, "\n",  &saveptr);
	memcpy(alarm_sig_data.spare, &cause, sizeof(int));
	LOGP(DLCTRL, LOGL_NOTICE, "BTS received MGR alarm cause=%d, text=%s\n", cause, alarm_sig_data.add_text);

	/* dispatch OML alarm signal * /
	osmo_signal_dispatch(SS_NM, S_NM_OML_BTS_MGR_ALARM, &alarm_sig_data);

	/* return with same alarm cause to MGR rather than OK string* /
	cmd->reply = talloc_asprintf(cmd, "%d", cause);
	return CTRL_CMD_REPLY;
}

CTRL_CMD_DEFINE_WO_NOVRF(oc2g_oml_ceased, "oc2g-oml-ceased");
static int set_oc2g_oml_ceased(struct ctrl_cmd *cmd, void *data)
{
	struct gsm_bts *bts = cmd->node;
	int cause = atoi(cmd->value);
	char *saveptr = NULL;

	alarm_sig_data.mo = &bts->mo;
	cause = atoi(strtok_r(cmd->value, ",",  &saveptr));
	alarm_sig_data.add_text = strtok_r(NULL, "\n",  &saveptr);
	memcpy(alarm_sig_data.spare, &cause, sizeof(int));
	LOGP(DLCTRL, LOGL_NOTICE, "BTS received MGR ceased alarm cause=%d, text=%s\n", cause, alarm_sig_data.add_text);

	/* dispatch OML alarm signal * /
	osmo_signal_dispatch(SS_NM, S_NM_OML_BTS_MGR_CEASED_ALARM, &alarm_sig_data);
	*/

	/* return with same alarm cause to MGR rather than OK string* /
	cmd->reply = talloc_asprintf(cmd, "%d", cause);
	return CTRL_CMD_REPLY;
}
*/

int bts_model_ctrl_cmds_install(struct gsm_bts *bts)
{
	int rc = 0;

	/* TODO(oramadan) MERGE 
	rc |= ctrl_cmd_install(CTRL_NODE_ROOT, &cmd_oc2g_oml_alert);
	rc |= ctrl_cmd_install(CTRL_NODE_ROOT, &cmd_oc2g_oml_ceased);
	*/

	return rc;
}
