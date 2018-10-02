/* VTY interface for NuRAN Wireless Litecell 1.5 */

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

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/phy_link.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/bts_model.h>
#include <osmo-bts/vty.h>

#include "lc15bts.h"
#include "l1_if.h"
#include "utils.h"

extern int lchan_activate(struct gsm_lchan *lchan);

#define TRX_STR "Transceiver related commands\n" "TRX number\n"

#define SHOW_TRX_STR				\
	SHOW_STR				\
	TRX_STR
#define DSP_TRACE_F_STR		"DSP Trace Flag\n"

static struct gsm_bts *vty_bts;

/* configuration */

DEFUN(cfg_phy_cal_path, cfg_phy_cal_path_cmd,
	"trx-calibration-path PATH",
	"Set the path name to TRX calibration data\n" "Path name\n")
{
	struct phy_instance *pinst = vty->index;

	if (pinst->u.lc15.calib_path)
		talloc_free(pinst->u.lc15.calib_path);

	pinst->u.lc15.calib_path = talloc_strdup(pinst, argv[0]);

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_dsp_trace_f, cfg_phy_dsp_trace_f_cmd,
	"HIDDEN", TRX_STR)
{
	struct phy_instance *pinst = vty->index;
	unsigned int flag;

	flag = get_string_value(lc15bts_tracef_names, argv[1]);
	pinst->u.lc15.dsp_trace_f |= ~flag;

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_no_dsp_trace_f, cfg_phy_no_dsp_trace_f_cmd,
	"HIDDEN", NO_STR TRX_STR)
{
	struct phy_instance *pinst = vty->index;
	unsigned int flag;

	flag = get_string_value(lc15bts_tracef_names, argv[1]);
	pinst->u.lc15.dsp_trace_f &= ~flag;

	return CMD_SUCCESS;
}


/* runtime */

DEFUN(show_dsp_trace_f, show_dsp_trace_f_cmd,
	"show trx <0-0> dsp-trace-flags",
	SHOW_TRX_STR "Display the current setting of the DSP trace flags")
{
	int trx_nr = atoi(argv[0]);
	struct gsm_bts_trx *trx = gsm_bts_trx_num(vty_bts, trx_nr);
	struct lc15l1_hdl *fl1h;
	int i;

	if (!trx)
		return CMD_WARNING;

	fl1h = trx_lc15l1_hdl(trx);

	vty_out(vty, "Litecell15 L1 DSP trace flags:%s", VTY_NEWLINE);
	for (i = 0; i < ARRAY_SIZE(lc15bts_tracef_names); i++) {
		const char *endis;

		if (lc15bts_tracef_names[i].value == 0 &&
		    lc15bts_tracef_names[i].str == NULL)
			break;

		if (fl1h->dsp_trace_f & lc15bts_tracef_names[i].value)
			endis = "enabled";
		else
			endis = "disabled";

		vty_out(vty, "DSP Trace %-15s %s%s",
			lc15bts_tracef_names[i].str, endis,
			VTY_NEWLINE);
	}

	return CMD_SUCCESS;

}

DEFUN(dsp_trace_f, dsp_trace_f_cmd, "HIDDEN", TRX_STR)
{
	int phy_nr = atoi(argv[0]);
	struct phy_instance *pinst;
	struct lc15l1_hdl *fl1h;
	unsigned int flag ;

	pinst = vty_get_phy_instance(vty, phy_nr, 0);
	if (!pinst)
		return CMD_WARNING;

	fl1h = pinst->u.lc15.hdl;
	flag = get_string_value(lc15bts_tracef_names, argv[1]);
	l1if_set_trace_flags(fl1h, fl1h->dsp_trace_f | flag);

	return CMD_SUCCESS;
}

