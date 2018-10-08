/* Temperature control for NuRAN OC-2G BTS management daemon */

/* Copyright (C) 2015 by Yves Godin <support@nuranwireless.com>
 * 
 * Based on sysmoBTS:
 *     sysmobts_mgr_temp.c
 *     (C) 2014 by Holger Hans Peter Freyther
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
#include <inttypes.h>
#include "misc/oc2gbts_mgr.h"
#include "misc/oc2gbts_misc.h"
#include "misc/oc2gbts_temp.h"
#include "misc/oc2gbts_power.h"
#include "misc/oc2gbts_led.h"
#include "misc/oc2gbts_swd.h"
#include "misc/oc2gbts_bid.h"
#include "limits.h"

#include <osmo-bts/logging.h>

#include <osmocom/core/timer.h>
#include <osmocom/core/utils.h>
#include <osmocom/core/linuxlist.h>

struct oc2gbts_mgr_instance *s_mgr;
static struct osmo_timer_list sensor_ctrl_timer;

static const struct value_string state_names[] = {
	{ STATE_NORMAL,			"NORMAL" },
	{ STATE_WARNING_HYST,	"WARNING (HYST)" },
	{ STATE_WARNING,		"WARNING" },
	{ STATE_CRITICAL,		"CRITICAL" },
	{ 0, NULL }
};

/* private function prototype */
static void sensor_ctrl_check(struct oc2gbts_mgr_instance *mgr);

const char *oc2gbts_mgr_sensor_get_state(enum oc2gbts_sensor_state state)
{
	return get_value_string(state_names, state);
}

static int next_state(enum oc2gbts_sensor_state current_state, int critical, int warning)
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
	/* switch on the PA */
	if (actions & SENSOR_ACT_NORM_PA_ON) {
		if (oc2gbts_power_set(OC2GBTS_POWER_PA, 1) != 0) {
			LOGP(DTEMP, LOGL_ERROR,
				"Failed to switch on the PA\n");
		} else {
			LOGP(DTEMP, LOGL_INFO,
				"Switched on the PA as normal action.\n");
		}
	}

	if (actions & SENSOR_ACT_NORM_BTS_SRV_ON) {
		LOGP(DTEMP, LOGL_INFO,
		"Going to switch on the BTS service\n");
		/*
		 * TODO: use/create something like nspawn that serializes
		 * and used SIGCHLD/waitpid to pick up the dead processes
		 * without invoking shell.
		 */
		system("/bin/systemctl start osmo-bts.service");
	}
}

static void handle_actions(int actions)
{
	/* switch off the PA */
	if (actions & SENSOR_ACT_PA_OFF) {
		if (oc2gbts_option_get(OC2GBTS_OPTION_PA)) {
			if (oc2gbts_power_set(OC2GBTS_POWER_PA, 0) != 0) {
				LOGP(DTEMP, LOGL_ERROR,
					"Failed to switch off the PA. Stop BTS?\n");
			} else {
				LOGP(DTEMP, LOGL_NOTICE,
					"Switched off the PA due temperature.\n");
			}
		}
	}

	if (actions & SENSOR_ACT_BTS_SRV_OFF) {
		LOGP(DTEMP, LOGL_NOTICE,
			"Going to switch off the BTS service\n");
		/*
		 * TODO: use/create something like nspawn that serializes
		 * and used SIGCHLD/waitpid to pick up the dead processes
		 * without invoking shell.
		 */
		system("/bin/systemctl stop osmo-bts.service");
	}
}

void handle_ceased_actions(struct oc2gbts_mgr_instance *mgr)
{	int i;
	uint32_t cause;

	if (!mgr->oc2gbts_ctrl.is_up)
		return;

	LOGP(DTEMP, LOGL_DEBUG, "handle_ceased_actions in state %s, warn_flags=0x%x, crit_flags=0x%x\n",
			oc2gbts_mgr_sensor_get_state(mgr->state.state),
			mgr->oc2gbts_ctrl.warn_flags,
			mgr->oc2gbts_ctrl.crit_flags);

	for (i = 0; i < 32; i++) {
		cause = 1 << i;
		/* clear warning flag without sending ceased alarm */
		if (mgr->oc2gbts_ctrl.warn_flags & cause)
			mgr->oc2gbts_ctrl.warn_flags &= ~cause;

		/* clear warning flag with sending ceased alarm */
		if (mgr->oc2gbts_ctrl.crit_flags & cause) {
			/* clear associated flag */
			mgr->oc2gbts_ctrl.crit_flags &= ~cause;
			/* dispatch ceased alarm */
			switch (cause) {
			case S_MGR_TEMP_SUPPLY_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_TEMP_SUPPLY_MAX_FAIL, "oc2g-oml-ceased", "Main power supply temperature is too high");
				break;
			case S_MGR_TEMP_SOC_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_TEMP_SOC_MAX_FAIL, "oc2g-oml-ceased", "SoC temperature is too high");
				break;
			case S_MGR_TEMP_FPGA_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_TEMP_FPGA_MAX_FAIL, "oc2g-oml-ceased", "FPGA temperature is too high");
				break;
			case S_MGR_TEMP_RMS_DET_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_TEMP_RMS_DET_MAX_FAIL, "oc2g-oml-ceased", "RMS detector temperature is too high");
				break;
			case S_MGR_TEMP_OCXO_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_TEMP_OCXO_MAX_FAIL, "oc2g-oml-ceased", "OCXO temperature is too high");
				break;
			case S_MGR_TEMP_TRX_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_TEMP_TRX_MAX_FAIL, "oc2g-oml-ceased", "TRX temperature is too high");
				break;
			case S_MGR_TEMP_PA_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_TEMP_PA_MAX_FAIL, "oc2g-oml-ceased", "PA temperature is too high");
				break;
			case S_MGR_SUPPLY_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_SUPPLY_MAX_FAIL, "oc2g-oml-ceased", "Power supply voltage is too high");
				break;
			case S_MGR_SUPPLY_CRIT_MIN_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_SUPPLY_MIN_FAIL, "oc2g-oml-ceased", "Power supply voltage is too low");
				break;
			case S_MGR_VSWR_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_VSWR_MAX_FAIL, "oc2g-oml-ceased", "VSWR is too high");
				break;
			case S_MGR_PWR_SUPPLY_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_PWR_SUPPLY_MAX_FAIL, "oc2g-oml-ceased", "Power supply consumption is too high");
				break;
			case S_MGR_PWR_PA_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_PWR_PA_MAX_FAIL, "oc2g-oml-ceased", "PA power consumption is too high");
				break;
			default:
				break;
			}
		}
	}
	return;
}

