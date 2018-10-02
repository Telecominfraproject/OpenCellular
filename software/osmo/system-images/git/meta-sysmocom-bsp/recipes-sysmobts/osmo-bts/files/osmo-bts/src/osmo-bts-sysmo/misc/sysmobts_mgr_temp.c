/* Temperature control for SysmoBTS management daemon */

/*
 * (C) 2014 by Holger Hans Peter Freyther
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

#include "misc/sysmobts_mgr.h"
#include "misc/sysmobts_misc.h"

#include <osmo-bts/logging.h>

#include <osmocom/core/timer.h>
#include <osmocom/core/utils.h>

#include <stdlib.h>

static struct sysmobts_mgr_instance *s_mgr;
static struct osmo_timer_list temp_ctrl_timer;

static const struct value_string state_names[] = {
	{ STATE_NORMAL,			"NORMAL" },
	{ STATE_WARNING_HYST,		"WARNING (HYST)" },
	{ STATE_WARNING,		"WARNING" },
	{ STATE_CRITICAL,		"CRITICAL" },
	{ 0, NULL }
};

const char *sysmobts_mgr_temp_get_state(enum sysmobts_temp_state state)
{
	return get_value_string(state_names, state);
}

static int next_state(enum sysmobts_temp_state current_state, int critical, int warning)
{
	int next_state = -1;
	switch (current_state) {
	case STATE_NORMAL:
		if (critical)
			next_state = STATE_CRITICAL;
		else if (warning)
			next_state = STATE_WARNING;
		break;
	case STATE_WARNING_HYST:
		if (critical)
			next_state = STATE_CRITICAL;
		else if (warning)
			next_state = STATE_WARNING;
		else
			next_state = STATE_NORMAL;
		break;
	case STATE_WARNING:
		if (critical)
			next_state = STATE_CRITICAL;
		else if (!warning)
			next_state = STATE_WARNING_HYST;
		break;
	case STATE_CRITICAL:
		if (!critical && !warning)
			next_state = STATE_WARNING;
		break;
	};

	return next_state;
}

static void handle_normal_actions(int actions)
{
	/* switch off the PA */
	if (actions & TEMP_ACT_NORM_PA_ON) {
		if (!is_sbts2050()) {
			LOGP(DTEMP, LOGL_NOTICE,
				"PA can only be switched-on on the master\n");
		} else if (sbts2050_uc_set_pa_power(1) != 0) {
			LOGP(DTEMP, LOGL_ERROR,
				"Failed to switch on the PA\n");
		} else {
			LOGP(DTEMP, LOGL_NOTICE,
				"Switched on the PA as normal action.\n");
		}
	}

	if (actions & TEMP_ACT_NORM_SLAVE_ON) {
		if (!is_sbts2050()) {
			LOGP(DTEMP, LOGL_NOTICE,
				"Slave on only possible on the sysmoBTS2050\n");
		} else if (sbts2050_uc_set_slave_power(1) != 0) {
			LOGP(DTEMP, LOGL_ERROR,
				"Failed to switch on the slave BTS\n");
		} else {
			LOGP(DTEMP, LOGL_NOTICE,
				"Switched on the slave as normal action.\n");
		}
	}

	if (actions & TEMP_ACT_NORM_BTS_SRV_ON) {
		LOGP(DTEMP, LOGL_NOTICE,
			"Going to switch on the BTS service\n");
		/*
		 * TODO: use/create something like nspawn that serializes
		 * and used SIGCHLD/waitpid to pick up the dead processes
		 * without invoking shell.
		 */
		system("/bin/systemctl start osmo-bts-sysmo");
	}
}

static void handle_actions(int actions)
{
	/* switch off the PA */
	if (actions & TEMP_ACT_PA_OFF) {
		if (!is_sbts2050()) {
			LOGP(DTEMP, LOGL_NOTICE,
				"PA can only be switched-off on the master\n");
		} else if (sbts2050_uc_set_pa_power(0) != 0) {
			LOGP(DTEMP, LOGL_ERROR,
				"Failed to switch off the PA. Stop BTS?\n");
		} else {
			LOGP(DTEMP, LOGL_NOTICE,
				"Switched off the PA due temperature.\n");
		}
	}

	if (actions & TEMP_ACT_SLAVE_OFF) {
		if (!is_sbts2050()) {
			LOGP(DTEMP, LOGL_NOTICE,
				"Slave off only possible on the sysmoBTS2050\n");
		} else if (sbts2050_uc_set_slave_power(0) != 0) {
			LOGP(DTEMP, LOGL_ERROR,
				"Failed to switch off the slave BTS\n");
		} else {
			LOGP(DTEMP, LOGL_NOTICE,
				"Switched off the slave due temperature\n");
		}
	}

	if (actions & TEMP_ACT_BTS_SRV_OFF) {
		LOGP(DTEMP, LOGL_NOTICE,
			"Going to switch off the BTS service\n");
		/*
		 * TODO: use/create something like nspawn that serializes
		 * and used SIGCHLD/waitpid to pick up the dead processes
		 * without invoking shell.
		 */
		system("/bin/systemctl stop osmo-bts-sysmo");
	}
}

/**
 * Go back to normal! Depending on the configuration execute the normal
 * actions that could (start to) undo everything we did in the other
 * states. What is still missing is the power increase/decrease depending
 * on the state. E.g. starting from WARNING_HYST we might want to slowly
 * ramp up the output power again.
 */
static void execute_normal_act(struct sysmobts_mgr_instance *manager)
{
	LOGP(DTEMP, LOGL_NOTICE, "System is back to normal temperature.\n");
	handle_normal_actions(manager->action_norm);
}

