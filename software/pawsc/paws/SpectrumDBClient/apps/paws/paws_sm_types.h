/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_SM_TYPES_H_
#define PAWS_SM_TYPES_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdbool.h>

#include "utils/types.h"
#include "json-parser/json.h"
#include "gps/gps.h"
#include "state-machine/state-machine.h"
#include "timers/timer.h"
#include "logger/logger.h"
#include "logger/cloud_logger.h"

#include "paws_types.h"


typedef void		(*paws_sm_func)(void* sm_);
typedef const char* (*IdToString)(void* sm_, int id);

#define PAWS_SM_FUNC_PROTO(fnc) static void fnc(void* sm_);

typedef struct {
	logger_cfg_t		cfg;
	void*				logger;
} paws_sm_logger_t;

typedef struct {
	// defintion of all static class functions called from STL
	void			(*store_state)(void* sm_, sm_state_info_t* sm_state_info);
	void			(*free_state)(void* sm_, sm_state_info_t* sm_state_info);
	paws_sm_func	read_state;
	void			(*process_state_attributes)(void* sm_, sm_state_info_t* state_info);
	void			(*process_state_timers)(void* sm_, char* timer_info);
	paws_sm_func	Start;
	paws_sm_func	Init_states;
	paws_sm_func	Clear_Spectrums;
	paws_sm_func	DLULSpectrumAvailable;
	paws_sm_func	DLULSpectrumNotAvailable;
	paws_sm_func	set_spectrum_InUse;
	paws_sm_func	clear_spectrum_InUse;
	paws_sm_func	check_spectrum_InUse;
	paws_sm_func	ActivateSpectrumRuleset;
	paws_sm_func	check_Notify_Required;
	paws_sm_func	Start_Retry_Timer;
	paws_sm_func	Stop_Retry_Timer;
	paws_sm_func	Start_maxPolling_Timer;
	paws_sm_func	Stop_maxPolling_Timer;
	paws_sm_func	Start_maxPollingQuick_Timer;
	paws_sm_func	Stop_maxPollingQuick_Timer;
	paws_sm_func	Start_SpectrumPending_Timer;
	paws_sm_func	Stop_SpectrumPending_Timer;
	paws_sm_func	activatePendingSpectrum;
	paws_sm_func	Start_SpectrumExpiry_Timer;
	paws_sm_func	Stop_SpectrumExpiry_Timer;
	paws_sm_func	combiner_selectDB;
	paws_sm_func	combiner_DBerror;
	// defintion of static class functions which are not called directly by STL
	bool			(*Check_HTTP_Result)(void* sm_, json_value* resp, bool raise);
	void			(*combiner_ProcessDbUpdate)(void* sm_, json_value* new_db_spec);
	void			(*set_db)(void* sm_, paws_db_item_t* db);
	bool			(*Check_Valid_Resp)(void* sm_, json_value* resp, bool raise);
	void			(*Process_Available_Spectrum_Resp)(void* sm_, json_value* resp, lte_direction_e dir, time_t resp_time);
	avail_spectrum_t* (*GetAvailSpectrumResp)(void* sm_);
	void			(*checkAvailableSpectrum)(void* sm_);
	bool			(*set_selected_spectrum)(void* sm_, spec_cfg_t* spec_cfg);
	spec_cfg_t*		(*get_spectrum_InUse)(void* sm_);
	void			(*clear_Ruleset)(void* sm_);
	float			(*get_max_location_change)(void* sm_);
	uint32_t		(*get_max_polling_secs)(void* sm_);
	void			(*Process_Notify_Use_Resp)(void* sm_, json_value* resp);
	void			(*timeout_handler)(void* sm_, uint32_t id);
	void			(*Retry_Timer_Hdlr)(void* sm_);
	void			(*set_maxPollingQuick)(void* sm_, uint32_t dur_secs);
	void			(*maxPolling_Timer_Hdlr)(void* sm_);
	void			(*maxPollingQuick_Timer_Hdlr)(void* sm_);
	void			(*SpectrumPending_Timer_Hdlr)(void* sm_);
	void			(*SpectrumExpiry_Timer_Hdlr)(void* sm_);
	void			(*set_master_info)(void* sm_, paws_device_info_t* master_info);
	void			(*set_defaultInfo)(void* sm_, float max_location_change, uint32_t max_polling_secs);
	void			(*set_gps)(void* sm_, paws_gps_location_t* gps);
	void			(*gps_changed)(void* sm_);
	const char*		(*state_id_2_str)(void* sm_, int id);
	const char*		(*event_id_2_str)(void* sm_, int id);
	void			(*state_changed_dbg)(void* sm_, int old_state, int new_state);
	void			(*event_processed_dbg)(void* sm_, int event_id);
	bool			(*set_LogInfo)(void* sm_, paws_sm_logger_t* logger, logger_cfg_t* cfg);
	bool			(*set_msgLogInfo)(void* sm_, logger_cfg_t* cfg);
	void			(*app_log)(void* sm_, const char* file_, int line_, const char* func_, const char* logline, log_level_e level);
	void			(*app_log_tvwsdb_msg)(void* sm_, const char* msg);
	void			(*msg_log_tvwsdb_msg)(void* sm_, char* cloud_log_j);
	void			(*raise_)(void* sm_, int event_id);
	bool			(*Init)(void* sm_, float min_dbm_100k, uint16_t db_retry_secs, uint32_t max_polling_quick_secs);
	paws_sm_func	process_next_timer_tick;
	int				(*process_events)(void* sm_);
	bool			(*stl_init)(void* sm_, State *state_transition_table, int initial_state, bool run_pre);
	void			(*stl_set_state_explicit)(void* sm_, int state);
} paws_sm_funcs_t;