void handle_alert_actions(struct oc2gbts_mgr_instance *mgr)
{	int i;
	uint32_t cause;

	if (!mgr->oc2gbts_ctrl.is_up)
		return;

	LOGP(DTEMP, LOGL_DEBUG, "handle_alert_actions in state %s, crit_flags=0x%x\n",
			oc2gbts_mgr_sensor_get_state(mgr->state.state),
			mgr->oc2gbts_ctrl.crit_flags);

	for (i = 0; i < 32; i++) {
		cause = 1 << i;
		if (mgr->oc2gbts_ctrl.crit_flags & cause) {
			switch(cause) {
			case S_MGR_TEMP_SUPPLY_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_TEMP_SUPPLY_MAX_FAIL, "oc2g-oml-alert", "Main power supply temperature is too high");
				break;
			case S_MGR_TEMP_SOC_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_TEMP_SOC_MAX_FAIL, "oc2g-oml-alert", "SoC temperature is too high");
				break;
			case S_MGR_TEMP_FPGA_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_TEMP_FPGA_MAX_FAIL, "oc2g-oml-alert", "FPGA temperature is too high");
				break;
			case S_MGR_TEMP_RMS_DET_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_TEMP_RMS_DET_MAX_FAIL, "oc2g-oml-alert", "RMS detector temperature is too high");
				break;
			case S_MGR_TEMP_OCXO_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_TEMP_OCXO_MAX_FAIL, "oc2g-oml-alert", "OCXO temperature is too high");
				break;
			case S_MGR_TEMP_TRX_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_TEMP_TRX_MAX_FAIL, "oc2g-oml-alert", "TRX temperature is too high");
				break;
			case S_MGR_TEMP_PA_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_TEMP_PA_MAX_FAIL, "oc2g-oml-alert", "PA temperature is too high");
				break;
			case S_MGR_SUPPLY_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_SUPPLY_MAX_FAIL, "oc2g-oml-alert", "Power supply voltage is too high");
				break;
			case S_MGR_SUPPLY_CRIT_MIN_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_SUPPLY_MIN_FAIL, "oc2g-oml-alert", "Power supply voltage is too low");
				break;
			case S_MGR_VSWR_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_VSWR_MAX_FAIL, "oc2g-oml-alert", "VSWR is too high");
				break;
			case S_MGR_PWR_SUPPLY_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_PWR_SUPPLY_MAX_FAIL, "oc2g-oml-alert", "Power supply consumption is too high");
				break;
			case S_MGR_PWR_PA_CRIT_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_CRIT_PWR_PA_MAX_FAIL, "oc2g-oml-alert", "PA power consumption is too high");
				break;
			default:
				break;
			}
		}
	}
	return;
}

void handle_warn_actions(struct oc2gbts_mgr_instance *mgr)
{	int i;
	uint32_t cause;

	if (!mgr->oc2gbts_ctrl.is_up)
		return;

	LOGP(DTEMP, LOGL_DEBUG, "handle_warn_actions in state %s, warn_flags=0x%x\n",
			oc2gbts_mgr_sensor_get_state(mgr->state.state),
			mgr->oc2gbts_ctrl.warn_flags);

	for (i = 0; i < 32; i++) {
		cause = 1 << i;
		if (mgr->oc2gbts_ctrl.warn_flags & cause) {
			switch(cause) {
			case S_MGR_TEMP_SUPPLY_WARN_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_TEMP_SUPPLY_HIGH_FAIL, "oc2g-oml-alert", "Main power supply temperature is high");
				break;
			case S_MGR_TEMP_SUPPLY_WARN_MIN_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_TEMP_SUPPLY_LOW_FAIL, "oc2g-oml-alert", "Main power supply temperature is low");
				break;
			case S_MGR_TEMP_SOC_WARN_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_TEMP_SOC_HIGH_FAIL, "oc2g-oml-alert", "SoC temperature is high");
				break;
			case S_MGR_TEMP_SOC_WARN_MIN_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_TEMP_SOC_LOW_FAIL, "oc2g-oml-alert", "SoC temperature is low");
				break;
			case S_MGR_TEMP_FPGA_WARN_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_TEMP_FPGA_HIGH_FAIL, "oc2g-oml-alert", "FPGA temperature is high");
				break;
			case S_MGR_TEMP_FPGA_WARN_MIN_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_TEMP_FPGA_LOW_FAIL, "oc2g-oml-alert", "FPGA temperature is low");
				break;
			case S_MGR_TEMP_RMS_DET_WARN_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_TEMP_RMS_DET_HIGH_FAIL, "oc2g-oml-alert", "RMS detector temperature is high");
				break;
			case S_MGR_TEMP_RMS_DET_WARN_MIN_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_TEMP_RMS_DET_LOW_FAIL, "oc2g-oml-alert", "RMS detector temperature is low");
				break;
			case S_MGR_TEMP_OCXO_WARN_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_TEMP_OCXO_HIGH_FAIL, "oc2g-oml-alert", "OCXO temperature is high");
				break;
			case S_MGR_TEMP_OCXO_WARN_MIN_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_TEMP_OCXO_LOW_FAIL, "oc2g-oml-alert", "OCXO temperature is low");
				break;
			case S_MGR_TEMP_TRX_WARN_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_TEMP_TRX_HIGH_FAIL, "oc2g-oml-alert", "TRX temperature is high");
				break;
			case S_MGR_TEMP_TRX_WARN_MIN_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_TEMP_TRX_LOW_FAIL, "oc2g-oml-alert", "TRX temperature is low");
				break;
			case S_MGR_TEMP_PA_WARN_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_TEMP_PA_HIGH_FAIL, "oc2g-oml-alert", "PA temperature is high");
				break;
			case S_MGR_TEMP_PA_WARN_MIN_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_TEMP_PA_LOW_FAIL, "oc2g-oml-alert", "PA temperature is low");
				break;
			case S_MGR_SUPPLY_WARN_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_SUPPLY_HIGH_FAIL, "oc2g-oml-alert", "Power supply voltage is high");
				break;
			case S_MGR_SUPPLY_WARN_MIN_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_SUPPLY_LOW_FAIL, "oc2g-oml-alert", "Power supply voltage is low");
				break;
			case S_MGR_VSWR_WARN_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_VSWR_HIGH_FAIL, "oc2g-oml-alert", "VSWR is high");
				break;
			case S_MGR_PWR_SUPPLY_WARN_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_PWR_SUPPLY_HIGH_FAIL, "oc2g-oml-alert", "Power supply consumption is high");
				break;
			case S_MGR_PWR_PA_WARN_MAX_ALARM:
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_PWR_PA_HIGH_FAIL, "oc2g-oml-alert", "PA power consumption is high");
				break;
			default:
				break;
			}
		}
	}
	return;
}

