/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "utils/utils.h"
#include "json-parser/json_utils.h"

#include "paws_globals.h"
#include "paws_messages.h"
#include "paws_sm.h"
#include "paws_common.h"
#include "paws_utils.h"
#include "paws_combiner_sm_types.h"
#include "paws_timer_info.h"
#include "paws_dal_types.h"
#include "paws_dal_utils.h"


// ##################### Function prototypes 
// definition of all static class functions called from STL
static void store_state(void* sm_, sm_state_info_t* sm_state_info);
static void free_state(void* sm_, sm_state_info_t* sm_state_info);
static void process_state_attributes(void* sm_, sm_state_info_t* state_info);
static void process_state_timers(void* sm_, char* timer_info);
PAWS_SM_FUNC_PROTO(Start)
PAWS_SM_FUNC_PROTO(Init_states)
PAWS_SM_FUNC_PROTO(Clear_Spectrums)
PAWS_SM_FUNC_PROTO(DLULSpectrumAvailable)
PAWS_SM_FUNC_PROTO(DLULSpectrumNotAvailable)
PAWS_SM_FUNC_PROTO(set_spectrum_InUse)
static spec_cfg_t* get_spectrum_InUse(void* sm_);
PAWS_SM_FUNC_PROTO(clear_spectrum_InUse)
PAWS_SM_FUNC_PROTO(check_spectrum_InUse)
PAWS_SM_FUNC_PROTO(check_Notify_Required)
PAWS_SM_FUNC_PROTO(Start_Retry_Timer)
PAWS_SM_FUNC_PROTO(Stop_Retry_Timer)
PAWS_SM_FUNC_PROTO(Start_maxPolling_Timer)
PAWS_SM_FUNC_PROTO(Stop_maxPolling_Timer)
static void set_maxPollingQuick(void* sm_, uint32_t dur_secs);
PAWS_SM_FUNC_PROTO(Start_maxPollingQuick_Timer)
PAWS_SM_FUNC_PROTO(Stop_maxPollingQuick_Timer)
PAWS_SM_FUNC_PROTO(Start_SpectrumPending_Timer)
PAWS_SM_FUNC_PROTO(Stop_SpectrumPending_Timer)
PAWS_SM_FUNC_PROTO(activatePendingSpectrum)
PAWS_SM_FUNC_PROTO(Start_SpectrumExpiry_Timer)
PAWS_SM_FUNC_PROTO(Stop_SpectrumExpiry_Timer)
PAWS_SM_FUNC_PROTO(combiner_selectDB)
PAWS_SM_FUNC_PROTO(combiner_DBerror)
// definition of static class functions which are not called directly by STL
static bool Check_HTTP_Result(void* sm_, json_value* resp, bool raise);
static void combiner_ProcessDbUpdate(void* sm_, json_value* new_db_spec);
static void set_db(void* sm_, paws_db_item_t* db);
static bool Check_Valid_Resp(void* sm_, json_value* resp, bool raise);
static void Process_Available_Spectrum_Resp(void* sm_, json_value* resp, lte_direction_e dir, time_t resp_time);
static avail_spectrum_t* GetAvailSpectrumResp(void* sm_);
static void checkAvailableSpectrum(void* sm_);
static bool set_selected_spectrum(void* sm_, spec_cfg_t* spec_cfg);
PAWS_SM_FUNC_PROTO(ActivateSpectrumRuleset)
PAWS_SM_FUNC_PROTO(clear_Ruleset)
static float get_max_location_change(void* sm_);
static uint32_t get_max_polling_secs(void* sm_);
static void Process_Notify_Use_Resp(void* sm_, json_value* resp);
static void timeout_handler(void* sm_, uint32_t id);
static void Retry_Timer_Hdlr(void* sm_);
static void maxPolling_Timer_Hdlr(void* sm_);
static void maxPollingQuick_Timer_Hdlr(void* sm_);
static void SpectrumPending_Timer_Hdlr(void* sm_);
static void SpectrumExpiry_Timer_Hdlr(void* sm_);
static void set_master_info(void* sm_, paws_device_info_t* master_info);
static void set_defaultInfo(void* sm_, float max_location_change, uint32_t max_polling_secs);
static void set_gps(void* sm_, paws_gps_location_t* new_gps);
static void gps_changed(void* sm_);
static const char* event_id_2_str(void* sm_, int id);
static void state_changed_dbg(void* sm_, int old_state, int new_state);
static void event_processed_dbg(void* sm_, int event_id);
static bool set_LogInfo(void* sm_, paws_sm_logger_t* logger, logger_cfg_t* cfg);
static bool set_msgLogInfo(void* sm_, logger_cfg_t* cfg);
static void app_log(void* sm_, const char* file_, int line_, const char* func_, const char* logline, log_level_e level);
static void app_log_tvwsdb_msg(void* sm_, const char* msg);
static void msg_log_tvwsdb_msg(void* sm_, char* cloud_log_j);
static void raise_(void* sm_, int event_id);
PAWS_SM_FUNC_PROTO(process_next_timer_tick)
static int process_events(void* sm_);
static void generic_timeout_handler(unsigned int timer_id, void* sm_);
static bool stl_init(void* sm_, State *state_transition_table, int initial_state, bool run_pre);
static void stl_set_state_explicit(void* sm_, int state);
static bool Init(void* sm_, float min_dbm_100k, uint16_t db_retry_secs, uint32_t max_polling_quick_secs);
static void paws_sm_data_free(paws_sm_t* paws_sm, paws_sm_data_t* data);
static void paws_sm_stl_data_free(paws_sm_t* paws_sm, paws_sm_stl_data_t* data);




//#######################################################################################
static void store_state(void* sm_, sm_state_info_t* sm_state_info)
{
	FUNC_DBG(sm_);

	strcpy(sm_state_info->unique_id, ((paws_sm_t*)sm_)->paws_sm_hdr.unique_id);
	sm_state_info->stl_current_state = PAWS_SM_STL_DATA(sm_)->stl->current_state_id;
	sm_state_info->timer_info = timer_manager_save_state(PAWS_SM_STL_DATA(sm_)->timer_manager);
	sm_state_info->default_max_location_change = PAWS_SM_DATA(sm_)->default_max_location_change;
	sm_state_info->default_max_polling_secs = PAWS_SM_DATA(sm_)->default_max_polling_secs;
	sm_state_info->specific_max_location_change = PAWS_SM_DATA(sm_)->specific_max_location_change;
	sm_state_info->specific_max_polling_secs = PAWS_SM_DATA(sm_)->specific_max_polling_secs;
	memcpy(&sm_state_info->gps, &PAWS_SM_DATA(sm_)->gps, sizeof(paws_gps_location_t));
	memcpy(&sm_state_info->selected_db, &PAWS_SM_DATA(sm_)->selected_db, sizeof(paws_db_item_t));
	sm_state_info->avail_spectrum_resp = avail_spectrum_vcopy(PAWS_SM_DATA(sm_)->avail_spectrum_resp);
	sm_state_info->available_spectrum = spec_cfg_vcopy(PAWS_SM_DATA(sm_)->available_spectrum);
	sm_state_info->pending_spectrum = spec_cfg_vcopy(PAWS_SM_DATA(sm_)->pending_spectrum);
	sm_state_info->selected_spectrum = spec_cfg_vcopy(PAWS_SM_DATA(sm_)->selected_spectrum);
	sm_state_info->spectrum_in_use = spec_cfg_vcopy(PAWS_SM_DATA(sm_)->spectrum_in_use);
}


