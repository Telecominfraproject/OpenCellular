/* OCXO calibration control for OC-2G BTS management daemon */

/* Copyright (C) 2015 by Yves Godin <support@nuranwireless.com>
 * 
 * Based on sysmoBTS:
 *     sysmobts_mgr_calib.c
 *     (C) 2014,2015 by Holger Hans Peter Freyther
 *     (C) 2014 by Harald Welte for the IPA code from the oml router
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

#include "misc/oc2gbts_mgr.h"
#include "misc/oc2gbts_misc.h"
#include "misc/oc2gbts_clock.h"
#include "misc/oc2gbts_swd.h"
#include "misc/oc2gbts_par.h"
#include "misc/oc2gbts_led.h"
#include "osmo-bts/msg_utils.h"

#include <osmocom/core/logging.h>
#include <osmocom/core/select.h>

#include <osmocom/ctrl/control_cmd.h>
#include <osmocom/ctrl/ports.h>

#include <osmocom/gsm/ipa.h>
#include <osmocom/gsm/protocol/ipaccess.h>

#include <osmocom/abis/abis.h>
#include <osmocom/abis/e1_input.h>
#include <osmocom/abis/ipa.h>

#include <time.h>
#include <sys/sysinfo.h>
#include <errno.h>

static void calib_adjust(struct oc2gbts_mgr_instance *mgr);
static void calib_state_reset(struct oc2gbts_mgr_instance *mgr, int reason);
static void calib_loop_run(void *_data);

static int ocxodac_saved_value = -1;

enum calib_state {
	CALIB_INITIAL,
	CALIB_IN_PROGRESS,
	CALIB_GPS_WAIT_FOR_FIX,
};

enum calib_result {
        CALIB_FAIL_START,
        CALIB_FAIL_GPSFIX,
        CALIB_FAIL_CLKERR,
        CALIB_FAIL_OCXODAC,
        CALIB_SUCCESS,
};

static int oc2gbts_par_get_uptime(void *ctx, int *ret)
{
	char *fpath;
	FILE *fp;
	int rc;

	fpath = talloc_asprintf(ctx, "%s", UPTIME_TMP_PATH);
	if (!fpath)
		return NULL;

	fp = fopen(fpath, "r");
	if (!fp)
		fprintf(stderr, "Failed to open %s due to '%s' error\n", fpath, strerror(errno));

	talloc_free(fpath);

	if (fp == NULL) {
		return -errno;
	}

	rc = fscanf(fp, "%d", ret);
	if (rc != 1) {
		fclose(fp);
		return -EIO;
	}
	fclose(fp);

	return 0;
}

static int oc2gbts_par_set_uptime(void *ctx, int val)
{
	char *fpath;
	FILE *fp;
	int rc;

	fpath = talloc_asprintf(ctx, "%s", UPTIME_TMP_PATH);
	if (!fpath)
		return NULL;

	fp = fopen(fpath, "w");
	if (!fp)
		fprintf(stderr, "Failed to open %s due to '%s' error\n", fpath, strerror(errno));

	talloc_free(fpath);

	if (fp == NULL) {
		return -errno;
	}

	rc = fprintf(fp, "%d", val);
	if (rc < 0) {
		fclose(fp);
		return -EIO;
	}
	fsync(fp);
	fclose(fp);

	return 0;
}

static void mgr_gps_close(struct oc2gbts_mgr_instance *mgr)
{
	if (!mgr->gps.gps_open)
		return;

	osmo_timer_del(&mgr->gps.fix_timeout);

	osmo_fd_unregister(&mgr->gps.gpsfd);
	gps_close(&mgr->gps.gpsdata);
	memset(&mgr->gps.gpsdata, 0, sizeof(mgr->gps.gpsdata));
	mgr->gps.gps_open = 0;
}

static void mgr_gps_checkfix(struct oc2gbts_mgr_instance *mgr)
{
	struct gps_data_t *data = &mgr->gps.gpsdata;

	/* No 3D fix yet */
	if (data->fix.mode < MODE_3D) {
		LOGP(DCALIB, LOGL_DEBUG, "Fix mode not enough: %d\n",
			data->fix.mode);
		return;
	}

	/* Satellite used checking */
	if (data->satellites_used < 1) {
		LOGP(DCALIB, LOGL_DEBUG, "Not enough satellites used: %d\n",
			data->satellites_used);
		return;
	}

	mgr->gps.gps_fix_now = (time_t) data->fix.time;
	LOGP(DCALIB, LOGL_INFO, "Got a GPS fix, satellites used: %d, timestamp: %ld\n",
			data->satellites_used, mgr->gps.gps_fix_now);
	osmo_timer_del(&mgr->gps.fix_timeout);
	mgr_gps_close(mgr);
}

