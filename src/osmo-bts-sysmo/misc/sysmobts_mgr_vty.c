/* (C) 2014 by sysmocom - s.f.m.c. GmbH
 *
 * All Rights Reserved
 *
 * Author: Alvaro Neira Ayuso <anayuso@sysmocom.de>
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

#include <osmocom/vty/vty.h>
#include <osmocom/vty/command.h>
#include <osmocom/vty/misc.h>

#include <osmo-bts/logging.h>

#include "sysmobts_misc.h"
#include "sysmobts_mgr.h"
#include "btsconfig.h"

static struct sysmobts_mgr_instance *s_mgr;

static const char copyright[] =
	"(C) 2012 by Harald Welte <laforge@gnumonks.org>\r\n"
	"(C) 2014 by Holger Hans Peter Freyther\r\n"
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
	case LIMIT_RF_NODE:
	case LIMIT_DIGITAL_NODE:
	case LIMIT_BOARD_NODE:
	case LIMIT_PA_NODE:
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
	case LIMIT_RF_NODE:
	case LIMIT_DIGITAL_NODE:
	case LIMIT_BOARD_NODE:
	case LIMIT_PA_NODE:
		return 1;
	default:
		return 0;
	}
}

static struct vty_app_info vty_info = {
	.name           = "sysmobts-mgr",
	.version        = PACKAGE_VERSION,
	.go_parent_cb   = go_to_parent,
	.is_config_node = is_config_node,
	.copyright	= copyright,
};


#define MGR_STR			"Configure sysmobts-mgr\n"

static struct cmd_node mgr_node = {
	MGR_NODE,
	"%s(sysmobts-mgr)# ",
	1,
};

static struct cmd_node act_norm_node = {
	ACT_NORM_NODE,
	"%s(action-normal)# ",
	1,
};

static struct cmd_node act_warn_node = {
	ACT_WARN_NODE,
	"%s(action-warn)# ",
	1,
};

static struct cmd_node act_crit_node = {
	ACT_CRIT_NODE,
	"%s(action-critical)# ",
	1,
};

static struct cmd_node limit_rf_node = {
	LIMIT_RF_NODE,
	"%s(limit-rf)# ",
	1,
};

static struct cmd_node limit_digital_node = {
	LIMIT_DIGITAL_NODE,
	"%s(limit-digital)# ",
	1,
};

static struct cmd_node limit_board_node = {
	LIMIT_BOARD_NODE,
	"%s(limit-board)# ",
	1,
};

static struct cmd_node limit_pa_node = {
	LIMIT_PA_NODE,
	"%s(limit-pa)# ",
	1,
};

DEFUN(cfg_mgr, cfg_mgr_cmd,
	"sysmobts-mgr",
	MGR_STR)
{
	vty->node = MGR_NODE;
	return CMD_SUCCESS;
}

static void write_temp_limit(struct vty *vty, const char *name,
				struct sysmobts_temp_limit *limit)
{
	vty_out(vty, " %s%s", name, VTY_NEWLINE);
	vty_out(vty, "   threshold warning %d%s",
		limit->thresh_warn, VTY_NEWLINE);
	vty_out(vty, "   threshold critical %d%s",
		limit->thresh_crit, VTY_NEWLINE);
}

static void write_norm_action(struct vty *vty, const char *name, int actions)
{
	vty_out(vty, " %s%s", name, VTY_NEWLINE);
	vty_out(vty, "  %spa-on%s",
		(actions & TEMP_ACT_NORM_PA_ON) ? "" : "no ", VTY_NEWLINE);
	vty_out(vty, "  %sbts-service-on%s",
		(actions & TEMP_ACT_NORM_BTS_SRV_ON) ? "" : "no ", VTY_NEWLINE);
	vty_out(vty, "  %sslave-on%s",
		(actions & TEMP_ACT_NORM_SLAVE_ON) ? "" : "no ", VTY_NEWLINE);
}

static void write_action(struct vty *vty, const char *name, int actions)
{
	vty_out(vty, " %s%s", name, VTY_NEWLINE);
#if 0
	vty_out(vty, "  %spower-control%s",
		(actions & TEMP_ACT_PWR_CONTRL) ? "" : "no ", VTY_NEWLINE);

	/* only on the sysmobts 2050 */
	vty_out(vty, "  %smaster-off%s",
		(actions & TEMP_ACT_MASTER_OFF) ? "" : "no ", VTY_NEWLINE);
	vty_out(vty, "  %sslave-off%s",
		(actions & TEMP_ACT_MASTER_OFF) ? "" : "no ", VTY_NEWLINE);