//#######################################################################################
static void free_state(void* sm_, sm_state_info_t* sm_state_info)
{
	FUNC_DBG(sm_);
	(void)sm_state_info;
}

//#######################################################################################
static void process_state_attributes(void* sm_, sm_state_info_t* sm_state_info)
{
	FUNC_DBG(sm_);

	if (sm_state_info)
	{
		// ruleset 
		PAWS_SM_DATA((paws_sm_t*)sm_)->default_max_location_change = sm_state_info->default_max_location_change;
		PAWS_SM_DATA((paws_sm_t*)sm_)->default_max_polling_secs = sm_state_info->default_max_polling_secs;
		PAWS_SM_DATA((paws_sm_t*)sm_)->specific_max_location_change = sm_state_info->specific_max_location_change;
		PAWS_SM_DATA((paws_sm_t*)sm_)->specific_max_polling_secs = sm_state_info->specific_max_polling_secs;

		// gps
		memcpy(&PAWS_SM_DATA((paws_sm_t*)sm_)->gps, &sm_state_info->gps, sizeof(paws_gps_location_t));

		// selected_db
		memcpy(&PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db, &sm_state_info->selected_db, sizeof(paws_db_item_t));

		// avail_spectrum_resp
		PAWS_SM_DATA((paws_sm_t*)sm_)->avail_spectrum_resp = avail_spectrum_vcopy(sm_state_info->avail_spectrum_resp);

		// available_spectrum
		PAWS_SM_DATA((paws_sm_t*)sm_)->available_spectrum = spec_cfg_vcopy(sm_state_info->available_spectrum);

		// pending_spectrum
		PAWS_SM_DATA((paws_sm_t*)sm_)->pending_spectrum = spec_cfg_vcopy(sm_state_info->pending_spectrum);

		// selected_spectrum
		PAWS_SM_DATA((paws_sm_t*)sm_)->selected_spectrum = spec_cfg_vcopy(sm_state_info->selected_spectrum);

		// spectrum_in_use
		PAWS_SM_DATA((paws_sm_t*)sm_)->spectrum_in_use = spec_cfg_vcopy(sm_state_info->spectrum_in_use);
	}
}


//#######################################################################################
static void process_state_timers(void* sm_, char* timer_info)
{
	FUNC_DBG(sm_);

	if (timer_info)
	{
		timer_manager_load_state(PAWS_SM_STL_DATA((paws_sm_t*)sm_)->timer_manager, timer_info);
	}
}

//#######################################################################################
static void Start(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_Start);
}

//#######################################################################################
static void Init_states(void* sm_)
{
	FUNC_DBG(sm_);
	paws_sm_data_free(sm_, PAWS_SM_DATA(sm_));
	LOCAL_FUNC(sm_, Clear_Spectrums)(sm_);
}

//#######################################################################################
static void Clear_Spectrums(void* sm_)
{
	FUNC_DBG(sm_);
	avail_spectrum_free(&PAWS_SM_DATA(sm_)->avail_spectrum_resp);
	spec_cfg_free(&PAWS_SM_DATA(sm_)->selected_spectrum);
}

//#######################################################################################
static void DLULSpectrumAvailable(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_,raise_)(sm_, (int)ev_DlUlSpectrumAvailable);
}


//#######################################################################################
static void DLULSpectrumNotAvailable(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_DlUlSpectrumNotAvailable);
}

//#######################################################################################
static void set_spectrum_InUse(void* sm_)
{
	FUNC_DBG(sm_);

	if (PAWS_SM_DATA(sm_)->selected_spectrum)
	{
		// remove current spectrum_in_use
		if (PAWS_SM_DATA(sm_)->spectrum_in_use)
			spec_cfg_free(&PAWS_SM_DATA(sm_)->spectrum_in_use);
		// update it
		PAWS_SM_DATA(sm_)->spectrum_in_use = spec_cfg_vcopy(PAWS_SM_DATA(sm_)->selected_spectrum);
	}
}

//#######################################################################################
static void clear_spectrum_InUse(void* sm_)
{
	FUNC_DBG(sm_);
	spec_cfg_free(&PAWS_SM_DATA(sm_)->spectrum_in_use);
}

//#######################################################################################
static void check_spectrum_InUse(void* sm_)
{
	FUNC_DBG(sm_);

	if ((PAWS_SM_DATA(sm_)->spectrum_in_use) && (PAWS_SM_DATA(sm_)->spectrum_in_use->sched))
	{
		time_t now_ = time(NULL);

		if ((PAWS_SM_DATA(sm_)->spectrum_in_use->sched->event_time_range.start_time <= now_) && (now_ <= PAWS_SM_DATA(sm_)->spectrum_in_use->sched->event_time_range.stop_time))
		{
			LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_SpectrumActive);
			return;
		}
	}
	LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_SpectrumNotActive);
}

//#######################################################################################
static void check_Notify_Required(void* sm_)
{
	FUNC_DBG(sm_);

	if (!(PAWS_SM_DATA(sm_)->selected_spectrum))
	{
		LOG_PRINT(sm_, LOG_ERROR, "No selected_spectrum");
	}
	else
	{
		if ((PAWS_SM_DATA(sm_)->selected_spectrum->spec) && (PAWS_SM_DATA(sm_)->selected_spectrum->spec->needsSpectrumReport))
		{
			LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_Notification_Required);
		}
		else
		{
			LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_Notification_Not_Required);
		}
	}
}

//#######################################################################################
static void Start_Retry_Timer(void* sm_)
{
	FUNC_DBG(sm_);
	LOG_PRINT(sm_, LOG_INFO, "Start_Retry_Timer [dur=%d]", PAWS_SM_DATA(sm_)->db_retry_secs);
	timer_manager_start_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, RETRY_TIMER_ID);
}