static int mgr_gps_read(struct osmo_fd *fd, unsigned int what)
{
	int rc;
	struct oc2gbts_mgr_instance *mgr = fd->data;

	rc = gps_read(&mgr->gps.gpsdata);
	if (rc == -1) {
		LOGP(DCALIB, LOGL_ERROR, "gpsd vanished during read.\n");
		calib_state_reset(mgr, CALIB_FAIL_GPSFIX);
		return -1;
	}

	if (rc > 0)
		mgr_gps_checkfix(mgr);
	return 0;
}

static void mgr_gps_fix_timeout(void *_data)
{
	struct oc2gbts_mgr_instance *mgr = _data;

	LOGP(DCALIB, LOGL_ERROR, "Failed to acquire GPS fix.\n");
	mgr_gps_close(mgr);
}

static void mgr_gps_open(struct oc2gbts_mgr_instance *mgr)
{
	int rc;

	if (mgr->gps.gps_open)
		return;

	rc = gps_open("localhost", DEFAULT_GPSD_PORT, &mgr->gps.gpsdata);
	if (rc != 0) {
		LOGP(DCALIB, LOGL_ERROR, "Failed to connect to GPS %d\n", rc);
		calib_state_reset(mgr, CALIB_FAIL_GPSFIX);
		return;
	}

	mgr->gps.gps_open = 1;
	gps_stream(&mgr->gps.gpsdata, WATCH_ENABLE, NULL);

	mgr->gps.gpsfd.data = mgr;
	mgr->gps.gpsfd.cb = mgr_gps_read;
	mgr->gps.gpsfd.when = BSC_FD_READ | BSC_FD_EXCEPT;
	mgr->gps.gpsfd.fd = mgr->gps.gpsdata.gps_fd;
	if (osmo_fd_register(&mgr->gps.gpsfd) < 0) {
		LOGP(DCALIB, LOGL_ERROR, "Failed to register GPSD fd\n");
		calib_state_reset(mgr, CALIB_FAIL_GPSFIX);
	}

	mgr->calib.state = CALIB_GPS_WAIT_FOR_FIX;
	mgr->gps.fix_timeout.data = mgr;
	mgr->gps.fix_timeout.cb = mgr_gps_fix_timeout;
	osmo_timer_schedule(&mgr->gps.fix_timeout, 60, 0);
	LOGP(DCALIB, LOGL_INFO, "Opened the GPSD connection waiting for fix: %d\n",
		mgr->gps.gpsfd.fd);
}

/* OC2G CTRL interface related functions */
static void send_ctrl_cmd(struct oc2gbts_mgr_instance *mgr, struct msgb *msg)
{
	ipa_prepend_header_ext(msg, IPAC_PROTO_EXT_CTRL);
	ipa_prepend_header(msg, IPAC_PROTO_OSMO);
	ipa_client_conn_send(mgr->oc2gbts_ctrl.bts_conn, msg);
}

static void send_set_ctrl_cmd_int(struct oc2gbts_mgr_instance *mgr, const char *key, const int val)
{
	struct msgb *msg;
	int ret;

	msg = msgb_alloc_headroom(1024, 128, "CTRL SET");
	ret = snprintf((char *) msg->data, 4096, "SET %u %s %d",
			mgr->oc2gbts_ctrl.last_seqno++, key, val);
	msg->l2h = msgb_put(msg, ret);
	return send_ctrl_cmd(mgr, msg);
}

static void send_set_ctrl_cmd(struct oc2gbts_mgr_instance *mgr, const char *key, const int val, const char *text)
{
	struct msgb *msg;
	int ret;

	msg = msgb_alloc_headroom(1024, 128, "CTRL SET");
	ret = snprintf((char *) msg->data, 4096, "SET %u %s %d, %s",
			mgr->oc2gbts_ctrl.last_seqno++, key, val, text);
	msg->l2h = msgb_put(msg, ret);
	return send_ctrl_cmd(mgr, msg);
}

