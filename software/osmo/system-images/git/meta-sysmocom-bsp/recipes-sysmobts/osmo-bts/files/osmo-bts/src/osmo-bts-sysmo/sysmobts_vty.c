/* VTY interface for sysmoBTS */

/* (C) 2011 by Harald Welte <laforge@gnumonks.org>
 * (C) 2012,2013 by Holger Hans Peter Freyther
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
#include <osmo-bts/rsl.h>

#include "femtobts.h"
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

DEFUN(cfg_phy_clkcal_eeprom, cfg_phy_clkcal_eeprom_cmd,
	"clock-calibration eeprom",
	"Use the eeprom clock calibration value\n")
{
	struct phy_instance *pinst = vty->index;

	pinst->u.sysmobts.clk_use_eeprom = 1;

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_clkcal_def, cfg_phy_clkcal_def_cmd,
	"clock-calibration default",
	"Set the clock calibration value\n" "Default Clock DAC value\n")
{
	struct phy_instance *pinst = vty->index;

	pinst->u.sysmobts.clk_use_eeprom = 0;
	pinst->u.sysmobts.clk_cal = 0xffff;

	return CMD_SUCCESS;
}

#ifdef HW_SYSMOBTS_V1
DEFUN(cfg_phy_clkcal, cfg_phy_clkcal_cmd,
	"clock-calibration <0-4095>",
	"Set the clock calibration value\n" "Clock DAC value\n")
{
	unsigned int clkcal = atoi(argv[0]);
	struct phy_instance *pinst = vty->index;

	pinst->u.sysmobts.clk_use_eeprom = 0;
	pinst->u.sysmobts.clk_cal = clkcal & 0xfff;

	return CMD_SUCCESS;
}
#else
DEFUN(cfg_phy_clkcal, cfg_phy_clkcal_cmd,
	"clock-calibration <-4095-4095>",
	"Set the clock calibration value\n" "Offset in PPB\n")
{
	int clkcal = atoi(argv[0]);
	struct phy_instance *pinst = vty->index;

	pinst->u.sysmobts.clk_use_eeprom = 0;
	pinst->u.sysmobts.clk_cal = clkcal;

	return CMD_SUCCESS;
}
#endif

DEFUN(cfg_phy_clksrc, cfg_phy_clksrc_cmd,
	"clock-source (tcxo|ocxo|ext|gps)",
	"Set the clock source value\n"
	"Use the TCXO\n"
	"Use the OCXO\n"
	"Use an external clock\n"
	"Use the GPS pps\n")
{
	struct phy_instance *pinst = vty->index;
	int rc;

	rc = get_string_value(femtobts_clksrc_names, argv[0]);
	if (rc < 0)
		return CMD_WARNING;

	pinst->u.sysmobts.clk_src = rc;

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_cal_path, cfg_phy_cal_path_cmd,
	"trx-calibration-path PATH",
	"Set the path name to TRX calibration data\n" "Path name\n")
{
	struct phy_instance *pinst = vty->index;

	if (pinst->u.sysmobts.calib_path)
		talloc_free(pinst->u.sysmobts.calib_path);

	pinst->u.sysmobts.calib_path = talloc_strdup(pinst, argv[0]);

	return CMD_SUCCESS;
}

DEFUN_DEPRECATED(cfg_trx_ul_power_target, cfg_trx_ul_power_target_cmd,
	"uplink-power-target <-110-0>",
	"Obsolete alias for bts uplink-power-target\n"
	"Target uplink Rx level in dBm\n")
{
	struct gsm_bts_trx *trx = vty->index;

	trx->bts->ul_power_target = atoi(argv[0]);

	return CMD_SUCCESS;
}

DEFUN(cfg_trx_nominal_power, cfg_trx_nominal_power_cmd,
	"nominal-tx-power <0-100>",
	"Set the nominal transmit output power in dBm\n"
	"Nominal transmit output power level in dBm\n")
{
	struct gsm_bts_trx *trx = vty->index;

	trx->nominal_power = atoi(argv[0]);

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_dsp_trace_f, cfg_phy_dsp_trace_f_cmd,
	"HIDDEN", TRX_STR)
{
	struct phy_instance *pinst = vty->index;
	unsigned int flag;

	flag = get_string_value(femtobts_tracef_names, argv[1]);
	pinst->u.sysmobts.dsp_trace_f |= ~flag;

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_no_dsp_trace_f, cfg_phy_no_dsp_trace_f_cmd,
	"HIDDEN", NO_STR TRX_STR)
{
	struct phy_instance *pinst = vty->index;
	unsigned int flag;

	flag = get_string_value(femtobts_tracef_names, argv[1]);
	pinst->u.sysmobts.dsp_trace_f &= ~flag;

	return CMD_SUCCESS;
}

/* runtime */