//#######################################################################################
static void Stop_Retry_Timer(void* sm_)
{
	FUNC_DBG(sm_);
	timer_manager_stop_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, RETRY_TIMER_ID);
}

//#######################################################################################
static void Retry_Timer_Hdlr(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_Retry_Timeout);

}

//#######################################################################################
static void Start_maxPolling_Timer(void* sm_)
{
	FUNC_DBG(sm_);

	int dur = LOCAL_FUNC(sm_, get_max_polling_secs)(sm_);

	// use tolerance
	dur -= MAX_POLLING_TIMER_TOLERANCE;

	if (dur > 0)
	{
		LOG_PRINT(sm_, LOG_INFO, "Start_maxPolling_Timer [dur=%d]", dur);
		timer_manager_set_duration(PAWS_SM_STL_DATA(sm_)->timer_manager, MAX_POLLING_TIMER_ID, dur);
		timer_manager_start_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, MAX_POLLING_TIMER_ID);
		Start_maxPollingQuick_Timer(sm_);
	}
	else
	{
		LOG_PRINT(sm_, LOG_INFO, "Start_maxPolling_Timer [duration=0 !!!!]");
		LOCAL_FUNC(sm_, maxPolling_Timer_Hdlr)(sm_);
	}
}

//#######################################################################################
static void Stop_maxPolling_Timer(void* sm_)
{
	FUNC_DBG(sm_);
	timer_manager_stop_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, MAX_POLLING_TIMER_ID);
}

//#######################################################################################
static void maxPolling_Timer_Hdlr(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_maxPolling_Timeout);
	LOCAL_FUNC(sm_, Start_maxPolling_Timer)(sm_);
}



//#######################################################################################
static void set_maxPollingQuick(void* sm_, uint32_t dur_secs)
{
	FUNC_DBG(sm_);

	PAWS_SM_DATA(sm_)->max_polling_quick_secs = dur_secs;
}


//#######################################################################################
static void Start_maxPollingQuick_Timer(void* sm_)
{
	FUNC_DBG(sm_);

	if (PAWS_SM_DATA(sm_)->max_polling_quick_secs == 0)			// 0 = disabled
	{
		return;
	}

	uint32_t dur = LOCAL_FUNC(sm_, get_max_polling_secs)(sm_);
	if (dur <= PAWS_SM_DATA(sm_)->max_polling_quick_secs)		// the quickTimer is > maxPolling from the DB, its pointless starting it
	{
		return;
	}
	dur = PAWS_SM_DATA(sm_)->max_polling_quick_secs;

	if (dur > 0)
	{
		LOG_PRINT(sm_, LOG_NOTICE, "maxPolling [dur=%d]", dur);
		timer_manager_set_duration(PAWS_SM_STL_DATA(sm_)->timer_manager, MAX_POLLING_QUICK_TIMER_ID, dur);
		timer_manager_start_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, MAX_POLLING_QUICK_TIMER_ID);
	}
	else
	{
		LOG_PRINT(sm_, LOG_NOTICE, "maxPolling [dur=0 !!!!]");
		LOCAL_FUNC(sm_, maxPollingQuick_Timer_Hdlr)(sm_);
	}
}

//#######################################################################################
static void Stop_maxPollingQuick_Timer(void* sm_)
{
	FUNC_DBG(sm_);
	timer_manager_stop_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, MAX_POLLING_QUICK_TIMER_ID);
}

//#######################################################################################
static void maxPollingQuick_Timer_Hdlr(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_maxPollingQuick_Timeout);
	LOCAL_FUNC(sm_, Start_maxPollingQuick_Timer)(sm_);
}

//#######################################################################################
static void Start_SpectrumPending_Timer(void* sm_)
{
	FUNC_DBG(sm_);

	spectrum_schedule_t* sched;
	if (!(sched = PAWS_SM_DATA(sm_)->pending_spectrum->sched))
	{
		LOG_PRINT(sm_, LOG_ERROR, "No pending_spectrum schedule");
		return;
	}

	time_t now_ = time(NULL);
	time_t stop_t = sched->event_time_range.start_time;

	if (now_ > stop_t)
		LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_SpectrumPending_Timeout);
	else
	{
		int dur = stop_t - now_;
		LOG_PRINT(sm_, LOG_NOTICE, "SpectrumPending [dur=%d]", dur);
		timer_manager_set_duration(PAWS_SM_STL_DATA(sm_)->timer_manager, SPECTRUM_PENDING_TIMER_ID, dur);
		timer_manager_start_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, SPECTRUM_PENDING_TIMER_ID);
	}
}

//#######################################################################################
static void Stop_SpectrumPending_Timer(void* sm_)
{
	FUNC_DBG(sm_);
	timer_manager_stop_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, SPECTRUM_PENDING_TIMER_ID);
}

//#######################################################################################
static void SpectrumPending_Timer_Hdlr(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_SpectrumPending_Timeout);
}

//#######################################################################################
static void activatePendingSpectrum(void* sm_)
{
	FUNC_DBG(sm_);

	if (PAWS_SM_DATA(sm_)->pending_spectrum)
	{
		// remove any currently available spectrum
		if (PAWS_SM_DATA(sm_)->available_spectrum)
			spec_cfg_free(&PAWS_SM_DATA(sm_)->available_spectrum);

		LOCAL_FUNC(sm_, clear_Ruleset)(sm_);

		// promote pending to available
		PAWS_SM_DATA(sm_)->available_spectrum = PAWS_SM_DATA(sm_)->pending_spectrum;
		PAWS_SM_DATA(sm_)->pending_spectrum = NULL;

		// update new Ruleset
		PAWS_SM_DATA(sm_)->specific_max_location_change = PAWS_SM_DATA(sm_)->available_spectrum->spec->ruleset_info.maxLocationChange;
		PAWS_SM_DATA(sm_)->specific_max_polling_secs = PAWS_SM_DATA(sm_)->available_spectrum->spec->ruleset_info.maxPollingSecs;
	}
	else
		LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_SpectrumNotAvailable);
}

//#######################################################################################
static void Start_SpectrumExpiry_Timer(void* sm_)
{
	FUNC_DBG(sm_);

	if (!(PAWS_SM_DATA(sm_)->available_spectrum))
	{
		LOG_PRINT(sm_, LOG_ERROR, "No available_spectrum");
		return;
	}
	spectrum_schedule_t* sched;
	if (!(sched = PAWS_SM_DATA(sm_)->available_spectrum->sched))
	{
		LOG_PRINT(sm_, LOG_ERROR, "No available_spectrum schedule");
		return;
	}

	time_t now_ = time(NULL);
	time_t stop_t = sched->event_time_range.stop_time;

	if (now_ > stop_t)
		LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_SpectrumExpiry_Timeout);
	else
	{
		int dur = stop_t - now_ ;
		LOG_PRINT(sm_, LOG_NOTICE, "SpectrumExpiry [dur=%d]", dur);
		timer_manager_set_duration(PAWS_SM_STL_DATA(sm_)->timer_manager, SPECTRUM_EXPIRY_TIMER_ID, dur);
		timer_manager_start_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, SPECTRUM_EXPIRY_TIMER_ID);
	}
}