static void calib_start(struct oc2gbts_mgr_instance *mgr)
{
	int rc;

	rc = oc2gbts_clock_err_open();
	if (rc != 0) {
		LOGP(DCALIB, LOGL_ERROR, "Failed to open clock error module %d\n", rc);
		calib_state_reset(mgr, CALIB_FAIL_CLKERR);
		return;
	}

	rc = oc2gbts_clock_dac_open();
	if (rc != 0) {
		LOGP(DCALIB, LOGL_ERROR, "Failed to open OCXO dac module %d\n", rc);
		calib_state_reset(mgr, CALIB_FAIL_OCXODAC);
		return;
	}

	calib_adjust(mgr);
}
static int get_uptime(int *uptime)
{
	struct sysinfo s_info;
	int rc;
	rc = sysinfo(&s_info);
	if(!rc)
		*uptime = s_info.uptime /(60 * 60);

	return rc;
}

static void calib_adjust(struct oc2gbts_mgr_instance *mgr)
{
	int rc;
	int fault;
	int error_ppt;
	int accuracy_ppq;
	int interval_sec;
	int dac_value;
	int new_dac_value;
	int dac_correction;
	int now = 0;

	/* Get GPS time via GPSD */
	mgr_gps_open(mgr);

	rc = oc2gbts_clock_err_get(&fault, &error_ppt, 
			&accuracy_ppq, &interval_sec);
	if (rc < 0) {
		LOGP(DCALIB, LOGL_ERROR, 
			"Failed to get clock error measurement %d\n", rc);
		calib_state_reset(mgr, CALIB_FAIL_CLKERR);
		return;
	}

	/* get current up time */
	rc = get_uptime(&now);
	if (rc < 0)
		LOGP(DTEMP, LOGL_ERROR, "Unable to read up time hours: %d (%s)\n", rc, strerror(errno));

	/* read last up time */
	rc = oc2gbts_par_get_uptime(tall_mgr_ctx, &mgr->gps.last_update);
	if (rc < 0)
		LOGP(DCALIB, LOGL_NOTICE, "Last GPS 3D fix can not read (%d). Last GPS 3D fix sets to zero\n", rc);

	if (fault) {
		LOGP(DCALIB, LOGL_NOTICE, "GPS has no fix, warn_flags=0x%08x, last=%d, now=%d\n",
					mgr->oc2gbts_ctrl.warn_flags, mgr->gps.last_update, now);
		if (now >= mgr->gps.last_update + mgr->gps.gps_fix_limit.thresh_warn_max * 24) {
			if (!(mgr->oc2gbts_ctrl.warn_flags & S_MGR_GPS_FIX_WARN_ALARM)) {
				mgr->oc2gbts_ctrl.warn_flags |= S_MGR_GPS_FIX_WARN_ALARM;
				oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_GPS_FIX_FAIL, "oc2g-oml-alert", "GPS 3D fix has been lost");

				LOGP(DCALIB, LOGL_NOTICE, "GPS has no fix since the last verification, warn_flags=0x%08x, last=%d, now=%d\n",
									mgr->oc2gbts_ctrl.warn_flags, mgr->gps.last_update, now);

				/* schedule LED pattern for GPS fix lost */
				mgr->alarms.gps_fix_lost = 1;
				/* update LED pattern */
				select_led_pattern(mgr);
			}
		} else {
			/* read from last GPS 3D fix timestamp */
			rc = oc2gbts_par_get_gps_fix(tall_mgr_ctx, &mgr->gps.last_gps_fix);
			if (rc < 0)
				LOGP(DCALIB, LOGL_NOTICE, "Last GPS 3D fix timestamp can not read (%d)\n", rc);

			if (difftime(mgr->gps.gps_fix_now, mgr->gps.last_gps_fix) > mgr->gps.gps_fix_limit.thresh_warn_max * 24 * 60 * 60) {
				if (!(mgr->oc2gbts_ctrl.warn_flags & S_MGR_GPS_FIX_WARN_ALARM)) {
					mgr->oc2gbts_ctrl.warn_flags |= S_MGR_GPS_FIX_WARN_ALARM;
					oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_GPS_FIX_FAIL, "oc2g-oml-alert", "GPS 3D fix has been lost");

					LOGP(DCALIB, LOGL_NOTICE, "GPS has no fix since the last known GPS fix, warn_flags=0x%08x, gps_last=%ld, gps_now=%ld\n",
						mgr->oc2gbts_ctrl.warn_flags, mgr->gps.last_gps_fix, mgr->gps.gps_fix_now);

					/* schedule LED pattern for GPS fix lost */
					mgr->alarms.gps_fix_lost = 1;
					/* update LED pattern */
					select_led_pattern(mgr);
				}
			}
		}

		rc = oc2gbts_clock_err_reset();
		if (rc < 0) {
			LOGP(DCALIB, LOGL_ERROR,
				"Failed to reset clock error module %d\n", rc);
			calib_state_reset(mgr, CALIB_FAIL_CLKERR);
			return;
		}

		calib_state_reset(mgr, CALIB_FAIL_GPSFIX);
		return;
	}

	if (!interval_sec) {
		LOGP(DCALIB, LOGL_INFO, "Skipping this iteration, no integration time\n");
		calib_state_reset(mgr, CALIB_SUCCESS);
		return;
	}

	/* We got GPS 3D fix */
	LOGP(DCALIB, LOGL_DEBUG, "Got GPS 3D fix warn_flags=0x%08x, uptime_last=%d, uptime_now=%d, gps_last=%ld, gps_now=%ld\n",
				mgr->oc2gbts_ctrl.warn_flags, mgr->gps.last_update, now, mgr->gps.last_gps_fix, mgr->gps.gps_fix_now);

	if (mgr->oc2gbts_ctrl.warn_flags & S_MGR_GPS_FIX_WARN_ALARM) {
		/* Store GPS fix as soon as we send ceased alarm */
		LOGP(DCALIB, LOGL_NOTICE, "Store GPS fix as soon as we send ceased alarm last=%ld, now=%ld\n",
				mgr->gps.last_gps_fix , mgr->gps.gps_fix_now);
		rc = oc2gbts_par_set_gps_fix(tall_mgr_ctx, mgr->gps.gps_fix_now);
		if (rc < 0)
			LOGP(DCALIB, LOGL_ERROR, "Failed to store GPS 3D fix to storage %d\n", rc);

		/* Store last up time */
		rc = oc2gbts_par_set_uptime(tall_mgr_ctx, now);
		if (rc < 0)
			LOGP(DCALIB, LOGL_ERROR, "Failed to store uptime to storage %d\n", rc);

		mgr->gps.last_update = now;

		/* schedule LED pattern for GPS fix resume */
		mgr->alarms.gps_fix_lost = 0;
		/* update LED pattern */
		select_led_pattern(mgr);
		/* send ceased alarm if possible */
		oc2gbts_mgr_dispatch_alarm(mgr, NM_EVT_CAUSE_WARN_GPS_FIX_FAIL, "oc2g-oml-ceased", "GPS 3D fix has been lost");
		mgr->oc2gbts_ctrl.warn_flags &= ~S_MGR_GPS_FIX_WARN_ALARM;

	}
	/* Store GPS fix at every hour */
	if (now > mgr->gps.last_update) {
		/* Store GPS fix every 60 minutes */
		LOGP(DCALIB, LOGL_INFO, "Store GPS fix every hour last=%ld, now=%ld\n",
				mgr->gps.last_gps_fix , mgr->gps.gps_fix_now);
		rc = oc2gbts_par_set_gps_fix(tall_mgr_ctx, mgr->gps.gps_fix_now);
		if (rc < 0)
			LOGP(DCALIB, LOGL_ERROR, "Failed to store GPS 3D fix to storage %d\n", rc);

		/* Store last up time every 60 minutes */
		rc = oc2gbts_par_set_uptime(tall_mgr_ctx, now);
		if (rc < 0)
			LOGP(DCALIB, LOGL_ERROR, "Failed to store uptime to storage %d\n", rc);

		/* update last uptime */
		mgr->gps.last_update = now;
	}

	rc = oc2gbts_clock_dac_get(&dac_value);
	if (rc < 0) {
		LOGP(DCALIB, LOGL_ERROR, 
			"Failed to get OCXO dac value %d\n", rc);
		calib_state_reset(mgr, CALIB_FAIL_OCXODAC);
		return;
	}

	/* Set OCXO initial dac value */
	if (ocxodac_saved_value < 0)
		ocxodac_saved_value = dac_value;

	LOGP(DCALIB, LOGL_INFO,
		"Calibration ERR(%f PPB) ACC(%f PPB) INT(%d) DAC(%d)\n",
		error_ppt / 1000., accuracy_ppq / 1000000., interval_sec, dac_value);

	/* 1 unit of correction equal about 0.5 - 1 PPB correction */
	dac_correction = (int)(-error_ppt * 0.0015);
	new_dac_value = dac_value + dac_correction;

	if (new_dac_value > 4095)
		new_dac_value = 4095;
	else if (new_dac_value < 0)
		new_dac_value = 0;

	/* We have a fix, make sure the measured error is
	meaningful (10 times the accuracy) */
	if ((new_dac_value != dac_value) && ((100l * abs(error_ppt)) > accuracy_ppq)) {

		LOGP(DCALIB, LOGL_INFO,
			"Going to apply %d as new clock setting.\n",
			new_dac_value);

		rc = oc2gbts_clock_dac_set(new_dac_value);
		if (rc < 0) {
			LOGP(DCALIB, LOGL_ERROR,
				"Failed to set OCXO dac value %d\n", rc);
			calib_state_reset(mgr, CALIB_FAIL_OCXODAC);
			return;
		}
		rc = oc2gbts_clock_err_reset();
		if (rc < 0) {
			LOGP(DCALIB, LOGL_ERROR,
				"Failed to reset clock error module %d\n", rc);
			calib_state_reset(mgr, CALIB_FAIL_CLKERR);
			return;
		}
	}
	/* New conditions to store DAC value:
	 *  - Resolution accuracy less or equal than 0.01PPB (or 10000 PPQ)
	 *  - Error less or equal than 2PPB (or 2000PPT)
	 *  - Solution different than the last one	*/
	else if (accuracy_ppq <= 10000) {
		if((dac_value != ocxodac_saved_value) && (abs(error_ppt) < 2000)) {
			LOGP(DCALIB, LOGL_INFO, "Saving OCXO DAC value to memory... val = %d\n", dac_value);
			rc = oc2gbts_clock_dac_save();
			if (rc < 0) {
				LOGP(DCALIB, LOGL_ERROR,
					"Failed to save OCXO dac value %d\n", rc);
				calib_state_reset(mgr, CALIB_FAIL_OCXODAC);
			} else {
				ocxodac_saved_value = dac_value;
			}
		}

		rc = oc2gbts_clock_err_reset();
		if (rc < 0) {
			LOGP(DCALIB, LOGL_ERROR,
				"Failed to reset clock error module %d\n", rc);
			calib_state_reset(mgr, CALIB_FAIL_CLKERR);
		}
	}

	calib_state_reset(mgr, CALIB_SUCCESS);
	return;
}

