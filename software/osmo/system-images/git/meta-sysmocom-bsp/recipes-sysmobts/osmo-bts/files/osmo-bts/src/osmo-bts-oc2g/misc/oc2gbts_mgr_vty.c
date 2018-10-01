/* Copyright (C) 2015 by Yves Godin <support@nuranwireless.com>
 * 
 * Based on sysmoBTS:
 *     sysmobts_mgr_vty.c
 *     (C) 2014 by oc2gcom - s.f.m.c. GmbH
 *
 * All Rights Reserved
 *
 * Author: Alvaro Neira Ayuso <anayuso@oc2gcom.de>
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
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>

#include <osmocom/vty/vty.h>
#include <osmocom/vty/command.h>
#include <osmocom/vty/misc.h>

#include <osmo-bts/logging.h>

#include "oc2gbts_misc.h"
#include "oc2gbts_mgr.h"
#include "oc2gbts_temp.h"
#include "oc2gbts_power.h"
#include "oc2gbts_bid.h"
#include "oc2gbts_led.h"
#include "btsconfig.h"

static struct oc2gbts_mgr_instance *s_mgr;

static const char copyright[] =
	"(C) 2012 by Harald Welte <laforge@gnumonks.org>\r\n"
	"(C) 2014 by Holger Hans Peter Freyther\r\n"
	"(C) 2015 by Yves Godin <support@nuranwireless.com>\r\n"
	"License AGPLv3+: GNU AGPL version 2 or later <http://gnu.org/licenses/agpl-3.0.html>\r\n"
	"This is free software: you are free to change and redistribute it.\r\n"
	"There is NO WARRANTY, to the extent permitted by law.\r\n";

static int go_to_parent(struct vty *vty)
{
	switch (vty->node) {
	case MGR_NODE:
		vty->node = CONFIG_NODE;
		break;
	case ACT_NORM_NODE:
	case ACT_WARN_NODE:
	case ACT_CRIT_NODE:
	case LIMIT_SUPPLY_TEMP_NODE:
	case LIMIT_SOC_NODE:
	case LIMIT_FPGA_NODE:
	case LIMIT_RMSDET_NODE:
	case LIMIT_OCXO_NODE:
	case LIMIT_TX_TEMP_NODE:
	case LIMIT_PA_TEMP_NODE:
	case LIMIT_SUPPLY_VOLT_NODE:
	case LIMIT_VSWR_NODE:
	case LIMIT_SUPPLY_PWR_NODE:
	case LIMIT_PA_PWR_NODE:
		vty->node = MGR_NODE;
		break;
	default:
		vty->node = CONFIG_NODE;
	}
	return vty->node;
}

static int is_config_node(struct vty *vty, int node)
{
	switch (node) {
	case MGR_NODE:
	case ACT_NORM_NODE:
	case ACT_WARN_NODE:
	case ACT_CRIT_NODE:
	case LIMIT_SUPPLY_TEMP_NODE:
	case LIMIT_SOC_NODE:
	case LIMIT_FPGA_NODE:
	case LIMIT_RMSDET_NODE:
	case LIMIT_OCXO_NODE:
	case LIMIT_TX_TEMP_NODE:
	case LIMIT_PA_TEMP_NODE:
	case LIMIT_SUPPLY_VOLT_NODE:
	case LIMIT_VSWR_NODE:
	case LIMIT_SUPPLY_PWR_NODE:
	case LIMIT_PA_PWR_NODE:
		return 1;
	default:
		return 0;
	}
}

static struct vty_app_info vty_info = {
	.name           = "oc2gbts-mgr",
	.version        = PACKAGE_VERSION,
	.go_parent_cb   = go_to_parent,
	.is_config_node = is_config_node,
	.copyright	= copyright,
};


#define MGR_STR			"Configure oc2gbts-mgr\n"

static struct cmd_node mgr_node = {
	MGR_NODE,
	"%s(oc2gbts-mgr)# ",
	1,
};

static struct cmd_node act_norm_node = {
	ACT_NORM_NODE,
	"%s(actions-normal)# ",
	1,
};

static struct cmd_node act_warn_node = {
	ACT_WARN_NODE,
	"%s(actions-warn)# ",
	1,
};

static struct cmd_node act_crit_node = {
	ACT_CRIT_NODE,
	"%s(actions-critical)# ",
	1,
};

static struct cmd_node limit_supply_temp_node = {
	LIMIT_SUPPLY_TEMP_NODE,
	"%s(limit-supply-temp)# ",
	1,
};

static struct cmd_node limit_soc_node = {
	LIMIT_SOC_NODE,
	"%s(limit-soc)# ",
	1,
};

static struct cmd_node limit_fpga_node = {
	LIMIT_FPGA_NODE,
	"%s(limit-fpga)# ",
	1,
};

static struct cmd_node limit_rmsdet_node = {
	LIMIT_RMSDET_NODE,
	"%s(limit-rmsdet)# ",
	1,
};

static struct cmd_node limit_ocxo_node = {
	LIMIT_OCXO_NODE,
	"%s(limit-ocxo)# ",
	1,
};

static struct cmd_node limit_tx_temp_node = {
	LIMIT_TX_TEMP_NODE,
	"%s(limit-tx-temp)# ",
	1,
};
static struct cmd_node limit_pa_temp_node = {
	LIMIT_PA_TEMP_NODE,
	"%s(limit-pa-temp)# ",
	1,
};
static struct cmd_node limit_supply_volt_node = {
	LIMIT_SUPPLY_VOLT_NODE,
	"%s(limit-supply-volt)# ",
	1,
};
static struct cmd_node limit_vswr_node = {
	LIMIT_VSWR_NODE,
	"%s(limit-vswr)# ",
	1,
};
static struct cmd_node limit_supply_pwr_node = {
	LIMIT_SUPPLY_PWR_NODE,
	"%s(limit-supply-pwr)# ",
	1,
};
static struct cmd_node limit_pa_pwr_node = {
	LIMIT_PA_PWR_NODE,
	"%s(limit-pa-pwr)# ",
	1,
};

static struct cmd_node limit_gps_fix_node = {
	LIMIT_GPS_FIX_NODE,
	"%s(limit-gps-fix)# ",
	1,
};

DEFUN(cfg_mgr, cfg_mgr_cmd,
	"oc2gbts-mgr",
	MGR_STR)
{
	vty->node = MGR_NODE;
	return CMD_SUCCESS;
}

static void write_volt_limit(struct vty *vty, const char *name,
				struct oc2gbts_volt_limit *limit)
{
	vty_out(vty, " %s%s", name, VTY_NEWLINE);
	vty_out(vty, "   threshold warning min %d%s",
		limit->thresh_warn_min, VTY_NEWLINE);
	vty_out(vty, "   threshold critical min %d%s",
		limit->thresh_crit_min, VTY_NEWLINE);
}

static void write_vswr_limit(struct vty *vty, const char *name,
				struct oc2gbts_vswr_limit *limit)
{
	vty_out(vty, " %s%s", name, VTY_NEWLINE);
	vty_out(vty, "   threshold warning max %d%s",
		limit->thresh_warn_max, VTY_NEWLINE);
}

static void write_pwr_limit(struct vty *vty, const char *name,
				struct oc2gbts_pwr_limit *limit)
{
	vty_out(vty, " %s%s", name, VTY_NEWLINE);
	vty_out(vty, "   threshold warning max %d%s",
		limit->thresh_warn_max, VTY_NEWLINE);
	vty_out(vty, "   threshold critical max %d%s",
		limit->thresh_crit_max, VTY_NEWLINE);
}

static void write_norm_action(struct vty *vty, const char *name, int actions)
{
	vty_out(vty, " %s%s", name, VTY_NEWLINE);
	vty_out(vty, "  %spa-on%s",
		(actions & SENSOR_ACT_NORM_PA_ON) ? "" : "no ", VTY_NEWLINE);
	vty_out(vty, "  %sbts-service-on%s",
		(actions & SENSOR_ACT_NORM_BTS_SRV_ON) ? "" : "no ", VTY_NEWLINE);
}

static void write_action(struct vty *vty, const char *name, int actions)
{
	vty_out(vty, " %s%s", name, VTY_NEWLINE);
	vty_out(vty, "  %spa-off%s",
		(actions & SENSOR_ACT_PA_OFF) ? "" : "no ", VTY_NEWLINE);
	vty_out(vty, "  %sbts-service-off%s",
		(actions & SENSOR_ACT_BTS_SRV_OFF) ? "" : "no ", VTY_NEWLINE);
}

static int config_write_mgr(struct vty *vty)
{
	vty_out(vty, "oc2gbts-mgr%s", VTY_NEWLINE);

	write_volt_limit(vty, "limits supply_volt", &s_mgr->volt.supply_volt_limit);
	write_pwr_limit(vty, "limits supply_pwr", &s_mgr->pwr.supply_pwr_limit);
	write_vswr_limit(vty, "limits vswr", &s_mgr->vswr.vswr_limit);

	write_norm_action(vty, "actions normal", s_mgr->state.action_norm);
	write_action(vty, "actions warn", s_mgr->state.action_warn);
	write_action(vty, "actions critical", s_mgr->state.action_crit);

	return CMD_SUCCESS;
}

static int config_write_dummy(struct vty *vty)
{
	return CMD_SUCCESS;
}

#define CFG_LIMIT_TEMP(name, expl, switch_to, variable)			\
DEFUN(cfg_limit_##name, cfg_limit_##name##_cmd,				\
	"limits " #name,						\
	"Configure Limits\n" expl)					\
{									\
	vty->node = switch_to;						\
	vty->index = &s_mgr->temp.variable;				\
	return CMD_SUCCESS;						\
}

CFG_LIMIT_TEMP(supply_temp, "SUPPLY TEMP\n", LIMIT_SUPPLY_TEMP_NODE, supply_temp_limit)
CFG_LIMIT_TEMP(soc_temp, "SOC TEMP\n", LIMIT_SOC_NODE, soc_temp_limit)
CFG_LIMIT_TEMP(fpga_temp, "FPGA TEMP\n", LIMIT_FPGA_NODE, fpga_temp_limit)
CFG_LIMIT_TEMP(rmsdet_temp, "RMSDET TEMP\n", LIMIT_RMSDET_NODE, rmsdet_temp_limit)
CFG_LIMIT_TEMP(ocxo_temp, "OCXO TEMP\n", LIMIT_OCXO_NODE, ocxo_temp_limit)
CFG_LIMIT_TEMP(tx_temp, "TX TEMP\n", LIMIT_TX_TEMP_NODE, tx_temp_limit)
CFG_LIMIT_TEMP(pa_temp, "PA TEMP\n", LIMIT_PA_TEMP_NODE, pa_temp_limit)
#undef CFG_LIMIT_TEMP

#define CFG_LIMIT_VOLT(name, expl, switch_to, variable)			\
DEFUN(cfg_limit_##name, cfg_limit_##name##_cmd,				\
	"limits " #name,						\
	"Configure Limits\n" expl)					\
{									\
	vty->node = switch_to;						\
	vty->index = &s_mgr->volt.variable;				\
	return CMD_SUCCESS;						\
}

CFG_LIMIT_VOLT(supply_volt, "SUPPLY VOLT\n", LIMIT_SUPPLY_VOLT_NODE, supply_volt_limit)
#undef CFG_LIMIT_VOLT

#define CFG_LIMIT_VSWR(name, expl, switch_to, variable)			\
DEFUN(cfg_limit_##name, cfg_limit_##name##_cmd,				\
	"limits " #name,						\
	"Configure Limits\n" expl)					\
{									\
	vty->node = switch_to;						\
	vty->index = &s_mgr->vswr.variable;				\
	return CMD_SUCCESS;						\
}

CFG_LIMIT_VSWR(vswr, "VSWR\n", LIMIT_VSWR_NODE, vswr_limit)
#undef CFG_LIMIT_VSWR

#define CFG_LIMIT_PWR(name, expl, switch_to, variable)			\
DEFUN(cfg_limit_##name, cfg_limit_##name##_cmd,				\
	"limits " #name,						\
	"Configure Limits\n" expl)					\
{									\
	vty->node = switch_to;						\
	vty->index = &s_mgr->pwr.variable;				\
	return CMD_SUCCESS;						\
}

CFG_LIMIT_PWR(supply_pwr, "SUPPLY PWR\n", LIMIT_SUPPLY_PWR_NODE, supply_pwr_limit)
CFG_LIMIT_PWR(pa_pwr, "PA PWR\n", LIMIT_PA_PWR_NODE, pa_pwr_limit)
#undef CFG_LIMIT_PWR

#define CFG_LIMIT_GPS_FIX(name, expl, switch_to, variable)			\
DEFUN(cfg_limit_##name, cfg_limit_##name##_cmd,				\
	"limits " #name,						\
	"Configure Limits\n" expl)					\
{									\
	vty->node = switch_to;						\
	vty->index = &s_mgr->gps.variable;				\
	return CMD_SUCCESS;						\
}

CFG_LIMIT_GPS_FIX(gps_fix, "GPS FIX\n", LIMIT_GPS_FIX_NODE, gps_fix_limit)
#undef CFG_LIMIT_GPS_FIX

DEFUN(cfg_limit_volt_warn_min, cfg_thresh_volt_warn_min_cmd,
	"threshold warning min <0-48000>",
	"Threshold to reach\n" "Warning level\n" "Range\n")
{
	struct oc2gbts_volt_limit *limit = vty->index;
	limit->thresh_warn_min = atoi(argv[0]);
	return CMD_SUCCESS;
}

DEFUN(cfg_limit_volt_crit_min, cfg_thresh_volt_crit_min_cmd,
	"threshold critical min <0-48000>",
	"Threshold to reach\n" "Critical level\n" "Range\n")
{
	struct oc2gbts_volt_limit *limit = vty->index;
	limit->thresh_crit_min = atoi(argv[0]);
	return CMD_SUCCESS;
}

DEFUN(cfg_limit_vswr_warn_max, cfg_thresh_vswr_warn_max_cmd,
	"threshold warning max <1000-200000>",
	"Threshold to reach\n" "Warning level\n" "Range\n")
{
	struct oc2gbts_vswr_limit *limit = vty->index;
	limit->thresh_warn_max = atoi(argv[0]);
	return CMD_SUCCESS;
}

DEFUN(cfg_limit_vswr_crit_max, cfg_thresh_vswr_crit_max_cmd,
	"threshold critical max <1000-200000>",
	"Threshold to reach\n" "Warning level\n" "Range\n")
{
	struct oc2gbts_vswr_limit *limit = vty->index;
	limit->thresh_crit_max = atoi(argv[0]);
	return CMD_SUCCESS;
}

DEFUN(cfg_limit_pwr_warn_max, cfg_thresh_pwr_warn_max_cmd,
	"threshold warning max <0-200>",
	"Threshold to reach\n" "Warning level\n" "Range\n")
{
	struct oc2gbts_pwr_limit *limit = vty->index;
	limit->thresh_warn_max = atoi(argv[0]);
	return CMD_SUCCESS;
}

DEFUN(cfg_limit_pwr_crit_max, cfg_thresh_pwr_crit_max_cmd,
	"threshold critical max <0-200>",
	"Threshold to reach\n" "Warning level\n" "Range\n")
{
	struct oc2gbts_pwr_limit *limit = vty->index;
	limit->thresh_crit_max = atoi(argv[0]);
	return CMD_SUCCESS;
}

#define CFG_ACTION(name, expl, switch_to, variable)			\
DEFUN(cfg_action_##name, cfg_action_##name##_cmd,			\
	"actions " #name,						\
	"Configure Actions\n" expl)					\
{									\
	vty->node = switch_to;						\
	vty->index = &s_mgr->state.variable;				\
	return CMD_SUCCESS;						\
}
CFG_ACTION(normal, "Normal Actions\n", ACT_NORM_NODE, action_norm)
CFG_ACTION(warn, "Warning Actions\n", ACT_WARN_NODE, action_warn)
CFG_ACTION(critical, "Critical Actions\n", ACT_CRIT_NODE, action_crit)
#undef CFG_ACTION

DEFUN(cfg_action_pa_on, cfg_action_pa_on_cmd,
	"pa-on",
	"Switch the Power Amplifier on\n")
{
	int *action = vty->index;
	*action |= SENSOR_ACT_NORM_PA_ON;
	return CMD_SUCCESS;
}

DEFUN(cfg_no_action_pa_on, cfg_no_action_pa_on_cmd,
	"no pa-on",
	NO_STR "Switch the Power Amplifier on\n")
{
	int *action = vty->index;
	*action &= ~SENSOR_ACT_NORM_PA_ON;
	return CMD_SUCCESS;
}

DEFUN(cfg_action_bts_srv_on, cfg_action_bts_srv_on_cmd,
	"bts-service-on",
	"Start the systemd oc2gbts.service\n")
{
	int *action = vty->index;
	*action |= SENSOR_ACT_NORM_BTS_SRV_ON;
	return CMD_SUCCESS;
}

DEFUN(cfg_no_action_bts_srv_on, cfg_no_action_bts_srv_on_cmd,
	"no bts-service-on",
	NO_STR "Start the systemd oc2gbts.service\n")
{
	int *action = vty->index;
	*action &= ~SENSOR_ACT_NORM_BTS_SRV_ON;
	return CMD_SUCCESS;
}

DEFUN(cfg_action_pa_off, cfg_action_pa_off_cmd,
	"pa-off",
	"Switch the Power Amplifier off\n")
{
	int *action = vty->index;
	*action |= SENSOR_ACT_PA_OFF;
	return CMD_SUCCESS;
}

DEFUN(cfg_no_action_pa_off, cfg_no_action_pa_off_cmd,
	"no pa-off",
	NO_STR "Do not switch off the Power Amplifier\n")
{
	int *action = vty->index;
	*action &= ~SENSOR_ACT_PA_OFF;
	return CMD_SUCCESS;
}

DEFUN(cfg_action_bts_srv_off, cfg_action_bts_srv_off_cmd,
	"bts-service-off",
	"Stop the systemd oc2gbts.service\n")
{
	int *action = vty->index;
	*action |= SENSOR_ACT_BTS_SRV_OFF;
	return CMD_SUCCESS;
}

DEFUN(cfg_no_action_bts_srv_off, cfg_no_action_bts_srv_off_cmd,
	"no bts-service-off",
	NO_STR "Stop the systemd oc2gbts.service\n")
{
	int *action = vty->index;
	*action &= ~SENSOR_ACT_BTS_SRV_OFF;
	return CMD_SUCCESS;
}

DEFUN(show_mgr, show_mgr_cmd, "show manager",
      SHOW_STR "Display information about the manager")
{
	int temp, volt, current, power, vswr;
	vty_out(vty, "Warning alarm flags: 0x%08x%s",
		s_mgr->oc2gbts_ctrl.warn_flags, VTY_NEWLINE);
	vty_out(vty, "Critical alarm flags: 0x%08x%s",
		s_mgr->oc2gbts_ctrl.crit_flags, VTY_NEWLINE);
	vty_out(vty, "Preventive action retried: %d%s",
		s_mgr->alarms.preventive_retry, VTY_NEWLINE);
	vty_out(vty, "Temperature control state: %s%s",
		oc2gbts_mgr_sensor_get_state(s_mgr->state.state), VTY_NEWLINE);
	vty_out(vty, "Current Temperatures%s", VTY_NEWLINE);
	oc2gbts_temp_get(OC2GBTS_TEMP_SUPPLY, &temp);
	vty_out(vty, " Main Supply : %4.2f Celcius%s",
		 temp/ 1000.0f,
		VTY_NEWLINE);
	oc2gbts_temp_get(OC2GBTS_TEMP_SOC, &temp);
	vty_out(vty, " SoC         : %4.2f Celcius%s",
		temp / 1000.0f,
		VTY_NEWLINE);
	oc2gbts_temp_get(OC2GBTS_TEMP_FPGA, &temp);
	vty_out(vty, " FPGA        : %4.2f Celcius%s",
		temp / 1000.0f,
		VTY_NEWLINE);
	if (oc2gbts_option_get(OC2GBTS_OPTION_RMS_FWD) ||
			oc2gbts_option_get(OC2GBTS_OPTION_RMS_REFL)) {
		oc2gbts_temp_get(OC2GBTS_TEMP_RMSDET, &temp);
		vty_out(vty, " RMSDet      : %4.2f Celcius%s",
			temp / 1000.0f,
			VTY_NEWLINE);
	}
	oc2gbts_temp_get(OC2GBTS_TEMP_OCXO, &temp);
	vty_out(vty, " OCXO        : %4.2f Celcius%s",
		temp / 1000.0f,
		VTY_NEWLINE);
	oc2gbts_temp_get(OC2GBTS_TEMP_TX, &temp);
	vty_out(vty, " TX          : %4.2f Celcius%s",
		temp / 1000.0f,
		VTY_NEWLINE);
	if (oc2gbts_option_get(OC2GBTS_OPTION_PA_TEMP)) {
		oc2gbts_temp_get(OC2GBTS_TEMP_PA, &temp);
		vty_out(vty, " Power Amp   : %4.2f Celcius%s",
			temp / 1000.0f,
			VTY_NEWLINE);
	}
	vty_out(vty, "Power Status%s", VTY_NEWLINE);
	oc2gbts_power_sensor_get(OC2GBTS_POWER_SUPPLY,
			OC2GBTS_POWER_VOLTAGE, &volt);
	oc2gbts_power_sensor_get(OC2GBTS_POWER_SUPPLY,
			OC2GBTS_POWER_CURRENT, &current);
	oc2gbts_power_sensor_get(OC2GBTS_POWER_SUPPLY,
			OC2GBTS_POWER_POWER, &power);
	vty_out(vty, " Main Supply :  ON  [%6.2f Vdc, %4.2f A, %6.2f W]%s",
		volt /1000.0f,
		current /1000.0f,
		power /1000000.0f,
		VTY_NEWLINE);
	if (oc2gbts_option_get(OC2GBTS_OPTION_PA)) {
		oc2gbts_power_sensor_get(OC2GBTS_POWER_PA,
				OC2GBTS_POWER_VOLTAGE, &volt);
		oc2gbts_power_sensor_get(OC2GBTS_POWER_PA,
				OC2GBTS_POWER_CURRENT, &current);
		oc2gbts_power_sensor_get(OC2GBTS_POWER_PA,
				OC2GBTS_POWER_POWER, &power);
		vty_out(vty, " Power Amp   :  %s [%6.2f Vdc, %4.2f A, %6.2f W]%s",
			oc2gbts_power_get(OC2GBTS_POWER_PA) ? "ON " : "OFF",
			volt /1000.0f,
			current /1000.0f,
			power /1000000.0f,
			VTY_NEWLINE);
	}
	if (oc2gbts_option_get(OC2GBTS_OPTION_PA) &&
			oc2gbts_option_get(OC2GBTS_OPTION_RMS_FWD) &&
			oc2gbts_option_get(OC2GBTS_OPTION_RMS_REFL)) {
		vty_out(vty, "VSWR Status%s", VTY_NEWLINE);
		oc2gbts_vswr_get(OC2GBTS_VSWR, &vswr);
		vty_out(vty, " VSWR        : %f %s",
			vswr / 1000.0f,
			VTY_NEWLINE);
	}
	return CMD_SUCCESS;
}

DEFUN(show_thresh, show_thresh_cmd, "show thresholds",
      SHOW_STR "Display information about the thresholds")
{
	vty_out(vty, "Temperature limits (Celsius)%s", VTY_NEWLINE);
	vty_out(vty, "  Main supply%s", VTY_NEWLINE);
	vty_out(vty, "    Critical max : %d%s",s_mgr->temp.supply_temp_limit.thresh_crit_max, VTY_NEWLINE);
	vty_out(vty, "    Warning max  : %d%s",s_mgr->temp.supply_temp_limit.thresh_warn_max, VTY_NEWLINE);
	vty_out(vty, "    Warning min  : %d%s",s_mgr->temp.supply_temp_limit.thresh_warn_min, VTY_NEWLINE);
	vty_out(vty, "  SoC%s", VTY_NEWLINE);
	vty_out(vty, "    Critical max : %d%s",s_mgr->temp.soc_temp_limit.thresh_crit_max, VTY_NEWLINE);
	vty_out(vty, "    Warning max  : %d%s",s_mgr->temp.soc_temp_limit.thresh_warn_max, VTY_NEWLINE);
	vty_out(vty, "    Warning min  : %d%s",s_mgr->temp.soc_temp_limit.thresh_warn_min, VTY_NEWLINE);
	vty_out(vty, "  FPGA%s", VTY_NEWLINE);
	vty_out(vty, "    Critical max : %d%s",s_mgr->temp.fpga_temp_limit.thresh_crit_max, VTY_NEWLINE);
	vty_out(vty, "    Warning max  : %d%s",s_mgr->temp.fpga_temp_limit.thresh_warn_max, VTY_NEWLINE);
	vty_out(vty, "    Warning min  : %d%s",s_mgr->temp.fpga_temp_limit.thresh_warn_min, VTY_NEWLINE);
	if (oc2gbts_option_get(OC2GBTS_OPTION_RMS_FWD) ||
			oc2gbts_option_get(OC2GBTS_OPTION_RMS_REFL)) {
		vty_out(vty, "  RMSDet%s", VTY_NEWLINE);
		vty_out(vty, "    Critical max : %d%s",s_mgr->temp.rmsdet_temp_limit.thresh_crit_max, VTY_NEWLINE);
		vty_out(vty, "    Warning max  : %d%s",s_mgr->temp.rmsdet_temp_limit.thresh_warn_max, VTY_NEWLINE);
		vty_out(vty, "    Warning min  : %d%s",s_mgr->temp.rmsdet_temp_limit.thresh_warn_min, VTY_NEWLINE);
	}
	vty_out(vty, "  OCXO%s", VTY_NEWLINE);
	vty_out(vty, "    Critical max : %d%s",s_mgr->temp.ocxo_temp_limit.thresh_crit_max, VTY_NEWLINE);
	vty_out(vty, "    Warning max  : %d%s",s_mgr->temp.ocxo_temp_limit.thresh_warn_max, VTY_NEWLINE);
	vty_out(vty, "    Warning min  : %d%s",s_mgr->temp.ocxo_temp_limit.thresh_warn_min, VTY_NEWLINE);
	vty_out(vty, "  TX%s", VTY_NEWLINE);
	vty_out(vty, "    Critical max : %d%s",s_mgr->temp.tx_temp_limit.thresh_crit_max, VTY_NEWLINE);
	vty_out(vty, "    Warning max  : %d%s",s_mgr->temp.tx_temp_limit.thresh_warn_max, VTY_NEWLINE);
	vty_out(vty, "    Warning min  : %d%s",s_mgr->temp.tx_temp_limit.thresh_warn_min, VTY_NEWLINE);
	if (oc2gbts_option_get(OC2GBTS_OPTION_PA_TEMP)) {
		vty_out(vty, "  PA%s", VTY_NEWLINE);
		vty_out(vty, "    Critical max : %d%s",s_mgr->temp.pa_temp_limit.thresh_crit_max, VTY_NEWLINE);
		vty_out(vty, "    Warning max  : %d%s",s_mgr->temp.pa_temp_limit.thresh_warn_max, VTY_NEWLINE);
		vty_out(vty, "    Warning min  : %d%s",s_mgr->temp.pa_temp_limit.thresh_warn_min, VTY_NEWLINE);
	}
	vty_out(vty, "Power limits%s", VTY_NEWLINE);
	vty_out(vty, "  Main supply (mV)%s", VTY_NEWLINE);
	vty_out(vty, "    Critical max : %d%s",s_mgr->volt.supply_volt_limit.thresh_crit_max, VTY_NEWLINE);
	vty_out(vty, "    Warning max  : %d%s",s_mgr->volt.supply_volt_limit.thresh_warn_max, VTY_NEWLINE);
	vty_out(vty, "    Warning min  : %d%s",s_mgr->volt.supply_volt_limit.thresh_warn_min, VTY_NEWLINE);
	vty_out(vty, "    Critical min : %d%s",s_mgr->volt.supply_volt_limit.thresh_crit_min, VTY_NEWLINE);
	vty_out(vty, "  Main supply power (W)%s", VTY_NEWLINE);
	vty_out(vty, "    Critical max : %d%s",s_mgr->pwr.supply_pwr_limit.thresh_crit_max, VTY_NEWLINE);
	vty_out(vty, "    Warning max  : %d%s",s_mgr->pwr.supply_pwr_limit.thresh_warn_max, VTY_NEWLINE);
	if (oc2gbts_option_get(OC2GBTS_OPTION_PA)) {
		vty_out(vty, "  PA power (W)%s", VTY_NEWLINE);
		vty_out(vty, "    Critical max : %d%s",s_mgr->pwr.pa_pwr_limit.thresh_crit_max, VTY_NEWLINE);
		vty_out(vty, "    Warning max  : %d%s",s_mgr->pwr.pa_pwr_limit.thresh_warn_max, VTY_NEWLINE);
	}
	if (oc2gbts_option_get(OC2GBTS_OPTION_PA) &&
			oc2gbts_option_get(OC2GBTS_OPTION_RMS_FWD) &&
			oc2gbts_option_get(OC2GBTS_OPTION_RMS_REFL)) {
		vty_out(vty, "VSWR limits%s", VTY_NEWLINE);
		vty_out(vty, "  TX%s", VTY_NEWLINE);
		vty_out(vty, "    Critical max : %d%s",s_mgr->vswr.vswr_limit.thresh_crit_max, VTY_NEWLINE);
		vty_out(vty, "    Warning max  : %d%s",s_mgr->vswr.vswr_limit.thresh_warn_max, VTY_NEWLINE);
	}
	vty_out(vty, "Days since last GPS 3D fix%s", VTY_NEWLINE);
	vty_out(vty, "    Warning max  : %d%s",s_mgr->gps.gps_fix_limit.thresh_warn_max, VTY_NEWLINE);

	return CMD_SUCCESS;
}

DEFUN(calibrate_clock, calibrate_clock_cmd,
      "calibrate clock",
      "Calibration commands\n" 
      "Calibrate clock against GPS PPS\n")
{
	if (oc2gbts_mgr_calib_run(s_mgr) < 0) {
		vty_out(vty, "%%Failed to start calibration.%s", VTY_NEWLINE);
		return CMD_WARNING;
	}
	return CMD_SUCCESS;
}

DEFUN(set_led_pattern, set_led_pattern_cmd,
      "set led pattern <0-255>",
      "Set LED pattern\n"
      "Set LED pattern for debugging purpose only. This pattern will be overridden after 60 seconds by LED pattern of actual system state\n")
{
	int pattern_id = atoi(argv[0]);

	if ((pattern_id < 0) || (pattern_id > BLINK_PATTERN_MAX_ITEM)) {
		vty_out(vty, "%%Invalid LED pattern ID. It must be in range of %d..%d %s", 0, BLINK_PATTERN_MAX_ITEM - 1, VTY_NEWLINE);
		return CMD_WARNING;
	}

	led_set(s_mgr, pattern_id);
	return CMD_SUCCESS;
}

DEFUN(force_mgr_state, force_mgr_state_cmd,
      "force manager state  <0-255>",
      "Force BTS manager state\n"
      "Force BTS manager state for debugging purpose only\n")
{
	int state = atoi(argv[0]);

	if ((state < 0) || (state > STATE_CRITICAL)) {
		vty_out(vty, "%%Invalid BTS manager state. It must be in range of %d..%d %s", 0, STATE_CRITICAL, VTY_NEWLINE);
		return CMD_WARNING;
	}

	s_mgr->state.state = state;
	return CMD_SUCCESS;
}

#define LIMIT_TEMP(name, limit, expl, variable, criticity, min_max)			\
DEFUN(limit_temp_##name##_##variable, limit_temp_##name##_##variable##_cmd,	\
	"limit temp " #name " " #criticity " " #min_max " <-200-200>",			\
	"Limit to reach\n" expl)												\
{																			\
	s_mgr->temp.limit.variable = atoi(argv[0]);								\
	return CMD_SUCCESS;														\
}

LIMIT_TEMP(supply, supply_temp_limit, "SUPPLY TEMP\n", thresh_warn_max, warning, max)
LIMIT_TEMP(supply, supply_temp_limit, "SUPPLY TEMP\n", thresh_crit_max, critical, max)
LIMIT_TEMP(supply, supply_temp_limit, "SUPPLY TEMP\n", thresh_warn_min, warning, min)
LIMIT_TEMP(soc, supply_temp_limit, "SOC TEMP\n", thresh_warn_max, warning, max)
LIMIT_TEMP(soc, supply_temp_limit, "SOC TEMP\n", thresh_crit_max, critical, max)
LIMIT_TEMP(soc, supply_temp_limit, "SOC TEMP\n", thresh_warn_min, warning, min)
LIMIT_TEMP(fpga, fpga_temp_limit, "FPGA TEMP\n", thresh_warn_max, warning, max)
LIMIT_TEMP(fpga, fpga_temp_limit, "FPGA TEMP\n", thresh_crit_max, critical, max)
LIMIT_TEMP(fpga, fpga_temp_limit, "FPGA TEMP\n", thresh_warn_min, warning, min)
LIMIT_TEMP(rmsdet, rmsdet_temp_limit, "RMSDET TEMP\n", thresh_warn_max, warning, max)
LIMIT_TEMP(rmsdet, rmsdet_temp_limit, "RMSDET TEMP\n", thresh_crit_max, critical, max)
LIMIT_TEMP(rmsdet, rmsdet_temp_limit, "RMSDET TEMP\n", thresh_warn_min, warning, min)
LIMIT_TEMP(ocxo, ocxo_temp_limit, "OCXO TEMP\n", thresh_warn_max, warning, max)
LIMIT_TEMP(ocxo, ocxo_temp_limit, "OCXO TEMP\n", thresh_crit_max, critical, max)
LIMIT_TEMP(ocxo, ocxo_temp_limit, "OCXO TEMP\n", thresh_warn_min, warning, min)
LIMIT_TEMP(tx, tx_temp_limit, "TX TEMP\n", thresh_warn_max, warning, max)
LIMIT_TEMP(tx, tx_temp_limit, "TX TEMP\n", thresh_crit_max, critical, max)
LIMIT_TEMP(tx, tx_temp_limit, "TX TEMP\n", thresh_warn_min, warning, min)
LIMIT_TEMP(pa, pa_temp_limit, "PA TEMP\n", thresh_warn_max, warning, max)
LIMIT_TEMP(pa, pa_temp_limit, "PA TEMP\n", thresh_crit_max, critical, max)
LIMIT_TEMP(pa, pa_temp_limit, "PA TEMP\n", thresh_warn_min, warning, min)
#undef LIMIT_TEMP

#define LIMIT_VOLT(name, limit, expl, variable, criticity, min_max)			\
DEFUN(limit_volt_##name##_##variable, limit_volt_##name##_##variable##_cmd,	\
	"limit " #name " " #criticity " " #min_max " <0-48000>",				\
	"Limit to reach\n" expl)												\
{																			\
	s_mgr->volt.limit.variable = atoi(argv[0]);								\
	return CMD_SUCCESS;														\
}

LIMIT_VOLT(supply, supply_volt_limit, "SUPPLY VOLT\n", thresh_warn_max, warning, max)
LIMIT_VOLT(supply, supply_volt_limit, "SUPPLY VOLT\n", thresh_crit_max, critical, max)
LIMIT_VOLT(supply, supply_volt_limit, "SUPPLY VOLT\n", thresh_warn_min, warning, min)
LIMIT_VOLT(supply, supply_volt_limit, "SUPPLY VOLT\n", thresh_crit_min, critical, min)
#undef LIMIT_VOLT

#define LIMIT_PWR(name, limit, expl, variable, criticity, min_max)					\
DEFUN(limit_pwr_##name##_##variable, limit_pwr_##name##_##variable##_cmd,			\
	"limit power " #name " " #criticity " " #min_max " <0-200>",					\
	"Limit to reach\n" expl)														\
{																					\
	s_mgr->pwr.limit.variable = atoi(argv[0]);										\
	return CMD_SUCCESS;																\
}

LIMIT_PWR(supply, supply_pwr_limit, "SUPPLY PWR\n", thresh_warn_max, warning, max)
LIMIT_PWR(supply, supply_pwr_limit, "SUPPLY PWR\n", thresh_crit_max, critical, max)
LIMIT_PWR(pa, pa_pwr_limit, "PA PWR\n", thresh_warn_max, warning, max)
LIMIT_PWR(pa, pa_pwr_limit, "PA PWR\n", thresh_crit_max, critical, max)
#undef LIMIT_PWR

#define LIMIT_VSWR(limit, expl, variable, criticity, min_max)		\
DEFUN(limit_vswr_##variable, limit_vswr_##variable##_cmd,			\
	"limit vswr " #criticity " " #min_max " <1000-200000>",			\
	"Limit to reach\n" expl)										\
{																	\
	s_mgr->vswr.limit.variable = atoi(argv[0]);						\
	return CMD_SUCCESS;												\
}

LIMIT_VSWR(vswr_limit, "VSWR\n", thresh_warn_max, warning, max)
LIMIT_VSWR(vswr_limit, "VSWR\n", thresh_crit_max, critical, max)
#undef LIMIT_VSWR

#define LIMIT_GPSFIX(limit, expl, variable, criticity, min_max)			\
DEFUN(limit_gpsfix_##variable, limit_gpsfix_##variable##_cmd,			\
	"limit gpsfix " #criticity " " #min_max " <0-365>",					\
	"Limit to reach\n" expl)											\
{																		\
	s_mgr->gps.limit.variable = atoi(argv[0]);							\
	return CMD_SUCCESS;													\
}

LIMIT_GPSFIX(gps_fix_limit, "GPS FIX\n", thresh_warn_max, warning, max)
#undef LIMIT_GPSFIX

static void register_limit(int limit, uint32_t unit)
{
	switch (unit) {
	case MGR_LIMIT_TYPE_VOLT:
		install_element(limit, &cfg_thresh_volt_warn_min_cmd);
		install_element(limit, &cfg_thresh_volt_crit_min_cmd);
		break;
	case MGR_LIMIT_TYPE_VSWR:
		install_element(limit, &cfg_thresh_vswr_warn_max_cmd);
		install_element(limit, &cfg_thresh_vswr_crit_max_cmd);
		break;
	case MGR_LIMIT_TYPE_PWR:
		install_element(limit, &cfg_thresh_pwr_warn_max_cmd);
		install_element(limit, &cfg_thresh_pwr_crit_max_cmd);
		break;
	default:
		break;
	}
}

static void register_normal_action(int act)
{
	if (oc2gbts_option_get(OC2GBTS_OPTION_PA)) {
		install_element(act, &cfg_action_pa_on_cmd);
		install_element(act, &cfg_no_action_pa_on_cmd);
	}
	install_element(act, &cfg_action_bts_srv_on_cmd);
	install_element(act, &cfg_no_action_bts_srv_on_cmd);
}

static void register_action(int act)
{
	if (oc2gbts_option_get(OC2GBTS_OPTION_PA)) {
		install_element(act, &cfg_action_pa_off_cmd);
		install_element(act, &cfg_no_action_pa_off_cmd);
	}
	install_element(act, &cfg_action_bts_srv_off_cmd);
	install_element(act, &cfg_no_action_bts_srv_off_cmd);
}

static void register_hidden_commands()
{
	install_element(ENABLE_NODE, &limit_temp_supply_thresh_warn_max_cmd);
	install_element(ENABLE_NODE, &limit_temp_supply_thresh_crit_max_cmd);
	install_element(ENABLE_NODE, &limit_temp_supply_thresh_warn_min_cmd);
	install_element(ENABLE_NODE, &limit_temp_soc_thresh_warn_max_cmd);
	install_element(ENABLE_NODE, &limit_temp_soc_thresh_crit_max_cmd);
	install_element(ENABLE_NODE, &limit_temp_soc_thresh_warn_min_cmd);
	install_element(ENABLE_NODE, &limit_temp_fpga_thresh_warn_max_cmd);
	install_element(ENABLE_NODE, &limit_temp_fpga_thresh_crit_max_cmd);
	install_element(ENABLE_NODE, &limit_temp_fpga_thresh_warn_min_cmd);
	install_element(ENABLE_NODE, &limit_temp_rmsdet_thresh_warn_max_cmd);
	install_element(ENABLE_NODE, &limit_temp_rmsdet_thresh_crit_max_cmd);
	install_element(ENABLE_NODE, &limit_temp_rmsdet_thresh_warn_min_cmd);
	install_element(ENABLE_NODE, &limit_temp_ocxo_thresh_warn_max_cmd);
	install_element(ENABLE_NODE, &limit_temp_ocxo_thresh_crit_max_cmd);
	install_element(ENABLE_NODE, &limit_temp_ocxo_thresh_warn_min_cmd);
	install_element(ENABLE_NODE, &limit_temp_tx_thresh_warn_max_cmd);
	install_element(ENABLE_NODE, &limit_temp_tx_thresh_crit_max_cmd);
	install_element(ENABLE_NODE, &limit_temp_tx_thresh_warn_min_cmd);
	if (oc2gbts_option_get(OC2GBTS_OPTION_PA_TEMP)) {
		install_element(ENABLE_NODE, &limit_temp_pa_thresh_warn_max_cmd);
		install_element(ENABLE_NODE, &limit_temp_pa_thresh_crit_max_cmd);
		install_element(ENABLE_NODE, &limit_temp_pa_thresh_warn_min_cmd);
	}

	install_element(ENABLE_NODE, &limit_volt_supply_thresh_warn_max_cmd);
	install_element(ENABLE_NODE, &limit_volt_supply_thresh_crit_max_cmd);
	install_element(ENABLE_NODE, &limit_volt_supply_thresh_warn_min_cmd);
	install_element(ENABLE_NODE, &limit_volt_supply_thresh_crit_min_cmd);

	install_element(ENABLE_NODE, &limit_pwr_supply_thresh_warn_max_cmd);
	install_element(ENABLE_NODE, &limit_pwr_supply_thresh_crit_max_cmd);

	if (oc2gbts_option_get(OC2GBTS_OPTION_PA)) {
		install_element(ENABLE_NODE, &limit_pwr_pa_thresh_warn_max_cmd);
		install_element(ENABLE_NODE, &limit_pwr_pa_thresh_crit_max_cmd);
	}

	if (oc2gbts_option_get(OC2GBTS_OPTION_PA) &&
			oc2gbts_option_get(OC2GBTS_OPTION_RMS_FWD) &&
			oc2gbts_option_get(OC2GBTS_OPTION_RMS_REFL)) {
		install_element(ENABLE_NODE, &limit_vswr_thresh_warn_max_cmd);
		install_element(ENABLE_NODE, &limit_vswr_thresh_crit_max_cmd);
	}

	install_element(ENABLE_NODE, &limit_gpsfix_thresh_warn_max_cmd);
}

int oc2gbts_mgr_vty_init(void)
{
	vty_init(&vty_info);

	install_element_ve(&show_mgr_cmd);
	install_element_ve(&show_thresh_cmd);

	install_element(ENABLE_NODE, &calibrate_clock_cmd);

	install_node(&mgr_node, config_write_mgr);
	install_element(CONFIG_NODE, &cfg_mgr_cmd);
	vty_install_default(MGR_NODE);

	/* install the limit nodes */
	install_node(&limit_supply_temp_node, config_write_dummy);
	install_element(MGR_NODE, &cfg_limit_supply_temp_cmd);
	vty_install_default(LIMIT_SUPPLY_TEMP_NODE);

	install_node(&limit_soc_node, config_write_dummy);
	install_element(MGR_NODE, &cfg_limit_soc_temp_cmd);
	vty_install_default(LIMIT_SOC_NODE);

	install_node(&limit_fpga_node, config_write_dummy);
	install_element(MGR_NODE, &cfg_limit_fpga_temp_cmd);
	vty_install_default(LIMIT_FPGA_NODE);

	if (oc2gbts_option_get(OC2GBTS_OPTION_RMS_FWD) ||
			oc2gbts_option_get(OC2GBTS_OPTION_RMS_REFL)) {
		install_node(&limit_rmsdet_node, config_write_dummy);
		install_element(MGR_NODE, &cfg_limit_rmsdet_temp_cmd);
		vty_install_default(LIMIT_RMSDET_NODE);
	}

	install_node(&limit_ocxo_node, config_write_dummy);
	install_element(MGR_NODE, &cfg_limit_ocxo_temp_cmd);
	vty_install_default(LIMIT_OCXO_NODE);

	install_node(&limit_tx_temp_node, config_write_dummy);
	install_element(MGR_NODE, &cfg_limit_tx_temp_cmd);
	vty_install_default(LIMIT_TX_TEMP_NODE);

	if (oc2gbts_option_get(OC2GBTS_OPTION_PA_TEMP)) {
		install_node(&limit_pa_temp_node, config_write_dummy);
		install_element(MGR_NODE, &cfg_limit_pa_temp_cmd);
		vty_install_default(LIMIT_PA_TEMP_NODE);
	}

	install_node(&limit_supply_volt_node, config_write_dummy);
	install_element(MGR_NODE, &cfg_limit_supply_volt_cmd);
	register_limit(LIMIT_SUPPLY_VOLT_NODE, MGR_LIMIT_TYPE_VOLT);
	vty_install_default(LIMIT_SUPPLY_VOLT_NODE);

	if (oc2gbts_option_get(OC2GBTS_OPTION_PA) &&
			oc2gbts_option_get(OC2GBTS_OPTION_RMS_FWD) &&
			oc2gbts_option_get(OC2GBTS_OPTION_RMS_REFL)) {
		install_node(&limit_vswr_node, config_write_dummy);
		install_element(MGR_NODE, &cfg_limit_vswr_cmd);
		register_limit(LIMIT_VSWR_NODE, MGR_LIMIT_TYPE_VSWR);
		vty_install_default(LIMIT_VSWR_NODE);
	}

	install_node(&limit_supply_pwr_node, config_write_dummy);
	install_element(MGR_NODE, &cfg_limit_supply_pwr_cmd);
	register_limit(LIMIT_SUPPLY_PWR_NODE, MGR_LIMIT_TYPE_PWR);
	vty_install_default(LIMIT_SUPPLY_PWR_NODE);

	if (oc2gbts_option_get(OC2GBTS_OPTION_PA)) {
		install_node(&limit_pa_pwr_node, config_write_dummy);
		install_element(MGR_NODE, &cfg_limit_pa_pwr_cmd);
		vty_install_default(LIMIT_PA_PWR_NODE);
	}

	install_node(&limit_gps_fix_node, config_write_dummy);
	install_element(MGR_NODE, &cfg_limit_gps_fix_cmd);
	vty_install_default(LIMIT_GPS_FIX_NODE);

	/* install the normal node */
	install_node(&act_norm_node, config_write_dummy);
	install_element(MGR_NODE, &cfg_action_normal_cmd);
	register_normal_action(ACT_NORM_NODE);

	/* install the warning and critical node */
	install_node(&act_warn_node, config_write_dummy);
	install_element(MGR_NODE, &cfg_action_warn_cmd);
	register_action(ACT_WARN_NODE);
	vty_install_default(ACT_WARN_NODE);

	install_node(&act_crit_node, config_write_dummy);
	install_element(MGR_NODE, &cfg_action_critical_cmd);
	register_action(ACT_CRIT_NODE);
	vty_install_default(ACT_CRIT_NODE);

	/* install LED pattern command for debugging purpose */
	install_element_ve(&set_led_pattern_cmd);
	install_element_ve(&force_mgr_state_cmd);

	register_hidden_commands();

	return 0;
}

int oc2gbts_mgr_parse_config(struct oc2gbts_mgr_instance *manager)
{
	int rc;

	s_mgr = manager;
	rc = vty_read_config_file(s_mgr->config_file, NULL);
	if (rc < 0) {
		fprintf(stderr, "Failed to parse the config file: '%s'\n",
				s_mgr->config_file);
		return rc;
	}

	return 0;
}