//#######################################################################################
static void Stop_SpectrumExpiry_Timer(void* sm_)
{
	FUNC_DBG(sm_);
	timer_manager_stop_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, SPECTRUM_EXPIRY_TIMER_ID);
}

//#######################################################################################
static void SpectrumExpiry_Timer_Hdlr(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_SpectrumExpiry_Timeout);
}

//#######################################################################################
static void combiner_selectDB(void* sm_)
{
	FUNC_DBG(sm_);
	COMBINER_PUBLIC_FUNC(CREATOR_SM(sm_), select_DB)(CREATOR_SM(sm_), sm_);
}

//#######################################################################################
static void combiner_DBerror(void* sm_)
{
	FUNC_DBG(sm_);
	LOG_PRINT(sm_, LOG_ERROR, "calling combiner:DBerror");
	COMBINER_PUBLIC_FUNC(CREATOR_SM(sm_), DBerror)(CREATOR_SM(sm_), sm_);
}

//#######################################################################################
static bool Check_HTTP_Result(void* sm_, json_value* resp, bool raise)
{
	FUNC_DBG(sm_);

	if (!resp)
	{
		if (raise) 
		{
			LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_DB_Error);		
		}
		return false;
	}
	return true;
}

//#######################################################################################
static void combiner_ProcessDbUpdate(void* sm_, json_value* new_db_spec)
{
	FUNC_DBG(sm_);
	COMBINER_PUBLIC_FUNC(CREATOR_SM(sm_), ProcessDbUpdate)(CREATOR_SM(sm_), new_db_spec);
}

//#######################################################################################
static void set_db(void* sm_, paws_db_item_t* new_db)
{
	FUNC_DBG(sm_);

	if ((strlen(new_db->name) > 0))
	{
		if ((strcmp(new_db->db_url.host, PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db.db_url.host) == 0))
		{
			LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_DB_Found);
		}
		else
		{
			LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_DB_Updated);
		}
		memcpy(&PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db, new_db, sizeof(paws_db_item_t));
	}
	else
	{
		memset(&PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db, 0, sizeof(paws_db_item_t));
		LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_DB_NotFound);
	}
}

//#######################################################################################
static bool Check_Valid_Resp(void* sm_, json_value* resp, bool raise)
{
	FUNC_DBG(sm_);

	json_value* error = NULL;

	// if 'error' in resp.keys() :
	if ((error = json_get(resp, "error")))
	{
		json_value* data = NULL;
		json_value* spec = NULL;
		// if ('data' in resp['error'].keys()) and ('spec' in resp['error']['data'].keys()) :
		if ((data = json_get(error, "data")) && (spec = json_get(data, "spec")) && (spec->type == json_object))
		{
			LOCAL_FUNC(sm_, combiner_ProcessDbUpdate)(sm_, spec);
		}
		else
		{
			int64_t code = 0;
			if (json_get_int(error, "code", &code))
			{
				if ((code >= PAWS_ERROR_COMPATIBILITY_MIN) && (code <= PAWS_ERROR_COMPATIBILITY_MAX))
				{
					if (raise) LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_DB_Error);
					LOG_PRINT(sm_, LOG_ERROR, "<---- DB COMPATIBILITY ERROR");
				}
				else if ((code >= PAWS_ERROR_REQERROR_MIN) && (code <= PAWS_ERROR_REQERROR_MAX))
				{
					if (raise) LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_DB_Error);
					LOG_PRINT(sm_, LOG_ERROR, "<---- DB REQUEST ERROR");
				}
				else if ((code >= PAWS_ERROR_AUTHORISATION_MIN) && (code <= PAWS_ERROR_AUTHORISATION_MIN))
				{
					if (raise) LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_DB_Error);
					LOG_PRINT(sm_, LOG_ERROR, "<---- AUTHORISATION ERROR");
				}
				else
				{
					if (raise) LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_DB_Error);
					LOG_PRINT(sm_, LOG_ERROR, "<---- DB GENERAL ERROR");
				}
			}
			else
			{
				if (raise) LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_DB_Error);
				LOG_PRINT(sm_, LOG_ERROR, "<---- DB GENERAL ERROR");
			}
		}
	}
	else
	{
		json_value* db_change = NULL;
		// if 'error' in resp.keys() :
		if ((db_change = json_get(resp, "result/databaseChange")) && (db_change->type == json_object))
		{
			LOCAL_FUNC(sm_, combiner_ProcessDbUpdate)(sm_, db_change);
		}
		else
		{
			bool result;
			if ((json_get_bool(resp, "result/result", &result)) && (result==false))
			{
				if (raise) LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_DB_Response_Failure);
				LOG_PRINT(sm_, LOG_ERROR, "<---- DB RESP FAILURE");
			}
			else
			{
				return true;
			}
		}

	}
	return false;
}

//#######################################################################################
static void Process_Available_Spectrum_Resp(void* sm_, json_value* resp, lte_direction_e dir, time_t resp_time)
{
	FUNC_DBG(sm_);

	if (LOCAL_FUNC(sm_, Check_HTTP_Result)(sm_, resp, true))
	{
		if (LOCAL_FUNC(sm_, Check_Valid_Resp)(sm_, resp, true))
		{
			// ## update availabe spectrum
			avail_spectrum_free(&PAWS_SM_DATA(sm_)->avail_spectrum_resp);
			if ((PAWS_SM_DATA(sm_)->avail_spectrum_resp = json_resp_2_avail_spectrum(sm_, resp, dir, 13, resp_time, PAWS_SM_DATA(sm_)->min_dbm_100k)))
			{
				//avail_spectrum_print(PAWS_SM_DATA(sm_)->avail_spectrum_resp);

				// raise event to show a valid RESP was received
				LOG_PRINT(sm_, LOG_NOTICE, "<---- AVAIL-SPEC-RESP");
				LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_Available_Spectrum_Resp);
			}
		}
	}
}


//#######################################################################################
static avail_spectrum_t* GetAvailSpectrumResp(void* sm_)
{
	FUNC_DBG(sm_);
	return PAWS_SM_DATA(sm_)->avail_spectrum_resp;
}