static void calib_close(struct oc2gbts_mgr_instance *mgr)
{
	oc2gbts_clock_err_close();
	oc2gbts_clock_dac_close();
}

static void calib_state_reset(struct oc2gbts_mgr_instance *mgr, int outcome)
{
	if (mgr->calib.calib_from_loop) {
		/*
		 * In case of success calibrate in two hours again
		 * and in case of a failure in some minutes.
		 *
		 * TODO NTQ: Select timeout based on last error and accuracy
		 */
		int timeout = 60;
		//int timeout = 2 * 60 * 60;
		//if (outcome != CALIB_SUCESS) }
		//	timeout = 5 * 60;
		//}

                mgr->calib.calib_timeout.data = mgr;
                mgr->calib.calib_timeout.cb = calib_loop_run;
                osmo_timer_schedule(&mgr->calib.calib_timeout, timeout, 0);
		/* TODO: do we want to notify if we got a calibration error, like no gps fix? */
		oc2gbts_swd_event(mgr, SWD_CHECK_CALIB);
        }

        mgr->calib.state = CALIB_INITIAL;
	calib_close(mgr);
}

static int calib_run(struct oc2gbts_mgr_instance *mgr, int from_loop)
{
	if (mgr->calib.state != CALIB_INITIAL) {
		LOGP(DCALIB, LOGL_ERROR, "Calib is already in progress.\n");
		return -1;
	}

	/* Validates if we have a bts connection */
	if (mgr->oc2gbts_ctrl.is_up) {
		LOGP(DCALIB, LOGL_DEBUG, "Bts connection is up.\n");
		oc2gbts_swd_event(mgr, SWD_CHECK_BTS_CONNECTION);
	}

	mgr->calib.calib_from_loop = from_loop;

	/* From now on everything will be handled from the failure */
	mgr->calib.state = CALIB_IN_PROGRESS;
	calib_start(mgr);
	return 0;
}