#endif
	vty_out(vty, "  %spa-off%s",
		(actions & TEMP_ACT_PA_OFF) ? "" : "no ", VTY_NEWLINE);
	vty_out(vty, "  %sbts-service-off%s",
		(actions & TEMP_ACT_BTS_SRV_OFF) ? "" : "no ", VTY_NEWLINE);
	vty_out(vty, "  %sslave-off%s",
		(actions & TEMP_ACT_SLAVE_OFF) ? "" : "no ", VTY_NEWLINE);
}

static int config_write_mgr(struct vty *vty)
{
	vty_out(vty, "sysmobts-mgr%s", VTY_NEWLINE);

	write_temp_limit(vty, "limits rf", &s_mgr->rf_limit);
	write_temp_limit(vty, "limits digital", &s_mgr->digital_limit);
	write_temp_limit(vty, "limits board", &s_mgr->board_limit);
	write_temp_limit(vty, "limits pa", &s_mgr->pa_limit);

	write_norm_action(vty, "actions normal", s_mgr->action_norm);
	write_action(vty, "actions warn", s_mgr->action_warn);
	write_action(vty, "actions critical", s_mgr->action_crit);

	return CMD_SUCCESS;
}

static int config_write_dummy(struct vty *vty)
{
	return CMD_SUCCESS;
}

#define CFG_LIMIT(name, expl, switch_to, variable)			\
DEFUN(cfg_limit_##name, cfg_limit_##name##_cmd,				\
	"limits " #name,						\
	"Configure Limits\n" expl)					\
{									\
	vty->node = switch_to;						\
	vty->index = &s_mgr->variable;					\
	return CMD_SUCCESS;						\
}

CFG_LIMIT(rf, "RF\n", LIMIT_RF_NODE, rf_limit)
CFG_LIMIT(digital, "Digital\n", LIMIT_DIGITAL_NODE, digital_limit)
CFG_LIMIT(board, "Board\n", LIMIT_BOARD_NODE, board_limit)
CFG_LIMIT(pa, "Power Amplifier\n", LIMIT_PA_NODE, pa_limit)
#undef CFG_LIMIT

DEFUN(cfg_limit_warning, cfg_thresh_warning_cmd,
	"threshold warning <0-200>",
	"Threshold to reach\n" "Warning level\n" "Range\n")
{
	struct sysmobts_temp_limit *limit = vty->index;
	limit->thresh_warn = atoi(argv[0]);
	return CMD_SUCCESS;
}

DEFUN(cfg_limit_crit, cfg_thresh_crit_cmd,
	"threshold critical <0-200>",
	"Threshold to reach\n" "Severe level\n" "Range\n")
{
	struct sysmobts_temp_limit *limit = vty->index;
	limit->thresh_crit = atoi(argv[0]);
	return CMD_SUCCESS;
}

#define CFG_ACTION(name, expl, switch_to, variable)			\
DEFUN(cfg_action_##name, cfg_action_##name##_cmd,			\
	"actions " #name,						\
	"Configure Actions\n" expl)					\
{									\
	vty->node = switch_to;						\
	vty->index = &s_mgr->variable;					\
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
	*action |= TEMP_ACT_NORM_PA_ON;
	return CMD_SUCCESS;
}

DEFUN(cfg_no_action_pa_on, cfg_no_action_pa_on_cmd,
	"no pa-on",
	NO_STR "Switch the Power Amplifier on\n")
{
	int *action = vty->index;
	*action &= ~TEMP_ACT_NORM_PA_ON;
	return CMD_SUCCESS;
}

DEFUN(cfg_action_bts_srv_on, cfg_action_bts_srv_on_cmd,
	"bts-service-on",
	"Start the systemd osmo-bts-sysmo.service\n")
{
	int *action = vty->index;
	*action |= TEMP_ACT_NORM_BTS_SRV_ON;
	return CMD_SUCCESS;
}

DEFUN(cfg_no_action_bts_srv_on, cfg_no_action_bts_srv_on_cmd,
	"no bts-service-on",
	NO_STR "Start the systemd osmo-bts-sysmo.service\n")
{
	int *action = vty->index;
	*action &= ~TEMP_ACT_NORM_BTS_SRV_ON;
	return CMD_SUCCESS;
}

