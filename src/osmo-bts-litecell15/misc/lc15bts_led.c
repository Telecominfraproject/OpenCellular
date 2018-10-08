/* Copyright (C) 2016 by NuRAN Wireless <support@nuranwireless.com>
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
#include "lc15bts_led.h"
#include "lc15bts_bts.h"
#include <osmocom/core/talloc.h>
#include <osmocom/core/linuxlist.h>

static struct lc15bts_led led_entries[] = {
	{
		.name = "led0",
		.fullname = "led red",
		.path = "/var/lc15/leds/led0/brightness"
	},
	{
		.name = "led1",
		.fullname = "led green",
		.path = "/var/lc15/leds/led1/brightness"
	}
};

static const struct value_string lc15bts_led_strs[] = {
	{ LC15BTS_LED_RED, "LED red" },
	{ LC15BTS_LED_GREEN, "LED green" },
	{ LC15BTS_LED_ORANGE, "LED orange" },
	{ LC15BTS_LED_OFF, "LED off" },
	{ 0, NULL }
};

static uint8_t led_priority[] = {
	BLINK_PATTERN_INIT,
	BLINK_PATTERN_INT_PROC_MALFUNC,
	BLINK_PATTERN_SUPPLY_PWR_MAX,
	BLINK_PATTERN_PA_PWR_MAX,
	BLINK_PATTERN_VSWR_MAX,
	BLINK_PATTERN_SUPPLY_VOLT_MIN,
	BLINK_PATTERN_TEMP_MAX,
	BLINK_PATTERN_SUPPLY_PWR_MAX2,
	BLINK_PATTERN_EXT_LINK_MALFUNC,
	BLINK_PATTERN_SUPPLY_VOLT_LOW,
	BLINK_PATTERN_TEMP_HIGH,
	BLINK_PATTERN_VSWR_HIGH,
	BLINK_PATTERN_SUPPLY_PWR_HIGH,
	BLINK_PATTERN_PA_PWR_HIGH,
	BLINK_PATTERN_GPS_FIX_LOST,
	BLINK_PATTERN_NORMAL
};


char *blink_pattern_command[] = BLINK_PATTERN_COMMAND;

static int lc15bts_led_write(char *path, char *str)
{
	int fd;

	if ((fd = open(path, O_WRONLY)) == -1)
	{
		return 0;
	}

	write(fd, str, strlen(str)+1);
	close(fd);
	return 1;
}

static void led_set_red()
{
	lc15bts_led_write(led_entries[0].path, "1");
	lc15bts_led_write(led_entries[1].path, "0");
}

static void led_set_green()
{
	lc15bts_led_write(led_entries[0].path, "0");
	lc15bts_led_write(led_entries[1].path, "1");
}

static void led_set_orange()
{
	lc15bts_led_write(led_entries[0].path, "1");
	lc15bts_led_write(led_entries[1].path, "1");
}

static void led_set_off()
{
	lc15bts_led_write(led_entries[0].path, "0");
	lc15bts_led_write(led_entries[1].path, "0");
}

static void led_sleep( struct lc15bts_mgr_instance *mgr, struct lc15bts_led_timer *led_timer, void (*led_timer_cb)(void *_data)) {
	/* Cancel any pending timer */
	osmo_timer_del(&led_timer->timer);
	/* Start LED timer */
	led_timer->timer.cb = led_timer_cb;
	led_timer->timer.data = mgr;
	mgr->lc15bts_leds.active_timer = led_timer->idx;
	osmo_timer_schedule(&led_timer->timer, led_timer->param.sleep_sec, led_timer->param.sleep_usec);
	LOGP(DTEMP, LOGL_DEBUG,"%s timer scheduled for %d sec + %d usec\n",
			get_value_string(lc15bts_led_strs, led_timer->idx),
			led_timer->param.sleep_sec,
			led_timer->param.sleep_usec);

	switch (led_timer->idx) {
	case LC15BTS_LED_RED:
		led_set_red();
		break;
	case LC15BTS_LED_GREEN:
		led_set_green();
		break;
	case LC15BTS_LED_ORANGE:
		led_set_orange();
		break;
	case LC15BTS_LED_OFF:
		led_set_off();
		break;
	default:
		led_set_off();
	}
}