DEFUN(show_phy_clksrc, show_trx_clksrc_cmd,
	"show phy <0-255> clock-source",
	SHOW_TRX_STR "Display the clock source for this TRX")
{
	int phy_nr = atoi(argv[0]);
	struct phy_instance *pinst = vty_get_phy_instance(vty, phy_nr, 0);

	if (!pinst)
		return CMD_WARNING;

	vty_out(vty, "PHY Clock Source: %s%s",
		get_value_string(femtobts_clksrc_names,
				 pinst->u.sysmobts.clk_src), VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(show_dsp_trace_f, show_dsp_trace_f_cmd,
	"show trx <0-0> dsp-trace-flags",
	SHOW_TRX_STR "Display the current setting of the DSP trace flags")
{
	int trx_nr = atoi(argv[0]);
	struct gsm_bts_trx *trx = gsm_bts_trx_num(vty_bts, trx_nr);
	struct femtol1_hdl *fl1h;
	int i;

	if (!trx)
		return CMD_WARNING;

	fl1h = trx_femtol1_hdl(trx);

	vty_out(vty, "Femto L1 DSP trace flags:%s", VTY_NEWLINE);
	for (i = 0; i < ARRAY_SIZE(femtobts_tracef_names); i++) {
		const char *endis;

		if (femtobts_tracef_names[i].value == 0 &&
		    femtobts_tracef_names[i].str == NULL)
			break;

		if (fl1h->dsp_trace_f & femtobts_tracef_names[i].value)
			endis = "enabled";
		else
			endis = "disabled";

		vty_out(vty, "DSP Trace %-15s %s%s",
			femtobts_tracef_names[i].str, endis,
			VTY_NEWLINE);
	}

	return CMD_SUCCESS;

}

DEFUN(dsp_trace_f, dsp_trace_f_cmd, "HIDDEN", TRX_STR)
{
	int phy_nr = atoi(argv[0]);
	struct phy_instance *pinst;
	struct femtol1_hdl *fl1h;
	unsigned int flag ;

	pinst = vty_get_phy_instance(vty, phy_nr, 0);
	if (!pinst)
		return CMD_WARNING;

	fl1h = pinst->u.sysmobts.hdl;
	flag = get_string_value(femtobts_tracef_names, argv[1]);
	l1if_set_trace_flags(fl1h, fl1h->dsp_trace_f | flag);

	return CMD_SUCCESS;
}

DEFUN(no_dsp_trace_f, no_dsp_trace_f_cmd, "HIDDEN", NO_STR TRX_STR)
{
	int phy_nr = atoi(argv[0]);
	struct phy_instance *pinst;
	struct femtol1_hdl *fl1h;
	unsigned int flag ;

	pinst = vty_get_phy_instance(vty, phy_nr, 0);
	if (!pinst)
		return CMD_WARNING;

	fl1h = pinst->u.sysmobts.hdl;
	flag = get_string_value(femtobts_tracef_names, argv[1]);
	l1if_set_trace_flags(fl1h, fl1h->dsp_trace_f & ~flag);

	return CMD_SUCCESS;
}

DEFUN(show_sys_info, show_sys_info_cmd,
	"show phy <0-255> instance <0-255> system-information",
	SHOW_TRX_STR "Display information about system\n")
{
	int phy_nr = atoi(argv[0]);
	int inst_nr = atoi(argv[1]);
	struct phy_link *plink = phy_link_by_num(phy_nr);
	struct phy_instance *pinst;
	struct femtol1_hdl *fl1h;
	int i;

	if (!plink) {
		vty_out(vty, "Cannot find PHY link %u%s",
			phy_nr, VTY_NEWLINE);
		return CMD_WARNING;
	}
	pinst = phy_instance_by_num(plink, inst_nr);
	if (!pinst) {
		vty_out(vty, "Cannot find PHY instance %u%s",
			phy_nr, VTY_NEWLINE);
		return CMD_WARNING;
	}
	fl1h = pinst->u.sysmobts.hdl;

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
	"trx <0-0> tx-power <-110-100>",
	TRX_STR
	"Set transmit power (override BSC)\n"
	"Transmit power in dBm\n")
{
	int trx_nr = atoi(argv[0]);
	int power = atoi(argv[1]);
	struct gsm_bts_trx *trx = gsm_bts_trx_num(vty_bts, trx_nr);

	power_ramp_start(trx, to_mdB(power), 1);

	return CMD_SUCCESS;
}

DEFUN(reset_rf_clock_ctr, reset_rf_clock_ctr_cmd,
      "trx <0-0> rf-clock-info reset",
      TRX_STR
      "RF Clock Information\n" "Reset the counter\n")
{
	int trx_nr = atoi(argv[0]);
	struct gsm_bts_trx *trx = gsm_bts_trx_num(vty_bts, trx_nr);
	struct femtol1_hdl *fl1h = trx_femtol1_hdl(trx);

	l1if_rf_clock_info_reset(fl1h);
	return CMD_SUCCESS;
}

DEFUN(correct_rf_clock_ctr, correct_rf_clock_ctr_cmd,
      "trx <0-0> rf-clock-info correct",
      TRX_STR
      "RF Clock Information\n" "Apply\n")
{
	int trx_nr = atoi(argv[0]);
	struct gsm_bts_trx *trx = gsm_bts_trx_num(vty_bts, trx_nr);
	struct femtol1_hdl *fl1h = trx_femtol1_hdl(trx);

	l1if_rf_clock_info_correct(fl1h);
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


void bts_model_config_write_bts(struct vty *vty, struct gsm_bts *bts)
{
}

void bts_model_config_write_trx(struct vty *vty, struct gsm_bts_trx *trx)
{
	if (trx->nominal_power != get_p_max_out_mdBm(trx))
		vty_out(vty, "  nominal-tx-power %d%s", trx->nominal_power,
			VTY_NEWLINE);
}

void bts_model_config_write_phy(struct vty *vty, struct phy_link *plink)
{
}

void bts_model_config_write_phy_inst(struct vty *vty, struct phy_instance *pinst)
{
	int i;

	for (i = 0; i < 32; i++) {
		if (pinst->u.sysmobts.dsp_trace_f & (1 << i)) {
			const char *name;
			name = get_value_string(femtobts_tracef_names, (1 << i));
			vty_out(vty, "  dsp-trace-flag %s%s", name,
				VTY_NEWLINE);
		}
	}

	if (pinst->u.sysmobts.clk_use_eeprom)
		vty_out(vty, "  clock-calibration eeprom%s", VTY_NEWLINE);
	else
		vty_out(vty, "  clock-calibration %d%s",
			pinst->u.sysmobts.clk_cal, VTY_NEWLINE);
	if (pinst->u.sysmobts.calib_path)
		vty_out(vty, "  trx-calibration-path %s%s",
			pinst->u.sysmobts.calib_path, VTY_NEWLINE);
	if (pinst->u.sysmobts.clk_src)
		vty_out(vty, "  clock-source %s%s",
			get_value_string(femtobts_clksrc_names,
					 pinst->u.sysmobts.clk_src), VTY_NEWLINE);
}

int bts_model_vty_init(struct gsm_bts *bts)
{
	vty_bts = bts;

	/* runtime-patch the command strings with debug levels */
	dsp_trace_f_cmd.string = vty_cmd_string_from_valstr(bts, femtobts_tracef_names,
						"trx <0-0> dsp-trace-flag (",
						"|",")", VTY_DO_LOWER);
	dsp_trace_f_cmd.doc = vty_cmd_string_from_valstr(bts, femtobts_tracef_docs,
						TRX_STR DSP_TRACE_F_STR,
						"\n", "", 0);

	no_dsp_trace_f_cmd.string = vty_cmd_string_from_valstr(bts, femtobts_tracef_names,
						"no trx <0-0> dsp-trace-flag (",
						"|",")", VTY_DO_LOWER);
	no_dsp_trace_f_cmd.doc = vty_cmd_string_from_valstr(bts, femtobts_tracef_docs,
						NO_STR TRX_STR DSP_TRACE_F_STR,
						"\n", "", 0);

	cfg_phy_dsp_trace_f_cmd.string =
		vty_cmd_string_from_valstr(bts, femtobts_tracef_names,
					   "dsp-trace-flag (", "|", ")",
					   VTY_DO_LOWER);
	cfg_phy_dsp_trace_f_cmd.doc =
		vty_cmd_string_from_valstr(bts, femtobts_tracef_docs,
					   DSP_TRACE_F_STR, "\n", "", 0);

	cfg_phy_no_dsp_trace_f_cmd.string =
		vty_cmd_string_from_valstr(bts, femtobts_tracef_names,
					   "no dsp-trace-flag (", "|", ")",
					   VTY_DO_LOWER);
	cfg_phy_no_dsp_trace_f_cmd.doc =
		vty_cmd_string_from_valstr(bts, femtobts_tracef_docs,
					   NO_STR DSP_TRACE_F_STR, "\n",
					   "", 0);

	install_element_ve(&show_dsp_trace_f_cmd);
	install_element_ve(&show_sys_info_cmd);
	install_element_ve(&show_trx_clksrc_cmd);
	install_element_ve(&dsp_trace_f_cmd);
	install_element_ve(&no_dsp_trace_f_cmd);

	install_element(ENABLE_NODE, &activate_lchan_cmd);
	install_element(ENABLE_NODE, &set_tx_power_cmd);
	install_element(ENABLE_NODE, &reset_rf_clock_ctr_cmd);
	install_element(ENABLE_NODE, &correct_rf_clock_ctr_cmd);

	install_element(ENABLE_NODE, &loopback_cmd);
	install_element(ENABLE_NODE, &no_loopback_cmd);

	install_element(BTS_NODE, &cfg_bts_auto_band_cmd);
	install_element(BTS_NODE, &cfg_bts_no_auto_band_cmd);

	install_element(TRX_NODE, &cfg_trx_ul_power_target_cmd);
	install_element(TRX_NODE, &cfg_trx_nominal_power_cmd);

	install_element(PHY_INST_NODE, &cfg_phy_dsp_trace_f_cmd);
	install_element(PHY_INST_NODE, &cfg_phy_no_dsp_trace_f_cmd);
	install_element(PHY_INST_NODE, &cfg_phy_clkcal_cmd);
	install_element(PHY_INST_NODE, &cfg_phy_clkcal_eeprom_cmd);
	install_element(PHY_INST_NODE, &cfg_phy_clkcal_def_cmd);
	install_element(PHY_INST_NODE, &cfg_phy_clksrc_cmd);
	install_element(PHY_INST_NODE, &cfg_phy_cal_path_cmd);

	return 0;
}