//#######################################################################################
static void checkAvailableSpectrum(void* sm_)
{
	FUNC_DBG(sm_);
	avail_spectrum_t* resp=NULL;
	time_t now_ = time(NULL);

	// ## this isnt actually selecting the spectrum.  It is just determining if there is a spectrum provided which is in the lte band frequency range and of sufficient power.
	// ## It also checks if it is valid now, or is pending.
	// ## No other selections are made if multiple profiles match this criteria.

	// delete current available_spectrum
	if (PAWS_SM_DATA(sm_)->available_spectrum)
	{
		spec_cfg_free(&PAWS_SM_DATA(sm_)->available_spectrum);
	}

	// delete the current pending_spectrum
	if (PAWS_SM_DATA(sm_)->pending_spectrum)
	{
		spec_cfg_free(&PAWS_SM_DATA(sm_)->pending_spectrum);
	}

	// check if there is a valid RESP
	if (!(resp = PAWS_SM_DATA(sm_)->avail_spectrum_resp))
	{
		LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_SpectrumNotAvailable);
	}

	time_t selected_start_t;
	spectrum_spec_t* spec = resp->spectrum_specs;
	spectrum_spec_t* selected_spec = NULL;
	while (spec)
	{
		spectrum_schedule_t* sched = NULL;
		// look at the times of the first schedule
		if (!(sched = spec->spectrum_schedules))
			goto error_hdl;

		// if start_time is in the past, set it to now
		time_t start_t = sched->event_time_range.start_time;
		if (now_ > start_t)
			start_t = now_;
		// if stop time is already expired, we cant use this
		// make sure that remaining duration is longer than SPECTRUM_EXPIRY_TIMER_TOLERANCE, otherwise we cant use it
		time_t stop_t = sched->event_time_range.stop_time;
		if (!((now_ + SPECTRUM_EXPIRY_TIMER_TOLERANCE) > stop_t))
		{
			// ## if an earlier spectrumSpec has already been selected, skip this one
			if (!((selected_spec) && (selected_start_t < start_t)))
			{
				// otherwise use this spec
				selected_spec = spec;
				selected_start_t = start_t;
			}
		}
		// move to the next spec
		spec = spec->next;
	}

	// if nothing at all was selected, the Resp was completely unusable
	if (!(selected_spec))
		goto error_hdl;
	
	// some spectrum was selected so determine if it pending or available now
	if (selected_start_t <= now_)
	{
		// it is available now

		// set as "available"
		if (!(PAWS_SM_DATA(sm_)->available_spectrum = spec_cfg_new()))
			goto error_hdl;
		if (!(PAWS_SM_DATA(sm_)->available_spectrum->spec = spectrum_spec_vcopy(selected_spec)))
			goto error_hdl;
		if (!(PAWS_SM_DATA(sm_)->available_spectrum->sched = spectrum_sched_vcopy(selected_spec->spectrum_schedules)))
			goto error_hdl;

		// clear the current ruleset
		LOCAL_FUNC(sm_, clear_Ruleset)(sm_);
		// update the ruleset
		PAWS_SM_DATA(sm_)->specific_max_location_change = selected_spec->ruleset_info.maxLocationChange;
		PAWS_SM_DATA(sm_)->specific_max_polling_secs = selected_spec->ruleset_info.maxPollingSecs;

		// raise event
		LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_SpectrumAvailable);
	}
	else
	{
		// set as "pending"
		if (!(PAWS_SM_DATA(sm_)->pending_spectrum = spec_cfg_new()))
			goto error_hdl;
		if (!(PAWS_SM_DATA(sm_)->pending_spectrum->spec = spectrum_spec_vcopy(selected_spec)))
			goto error_hdl;
		if (!(PAWS_SM_DATA(sm_)->pending_spectrum->sched = spectrum_sched_vcopy(selected_spec->spectrum_schedules)))
			goto error_hdl;

		// raise event
		LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_SpectrumPending);
	}
	return;

error_hdl:
	// clear the current ruleset
	LOCAL_FUNC(sm_, clear_Ruleset)(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_SpectrumNotAvailable);
	avail_spectrum_free(&PAWS_SM_DATA(sm_)->avail_spectrum_resp);
	spec_cfg_free(&PAWS_SM_DATA(sm_)->available_spectrum);
	spec_cfg_free(&PAWS_SM_DATA(sm_)->pending_spectrum);
}



//#######################################################################################
static bool set_selected_spectrum(void* sm_, spec_cfg_t* spec_cfg)
{
	FUNC_DBG(sm_);

	if (!spec_cfg)
		return false;
	
	// remove any currently assigned spectrum
	if (PAWS_SM_DATA(sm_)->selected_spectrum)
		spec_cfg_free(&PAWS_SM_DATA(sm_)->selected_spectrum);

	// set new
	PAWS_SM_DATA(sm_)->selected_spectrum = spec_cfg_vcopy(spec_cfg);

	return true;
}

//#######################################################################################
static spec_cfg_t* get_spectrum_InUse(void* sm_)
{
	FUNC_DBG(sm_);
	return PAWS_SM_DATA(sm_)->spectrum_in_use;
}

//#######################################################################################
static void ActivateSpectrumRuleset(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, Start_maxPolling_Timer)(sm_);
}

//#######################################################################################
static void clear_Ruleset(void* sm_)
{
	FUNC_DBG(sm_);
	PAWS_SM_DATA(sm_)->specific_max_location_change = 0;
	PAWS_SM_DATA(sm_)->specific_max_polling_secs = 0;
	LOCAL_FUNC(sm_, Start_maxPolling_Timer)(sm_);
}


//#######################################################################################
static float get_max_location_change(void* sm_)
{
	FUNC_DBG(sm_);

	if (PAWS_SM_DATA(sm_)->specific_max_location_change > 0)
		return PAWS_SM_DATA(sm_)->specific_max_location_change;

	return PAWS_SM_DATA(sm_)->default_max_location_change;
}

//#######################################################################################
static uint32_t get_max_polling_secs(void* sm_)
{
	FUNC_DBG(sm_);
	
	if (PAWS_SM_DATA(sm_)->specific_max_polling_secs > 0)
		return PAWS_SM_DATA(sm_)->specific_max_polling_secs;

	return PAWS_SM_DATA(sm_)->default_max_polling_secs;
}

//#######################################################################################
static void Process_Notify_Use_Resp(void* sm_, json_value* resp)
{
	FUNC_DBG(sm_);

	if (LOCAL_FUNC(sm_, Check_HTTP_Result)(sm_, resp, true))
	{
		if (LOCAL_FUNC(sm_, Check_Valid_Resp)(sm_, resp, true))
		{
			// raise event to show a valid RESP was received
			LOG_PRINT(sm_, LOG_NOTICE, "<---- NOTIFY-RESP");
			LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_Notification_Success);
		}
	}
}

