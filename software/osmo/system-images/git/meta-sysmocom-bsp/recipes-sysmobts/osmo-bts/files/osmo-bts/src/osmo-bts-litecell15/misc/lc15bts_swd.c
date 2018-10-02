/* Systemd service wd notification for Litecell 1.5 BTS management daemon */

/* Copyright (C) 2015 by Yves Godin <support@nuranwireless.com>
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

#include "misc/lc15bts_mgr.h"
#include "misc/lc15bts_swd.h"
#include <osmocom/core/logging.h>

/* Needed for service watchdog notification */
#include <systemd/sd-daemon.h>

/* This is the period used to verify if all events have been registered to be allowed
   to notify the systemd service watchdog
*/
#define SWD_PERIOD 30

static void swd_start(struct lc15bts_mgr_instance *mgr);
static void swd_process(struct lc15bts_mgr_instance *mgr);
static void swd_close(struct lc15bts_mgr_instance *mgr);
static void swd_state_reset(struct lc15bts_mgr_instance *mgr, int reason);
static int swd_run(struct lc15bts_mgr_instance *mgr, int from_loop);
static void swd_loop_run(void *_data);

enum swd_state {
	SWD_INITIAL,
	SWD_IN_PROGRESS,
};

enum swd_result {
        SWD_FAIL_START,
        SWD_FAIL_NOTIFY,
        SWD_SUCCESS,
};

static void swd_start(struct lc15bts_mgr_instance *mgr)
{
	swd_process(mgr);
}

static void swd_process(struct lc15bts_mgr_instance *mgr)
{
	int rc = 0, notify = 0;

	/* Did we get all needed conditions ? */
	if (mgr->swd.swd_eventmasks == mgr->swd.swd_events) {
	        /* Ping systemd service wd if enabled */
		rc = sd_notify(0, "WATCHDOG=1");
		LOGP(DSWD, LOGL_NOTICE, "Watchdog notification attempt\n");
		notify = 1;
	}
	else {
		LOGP(DSWD, LOGL_NOTICE, "Missing watchdog events: e:0x%016llx,m:0x%016llx\n",mgr->swd.swd_events,mgr->swd.swd_eventmasks);
	}

	if (rc < 0) {
		LOGP(DSWD, LOGL_ERROR,
			"Failed to notify system service watchdog: %d\n", rc);
		swd_state_reset(mgr, SWD_FAIL_NOTIFY);
		return;
	}
	else {
		/* Did we notified the watchdog? */
		if (notify) {
			mgr->swd.swd_events = 0;
			/* Makes sure we really cleared it in case any event was notified at this same moment (it would be lost) */
			if (mgr->swd.swd_events != 0)
				mgr->swd.swd_events = 0;
		}
	}

	swd_state_reset(mgr, SWD_SUCCESS);
	return;
}

static void swd_close(struct lc15bts_mgr_instance *mgr)
{
}

static void swd_state_reset(struct lc15bts_mgr_instance *mgr, int outcome)
{
	if (mgr->swd.swd_from_loop) {
                mgr->swd.swd_timeout.data = mgr;
                mgr->swd.swd_timeout.cb = swd_loop_run;
                osmo_timer_schedule(&mgr->swd.swd_timeout, SWD_PERIOD, 0);
        }

        mgr->swd.state = SWD_INITIAL;
	swd_close(mgr);
}

static int swd_run(struct lc15bts_mgr_instance *mgr, int from_loop)
{
	if (mgr->swd.state != SWD_INITIAL) {
		LOGP(DSWD, LOGL_ERROR, "Swd is already in progress.\n");
		return -1;
	}

	mgr->swd.swd_from_loop = from_loop;

	/* From now on everything will be handled from the failure */
	mgr->swd.state = SWD_IN_PROGRESS;
	swd_start(mgr);
	return 0;
}

static void swd_loop_run(void *_data)
{
        int rc;
        struct lc15bts_mgr_instance *mgr = _data;

        LOGP(DSWD, LOGL_NOTICE, "Going to check for watchdog notification.\n");
        rc = swd_run(mgr, 1);
        if (rc != 0) {
                swd_state_reset(mgr, SWD_FAIL_START);
	}
}

/* 'swd_num_events' configures the number of events to be monitored before notifying the
   systemd service watchdog. It must be in the range of [1,64]. Events are notified
   through the function 'lc15bts_swd_event'
*/
int lc15bts_swd_init(struct lc15bts_mgr_instance *mgr, int swd_num_events)
{
	/* Checks for a valid number of events to validate */
	if (swd_num_events < 1 || swd_num_events > 64)
		return(-1);

	mgr->swd.state = SWD_INITIAL;
	mgr->swd.swd_timeout.data = mgr;
	mgr->swd.swd_timeout.cb = swd_loop_run;
	osmo_timer_schedule(&mgr->swd.swd_timeout, 0, 0);

	if (swd_num_events == 64){
		mgr->swd.swd_eventmasks = 0xffffffffffffffffULL;
	}
	else {
		mgr->swd.swd_eventmasks = ((1ULL << swd_num_events) - 1);
	}
	mgr->swd.swd_events = 0;
	mgr->swd.num_events = swd_num_events;

	return 0;
}

/* Notifies that the specified event 'swd_event' happened correctly;
   the value must be in the range of [0,'swd_num_events'[ (see lc15bts_swd_init).
   For example, if 'swd_num_events' was 64, 'swd_event' events are numbered 0 to 63.
   WARNING: if this function can be used from multiple threads at the same time,
   it must be protected with a kind of mutex to avoid loosing event notification.
*/
int lc15bts_swd_event(struct lc15bts_mgr_instance *mgr, enum mgr_swd_events swd_event)
{
	/* Checks for a valid specified event (smaller than max possible) */
	if ((int)(swd_event) < 0 || (int)(swd_event) >= mgr->swd.num_events)
		return(-1);

	mgr->swd.swd_events = mgr->swd.swd_events | ((unsigned long long int)(1) << (int)(swd_event));

	/* !!! Uncomment following line to debug events notification */
	LOGP(DSWD, LOGL_DEBUG,"Swd event notified: %d\n", (int)(swd_event));

	return 0;
}