static void calib_loop_run(void *_data)
{
        int rc;
        struct oc2gbts_mgr_instance *mgr = _data;

        LOGP(DCALIB, LOGL_INFO, "Going to calibrate the system.\n");
        rc = calib_run(mgr, 1);
        if (rc != 0) {
                calib_state_reset(mgr, CALIB_FAIL_START);
	}
}

int oc2gbts_mgr_calib_run(struct oc2gbts_mgr_instance *mgr)
{
        return calib_run(mgr, 0);
}

static void schedule_bts_connect(struct oc2gbts_mgr_instance *mgr)
{
	DEBUGP(DLCTRL, "Scheduling BTS connect\n");
	osmo_timer_schedule(&mgr->oc2gbts_ctrl.recon_timer, 1, 0);
}

/* link to BSC has gone up or down */
static void bts_updown_cb(struct ipa_client_conn *link, int up)
{
	struct oc2gbts_mgr_instance *mgr = link->data;

	LOGP(DLCTRL, LOGL_INFO, "BTS connection %s\n", up ? "up" : "down");

	if (up) {
		mgr->oc2gbts_ctrl.is_up = 1;
		mgr->oc2gbts_ctrl.last_seqno = 0;
		/* handle any pending alarm */
		handle_alert_actions(mgr);
		handle_warn_actions(mgr);
	} else {
		mgr->oc2gbts_ctrl.is_up = 0;
		schedule_bts_connect(mgr);
	}

}