//#######################################################################################
// this needs to be special as we need to register a static function with the timer manager
static void generic_timeout_handler(unsigned int timer_id, void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, timeout_handler)(sm_, timer_id);
}

//#######################################################################################
static void timeout_handler(void* sm_, uint32_t id)
{
	FUNC_DBG(sm_);

	switch (id)
	{
		case RETRY_TIMER_ID:
			Retry_Timer_Hdlr(sm_);
			break;
		case SPECTRUM_EXPIRY_TIMER_ID:
			SpectrumExpiry_Timer_Hdlr(sm_);
			break;
		case SPECTRUM_PENDING_TIMER_ID:
			SpectrumPending_Timer_Hdlr(sm_);
			break;
		case MAX_POLLING_TIMER_ID:
			maxPolling_Timer_Hdlr(sm_);
			break;
		case MAX_POLLING_QUICK_TIMER_ID:
			maxPollingQuick_Timer_Hdlr(sm_);
			break;
		default:
			// unknown
			LOG_PRINT(sm_, LOG_ERROR, "Unknown timer [id=%d]", id);
			break;
		}
}


//#######################################################################################
static void set_master_info(void* sm_, paws_device_info_t* master_info)
{
	FUNC_DBG(sm_);
	memcpy(&(PAWS_SM_DATA(sm_)->master_info), master_info, sizeof(paws_device_info_t));
}


//#######################################################################################
static void set_defaultInfo(void* sm_, float max_location_change, uint32_t max_polling_secs)
{
	FUNC_DBG(sm_);
	PAWS_SM_DATA(sm_)->default_max_location_change = max_location_change;
	PAWS_SM_DATA(sm_)->default_max_polling_secs = max_polling_secs;
}

//#######################################################################################
static void set_gps(void* sm_, paws_gps_location_t* new_gps)
{
	FUNC_DBG(sm_);

	paws_gps_location_t* curr_gps = &PAWS_SM_DATA(sm_)->gps;
	
	if ((PAWS_SM_DATA(sm_)->default_max_location_change > 0) && (curr_gps->fixed))
	{
		float max_location_change = LOCAL_FUNC(sm_, get_max_location_change)(sm_);
		float distance = gps_distance_between_two_locations(curr_gps->latitude, curr_gps->longitude, new_gps->latitude, new_gps->longitude);
		if (distance > max_location_change) 
		{
			LOG_PRINT(sm_, LOG_ERROR, "GPS distance has changed by %.4f metres [max=%.4f]", distance, max_location_change)
			LOCAL_FUNC(sm_, gps_changed)(sm_);
		}
	}
	memcpy(curr_gps, new_gps, sizeof(paws_gps_location_t));
}

//#######################################################################################
static void gps_changed(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_GPS_location_changed);
}

// #######################################################################################
// STL decoders
#define GEN_EV_STR(STRING) #STRING,
const char *paws_ev_str[] = {
	"dummy",
	FOREACH_EV(GEN_EV_STR)
};

static const char* event_id_2_str(void* sm_, int id)
{
	UNUSED_PARAM(sm_);

	if ((id > ev_Dummy) && (id < ev_PAWS_SM_END_EV_MARKER))
	{
		return paws_ev_str[(id - ev_Dummy)];
	}
	// otherwise its unknown
	return "Unknown";
}
//#######################################################################################
static void state_changed_dbg(void* sm_, int old_state, int new_state)
{
	LOG_PRINT(sm_, LOG_INFO, "state_change : %s --> %s", LOCAL_FUNC(sm_, state_id_2_str)(sm_, old_state), LOCAL_FUNC(sm_, state_id_2_str)(sm_, new_state));
}

//#######################################################################################
static void event_processed_dbg(void* sm_, int event_id)
{
	LOG_PRINT(sm_, LOG_INFO, "process event : %s", LOCAL_FUNC(sm_, event_id_2_str)(sm_, event_id));
}


//#######################################################################################
static bool set_LogInfo(void* sm_, paws_sm_logger_t* logger, logger_cfg_t* cfg)
{
	FUNC_DBG(sm_);

	if ((!logger) || (!cfg))
	{
		return  false;
	}

	if ((!logger->logger) || (memcmp(&logger->cfg, cfg, sizeof(logger_cfg_t)) != 0))		// if not already created, or config has changed
	{
		memcpy(&logger->cfg, cfg, sizeof(logger_cfg_t));

		// if already created free old one
		if (logger->logger)
			logger_free(&logger->logger);

		// if there is a filename configured
		if (strlen(cfg->logname))
		{
			if (!(logger->logger = logger_create(cfg)))
				return false;
		}
	}

	return true;
}


//#######################################################################################
static bool set_msgLogInfo(void* sm_, logger_cfg_t* cfg)
{
	FUNC_DBG(sm_);
	return set_LogInfo(sm_, &(PAWS_SM_DATA(sm_)->msg_log), cfg);
}


//#######################################################################################
static void app_log(void* sm_, const char* file_, int line_, const char* func_, const char* logline, log_level_e level)
{
	if (gPawsAppLogger)
	{
		logger_log(gPawsAppLogger, logline, level, ((paws_sm_t*)sm_)->paws_sm_hdr.unique_id,  true, true, file_, line_, func_, gPawsCloudLogger, gDeviceName, PAWS_LOG_TYPE);
	}
}


//#######################################################################################
static void app_log_tvwsdb_msg(void* sm_, const char* msg)				// Used just to store sent messages in local file
{
	if (PAWS_SM_DATA(sm_)->msg_log.logger)
	{
		logger_log(PAWS_SM_DATA(sm_)->msg_log.logger, msg, LOG_INFO, NULL, false, false, NULL, 0, NULL, NULL, gDeviceName, PAWS_LOG_TYPE);
	}
}


//#######################################################################################
static void msg_log_tvwsdb_msg(void* sm_, char* cloud_log_j)
{
	if (gPawsCloudLogger)
	{
		cloud_logger_log(gPawsCloudLogger, ((paws_sm_t*)sm_)->paws_sm_hdr.unique_id, gDeviceName, CLOUD_LOGGER_TVWSDB_MSG, PAWS_LOG_TYPE, cloud_log_j);
	}
}



//#######################################################################################
static void raise_(void* sm_, int event_id)
{
	FUNC_DBG(sm_);
	
	if (PAWS_SM_STL_DATA(sm_)->stl)
	{
		LOG_PRINT(sm_, LOG_INFO, "raising %s", LOCAL_FUNC(sm_, event_id_2_str)(sm_, event_id));
        sm_raise_event(PAWS_SM_STL_DATA(sm_)->stl, event_id);
	}
}