static void led_sleep_cb(void *_data) {
	struct lc15bts_mgr_instance *mgr = _data;
	struct lc15bts_led_timer_list *led_list;

	/* make sure the timer list is not empty */
	if (llist_empty(&mgr->lc15bts_leds.list))
		return;

	llist_for_each_entry(led_list, &mgr->lc15bts_leds.list, list) {
		if (led_list->led_timer.idx == mgr->lc15bts_leds.active_timer) {
			LOGP(DTEMP, LOGL_DEBUG,"Delete expired %s timer %d sec + %d usec\n",
				get_value_string(lc15bts_led_strs, led_list->led_timer.idx),
				led_list->led_timer.param.sleep_sec,
				led_list->led_timer.param.sleep_usec);

			/* Delete current timer */
			osmo_timer_del(&led_list->led_timer.timer);
			/* Rotate the timer list */
			 llist_move_tail(&led_list->list, &mgr->lc15bts_leds.list);
			break;
		}
	}

	/* Execute next timer */
	led_list = llist_first_entry(&mgr->lc15bts_leds.list, struct lc15bts_led_timer_list, list);
	if (led_list) {
		LOGP(DTEMP, LOGL_DEBUG,"Execute %s timer %d sec + %d usec, total entries=%d\n",
			get_value_string(lc15bts_led_strs, led_list->led_timer.idx),
			led_list->led_timer.param.sleep_sec,
			led_list->led_timer.param.sleep_usec,
			llist_count(&mgr->lc15bts_leds.list));

		led_sleep(mgr, &led_list->led_timer, led_sleep_cb);
	}

}

static void delete_led_timer_entries(struct lc15bts_mgr_instance *mgr)
{
	struct lc15bts_led_timer_list *led_list, *led_list2;

	if (llist_empty(&mgr->lc15bts_leds.list))
		return;

	llist_for_each_entry_safe(led_list, led_list2, &mgr->lc15bts_leds.list, list) {
		/* Delete the timer in list */
		if (led_list) {
			LOGP(DTEMP, LOGL_DEBUG,"Delete %s timer entry from list, %d sec + %d usec\n",
				get_value_string(lc15bts_led_strs, led_list->led_timer.idx),
				led_list->led_timer.param.sleep_sec,
				led_list->led_timer.param.sleep_usec);

			/* Delete current timer */
			osmo_timer_del(&led_list->led_timer.timer);
			llist_del(&led_list->list);
			talloc_free(led_list);
		}
	}
	return;
}

static int add_led_timer_entry(struct lc15bts_mgr_instance *mgr, char *cmdstr)
{
	double sec, int_sec, frac_sec;
	struct lc15bts_sleep_time led_param;

	led_param.sleep_sec = 0;
	led_param.sleep_usec = 0;

	if (strstr(cmdstr, "set red") != NULL)
		mgr->lc15bts_leds.led_idx = LC15BTS_LED_RED;
	else if (strstr(cmdstr, "set green") != NULL)
		mgr->lc15bts_leds.led_idx = LC15BTS_LED_GREEN;
	else if (strstr(cmdstr, "set orange") != NULL)
		mgr->lc15bts_leds.led_idx = LC15BTS_LED_ORANGE;
	else if (strstr(cmdstr, "set off") != NULL)
		mgr->lc15bts_leds.led_idx = LC15BTS_LED_OFF;
	else if (strstr(cmdstr, "sleep") != NULL) {
		sec = atof(cmdstr + 6);
		/* split time into integer and fractional of seconds */
		frac_sec = modf(sec, &int_sec) * 1000000.0;
		led_param.sleep_sec = (int)int_sec;
		led_param.sleep_usec = (int)frac_sec;

		if ((mgr->lc15bts_leds.led_idx >= LC15BTS_LED_RED) && (mgr->lc15bts_leds.led_idx < _LC15BTS_LED_MAX)) {
			struct lc15bts_led_timer_list *led_list;

			/* allocate timer entry */
			led_list = talloc_zero(tall_mgr_ctx, struct lc15bts_led_timer_list);
			if (led_list) {
				led_list->led_timer.idx = mgr->lc15bts_leds.led_idx;
				led_list->led_timer.param.sleep_sec = led_param.sleep_sec;
				led_list->led_timer.param.sleep_usec = led_param.sleep_usec;
				llist_add_tail(&led_list->list, &mgr->lc15bts_leds.list);

				LOGP(DTEMP, LOGL_DEBUG,"Add %s timer to list, %d sec + %d usec, total entries=%d\n",
					get_value_string(lc15bts_led_strs, mgr->lc15bts_leds.led_idx),
					led_list->led_timer.param.sleep_sec,
					led_list->led_timer.param.sleep_usec,
					llist_count(&mgr->lc15bts_leds.list));
			}
		}
	} else
		return -1;

	return 0;
}