/* BTS re-connect timer call-back */
static void bts_recon_timer_cb(void *data)
{
	int rc;
	struct oc2gbts_mgr_instance *mgr = data;

	/* update LED pattern */
	select_led_pattern(mgr);

	/* The connection failures are to be expected during boot */
	mgr->oc2gbts_ctrl.bts_conn->ofd->when |= BSC_FD_WRITE;
	rc = ipa_client_conn_open(mgr->oc2gbts_ctrl.bts_conn);
	if (rc < 0) {
		LOGP(DLCTRL, LOGL_NOTICE, "Failed to connect to BTS.\n");
		schedule_bts_connect(mgr);
	}
}

static void oc2gbts_handle_ctrl(struct oc2gbts_mgr_instance *mgr, struct msgb *msg)
{
	struct ctrl_cmd *cmd = ctrl_cmd_parse(tall_mgr_ctx, msg);
	int cause = atoi(cmd->reply);

	if (!cmd) {
		LOGP(DCALIB, LOGL_ERROR, "Failed to parse command/response\n");
		return;
	}

	switch (cmd->type) {
	case CTRL_TYPE_GET_REPLY:
		LOGP(DCALIB, LOGL_INFO, "Got GET_REPLY from BTS cause=0x%x\n", cause);
		break;
	case CTRL_TYPE_SET_REPLY:
		LOGP(DCALIB, LOGL_INFO, "Got SET_REPLY from BTS cause=0x%x\n", cause);
		break;
	default:
		LOGP(DCALIB, LOGL_ERROR,
			"Unhandled CTRL response: %d. Resetting state\n",
			cmd->type);
		break;
	}

	talloc_free(cmd);
	return;
}