//#######################################################################################
static void process_next_timer_tick(void* sm_)
{
//	FUNC_DBG(sm_);

	if (PAWS_SM_STL_DATA(sm_)->timer_manager)
	{
		timer_manager_do_tick(PAWS_SM_STL_DATA(sm_)->timer_manager);
	}
}

//#######################################################################################
static int process_events(void* sm_)
{
//	FUNC_DBG(sm_);

	if (PAWS_SM_STL_DATA(sm_)->stl)
	{
		return sm_process_events(PAWS_SM_STL_DATA(sm_)->stl);
	}
	return 0;
}


//#######################################################################################
static bool stl_init(void* sm_, State *state_transition_table, int initial_state, bool run_pre)
{
	FUNC_DBG(sm_);

	if (!((PAWS_SM_STL_DATA(sm_)->stl) = sm_create(state_transition_table, initial_state, sm_, run_pre)))    
	{
		return false;
	}
	sm_register_event_processed_func(PAWS_SM_STL_DATA(sm_)->stl, LOCAL_FUNC(sm_, event_processed_dbg));
	sm_register_state_changed_func(PAWS_SM_STL_DATA(sm_)->stl, LOCAL_FUNC(sm_, state_changed_dbg));

	return true;
}


//#######################################################################################
static void stl_set_state_explicit(void* sm_, int state)
{
	FUNC_DBG(sm_);

	if (PAWS_SM_STL_DATA(sm_)->stl)
	{
		LOCAL_FUNC(sm_, state_changed_dbg)(sm_, PAWS_SM_STL_DATA(sm_)->stl->current_state_id, state);
		PAWS_SM_STL_DATA(sm_)->stl->current_state_id = state;
	}
}


//#######################################################################################
static bool Init(void* sm_, float min_dbm_100k, uint16_t db_retry_secs, uint32_t max_polling_quick_secs)
{
	FUNC_DBG(sm_);

	paws_sm_t* paws_sm = (paws_sm_t*)sm_;

	// paws cfg
	PAWS_SM_DATA(paws_sm)->min_dbm_100k = min_dbm_100k;
	PAWS_SM_DATA(paws_sm)->db_retry_secs = db_retry_secs;
	PAWS_SM_DATA(paws_sm)->max_polling_quick_secs = max_polling_quick_secs;

	// create timer manager 
	if (PAWS_SM_STL_DATA(paws_sm)->timer_manager)
		timer_manager_delete(PAWS_SM_STL_DATA(paws_sm)->timer_manager);

	if (!((PAWS_SM_STL_DATA(sm_)->timer_manager) = timer_manager_create(generic_timeout_handler, sm_)))
	{
		LOG_PRINT(sm_, LOG_ERROR, "Problem in timer_manager_create");
		goto error_hdl;
	}

	// create the Retry timer
	if ((timer_manager_create_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, RETRY_TIMER_ID, PAWS_SM_DATA(sm_)->db_retry_secs)) < 0)
	{
		LOG_PRINT(sm_, LOG_ERROR, "Problem in timer_manager_create_timer RETRY_TIMER_ID");
		goto error_hdl;
	}
	// create the SpectrumPending timer
	if ((timer_manager_create_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, SPECTRUM_PENDING_TIMER_ID, 0)) < 0)
	{
		LOG_PRINT(sm_, LOG_ERROR, "Problem in timer_manager_create_timer SPECTRUM_PENDING_TIMER_ID");
		goto error_hdl;
	}
	// create the SpectrumExpiry timer
	if ((timer_manager_create_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, SPECTRUM_EXPIRY_TIMER_ID, 0)) < 0)
	{
		LOG_PRINT(sm_, LOG_ERROR, "Problem in timer_manager_create_timer SPECTRUM_EXPIRY_TIMER_ID");
		goto error_hdl;
	}
	// create the maxPolling timer
	if ((timer_manager_create_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, MAX_POLLING_TIMER_ID, 0)) < 0)
	{
		LOG_PRINT(sm_, LOG_ERROR, "Problem in timer_manager_create_timer MAX_POLLING_TIMER_ID");
		goto error_hdl;
	}
	// create the maxPollingQuick timer
	if ((timer_manager_create_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, MAX_POLLING_QUICK_TIMER_ID, 0)) < 0)
	{
		LOG_PRINT(sm_, LOG_ERROR, "Problem in timer_manager_create_timer MAX_POLLING_QUICK_TIMER_ID");
		goto error_hdl;
	}

	return true;

error_hdl:
	paws_sm_data_free(paws_sm, paws_sm->paws_sm_hdr.data);
	return false;

}

//#######################################################################################

#define POPULATE_CLASS_FUNC(sm,fnc) \
	sm->paws_sm_hdr.funcs->fnc = fnc;		\
	sm->local_funcs.fnc = fnc;