static int parse_led_pattern(char *pattern, struct lc15bts_mgr_instance *mgr)
{
	char str[1024];
	char *pstr;
	char *sep;
	int rc = 0;

	strcpy(str, pattern);
	pstr = str;
	while ((sep = strsep(&pstr, ";")) != NULL) {
		rc = add_led_timer_entry(mgr, sep);
		if (rc < 0) {
			break;
		}

	}
	return rc;
}

/*** led interface ***/

void led_set(struct lc15bts_mgr_instance *mgr, int pattern_id)
{
	int rc;
	struct lc15bts_led_timer_list *led_list;

	if (pattern_id > BLINK_PATTERN_MAX_ITEM - 1) {
		LOGP(DTEMP, LOGL_ERROR, "Invalid LED pattern : %d. LED pattern must be between %d..%d\n",
				pattern_id,
				BLINK_PATTERN_POWER_ON,
				BLINK_PATTERN_MAX_ITEM - 1);
		return;
	}
	if (pattern_id == mgr->lc15bts_leds.last_pattern_id)
		return;

	mgr->lc15bts_leds.last_pattern_id = pattern_id;

	LOGP(DTEMP, LOGL_NOTICE, "blink pattern command : %d\n", pattern_id);
	LOGP(DTEMP, LOGL_NOTICE, "%s\n", blink_pattern_command[pattern_id]);

	/* Empty existing LED timer in the list */
	delete_led_timer_entries(mgr);

	/* parse LED pattern */
	rc = parse_led_pattern(blink_pattern_command[pattern_id], mgr);
	if (rc < 0) {
		LOGP(DTEMP, LOGL_ERROR,"LED pattern not found or invalid LED pattern\n");
		return;
	}

	/* make sure the timer list is not empty */
	if (llist_empty(&mgr->lc15bts_leds.list))
		return;

	/* Start the first LED timer in the list */
	led_list = llist_first_entry(&mgr->lc15bts_leds.list, struct lc15bts_led_timer_list, list);
	if (led_list) {
		LOGP(DTEMP, LOGL_DEBUG,"Execute timer %s for %d sec + %d usec\n",
			get_value_string(lc15bts_led_strs, led_list->led_timer.idx),
			led_list->led_timer.param.sleep_sec,
			led_list->led_timer.param.sleep_usec);

		led_sleep(mgr, &led_list->led_timer, led_sleep_cb);
	}

}

void select_led_pattern(struct lc15bts_mgr_instance *mgr)
{
	int i;
	uint8_t led[BLINK_PATTERN_MAX_ITEM] = {0};

	/* set normal LED pattern at first */
	led[BLINK_PATTERN_NORMAL] = 1;

	/* check on-board sensors for new LED pattern */
	check_sensor_led_pattern(mgr, led);

	/* check BTS status for new LED pattern */
	check_bts_led_pattern(led);

	/* check by priority */
	for (i = 0; i < sizeof(led_priority)/sizeof(uint8_t); i++) {
		if(led[led_priority[i]] == 1) {
			led_set(mgr, led_priority[i]);
			break;
		}
	}
}