/**
 * Go back to normal! Depending on the configuration execute the normal
 * actions that could (start to) undo everything we did in the other
 * states. What is still missing is the power increase/decrease depending
 * on the state. E.g. starting from WARNING_HYST we might want to slowly
 * ramp up the output power again.
 */
static void execute_normal_act(struct oc2gbts_mgr_instance *manager)
{
	LOGP(DTEMP, LOGL_NOTICE, "System is back to normal state.\n");
	handle_ceased_actions(manager);
	handle_normal_actions(manager->state.action_norm);
}

static void execute_warning_act(struct oc2gbts_mgr_instance *manager)
{
	LOGP(DTEMP, LOGL_NOTICE, "System has reached warning state.\n");
	handle_warn_actions(manager);
	handle_actions(manager->state.action_warn);
}

/* Preventive timer call-back */
static void preventive_timer_cb(void *_data)
{
	struct oc2gbts_mgr_instance *mgr = _data;

	/* Delete current preventive timer if possible */
	osmo_timer_del(&mgr->alarms.preventive_timer);

	LOGP(DTEMP, LOGL_DEBUG, "Preventive timer expired in %d sec, retry=%d\n",
			mgr->alarms.preventive_duration,
			mgr->alarms.preventive_retry);

	/* Turn on PA and clear action flag */
	if (mgr->state.action_comb & SENSOR_ACT_PA_OFF) {
		mgr->state.action_comb &= ~SENSOR_ACT_PA_OFF;

		if (oc2gbts_option_get(OC2GBTS_OPTION_PA)) {
			if (oc2gbts_power_set(OC2GBTS_POWER_PA, 1))
				LOGP(DTEMP, LOGL_ERROR, "Failed to switch on the PA\n");
			else
				LOGP(DTEMP, LOGL_DEBUG, "Re-enable PA after preventive timer expired in %d sec\n",
					mgr->alarms.preventive_duration);
		}
	}

	/* restart check sensor timer */
	osmo_timer_del(&sensor_ctrl_timer);
	osmo_timer_schedule(&sensor_ctrl_timer, OC2GBTS_SENSOR_TIMER_DURATION, 0);

	return;

}

static void execute_preventive_act(struct oc2gbts_mgr_instance *manager)
{
	struct oc2gbts_preventive_list *prevent_list, *prevent_list2;

	/* update LED pattern */
	select_led_pattern(manager);

	/* do nothing if the preventive action list is empty */
	if (llist_empty(&manager->alarms.list))
		return;

	llist_for_each_entry_safe(prevent_list, prevent_list2, &manager->alarms.list, list) {
		/* Delete the timer in list and perform action*/
		if (prevent_list) {
			/* Delete current preventive timer if possible */
			osmo_timer_del(&manager->alarms.preventive_timer);

			/* Start/restart preventive timer */
			if (prevent_list->param.sleep_sec) {
				manager->alarms.preventive_timer.cb = preventive_timer_cb;
				manager->alarms.preventive_timer.data = manager;
				osmo_timer_schedule(&manager->alarms.preventive_timer, prevent_list->param.sleep_sec, 0);

				LOGP(DTEMP, LOGL_DEBUG,"Preventive timer scheduled for %d sec, preventive flags=0x%x\n",
						prevent_list->param.sleep_sec,
						prevent_list->action_flag);
			}
			/* Update active flags */
			manager->state.action_comb |= prevent_list->action_flag;

			/* Turn off PA */
			if (manager->state.action_comb & SENSOR_ACT_PA_OFF) {
				if (oc2gbts_power_set(OC2GBTS_POWER_PA, 0))
					LOGP(DTEMP, LOGL_ERROR, "Failed to switch off the PA\n");
			}

			/* Delete this preventive entry */
			llist_del(&prevent_list->list);
			talloc_free(prevent_list);
			LOGP(DTEMP, LOGL_DEBUG,"Deleted preventive entry from list, entries left=%d\n",
					llist_count(&manager->alarms.list));

			/* stay in last state is preventive active has exceed maximum number of retries */
			if (manager->alarms.preventive_retry > OC2GBTS_PREVENT_RETRY)
				LOGP(DTEMP, LOGL_NOTICE, "Maximum number of preventive active exceed\n");
			else
				/* increase retry counter */
				manager->alarms.preventive_retry++;
		}
	}
	return;
}

static void execute_critical_act(struct oc2gbts_mgr_instance *manager)
{
	LOGP(DTEMP, LOGL_NOTICE, "System has reached critical warning.\n");
	handle_alert_actions(manager);
	handle_actions(manager->state.action_crit);

}

