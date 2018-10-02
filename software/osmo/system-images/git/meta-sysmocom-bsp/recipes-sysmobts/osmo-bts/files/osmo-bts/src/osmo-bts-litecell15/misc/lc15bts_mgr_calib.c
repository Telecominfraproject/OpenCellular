/* OCXO calibration control for Litecell 1.5 BTS management daemon */

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

#include "misc/lc15bts_mgr.h"
#include "misc/lc15bts_misc.h"
#include "misc/lc15bts_clock.h"
#include "misc/lc15bts_swd.h"
#include "misc/lc15bts_par.h"
#include "misc/lc15bts_led.h"
#include "osmo-bts/msg_utils.h"

#include <osmocom/core/logging.h>
#include <osmocom/core/select.h>

#include <osmocom/ctrl/control_cmd.h>

#include <osmocom/gsm/ipa.h>
#include <osmocom/gsm/protocol/ipaccess.h>

#include <osmocom/abis/abis.h>
#include <osmocom/abis/e1_input.h>
#include <osmocom/abis/ipa.h>

#include <time.h>

static void calib_adjust(struct lc15bts_mgr_instance *mgr);
static void calib_state_reset(struct lc15bts_mgr_instance *mgr, int reason);
static void calib_loop_run(void *_data);

static int ocxodac_saved_value = -1;

enum calib_state {
	CALIB_INITIAL,
	CALIB_IN_PROGRESS,
};

enum calib_result {
        CALIB_FAIL_START,
        CALIB_FAIL_GPSFIX,
        CALIB_FAIL_CLKERR,
        CALIB_FAIL_OCXODAC,
        CALIB_SUCCESS,
};

static void calib_start(struct lc15bts_mgr_instance *mgr)
{
	int rc;

	rc = lc15bts_clock_err_open();
	if (rc != 0) {
		LOGP(DCALIB, LOGL_ERROR, "Failed to open clock error module %d\n", rc);
		calib_state_reset(mgr, CALIB_FAIL_CLKERR);
		return;
	}

	rc = lc15bts_clock_dac_open();
	if (rc != 0) {
		LOGP(DCALIB, LOGL_ERROR, "Failed to open OCXO dac module %d\n", rc);
		calib_state_reset(mgr, CALIB_FAIL_OCXODAC);
		return;
	}

	calib_adjust(mgr);
}

static void calib_adjust(struct lc15bts_mgr_instance *mgr)
{
	int rc;
	int fault;
	int error_ppt;
	int accuracy_ppq;
	int interval_sec;
	int dac_value;
	int new_dac_value;
	int dac_correction;
	time_t now;
	time_t last_gps_fix;

	rc = lc15bts_clock_err_get(&fault, &error_ppt, 
			&accuracy_ppq, &interval_sec);
	if (rc < 0) {
		LOGP(DCALIB, LOGL_ERROR, 
			"Failed to get clock error measurement %d\n", rc);
		calib_state_reset(mgr, CALIB_FAIL_CLKERR);
		return;
	}

	/* get current time */
	now = time(NULL);

	/* first time after start of manager program */
	if (mgr->gps.last_update == 0)
		mgr->gps.last_update = now;

	/* read last GPS 3D fix from storage */
	rc = lc15bts_par_get_gps_fix(&last_gps_fix);
	if (rc < 0) {
		LOGP(DCALIB, LOGL_NOTICE, "Last GPS 3D fix can not read (%d). Last GPS 3D fix sets to zero\n", rc);
		last_gps_fix = 0;
	}

	if (fault) {
		LOGP(DCALIB, LOGL_NOTICE, "GPS has no fix\n");
		calib_state_reset(mgr, CALIB_FAIL_GPSFIX);
		return;
	}

	/* We got GPS 3D fix */
	LOGP(DCALIB, LOGL_DEBUG, "Got GPS 3D fix warn_flags=0x%08x, last=%lld, now=%lld\n",
			mgr->lc15bts_ctrl.warn_flags,
			(long long)last_gps_fix,
			(long long)now);

	rc = lc15bts_clock_dac_get(&dac_value);
	if (rc < 0) {
		LOGP(DCALIB, LOGL_ERROR, 
			"Failed to get OCXO dac value %d\n", rc);
		calib_state_reset(mgr, CALIB_FAIL_OCXODAC);
		return;
	}

	/* Set OCXO initial dac value */
	if (ocxodac_saved_value < 0)
		ocxodac_saved_value = dac_value;

	LOGP(DCALIB, LOGL_NOTICE,
		"Calibration ERR(%f PPB) ACC(%f PPB) INT(%d) DAC(%d)\n",
		error_ppt / 1000., accuracy_ppq / 1000000., interval_sec, dac_value);

	/* Need integration time to correct */
	if (interval_sec) {
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

			LOGP(DCALIB, LOGL_NOTICE,
				"Going to apply %d as new clock setting.\n",
				new_dac_value);

			rc = lc15bts_clock_dac_set(new_dac_value);
			if (rc < 0) {
				LOGP(DCALIB, LOGL_ERROR,
					"Failed to set OCXO dac value %d\n", rc);
				calib_state_reset(mgr, CALIB_FAIL_OCXODAC);
				return;
			}
			rc = lc15bts_clock_err_reset();
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
				LOGP(DCALIB, LOGL_NOTICE, "Saving OCXO DAC value to memory... val = %d\n", dac_value);
				rc = lc15bts_clock_dac_save();
				if (rc < 0) {
					LOGP(DCALIB, LOGL_ERROR,
						"Failed to save OCXO dac value %d\n", rc);
					calib_state_reset(mgr, CALIB_FAIL_OCXODAC);
				} else {
					ocxodac_saved_value = dac_value;
				}
			}

			rc = lc15bts_clock_err_reset();
			if (rc < 0) {
				LOGP(DCALIB, LOGL_ERROR,
					"Failed to reset clock error module %d\n", rc);
				calib_state_reset(mgr, CALIB_FAIL_CLKERR);
			}
		}
	} else {
		LOGP(DCALIB, LOGL_NOTICE, "Skipping this iteration, no integration time\n");
	}

	calib_state_reset(mgr, CALIB_SUCCESS);
	return;
}

static void calib_close(struct lc15bts_mgr_instance *mgr)
{
	lc15bts_clock_err_close();
	lc15bts_clock_dac_close();
}

static void calib_state_reset(struct lc15bts_mgr_instance *mgr, int outcome)
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
		lc15bts_swd_event(mgr, SWD_CHECK_CALIB);
        }

        mgr->calib.state = CALIB_INITIAL;
	calib_close(mgr);
}

static int calib_run(struct lc15bts_mgr_instance *mgr, int from_loop)
{
	if (mgr->calib.state != CALIB_INITIAL) {
		LOGP(DCALIB, LOGL_ERROR, "Calib is already in progress.\n");
		return -1;
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
        struct lc15bts_mgr_instance *mgr = _data;

        LOGP(DCALIB, LOGL_NOTICE, "Going to calibrate the system.\n");
        rc = calib_run(mgr, 1);
        if (rc != 0) {
                calib_state_reset(mgr, CALIB_FAIL_START);
	}
}

int lc15bts_mgr_calib_run(struct lc15bts_mgr_instance *mgr)
{
        return calib_run(mgr, 0);
}

int lc15bts_mgr_calib_init(struct lc15bts_mgr_instance *mgr)
{
	mgr->calib.state = CALIB_INITIAL;
	mgr->calib.calib_timeout.data = mgr;
	mgr->calib.calib_timeout.cb = calib_loop_run;
	osmo_timer_schedule(&mgr->calib.calib_timeout, 0, 0);

	return 0;
}