static void execute_warning_act(struct sysmobts_mgr_instance *manager)
{
	LOGP(DTEMP, LOGL_NOTICE, "System has reached temperature warning.\n");
	handle_actions(manager->action_warn);
}

static void execute_critical_act(struct sysmobts_mgr_instance *manager)
{
	LOGP(DTEMP, LOGL_NOTICE, "System has reached critical warning.\n");
	handle_actions(manager->action_crit);
}

static void sysmobts_mgr_temp_handle(struct sysmobts_mgr_instance *manager,
				     struct ctrl_connection *ctrl, int critical,
				     int warning)
{
	int new_state = next_state(manager->state, critical, warning);
	struct ctrl_cmd *rep;
	char *oml_alert = NULL;

	/* Nothing changed */
	if (new_state < 0)
		return;

	LOGP(DTEMP, LOGL_NOTICE, "Moving from state %s to %s.\n",
		get_value_string(state_names, manager->state),
		get_value_string(state_names, new_state));
	manager->state = new_state;
	switch (manager->state) {
	case STATE_NORMAL:
		execute_normal_act(manager);
		break;
	case STATE_WARNING_HYST:
		/* do nothing? Maybe start to increase transmit power? */
		break;
	case STATE_WARNING:
		execute_warning_act(manager);
		oml_alert = "Temperature Warning";
		break;
	case STATE_CRITICAL:
		execute_critical_act(manager);
		oml_alert = "Temperature Critical";
		break;
	};

	if (!oml_alert)
		return;

	rep = ctrl_cmd_create(tall_mgr_ctx, CTRL_TYPE_SET);
	if (!rep) {
		LOGP(DTEMP, LOGL_ERROR, "OML alert creation failed for %s.\n",
		     oml_alert);
		return;
	}

	rep->id = talloc_asprintf(rep, "%d", rand());
	rep->variable = "oml-alert";
	rep->value = oml_alert;
	LOGP(DTEMP, LOGL_ERROR, "OML alert sent: %d\n",
	     ctrl_cmd_send(&ctrl->write_queue, rep));
	talloc_free(rep);
}

static void temp_ctrl_check(struct ctrl_connection *ctrl)
{
	int rc;
	int warn_thresh_passed = 0;
	int crit_thresh_passed = 0;

	LOGP(DTEMP, LOGL_DEBUG, "Going to check the temperature.\n");

	/* Read the current digital temperature */
	rc = sysmobts_temp_get(SYSMOBTS_TEMP_DIGITAL, SYSMOBTS_TEMP_INPUT);
	if (rc < 0) {
		LOGP(DTEMP, LOGL_ERROR,
			"Failed to read the digital temperature. rc=%d\n", rc);
		warn_thresh_passed = crit_thresh_passed = 1;
	} else {
		int temp = rc / 1000;
		if (temp > s_mgr->digital_limit.thresh_warn)
			warn_thresh_passed = 1;
		if (temp > s_mgr->digital_limit.thresh_crit)
			crit_thresh_passed = 1;
		LOGP(DTEMP, LOGL_DEBUG, "Digital temperature is: %d\n", temp);
	}

	/* Read the current RF temperature */
	rc = sysmobts_temp_get(SYSMOBTS_TEMP_RF, SYSMOBTS_TEMP_INPUT);
	if (rc < 0) {
		LOGP(DTEMP, LOGL_ERROR,
			"Failed to read the RF temperature. rc=%d\n", rc);
		warn_thresh_passed = crit_thresh_passed = 1;
	} else {
		int temp = rc / 1000;
		if (temp > s_mgr->rf_limit.thresh_warn)
			warn_thresh_passed = 1;
		if (temp > s_mgr->rf_limit.thresh_crit)
			crit_thresh_passed = 1;
		LOGP(DTEMP, LOGL_DEBUG, "RF temperature is: %d\n", temp);
	}

	if (is_sbts2050()) {
		int temp_pa, temp_board;

		rc = sbts2050_uc_check_temp(&temp_pa, &temp_board);
		if (rc != 0) {
			/* XXX what do here? */
			LOGP(DTEMP, LOGL_ERROR,
				"Failed to read the temperature! Reboot?!\n");
			warn_thresh_passed = 1;
			crit_thresh_passed = 1;
		} else {
			LOGP(DTEMP, LOGL_DEBUG, "SBTS2050 board(%d) PA(%d)\n",
				temp_board, temp_pa);
			if (temp_pa > s_mgr->pa_limit.thresh_warn)
				warn_thresh_passed = 1;
			if (temp_pa > s_mgr->pa_limit.thresh_crit)
				crit_thresh_passed = 1;
			if (temp_board > s_mgr->board_limit.thresh_warn)
				warn_thresh_passed = 1;
			if (temp_board > s_mgr->board_limit.thresh_crit)
				crit_thresh_passed = 1;
		}
	}

	sysmobts_mgr_temp_handle(s_mgr, ctrl, crit_thresh_passed,
				 warn_thresh_passed);
}

static void temp_ctrl_check_cb(void *ctrl)
{
	temp_ctrl_check(ctrl);
	/* Check every two minutes? XXX make it configurable! */
	osmo_timer_schedule(&temp_ctrl_timer, 2 * 60, 0);
}

int sysmobts_mgr_temp_init(struct sysmobts_mgr_instance *mgr,
			   struct ctrl_connection *ctrl)
{
	s_mgr = mgr;
	temp_ctrl_timer.cb = temp_ctrl_check_cb;
	temp_ctrl_timer.data = ctrl;
	temp_ctrl_check_cb(ctrl);
	return 0;
}