paws_sm_t* paws_sm_create(void* creator, paws_sm_funcs_t* child_funcs, const char* sm_name)
{
	paws_sm_t* paws_sm = NULL;
	if ((paws_sm = malloc(sizeof(paws_sm_t))))
	{
		int slen;
		memset(paws_sm, 0, sizeof(paws_sm_t));
		paws_sm->paws_sm_hdr.creator = creator;
		paws_sm->paws_sm_hdr.funcs = child_funcs;
		paws_sm->paws_sm_hdr.data = &paws_sm->paws_sm_data_store;
		paws_sm->paws_sm_hdr.stl_data = &paws_sm->paws_sm_stl_data_store;
		slen = snprintf(paws_sm->paws_sm_hdr.unique_id, MAX_DEVICE_NAME_LEN, "%s", sm_name);
		if ((slen < 0) || (slen >= MAX_DEVICE_NAME_LEN))
		{
			free_and_null((void**)&paws_sm);
		}
		else
		{
			POPULATE_CLASS_FUNC(paws_sm, store_state);
			POPULATE_CLASS_FUNC(paws_sm, free_state);
			POPULATE_CLASS_FUNC(paws_sm, process_state_attributes);
			POPULATE_CLASS_FUNC(paws_sm, process_state_timers);
			POPULATE_CLASS_FUNC(paws_sm, Start);
			POPULATE_CLASS_FUNC(paws_sm, Init_states);
			POPULATE_CLASS_FUNC(paws_sm, Clear_Spectrums);
			POPULATE_CLASS_FUNC(paws_sm, DLULSpectrumAvailable);
			POPULATE_CLASS_FUNC(paws_sm, DLULSpectrumNotAvailable);
			POPULATE_CLASS_FUNC(paws_sm, set_spectrum_InUse);
			POPULATE_CLASS_FUNC(paws_sm, get_spectrum_InUse);
			POPULATE_CLASS_FUNC(paws_sm, clear_spectrum_InUse);
			POPULATE_CLASS_FUNC(paws_sm, check_spectrum_InUse);
			POPULATE_CLASS_FUNC(paws_sm, check_Notify_Required);
			POPULATE_CLASS_FUNC(paws_sm, Start_Retry_Timer);
			POPULATE_CLASS_FUNC(paws_sm, Stop_Retry_Timer);
			POPULATE_CLASS_FUNC(paws_sm, Start_maxPolling_Timer);
			POPULATE_CLASS_FUNC(paws_sm, Stop_maxPolling_Timer);
			POPULATE_CLASS_FUNC(paws_sm, set_maxPollingQuick);
			POPULATE_CLASS_FUNC(paws_sm, Start_maxPollingQuick_Timer);
			POPULATE_CLASS_FUNC(paws_sm, Stop_maxPollingQuick_Timer);
			POPULATE_CLASS_FUNC(paws_sm, Start_SpectrumPending_Timer);
			POPULATE_CLASS_FUNC(paws_sm, Stop_SpectrumPending_Timer);
			POPULATE_CLASS_FUNC(paws_sm, activatePendingSpectrum);
			POPULATE_CLASS_FUNC(paws_sm, Start_SpectrumExpiry_Timer);
			POPULATE_CLASS_FUNC(paws_sm, Stop_SpectrumExpiry_Timer);
			POPULATE_CLASS_FUNC(paws_sm, combiner_selectDB);
			POPULATE_CLASS_FUNC(paws_sm, combiner_DBerror);
			// definition of static class functions which are not called directly by STL
			POPULATE_CLASS_FUNC(paws_sm, Check_HTTP_Result);
			POPULATE_CLASS_FUNC(paws_sm, combiner_ProcessDbUpdate);
			POPULATE_CLASS_FUNC(paws_sm, set_db);
			POPULATE_CLASS_FUNC(paws_sm, Check_Valid_Resp);
			POPULATE_CLASS_FUNC(paws_sm, Process_Available_Spectrum_Resp);
			POPULATE_CLASS_FUNC(paws_sm, GetAvailSpectrumResp);
			POPULATE_CLASS_FUNC(paws_sm, checkAvailableSpectrum);
			POPULATE_CLASS_FUNC(paws_sm, set_selected_spectrum);
			POPULATE_CLASS_FUNC(paws_sm, ActivateSpectrumRuleset);
			POPULATE_CLASS_FUNC(paws_sm, clear_Ruleset);
			POPULATE_CLASS_FUNC(paws_sm, get_max_location_change);
			POPULATE_CLASS_FUNC(paws_sm, get_max_polling_secs);
			POPULATE_CLASS_FUNC(paws_sm, Process_Notify_Use_Resp);
			POPULATE_CLASS_FUNC(paws_sm, timeout_handler);
			POPULATE_CLASS_FUNC(paws_sm, Retry_Timer_Hdlr);
			POPULATE_CLASS_FUNC(paws_sm, maxPolling_Timer_Hdlr);
			POPULATE_CLASS_FUNC(paws_sm, maxPollingQuick_Timer_Hdlr);
			POPULATE_CLASS_FUNC(paws_sm, SpectrumPending_Timer_Hdlr);
			POPULATE_CLASS_FUNC(paws_sm, SpectrumExpiry_Timer_Hdlr);
			POPULATE_CLASS_FUNC(paws_sm, set_master_info);
			POPULATE_CLASS_FUNC(paws_sm, set_defaultInfo);
			POPULATE_CLASS_FUNC(paws_sm, set_gps);
			POPULATE_CLASS_FUNC(paws_sm, gps_changed);
			POPULATE_CLASS_FUNC(paws_sm, event_id_2_str);
			POPULATE_CLASS_FUNC(paws_sm, state_changed_dbg);
			POPULATE_CLASS_FUNC(paws_sm, event_processed_dbg);
			POPULATE_CLASS_FUNC(paws_sm, set_LogInfo);
			POPULATE_CLASS_FUNC(paws_sm, set_msgLogInfo);
			POPULATE_CLASS_FUNC(paws_sm, app_log);
			POPULATE_CLASS_FUNC(paws_sm, app_log_tvwsdb_msg);
			POPULATE_CLASS_FUNC(paws_sm, msg_log_tvwsdb_msg);
			POPULATE_CLASS_FUNC(paws_sm, raise_);
			POPULATE_CLASS_FUNC(paws_sm, process_events);
			POPULATE_CLASS_FUNC(paws_sm, process_next_timer_tick);
			POPULATE_CLASS_FUNC(paws_sm, Init);
			POPULATE_CLASS_FUNC(paws_sm, stl_init);
			POPULATE_CLASS_FUNC(paws_sm, stl_set_state_explicit);

			return paws_sm;
		}
	}
	return NULL;
}


static void paws_sm_data_free(paws_sm_t* paws_sm, paws_sm_data_t* data)
{
	FUNC_DBG(paws_sm);

	if (data)
	{
		// loggers
		if (data->msg_log.logger)
			logger_free(&data->msg_log.logger);

		// avail_spectrum_resp
		avail_spectrum_free(&data->avail_spectrum_resp);

		// available_spectrum
		spec_cfg_free(&data->available_spectrum);

		// pending_spectrum 
		spec_cfg_free(&data->pending_spectrum);

		// selected_spectrum
		spec_cfg_free(&data->selected_spectrum);

		// spectrum_in_use
		spec_cfg_free(&data->spectrum_in_use);

		// zero it
		memset(data, 0, sizeof(paws_sm_data_t));
	}
}


static void paws_sm_stl_data_free(paws_sm_t* paws_sm, paws_sm_stl_data_t* data)
{
	FUNC_DBG(paws_sm);

	if (data)
	{
		// stl
		sm_delete(data->stl);

		// timer manager
		timer_manager_delete(data->timer_manager);

		// zero it
		memset(data, 0, sizeof(paws_sm_stl_data_t));
	}
}


void paws_sm_free(paws_sm_t** paws_sm)
{
	FUNC_DBG(*paws_sm);

	// check that state machine exists
	if ((paws_sm) && (*paws_sm))
	{
		// free the stl
		paws_sm_stl_data_free(*paws_sm, (*paws_sm)->paws_sm_hdr.stl_data);

		// free the data attributes
		paws_sm_data_free(*paws_sm, (*paws_sm)->paws_sm_hdr.data);

		// free the struct
		free_and_null((void**)paws_sm);
	}
}