static int bts_read_cb(struct ipa_client_conn *link, struct msgb *msg)
{
	int rc;
	struct ipaccess_head *hh = (struct ipaccess_head *) msgb_l1(msg);
	struct ipaccess_head_ext *hh_ext;

	LOGP(DLCTRL, LOGL_DEBUG, "Received data from BTS: %s\n",
		osmo_hexdump(msgb_data(msg), msgb_length(msg)));

	/* regular message handling */
	rc = msg_verify_ipa_structure(msg);
	if (rc < 0) {
		LOGP(DCALIB, LOGL_ERROR,
			"Invalid IPA message from BTS (rc=%d)\n", rc);
		goto err;
	}

	switch (hh->proto) {
	case IPAC_PROTO_OSMO:
		hh_ext = (struct ipaccess_head_ext *) hh->data;
		switch (hh_ext->proto) {
		case IPAC_PROTO_EXT_CTRL:
			oc2gbts_handle_ctrl(link->data, msg);
			break;
		default:
			LOGP(DCALIB, LOGL_NOTICE,
				"Unhandled osmo ID %u from BTS\n", hh_ext->proto);
		};
		msgb_free(msg);
		break;
	default:
		LOGP(DCALIB, LOGL_NOTICE,
			 "Unhandled stream ID %u from BTS\n", hh->proto);
		msgb_free(msg);
		break;
	}
	return 0;
err:
	msgb_free(msg);
	return -1;
}

int oc2gbts_mgr_calib_init(struct oc2gbts_mgr_instance *mgr)
{
	int rc;

	/* initialize last uptime */
	mgr->gps.last_update = 0;
	rc = oc2gbts_par_set_uptime(tall_mgr_ctx, mgr->gps.last_update);
	if (rc < 0)
		LOGP(DCALIB, LOGL_ERROR, "Failed to store uptime to storage %d\n", rc);

	/* get last GPS 3D fix timestamp */
	mgr->gps.last_gps_fix = 0;
	rc = oc2gbts_par_get_gps_fix(tall_mgr_ctx, &mgr->gps.last_gps_fix);
	if (rc < 0) {
		LOGP(DCALIB, LOGL_ERROR, "Failed to get last GPS 3D fix timestamp from storage. Create it anyway %d\n", rc);
		rc = oc2gbts_par_set_gps_fix(tall_mgr_ctx, mgr->gps.last_gps_fix);
		if (rc < 0)
			LOGP(DCALIB, LOGL_ERROR, "Failed to store initial GPS fix to storage %d\n", rc);
	}

	mgr->calib.state = CALIB_INITIAL;
	mgr->calib.calib_timeout.data = mgr;
	mgr->calib.calib_timeout.cb = calib_loop_run;
	osmo_timer_schedule(&mgr->calib.calib_timeout, 0, 0);

	return rc;
}

int oc2gbts_mgr_control_init(struct oc2gbts_mgr_instance *mgr)
{
	mgr->oc2gbts_ctrl.bts_conn = ipa_client_conn_create(tall_mgr_ctx, NULL, 0,
					"127.0.0.1", OSMO_CTRL_PORT_BTS,
					bts_updown_cb, bts_read_cb,
					NULL, mgr);
	if (!mgr->oc2gbts_ctrl.bts_conn) {
		LOGP(DLCTRL, LOGL_ERROR, "Failed to create IPA connection to BTS\n");
		return -1;
	}

	mgr->oc2gbts_ctrl.recon_timer.cb = bts_recon_timer_cb;
	mgr->oc2gbts_ctrl.recon_timer.data = mgr;
	schedule_bts_connect(mgr);

	return 0;
}

void oc2gbts_mgr_dispatch_alarm(struct oc2gbts_mgr_instance *mgr, const int cause, const char *key, const char *text)
{
	/* Make sure the control link is ready before sending alarm */
	if (mgr->oc2gbts_ctrl.bts_conn->state != IPA_CLIENT_LINK_STATE_CONNECTED) {
		LOGP(DLCTRL, LOGL_NOTICE, "MGR losts connection to BTS.\n");
		LOGP(DLCTRL, LOGL_NOTICE, "MGR drops an alert cause=0x%x, text=%s to BTS\n", cause, text);
		return;
	}

	LOGP(DLCTRL, LOGL_DEBUG, "MGR sends an alert cause=0x%x, text=%s to BTS\n", cause, text);
	send_set_ctrl_cmd(mgr, key, cause, text);
	return;
}