DEFUN(no_dsp_trace_f, no_dsp_trace_f_cmd, "HIDDEN", NO_STR TRX_STR)
{
	int phy_nr = atoi(argv[0]);
	struct phy_instance *pinst;
	struct lc15l1_hdl *fl1h;
	unsigned int flag ;

	pinst = vty_get_phy_instance(vty, phy_nr, 0);
	if (!pinst)
		return CMD_WARNING;

	fl1h = pinst->u.lc15.hdl;
	flag = get_string_value(lc15bts_tracef_names, argv[1]);
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
	struct lc15l1_hdl *fl1h;
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
	fl1h = pinst->u.lc15.hdl;

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
	vty_out(vty, "Min Tx Power: %d dBm%s", fl1h->phy_inst->u.lc15.minTxPower, VTY_NEWLINE);
	vty_out(vty, "Max Tx Power: %d dBm%s", fl1h->phy_inst->u.lc15.maxTxPower, VTY_NEWLINE);

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

void bts_model_config_write_bts(struct vty *vty, struct gsm_bts *bts)
{
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
		if (pinst->u.lc15.dsp_trace_f & (1 << i)) {
			const char *name;
			name = get_value_string(lc15bts_tracef_names, (1 << i));
			vty_out(vty, "  dsp-trace-flag %s%s", name,
				VTY_NEWLINE);
		}
	}
	if (pinst->u.lc15.calib_path)
		vty_out(vty, "  trx-calibration-path %s%s",
			pinst->u.lc15.calib_path, VTY_NEWLINE);
}

int bts_model_vty_init(struct gsm_bts *bts)
{
	vty_bts = bts;

	/* runtime-patch the command strings with debug levels */
	dsp_trace_f_cmd.string = vty_cmd_string_from_valstr(bts, lc15bts_tracef_names,
						"phy <0-0> dsp-trace-flag (",
						"|",")", VTY_DO_LOWER);
	dsp_trace_f_cmd.doc = vty_cmd_string_from_valstr(bts, lc15bts_tracef_docs,
						TRX_STR DSP_TRACE_F_STR,
						"\n", "", 0);

	no_dsp_trace_f_cmd.string = vty_cmd_string_from_valstr(bts, lc15bts_tracef_names,
						"no phy <0-0> dsp-trace-flag (",
						"|",")", VTY_DO_LOWER);
	no_dsp_trace_f_cmd.doc = vty_cmd_string_from_valstr(bts, lc15bts_tracef_docs,
						NO_STR TRX_STR DSP_TRACE_F_STR,
						"\n", "", 0);

	cfg_phy_dsp_trace_f_cmd.string = vty_cmd_string_from_valstr(bts,
						lc15bts_tracef_names,
						"dsp-trace-flag (",
						"|",")", VTY_DO_LOWER);
	cfg_phy_dsp_trace_f_cmd.doc = vty_cmd_string_from_valstr(bts,
						lc15bts_tracef_docs,
						DSP_TRACE_F_STR,
						"\n", "", 0);

	cfg_phy_no_dsp_trace_f_cmd.string = vty_cmd_string_from_valstr(bts,
						lc15bts_tracef_names,
						"no dsp-trace-flag (",
						"|",")", VTY_DO_LOWER);
	cfg_phy_no_dsp_trace_f_cmd.doc = vty_cmd_string_from_valstr(bts,
						lc15bts_tracef_docs,
						NO_STR DSP_TRACE_F_STR,
						"\n", "", 0);

	install_element_ve(&show_dsp_trace_f_cmd);
	install_element_ve(&show_sys_info_cmd);
	install_element_ve(&dsp_trace_f_cmd);
	install_element_ve(&no_dsp_trace_f_cmd);

	install_element(ENABLE_NODE, &activate_lchan_cmd);
	install_element(ENABLE_NODE, &set_tx_power_cmd);

	install_element(ENABLE_NODE, &loopback_cmd);
	install_element(ENABLE_NODE, &no_loopback_cmd);

	install_element(BTS_NODE, &cfg_bts_auto_band_cmd);
	install_element(BTS_NODE, &cfg_bts_no_auto_band_cmd);

	install_element(TRX_NODE, &cfg_trx_nominal_power_cmd);

	install_element(PHY_INST_NODE, &cfg_phy_dsp_trace_f_cmd);
	install_element(PHY_INST_NODE, &cfg_phy_no_dsp_trace_f_cmd);
	install_element(PHY_INST_NODE, &cfg_phy_cal_path_cmd);

	return 0;
}

int bts_model_ctrl_cmds_install(struct gsm_bts *bts)
{
	return 0;
}