static void oc2gbts_mgr_sensor_handle(struct oc2gbts_mgr_instance *manager,
			int critical, int warning)
{
	int new_state = next_state(manager->state.state, critical, warning);

	/* run preventive action if it is possible */
	execute_preventive_act(manager);

	/* Nothing changed */
	if (new_state < 0)
		return;
	LOGP(DTEMP, LOGL_INFO, "Moving from state %s to %s.\n",
		get_value_string(state_names, manager->state.state),
		get_value_string(state_names, new_state));
	manager->state.state = new_state;
	switch (manager->state.state) {
	case STATE_NORMAL:
		execute_normal_act(manager);
		/* reset alarms */
		manager->alarms.temp_high = 0;
		manager->alarms.temp_max = 0;
		manager->alarms.vswr_high = 0;
		manager->alarms.vswr_max = 0;
		manager->alarms.supply_low = 0;
		manager->alarms.supply_min = 0;
		manager->alarms.supply_pwr_high = 0;
		manager->alarms.supply_pwr_max = 0;
		manager->alarms.pa_pwr_max = 0;
		manager->alarms.pa_pwr_high = 0;
		manager->state.action_comb = 0;
		manager->alarms.preventive_retry = 0;
		/* update LED pattern */
		select_led_pattern(manager);
		break;
	case STATE_WARNING_HYST:
		/* do nothing? Maybe start to increase transmit power? */
		break;
	case STATE_WARNING:
		execute_warning_act(manager);
		/* update LED pattern */
		select_led_pattern(manager);
		break;
	case STATE_CRITICAL:
		execute_critical_act(manager);
		/* update LED pattern */
		select_led_pattern(manager);
		break;
	};
} 

static void schedule_preventive_action(struct oc2gbts_mgr_instance *mgr, int action, int duration)
{
	struct oc2gbts_preventive_list *prevent_list;

	/* add to pending list */
	prevent_list = talloc_zero(tall_mgr_ctx, struct oc2gbts_preventive_list);
	if (prevent_list) {
		prevent_list->action_flag = action;
		prevent_list->param.sleep_sec = duration;
		prevent_list->param.sleep_usec = 0;
		llist_add_tail(&prevent_list->list, &mgr->alarms.list);
		LOGP(DTEMP, LOGL_DEBUG,"Added preventive action to list, duration=%d sec, total entries=%d\n",
			prevent_list->param.sleep_sec,
			llist_count(&mgr->alarms.list));
	}
	return;
}