DEFUN(cfg_action_slave_on, cfg_action_slave_on_cmd,
	"slave-on",
	"Power-on secondary device on sysmoBTS2050\n")
{
	int *action = vty->index;
	*action |= TEMP_ACT_NORM_SLAVE_ON;
	return CMD_SUCCESS;
}

DEFUN(cfg_no_action_slave_on, cfg_no_action_slave_on_cmd,
	"no slave-on",
	NO_STR "Power-on secondary device on sysmoBTS2050\n")
{
	int *action = vty->index;
	*action &= ~TEMP_ACT_NORM_SLAVE_ON;
	return CMD_SUCCESS;
}

DEFUN(cfg_action_pa_off, cfg_action_pa_off_cmd,
	"pa-off",
	"Switch the Power Amplifier off\n")
{
	int *action = vty->index;
	*action |= TEMP_ACT_PA_OFF;
	return CMD_SUCCESS;
}

DEFUN(cfg_no_action_pa_off, cfg_no_action_pa_off_cmd,
	"no pa-off",
	NO_STR "Do not switch off the Power Amplifier\n")
{
	int *action = vty->index;
	*action &= ~TEMP_ACT_PA_OFF;
	return CMD_SUCCESS;
}

DEFUN(cfg_action_bts_srv_off, cfg_action_bts_srv_off_cmd,
	"bts-service-off",
	"Stop the systemd osmo-bts-sysmo.service\n")
{
	int *action = vty->index;
	*action |= TEMP_ACT_BTS_SRV_OFF;
	return CMD_SUCCESS;
}

DEFUN(cfg_no_action_bts_srv_off, cfg_no_action_bts_srv_off_cmd,
	"no bts-service-off",
	NO_STR "Stop the systemd osmo-bts-sysmo.service\n")
{
	int *action = vty->index;
	*action &= ~TEMP_ACT_BTS_SRV_OFF;
	return CMD_SUCCESS;
}

DEFUN(cfg_action_slave_off, cfg_action_slave_off_cmd,
	"slave-off",
	"Power-off secondary device on sysmoBTS2050\n")
{
	int *action = vty->index;
	*action |= TEMP_ACT_SLAVE_OFF;
	return CMD_SUCCESS;
}

DEFUN(cfg_no_action_slave_off, cfg_no_action_slave_off_cmd,
	"no slave-off",
	NO_STR "Power-off secondary device on sysmoBTS2050\n")
{
	int *action = vty->index;
	*action &= ~TEMP_ACT_SLAVE_OFF;
	return CMD_SUCCESS;
}

DEFUN(show_mgr, show_mgr_cmd, "show manager",
      SHOW_STR "Display information about the manager")
{
	vty_out(vty, "BTS Control Interface: %s%s",
		s_mgr->calib.is_up ? "connected" : "disconnected", VTY_NEWLINE);
	vty_out(vty, "Temperature control state: %s%s",
		sysmobts_mgr_temp_get_state(s_mgr->state), VTY_NEWLINE);
	vty_out(vty, "Current Temperatures%s", VTY_NEWLINE);
	vty_out(vty, " Digital: %f Celcius%s",
		sysmobts_temp_get(SYSMOBTS_TEMP_DIGITAL,
					SYSMOBTS_TEMP_INPUT) / 1000.0f,
		VTY_NEWLINE);
	vty_out(vty, " RF:      %f Celcius%s",
		sysmobts_temp_get(SYSMOBTS_TEMP_RF,
					SYSMOBTS_TEMP_INPUT) / 1000.0f,
		VTY_NEWLINE);
	if (is_sbts2050()) {
		int temp_pa, temp_board;
		struct sbts2050_power_status status;

		vty_out(vty, " sysmoBTS 2050 is %s%s",
			is_sbts2050_master() ? "master" : "slave", VTY_NEWLINE);

		sbts2050_uc_check_temp(&temp_pa, &temp_board);
		vty_out(vty, " sysmoBTS 2050 PA: %d Celcius%s", temp_pa, VTY_NEWLINE);
		vty_out(vty, " sysmoBTS 2050 PA: %d Celcius%s", temp_board, VTY_NEWLINE);

		sbts2050_uc_get_status(&status);
		vty_out(vty, "Power Status%s", VTY_NEWLINE);
		vty_out(vty, " Main Supply :(ON)  [(24.00)Vdc, %4.2f A]%s",
			status.main_supply_current, VTY_NEWLINE);
		vty_out(vty, " Master SF   : %s  [%6.2f Vdc, %4.2f A]%s",
			status.master_enabled ? "ON " : "OFF",
			status.master_voltage, status.master_current,
			VTY_NEWLINE);
		vty_out(vty, " Slave SF    : %s   [%6.2f Vdc, %4.2f A]%s",
			status.slave_enabled ? "ON" : "OFF",
			status.slave_voltage, status.slave_current,
			VTY_NEWLINE);
		vty_out(vty, " Power Amp   : %s  [%6.2f Vdc, %4.2f A]%s",
			status.pa_enabled ? "ON" : "OFF",
			status.pa_voltage, status.pa_current,
			VTY_NEWLINE);
		vty_out(vty, " PA Bias     : %s  [%6.2f Vdc, ---- A]%s",
			status.pa_enabled ? "ON" : "OFF",
			status.pa_bias_voltage,
			VTY_NEWLINE);
	}

	return CMD_SUCCESS;
}