typedef struct {
	paws_db_item_t	    selected_db;
	uint8_t				db_ok;								
	paws_gps_location_t	gps;

	// log info
	paws_sm_logger_t	msg_log;

	// paws settings
	float				min_dbm_100k;						// min dbm per 100k to qualify for selection
	uint16_t			db_retry_secs;						// how often to retry events (default=15)

	// ruleset
	float				default_max_location_change;
	float				specific_max_location_change;
	uint32_t			default_max_polling_secs;
	uint32_t			specific_max_polling_secs;
	uint32_t			max_polling_quick_secs;


	avail_spectrum_t*	avail_spectrum_resp;				// PAWS SPECTRUM-AVAIL-RESP
	spec_cfg_t*			available_spectrum;					// available, but might not have a corresponding paired spec.
	spec_cfg_t*			pending_spectrum;					// resp has a spectrum, but start_time is in future.
	spec_cfg_t*			selected_spectrum;					// selected to be used
	spec_cfg_t*			spectrum_in_use;					// selected actually being used

	paws_device_info_t	master_info;
} paws_sm_data_t;

typedef struct {
	StateMachine*		stl;
	TimerManager*		timer_manager;
} paws_sm_stl_data_t;

typedef struct {
	device_name_t			unique_id;
	void*				creator;				// state machine which has created this entity
	paws_sm_funcs_t*	funcs;					// THIS MUST BE FIRST.    
	paws_sm_data_t*		data;
	paws_sm_stl_data_t*	stl_data;
} paws_sm_header_t;

typedef struct {
	paws_sm_header_t	paws_sm_hdr;			// THIS MUST BE FIRST IN ANY SM WHICH HAS A PAWS_SM. 
	paws_sm_data_t		paws_sm_data_store;
	paws_sm_stl_data_t	paws_sm_stl_data_store;

	paws_sm_funcs_t		local_funcs;
} paws_sm_t;



#define FOREACH_EV(EV) \
	EV(LocalDB_Error) \
	EV(Start) \
	EV(GPS_location_changed) \
	EV(DB_UpdateTriggered) \
	EV(DB_Error) \
	EV(DB_Updated) \
	EV(DB_Found) \
	EV(DB_NotFound) \
	EV(DB_Response_Failure) \
	EV(Available_Spectrum_Resp) \
	EV(SpectrumAvailable) \
	EV(SpectrumNotAvailable) \
	EV(SpectrumPending) \
	EV(DlUlSpectrumAvailable) \
	EV(DlUlSpectrumNotAvailable) \
	EV(SpectrumActive) \
	EV(SpectrumNotActive) \
	EV(Notification_Required) \
	EV(Notification_Not_Required) \
	EV(Notification_Success) \
	EV(Notification_Failure) \
	EV(Retry_Timeout) \
	EV(maxPolling_Timeout) \
	EV(maxPollingQuick_Timeout) \
	EV(SpectrumPending_Timeout) \
	EV(SpectrumExpiry_Timeout) \
	EV(PAWS_SM_END_EV_MARKER)

#define GEN_EV_ENUM(ENUM) ev_##ENUM,

enum paws_event_e {
	ev_Dummy = SM_EVENT_START_OF_USER_IDS,
	FOREACH_EV(GEN_EV_ENUM)
};


typedef enum {
	RETRY_TIMER_ID = 1,
	SPECTRUM_EXPIRY_TIMER_ID,
	SPECTRUM_PENDING_TIMER_ID,
	MAX_POLLING_TIMER_ID,
	MAX_POLLING_QUICK_TIMER_ID,

	PAWS_SM_TIMER_END
} paws_sm_timers_e;


#endif // #define PAWS_SM_TYPES_H_