static void sensor_ctrl_check(struct oc2gbts_mgr_instance *mgr)
{
	int rc;
	int temp, volt, vswr, power = 0;
	int warn_thresh_passed = 0;
	int crit_thresh_passed = 0;
	int action = 0;

	LOGP(DTEMP, LOGL_INFO, "Going to check the temperature.\n");

	/* Read the current supply temperature */
	rc = oc2gbts_temp_get(OC2GBTS_TEMP_SUPPLY, &temp);
	if (rc < 0) {
		LOGP(DTEMP, LOGL_NOTICE,
			"Failed to read the supply temperature. rc=%d\n", rc);
		warn_thresh_passed = crit_thresh_passed = 1;
	} else {
		temp = temp / 1000;
		if (temp > mgr->temp.supply_temp_limit.thresh_warn_max) {
			LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because supply temperature is over %d\n", mgr->temp.supply_temp_limit.thresh_warn_max);
			warn_thresh_passed = 1;
			mgr->alarms.temp_high = 1;
			mgr->oc2gbts_ctrl.warn_flags |= S_MGR_TEMP_SUPPLY_WARN_MAX_ALARM;
			/* add to pending list */
			schedule_preventive_action(mgr, action, OC2GBTS_PREVENT_TIMER_DURATION);
		}
		if (temp < mgr->temp.supply_temp_limit.thresh_warn_min){
			LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because supply temperature is under %d\n", mgr->temp.supply_temp_limit.thresh_warn_min);
			warn_thresh_passed = 1;
			mgr->oc2gbts_ctrl.warn_flags |= S_MGR_TEMP_SUPPLY_WARN_MIN_ALARM;
		}
		if (temp > mgr->temp.supply_temp_limit.thresh_crit_max) {
			LOGP(DTEMP, LOGL_NOTICE, "System has reached critical because supply temperature is over %d\n", mgr->temp.supply_temp_limit.thresh_crit_max);
			crit_thresh_passed = 1;
			mgr->alarms.temp_max = 1;
			mgr->oc2gbts_ctrl.crit_flags |= S_MGR_TEMP_SUPPLY_CRIT_MAX_ALARM;
			mgr->oc2gbts_ctrl.warn_flags &= ~S_MGR_TEMP_SUPPLY_WARN_MAX_ALARM;
			action = SENSOR_ACT_PA_OFF;
			/* add to pending list */
			schedule_preventive_action(mgr, action, OC2GBTS_PREVENT_TIMER_DURATION);
		}
		LOGP(DTEMP, LOGL_INFO, "Supply temperature is: %d\n", temp);
	}

	/* Read the current SoC temperature */
	rc = oc2gbts_temp_get(OC2GBTS_TEMP_SOC, &temp);
	if (rc < 0) {
		LOGP(DTEMP, LOGL_NOTICE,
			"Failed to read the SoC temperature. rc=%d\n", rc);
		warn_thresh_passed = crit_thresh_passed = 1;
	} else {
		temp = temp / 1000;
		if (temp > mgr->temp.soc_temp_limit.thresh_warn_max) {
			LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because SoC temperature is over %d\n", mgr->temp.soc_temp_limit.thresh_warn_max);
			warn_thresh_passed = 1;
			mgr->alarms.temp_high = 1;
			mgr->oc2gbts_ctrl.warn_flags |= S_MGR_TEMP_SOC_WARN_MAX_ALARM;
			/* add to pending list */
			schedule_preventive_action(mgr, action, OC2GBTS_PREVENT_TIMER_DURATION);
		}
		if (temp < mgr->temp.soc_temp_limit.thresh_warn_min){
			LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because SoC temperature is under %d\n", mgr->temp.soc_temp_limit.thresh_warn_min);
			warn_thresh_passed = 1;
			mgr->oc2gbts_ctrl.warn_flags |= S_MGR_TEMP_SOC_WARN_MIN_ALARM;
		}
		if (temp > mgr->temp.soc_temp_limit.thresh_crit_max) {
			LOGP(DTEMP, LOGL_NOTICE, "System has reached critical because SoC temperature is over %d\n", mgr->temp.soc_temp_limit.thresh_crit_max);
			crit_thresh_passed = 1;
			mgr->alarms.temp_max = 1;
			mgr->oc2gbts_ctrl.crit_flags |= S_MGR_TEMP_SOC_CRIT_MAX_ALARM;
			mgr->oc2gbts_ctrl.warn_flags &= ~S_MGR_TEMP_SOC_WARN_MAX_ALARM;
			action = SENSOR_ACT_PA_OFF;
			/* add to pending list */
			schedule_preventive_action(mgr, action, OC2GBTS_PREVENT_TIMER_DURATION);
		}
		LOGP(DTEMP, LOGL_INFO, "SoC temperature is: %d\n", temp);
	}

	/* Read the current fpga temperature */
	rc = oc2gbts_temp_get(OC2GBTS_TEMP_FPGA, &temp);
	if (rc < 0) {
		LOGP(DTEMP, LOGL_NOTICE,
			"Failed to read the fpga temperature. rc=%d\n", rc);
		warn_thresh_passed = crit_thresh_passed = 1;
	} else {
		temp = temp / 1000;
		if (temp > mgr->temp.fpga_temp_limit.thresh_warn_max) {
			LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because fpga temperature is over %d\n", mgr->temp.fpga_temp_limit.thresh_warn_max);
			warn_thresh_passed = 1;
			mgr->alarms.temp_high = 1;
			mgr->oc2gbts_ctrl.warn_flags |= S_MGR_TEMP_FPGA_WARN_MAX_ALARM;
			/* add to pending list */
			schedule_preventive_action(mgr, action, OC2GBTS_PREVENT_TIMER_DURATION);
		}
		if (temp < mgr->temp.fpga_temp_limit.thresh_warn_min) {
			LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because fpga temperature is under %d\n", mgr->temp.fpga_temp_limit.thresh_warn_min);
			warn_thresh_passed = 1;
			mgr->oc2gbts_ctrl.warn_flags |= S_MGR_TEMP_FPGA_WARN_MIN_ALARM;
		}
		if (temp > mgr->temp.fpga_temp_limit.thresh_crit_max) {
			LOGP(DTEMP, LOGL_NOTICE, "System has reached critical because fpga temperature is over %d\n", mgr->temp.fpga_temp_limit.thresh_crit_max);
			crit_thresh_passed = 1;
			mgr->alarms.temp_max = 1;
			mgr->oc2gbts_ctrl.crit_flags |= S_MGR_TEMP_FPGA_CRIT_MAX_ALARM;
			mgr->oc2gbts_ctrl.warn_flags &= ~S_MGR_TEMP_FPGA_WARN_MAX_ALARM;
			action = SENSOR_ACT_PA_OFF;
			/* add to pending list */
			schedule_preventive_action(mgr, action, OC2GBTS_PREVENT_TIMER_DURATION);
		}
		LOGP(DTEMP, LOGL_INFO, "FPGA temperature is: %d\n", temp);
	}

	if (oc2gbts_option_get(OC2GBTS_OPTION_RMS_FWD) || oc2gbts_option_get(OC2GBTS_OPTION_RMS_REFL)) {
		/* Read the current RMS detector temperature */
		rc = oc2gbts_temp_get(OC2GBTS_TEMP_RMSDET, &temp);
		if (rc < 0) {
			LOGP(DTEMP, LOGL_NOTICE,
				"Failed to read the RMS detector temperature. rc=%d\n", rc);
			warn_thresh_passed = crit_thresh_passed = 1;
		} else {
			temp = temp / 1000;
			if (temp > mgr->temp.rmsdet_temp_limit.thresh_warn_max) {
				LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because RMS detector temperature is over %d\n", mgr->temp.rmsdet_temp_limit.thresh_warn_max);
				warn_thresh_passed = 1;
				mgr->alarms.temp_high = 1;
				mgr->oc2gbts_ctrl.warn_flags |= S_MGR_TEMP_RMS_DET_WARN_MAX_ALARM;
				/* add to pending list */
				schedule_preventive_action(mgr, action, OC2GBTS_PREVENT_TIMER_DURATION);
			}
			if (temp < mgr->temp.rmsdet_temp_limit.thresh_warn_min) {
				LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because RMS detector temperature is under %d\n", mgr->temp.rmsdet_temp_limit.thresh_warn_min);
				warn_thresh_passed = 1;
				mgr->oc2gbts_ctrl.warn_flags |= S_MGR_TEMP_RMS_DET_WARN_MIN_ALARM;
			}
			if (temp > mgr->temp.rmsdet_temp_limit.thresh_crit_max) {
				LOGP(DTEMP, LOGL_NOTICE, "System has reached critical because RMS detector temperature is over %d\n", mgr->temp.rmsdet_temp_limit.thresh_crit_max);
				crit_thresh_passed = 1;
				mgr->alarms.temp_max = 1;
				mgr->oc2gbts_ctrl.crit_flags |= S_MGR_TEMP_RMS_DET_CRIT_MAX_ALARM;
				mgr->oc2gbts_ctrl.warn_flags &= ~S_MGR_TEMP_RMS_DET_WARN_MAX_ALARM;
				action = SENSOR_ACT_PA_OFF;
				/* add to pending list */
				schedule_preventive_action(mgr, action, OC2GBTS_PREVENT_TIMER_DURATION);
			}
			LOGP(DTEMP, LOGL_INFO, "RMS detector temperature is: %d\n", temp);
		}
	}

	/* Read the current OCXO temperature */
	rc = oc2gbts_temp_get(OC2GBTS_TEMP_OCXO, &temp);
	if (rc < 0) {
		LOGP(DTEMP, LOGL_NOTICE,
			"Failed to read the OCXO temperature. rc=%d\n", rc);
		warn_thresh_passed = crit_thresh_passed = 1;
	} else {
		temp = temp / 1000;
		if (temp > mgr->temp.ocxo_temp_limit.thresh_warn_max) {
			LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because OCXO temperature is over %d\n", mgr->temp.ocxo_temp_limit.thresh_warn_max);
			warn_thresh_passed = 1;
			mgr->alarms.temp_high = 1;
			mgr->oc2gbts_ctrl.warn_flags |= S_MGR_TEMP_OCXO_WARN_MAX_ALARM;
			/* add to pending list */
			schedule_preventive_action(mgr, action, OC2GBTS_PREVENT_TIMER_DURATION);
		}
		if (temp < mgr->temp.ocxo_temp_limit.thresh_warn_min) {
			LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because OCXO temperature is under %d\n", mgr->temp.ocxo_temp_limit.thresh_warn_min);
			warn_thresh_passed = 1;
			mgr->oc2gbts_ctrl.warn_flags |= S_MGR_TEMP_OCXO_WARN_MIN_ALARM;
		}
		if (temp > mgr->temp.ocxo_temp_limit.thresh_crit_max) {
			LOGP(DTEMP, LOGL_NOTICE, "System has reached critical because OCXO temperature is over %d\n", mgr->temp.ocxo_temp_limit.thresh_crit_max);
			crit_thresh_passed = 1;
			mgr->alarms.temp_max = 1;
			mgr->oc2gbts_ctrl.crit_flags |= S_MGR_TEMP_OCXO_CRIT_MAX_ALARM;
			mgr->oc2gbts_ctrl.warn_flags &= ~S_MGR_TEMP_OCXO_WARN_MAX_ALARM;
			action = SENSOR_ACT_PA_OFF;
			/* add to pending list */
			schedule_preventive_action(mgr, action, OC2GBTS_PREVENT_TIMER_DURATION);
		}
		LOGP(DTEMP, LOGL_INFO, "OCXO temperature is: %d\n", temp);
	}

	/* Read the current TX temperature */
	rc = oc2gbts_temp_get(OC2GBTS_TEMP_TX, &temp);
	if (rc < 0) {
		LOGP(DTEMP, LOGL_NOTICE,
			"Failed to read the TX temperature. rc=%d\n", rc);
		warn_thresh_passed = crit_thresh_passed = 1;
	} else {
		temp = temp / 1000;
		if (temp > mgr->temp.tx_temp_limit.thresh_warn_max) {
			LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because TX temperature is over %d\n", mgr->temp.tx_temp_limit.thresh_warn_max);
			warn_thresh_passed = 1;
			mgr->alarms.temp_high = 1;
			mgr->oc2gbts_ctrl.warn_flags |= S_MGR_TEMP_TRX_WARN_MAX_ALARM;
			/* add to pending list */
			schedule_preventive_action(mgr, action, OC2GBTS_PREVENT_TIMER_DURATION);
		}
		if (temp < mgr->temp.tx_temp_limit.thresh_warn_min) {
			LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because TX temperature is under %d\n", mgr->temp.tx_temp_limit.thresh_warn_min);
			warn_thresh_passed = 1;
			mgr->oc2gbts_ctrl.warn_flags |= S_MGR_TEMP_TRX_WARN_MIN_ALARM;
		}
		if (temp > mgr->temp.tx_temp_limit.thresh_crit_max) {
			LOGP(DTEMP, LOGL_NOTICE, "System has reached critical because TX temperature is over %d\n", mgr->temp.tx_temp_limit.thresh_crit_max);
			crit_thresh_passed = 1;
			mgr->alarms.temp_max = 1;
			mgr->oc2gbts_ctrl.crit_flags |= S_MGR_TEMP_TRX_CRIT_MAX_ALARM;
			mgr->oc2gbts_ctrl.warn_flags &= ~S_MGR_TEMP_TRX_WARN_MAX_ALARM;
			action = SENSOR_ACT_PA_OFF;
			/* add to pending list */
			schedule_preventive_action(mgr, action, OC2GBTS_PREVENT_TIMER_DURATION);
		}
		LOGP(DTEMP, LOGL_INFO, "TX temperature is: %d\n", temp);
	}

	if (oc2gbts_option_get(OC2GBTS_OPTION_PA_TEMP)) {
		/* Read the current PA temperature */
		rc = oc2gbts_temp_get(OC2GBTS_TEMP_PA, &temp);
		if (rc < 0) {
			LOGP(DTEMP, LOGL_NOTICE,
				"Failed to read the PA temperature. rc=%d\n", rc);
			warn_thresh_passed = crit_thresh_passed = 1;
		} else {
			temp = temp / 1000;
			if (temp > mgr->temp.pa_temp_limit.thresh_warn_max) {
				LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because PA temperature because is over %d\n", mgr->temp.pa_temp_limit.thresh_warn_max);
				warn_thresh_passed = 1;
				mgr->alarms.temp_high = 1;
				mgr->oc2gbts_ctrl.warn_flags |= S_MGR_TEMP_PA_WARN_MAX_ALARM;
				action = SENSOR_ACT_PA_OFF;
				/* add to pending list */
				schedule_preventive_action(mgr, action, OC2GBTS_PREVENT_TIMER_DURATION);
			}
			if (temp < mgr->temp.pa_temp_limit.thresh_warn_min) {
				LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because PA temperature because is under %d\n", mgr->temp.pa_temp_limit.thresh_warn_min);
				warn_thresh_passed = 1;
				mgr->oc2gbts_ctrl.warn_flags |= S_MGR_TEMP_PA_WARN_MIN_ALARM;
			}
			if (temp > mgr->temp.pa_temp_limit.thresh_crit_max) {
				LOGP(DTEMP, LOGL_NOTICE, "System has reached critical because PA temperature because is over %d\n", mgr->temp.pa_temp_limit.thresh_crit_max);
				crit_thresh_passed = 1;
				mgr->alarms.temp_max = 1;
				mgr->oc2gbts_ctrl.crit_flags |= S_MGR_TEMP_PA_CRIT_MAX_ALARM;
				mgr->oc2gbts_ctrl.warn_flags &= ~S_MGR_TEMP_PA_WARN_MAX_ALARM;
				action = SENSOR_ACT_PA_OFF;
				/* add to pending list */
				schedule_preventive_action(mgr, action, OC2GBTS_PREVENT_TIMER_DURATION);
			}
			LOGP(DTEMP, LOGL_INFO, "PA temperature is: %d\n", temp);
		}
	}

	/* Read the current main supply voltage */
	if (oc2gbts_power_get(OC2GBTS_POWER_SUPPLY)) {
		rc = oc2gbts_power_sensor_get(OC2GBTS_POWER_SUPPLY, OC2GBTS_POWER_VOLTAGE, &volt);
		if (rc < 0) {
			LOGP(DTEMP, LOGL_NOTICE,
				"Failed to read the main supply voltage. rc=%d\n", rc);
			warn_thresh_passed = crit_thresh_passed = 1;
		} else {
			if (volt > mgr->volt.supply_volt_limit.thresh_warn_max) {
				LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because supply voltage is over %d\n", mgr->volt.supply_volt_limit.thresh_warn_max);
				warn_thresh_passed = 1;
				mgr->oc2gbts_ctrl.warn_flags |= S_MGR_SUPPLY_WARN_MAX_ALARM;
			}
			if (volt < mgr->volt.supply_volt_limit.thresh_warn_min) {
				LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because supply voltage is under %d\n", mgr->volt.supply_volt_limit.thresh_warn_min);
				warn_thresh_passed = 1;
				mgr->alarms.supply_low = 1;
				mgr->oc2gbts_ctrl.warn_flags |= S_MGR_SUPPLY_WARN_MIN_ALARM;
			}
			if (volt > mgr->volt.supply_volt_limit.thresh_crit_max) {
				LOGP(DTEMP, LOGL_NOTICE, "System has reached critical because supply voltage is over %d\n", mgr->volt.supply_volt_limit.thresh_crit_max);
				crit_thresh_passed = 1;
				mgr->oc2gbts_ctrl.crit_flags |= S_MGR_SUPPLY_CRIT_MAX_ALARM;
				mgr->oc2gbts_ctrl.warn_flags &= ~S_MGR_SUPPLY_WARN_MAX_ALARM;
			}

			if (volt < mgr->volt.supply_volt_limit.thresh_crit_min) {
				LOGP(DTEMP, LOGL_NOTICE, "System has reached critical because supply voltage is under %d\n", mgr->volt.supply_volt_limit.thresh_crit_min);
				crit_thresh_passed = 1;
				mgr->alarms.supply_min = 1;
				mgr->oc2gbts_ctrl.crit_flags |= S_MGR_SUPPLY_CRIT_MIN_ALARM;
				mgr->oc2gbts_ctrl.warn_flags &= ~S_MGR_SUPPLY_WARN_MIN_ALARM;
				action = SENSOR_ACT_PA_OFF;
				/* add to pending list */
				schedule_preventive_action(mgr, action, OC2GBTS_PREVENT_TIMER_NONE);
			}
			LOGP(DTEMP, LOGL_INFO, "Main supply voltage is: %d\n", volt);
		}
	}

	/* Read the main supply power consumption */
	if (oc2gbts_power_get(OC2GBTS_POWER_SUPPLY)) {
		rc = oc2gbts_power_sensor_get(OC2GBTS_POWER_SUPPLY, OC2GBTS_POWER_POWER, &power);
		if (rc < 0) {
			LOGP(DTEMP, LOGL_NOTICE,
				"Failed to read the power supply current. rc=%d\n", rc);
			warn_thresh_passed = crit_thresh_passed = 1;
		} else {
			power /= 1000000;
			if (power > mgr->pwr.supply_pwr_limit.thresh_warn_max) {
				LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because main supply power consumption is over %d\n",  mgr->pwr.supply_pwr_limit.thresh_warn_max);
				warn_thresh_passed = 1;
				mgr->alarms.supply_pwr_high = 1;
				mgr->oc2gbts_ctrl.warn_flags |= S_MGR_PWR_SUPPLY_WARN_MAX_ALARM;
			}
			if (power > mgr->pwr.supply_pwr_limit.thresh_crit_max) {
				LOGP(DTEMP, LOGL_NOTICE, "System has reached critical because main supply power consumption is over %d\n",  mgr->pwr.supply_pwr_limit.thresh_crit_max);
				crit_thresh_passed = 1;

				mgr->oc2gbts_ctrl.crit_flags |= S_MGR_PWR_SUPPLY_CRIT_MAX_ALARM;
				mgr->oc2gbts_ctrl.warn_flags &= ~S_MGR_PWR_SUPPLY_WARN_MAX_ALARM;

				if (oc2gbts_power_get(OC2GBTS_POWER_PA)) {
						mgr->alarms.supply_pwr_max = 1;
						/* schedule to turn off PA */
						action = SENSOR_ACT_PA_OFF;
						/* repeat same alarm to BSC */
						handle_alert_actions(mgr);
				}
				/* add to pending list */
				schedule_preventive_action(mgr, action, OC2GBTS_PREVENT_TIMER_SHORT_DURATION);
			}
			LOGP(DTEMP, LOGL_INFO, "Main supply current power consumption is: %d\n", power);
		}
	} else {
		/* keep last state */
		if (mgr->oc2gbts_ctrl.crit_flags & S_MGR_PWR_SUPPLY_CRIT_MAX_ALARM) {
			warn_thresh_passed = 1;
			crit_thresh_passed = 1;
		}
	}

	if (oc2gbts_option_get(OC2GBTS_OPTION_PA)) {
		/* Read the current PA power consumption */
		if (oc2gbts_power_get(OC2GBTS_POWER_PA)) {
			rc = oc2gbts_power_sensor_get(OC2GBTS_POWER_PA, OC2GBTS_POWER_POWER, &power);
			if (rc < 0) {
				LOGP(DTEMP, LOGL_NOTICE,
					"Failed to read the PA power. rc=%d\n", rc);
				warn_thresh_passed = crit_thresh_passed = 1;
			} else {
				power /= 1000000;
				if (power > mgr->pwr.pa_pwr_limit.thresh_warn_max) {
					LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because PA power consumption is over %d\n",  mgr->pwr.pa_pwr_limit.thresh_warn_max);
					warn_thresh_passed = 1;
					mgr->alarms.pa_pwr_high = 1;
					mgr->oc2gbts_ctrl.warn_flags |= S_MGR_PWR_PA_WARN_MAX_ALARM;
				}
				if (power > mgr->pwr.pa_pwr_limit.thresh_crit_max) {
					LOGP(DTEMP, LOGL_NOTICE, "System has reached critical because PA power consumption is over %d\n",  mgr->pwr.pa_pwr_limit.thresh_crit_max);
					crit_thresh_passed = 1;
					mgr->alarms.pa_pwr_max = 1;
					mgr->oc2gbts_ctrl.crit_flags |= S_MGR_PWR_PA_CRIT_MAX_ALARM;
					mgr->oc2gbts_ctrl.warn_flags &= ~S_MGR_PWR_PA_WARN_MAX_ALARM;
					action = SENSOR_ACT_PA_OFF;
					/* add to pending list */
					schedule_preventive_action(mgr, action, OC2GBTS_PREVENT_TIMER_SHORT_DURATION);
				}
				LOGP(DTEMP, LOGL_INFO, "PA power consumption is: %d\n", power);
			}
		} else {
			/* keep last state */
			if (mgr->oc2gbts_ctrl.crit_flags & S_MGR_PWR_PA_CRIT_MAX_ALARM) {
				warn_thresh_passed = 1;
				crit_thresh_passed = 1;
			}
		}
	}

	if (oc2gbts_option_get(OC2GBTS_OPTION_PA) &&
			oc2gbts_option_get(OC2GBTS_OPTION_RMS_FWD) &&
			oc2gbts_option_get(OC2GBTS_OPTION_RMS_REFL)) {
		/* Read the current VSWR of powered ON PA*/
		if (oc2gbts_power_get(OC2GBTS_POWER_PA)) {
			rc = oc2gbts_vswr_get(OC2GBTS_VSWR, &vswr);
			if (rc < 0) {
				LOGP(DTEMP, LOGL_NOTICE,
					"Failed to read the VSWR. rc=%d\n", rc);
				warn_thresh_passed = crit_thresh_passed = 1;
			} else {
				if ((vswr > mgr->vswr.vswr_limit.thresh_warn_max) && (mgr->vswr.last_vswr > mgr->vswr.vswr_limit.thresh_warn_max)) {
					LOGP(DTEMP, LOGL_NOTICE, "System has reached warning because VSWR is over %d\n", mgr->vswr.vswr_limit.thresh_warn_max);
					warn_thresh_passed = 1;
					mgr->alarms.vswr_high = 1;
					mgr->oc2gbts_ctrl.warn_flags |= S_MGR_VSWR_WARN_MAX_ALARM;
				}
				if ((vswr > mgr->vswr.vswr_limit.thresh_crit_max) && (mgr->vswr.last_vswr > mgr->vswr.vswr_limit.thresh_crit_max)) {
					LOGP(DTEMP, LOGL_NOTICE, "System has reached critical because VSWR is over %d\n", mgr->vswr.vswr_limit.thresh_crit_max);
					crit_thresh_passed = 1;
					mgr->alarms.vswr_max = 1;
					mgr->oc2gbts_ctrl.crit_flags |= S_MGR_VSWR_CRIT_MAX_ALARM;
					mgr->oc2gbts_ctrl.warn_flags &= ~S_MGR_VSWR_WARN_MAX_ALARM;
					action = SENSOR_ACT_PA_OFF;
					/* add to pending list */
					schedule_preventive_action(mgr, action, OC2GBTS_PREVENT_TIMER_DURATION);
				}
				LOGP(DTEMP, LOGL_INFO, "VSWR is: current = %d, last = %d\n", vswr, mgr->vswr.last_vswr);

				/* update last VSWR */
				mgr->vswr.last_vswr = vswr;
			}
		} else {
			/* keep last state */
			if (mgr->oc2gbts_ctrl.crit_flags & S_MGR_VSWR_CRIT_MAX_ALARM) {
				warn_thresh_passed = 1;
				crit_thresh_passed = 1;
			}
		}
	}

	select_led_pattern(mgr);
	oc2gbts_mgr_sensor_handle(mgr, crit_thresh_passed, warn_thresh_passed);
}