DEFUN(calibrate_trx, calibrate_trx_cmd,
      "trx 0 calibrate-clock",
      "Transceiver commands\n" "Transceiver 0\n"
      "Calibrate clock against GPS PPS\n")
{
	if (sysmobts_mgr_calib_run(s_mgr) < 0) {
		vty_out(vty, "%%Failed to start calibration.%s", VTY_NEWLINE);
		return CMD_WARNING;
	}
	return CMD_SUCCESS;
}

static void register_limit(int limit)
{
	install_element(limit, &cfg_thresh_warning_cmd);
	install_element(limit, &cfg_thresh_crit_cmd);
}

static void register_normal_action(int act)
{
	install_element(act, &cfg_action_pa_on_cmd);
	install_element(act, &cfg_no_action_pa_on_cmd);
	install_element(act, &cfg_action_bts_srv_on_cmd);
	install_element(act, &cfg_no_action_bts_srv_on_cmd);

	/* these only work on the sysmobts 2050 */
	install_element(act, &cfg_action_slave_on_cmd);
	install_element(act, &cfg_no_action_slave_on_cmd);
}

static void register_action(int act)
{
#if 0
	install_element(act, &cfg_action_pwr_contrl_cmd);
	install_element(act, &cfg_no_action_pwr_contrl_cmd);
#endif
	install_element(act, &cfg_action_pa_off_cmd);
	install_element(act, &cfg_no_action_pa_off_cmd);
	install_element(act, &cfg_action_bts_srv_off_cmd);
	install_element(act, &cfg_no_action_bts_srv_off_cmd);

	/* these only work on the sysmobts 2050 */
	install_element(act, &cfg_action_slave_off_cmd);
	install_element(act, &cfg_no_action_slave_off_cmd);
}

int sysmobts_mgr_vty_init(void)
{
	vty_init(&vty_info);

	install_element_ve(&show_mgr_cmd);

	install_element(ENABLE_NODE, &calibrate_trx_cmd);

	install_node(&mgr_node, config_write_mgr);
	install_element(CONFIG_NODE, &cfg_mgr_cmd);

	/* install the limit nodes */
	install_node(&limit_rf_node, config_write_dummy);
	install_element(MGR_NODE, &cfg_limit_rf_cmd);
	register_limit(LIMIT_RF_NODE);

	install_node(&limit_digital_node, config_write_dummy);
	install_element(MGR_NODE, &cfg_limit_digital_cmd);
	register_limit(LIMIT_DIGITAL_NODE);

	install_node(&limit_board_node, config_write_dummy);
	install_element(MGR_NODE, &cfg_limit_board_cmd);
	register_limit(LIMIT_BOARD_NODE);

	install_node(&limit_pa_node, config_write_dummy);
	install_element(MGR_NODE, &cfg_limit_pa_cmd);
	register_limit(LIMIT_PA_NODE);

	/* install the normal node */
	install_node(&act_norm_node, config_write_dummy);
	install_element(MGR_NODE, &cfg_action_normal_cmd);
	register_normal_action(ACT_NORM_NODE);

	/* install the warning and critical node */
	install_node(&act_warn_node, config_write_dummy);
	install_element(MGR_NODE, &cfg_action_warn_cmd);
	register_action(ACT_WARN_NODE);

	install_node(&act_crit_node, config_write_dummy);
	install_element(MGR_NODE, &cfg_action_critical_cmd);
	register_action(ACT_CRIT_NODE);

	return 0;
}

int sysmobts_mgr_parse_config(struct sysmobts_mgr_instance *manager)
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