static void sensor_ctrl_check_cb(void *_data)
{
	struct oc2gbts_mgr_instance *mgr = _data;
	sensor_ctrl_check(mgr);
	/* Check every minute? XXX make it configurable! */
	osmo_timer_schedule(&sensor_ctrl_timer, OC2GBTS_SENSOR_TIMER_DURATION, 0);
	LOGP(DTEMP, LOGL_DEBUG,"Check sensors timer expired\n");
	/* TODO: do we want to notify if some sensors could not be read? */
	oc2gbts_swd_event(mgr, SWD_CHECK_TEMP_SENSOR);
}

int oc2gbts_mgr_sensor_init(struct oc2gbts_mgr_instance *mgr)
{
	int rc = 0;

	/* always enable PA GPIO for OC-2G */
	if (!oc2gbts_power_get(OC2GBTS_POWER_PA)) {
		rc = oc2gbts_power_set(OC2GBTS_POWER_PA, 1);
		if (!rc)
			LOGP(DTEMP, LOGL_ERROR, "Failed to set GPIO for internal PA\n");
	}

	s_mgr = mgr;
	sensor_ctrl_timer.cb = sensor_ctrl_check_cb;
	sensor_ctrl_timer.data = s_mgr;
	sensor_ctrl_check_cb(s_mgr);
	return rc;
}

