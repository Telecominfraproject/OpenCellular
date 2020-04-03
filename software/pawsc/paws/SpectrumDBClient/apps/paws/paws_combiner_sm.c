/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>



#include "utils/utils.h"
#include "http-client/http.h"
#include "https-client/https.h"
#include "state-machine/state-machine.h"
#include "json-parser/json_utils.h"

#include "paws_dal_types.h"
#include "paws_utils.h"
#include "paws_globals.h"
#include "paws_combiner_sm.h"
#include "paws_sm.h"
#include "paws_master_sm.h"
#include "paws_slave_sm.h"
#include "paws_common.h"
#include "paws_timer_info.h"
#include "paws_messages.h"
#include "lte_utils.h"

#include <openssl/ssl.h>





typedef enum {
	DB_DISCOVERY_TIMER_ID = PAWS_SM_TIMER_END,
	GPS_PERIODIC_TIMER_ID,
	READ_SETTINGS_TIMER_ID,
	READ_DEVICES_TIMER_ID,
	CONTROL_PLANE_TIMER_ID,
	LOCAL_ISSUE_BACKOFF_TIMER_ID
} paws_combiner_sm_timers_e;


// non SM prototypes
static int sop_sm_entity_free(void* sm_entity_);
static int device_name_compare(void* sm_entity_, anytype_u device_name);


// ### static function prototypes
// -- overridden paws_sm functions 
PAWS_SM_FUNC_PROTO(read_state);
PAWS_SM_FUNC_PROTO(process_state);
// -- private functions
// definition of all static class functions called from state machines
PAWS_SM_FUNC_PROTO(setDLSpecState_NotAvail);
PAWS_SM_FUNC_PROTO(setULSpecState_NotAvail);
PAWS_SM_FUNC_PROTO(setDLSpecState_Avail);			
PAWS_SM_FUNC_PROTO(setULSpecState_Avail);
PAWS_SM_FUNC_PROTO(setDLSpecState_NotifSuccess);
PAWS_SM_FUNC_PROTO(setULSpecState_NotifSuccess);
PAWS_SM_FUNC_PROTO(checkSpecStates);
PAWS_SM_FUNC_PROTO(enable_control_plane);
PAWS_SM_FUNC_PROTO(disable_control_plane);
PAWS_SM_FUNC_PROTO(check_gps);
PAWS_SM_FUNC_PROTO(kill_master_sm);
PAWS_SM_FUNC_PROTO(create_master_sm);
PAWS_SM_FUNC_PROTO(kill_slave_sms);
PAWS_SM_FUNC_PROTO(checkMasterInfo);
PAWS_SM_FUNC_PROTO(checkSlavesPresent);
static void checkSlaveInfo(void* sm_, paws_slave_info_t* gops, paws_slave_info_t* sops);
PAWS_SM_FUNC_PROTO(masterDevice_disable);
PAWS_SM_FUNC_PROTO(write_all_state);
PAWS_SM_FUNC_PROTO(process_state_sms);
static ul_dl_spec_cfg_t* select_profile(void* sm_, uint32_t bandwidth, avail_spectrum_t* dl_resp, avail_spectrum_t* ul_resp);
PAWS_SM_FUNC_PROTO(checkSOPSpectrums);
PAWS_SM_FUNC_PROTO(SelectDLULSpectrumGOP);
PAWS_SM_FUNC_PROTO(SelectDLULSpectrumSOP_dl);
PAWS_SM_FUNC_PROTO(SelectDLULSpectrumSOP_ul);
PAWS_SM_FUNC_PROTO(DlSpectrumNotAvailable);
PAWS_SM_FUNC_PROTO(DlSpectrumAvailable);
PAWS_SM_FUNC_PROTO(UlSpectrumNotAvailable);
PAWS_SM_FUNC_PROTO(UlSpectrumAvailable);
PAWS_SM_FUNC_PROTO(UlSOPSpectrumNotAvailable);
static void UlSOPSpectrumAvailable(void* sm_, avail_spectrum_t* spectrum_resp, void* caller);
PAWS_SM_FUNC_PROTO(DlNotificationSuccess);
PAWS_SM_FUNC_PROTO(UlNotificationSuccess);
static void setDefaultInfo(void* sm_, float default_max_location_change, uint32_t default_max_polling_secs);
static void timeout_handler(void* sm_, uint32_t id);
PAWS_SM_FUNC_PROTO(Start_Gps_Periodic_Timer);
PAWS_SM_FUNC_PROTO(Gps_Periodic_Timer_Hdlr);
static void Start_Read_Settings_Timer(void* sm_);
PAWS_SM_FUNC_PROTO(Read_Settings_Timer_Hdlr);
static void Start_Read_Devices_Timer(void* sm_);
PAWS_SM_FUNC_PROTO(Control_Plane_Timer_Hdlr);
static void Start_Control_Plane_Timer(void* sm_);
static void Read_Devices_Timer_Hdlr(void* sm_);
PAWS_SM_FUNC_PROTO(Read_Devices_Timer_Hdlr);
static void Start_LocalIssueBackoff_Timer(void* sm_);
static void LocalIssueBackoff_Timer_Hdlr(void* sm_);
PAWS_SM_FUNC_PROTO(raise_slaveDLULSpectrumAvailable);
PAWS_SM_FUNC_PROTO(raise_masterDLULSpectrumAvailable);
PAWS_SM_FUNC_PROTO(masterDevice_processOverrideCfg);
PAWS_SM_FUNC_PROTO(masterDevice_updateCfg);
PAWS_SM_FUNC_PROTO(create_gop_slave_sm);
static paws_sop_slave_sm_t* create_sop_slave_sm(void* sm_, paws_device_info_t* device_info, sm_state_info_t* sm_state_info);
static void create_start_sop_slave_sm(void* sm_, paws_device_info_t* device_info);
static void delete_sop_slave_sm(void* sm_, device_name_t slave_id);
PAWS_SM_FUNC_PROTO(create_slave_sms);
PAWS_SM_FUNC_PROTO(Start_DBdiscovery_Timer);
PAWS_SM_FUNC_PROTO(DBdiscovery_Timer_Hdlr);
PAWS_SM_FUNC_PROTO(db_discovery);
static char* get_db_discovery_list_http(void* sm_, char* host, char* fname, int *body_offset);
static char* get_db_discovery_list_https(void* sm_, char* host, char* fname, int *body_offset);
static paws_weblist_t* get_db_discovery_list(void* sm_, paws_weblist_url_t* discovery_list_url);
static bool process_db_discovery(void* sm_, paws_weblist_t* weblist);
static void check_db_validity(void* sm_, paws_db_info_t* dbinfo);
static void invalidate_all_db(void* sm_, paws_db_info_t* db_info);
static void select_DB(void* sm_, void* caller);
PAWS_SM_FUNC_PROTO(unBarr_DBs);
PAWS_SM_FUNC_PROTO(Barr_DB);
static void DBerror(void* sm_, void* caller);
PAWS_SM_FUNC_PROTO(LocalDBerror);
static void ProcessDbUpdate(void* sm_, json_value* new_db_spec);
PAWS_SM_FUNC_PROTO(Run);
PAWS_SM_FUNC_PROTO(Init_states);
static bool init_vars(void* sm_);
static bool Init(void* sm_, State* stl, uint32_t control_plane_status_port, uint32_t control_plane_status_periodicity);
static void paws_combiner_private_data_free(paws_combiner_sm_t* sm_, paws_combiner_private_data_t* data);


#define MASTER_SM(sm)					PRIVATE_DATA(sm).paws_master_sm
#define MASTER_LOCAL_FUNC(sm,fnc)		LOCAL_FUNC(MASTER_SM(sm),fnc)
#define GOP_SM(sm)						PRIVATE_DATA(sm).paws_gop_slave_sm
#define GOP_LOCAL_FUNC(sm,fnc)			LOCAL_FUNC(GOP_SM(sm),fnc)

// macro to populate a call to GOP and all SOPS
#define CALL_ALL_SLAVES(sm, fnc, args...) { \
	if (GOP_SM(sm)) \
		{ \
		GOP_LOCAL_FUNC(sm, fnc)(GOP_SM(sm), ##args); \
		/* loop through all SOP SMs, and call func */ \
		if (PRIVATE_DATA(sm).sop_sm_list) \
		{ \
			sop_slave_sm_entity_t* head = llist_get_head(PRIVATE_DATA(sm).sop_sm_list); \
			sop_slave_sm_entity_t* sm_entity; \
			for (sm_entity = head; sm_entity != NULL; sm_entity = get_next_item_entry(sm_entity->l_item)) \
			{ \
				LOCAL_FUNC(sm_entity->sm, fnc)(sm_entity->sm, ##args); \
			} \
		} \
		/* sleep for 1ms - we do this to try and prevent CPU hog */ \
        usleep(1000); \
	} \
}

#define CALL_GOP_SLAVE(sm, fnc, args...) { \
	if (GOP_SM(sm)) \
	{ \
		GOP_LOCAL_FUNC(sm, fnc)(GOP_SM(sm), ##args); \
	} \
}

#define CALL_ALL_SOP_SLAVES(sm, fnc, args...) { \
	if (GOP_SM(sm)) \
	{ \
		/* loop through all SOP SMs, and call func */ \
		if (PRIVATE_DATA(sm).sop_sm_list) \
		{ \
			sop_slave_sm_entity_t* head = llist_get_head(PRIVATE_DATA(sm).sop_sm_list); \
			sop_slave_sm_entity_t* sm_entity; \
			for (sm_entity = head; sm_entity != NULL; sm_entity = get_next_item_entry(sm_entity->l_item)) \
			{ \
				LOCAL_FUNC(sm_entity->sm, fnc)(sm_entity->sm, ##args); \
			} \
		}\
	} \
}



// #############################################################################################################################
// #############################################################################################################################
// ###   STL definition

#define PAWSF(fnc) \
static void _##fnc(void* sm) \
{\
	LOCAL_FUNC((paws_combiner_sm_t*)sm, fnc)(sm); \
}


#define QUO		SM_STATE_NO_CHANGE	

// ######## STL states
#define FOREACH_STATE(STATE) \
	STATE(INIT) \
	STATE(WAIT_FOR_SETTINGS) \
    STATE(WAIT_FOR_GPS) \
    STATE(PREBOOT_STATE) \
    STATE(WAIT_FOR_MASTER) \
	STATE(WAIT_FOR_SLAVES) \
	STATE(HANDLE_SPECTRUMS) \
	STATE(DL_SPEC_AVAIL) \
	STATE(SOPS_SPEC_AVAIL) \
	STATE(LOCAL_ISSUE) \
	STATE(COMBINER_STATE_END_MARKER)

// ######## STL events.  Note also there are common events also defined in paws_sm.h
#define FOREACH_C_EV(EV) \
	EV(SETTINGS) \
	EV(GPS_ACQUIRED) \
	EV(MASTER_DEFINED) \
	EV(MASTER_NOT_DEFINED)\
	EV(SLAVE_DEFINED) \
	EV(SLAVE_NOT_DEFINED) \
	EV(DlSpectrumAvailable) \
	EV(UlSpectrumAvailable) \
	EV(DlSpectrumNotAvailable) \
	EV(UlSpectrumNotAvailable) \
	EV(UlSOPSpectrumAvailable) \
	EV(UlSOPSpectrumNotAvailable) \
	EV(AllSOPs_Available) \
	EV(NotAllSOPs_Available) \
	EV(ULNotificationSuccess) \
	EV(MasterNotificationSuccess) \
	EV(PREBOOT_NOT_PRESENT) \
	EV(PREBOOT_PRESENT) \
	EV(LocalIssueBackOff_Expiry) \
	EV(COMBINER_EV_END_MARKER) 


#define GEN_STATE_ENUM(ENUM) ENUM,
#define GEN_STATE_STR(STRING) #STRING,

enum combiner_state_e {
	cDummyState = SM_STATE_START_OF_USER_IDS,
	FOREACH_STATE(GEN_STATE_ENUM)
};

static const char *combiner_state_str[] = {
	"dummy",
	FOREACH_STATE(GEN_STATE_STR)
};


#define GEN_C_EV_ENUM(ENUM) ev_##ENUM,
#define GEN_C_EV_STR(STRING) #STRING,

enum combiner_event_e {
	ev_cDummy = ev_PAWS_SM_END_EV_MARKER,
	FOREACH_C_EV(GEN_C_EV_ENUM)
};

const char *combiner_ev_str[] = {
	"dummy",
	FOREACH_C_EV(GEN_C_EV_STR)
};

//#######################################################################################
// STL debug functions
static const char* state_id_2_str(void* sm_, int id)
{
	UNUSED_PARAM(sm_);

	if ((id > cDummyState) && (id < COMBINER_STATE_END_MARKER))
	{
		return combiner_state_str[(id - cDummyState)];
	}
	// otherwise its unknown
	return "Unknown";
}

static const char* event_id_2_str(void* sm_, int id)
{
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	if ((id > ev_cDummy) && (id < ev_COMBINER_EV_END_MARKER))
	{
		return combiner_ev_str[(id - ev_cDummy)];
	}
	// otherwise call paws_sm 
	return (PAWS_SM_FUNC(sm, event_id_2_str)(sm_, id));
}

//#######################################################################################


/*                      State-wide            State-wide                                                               Event specific */
/* State                'pre' funcs           'post' funcs     Event                               Next state          'post' funcs    */
static State state_transition_table[] = {
	{ INIT,				 { NULL },              { NULL },     { { ev_Start,                        WAIT_FOR_SETTINGS,   {                                    } } } },
	{ WAIT_FOR_SETTINGS, { Init_states,
                          kill_master_sm, 
                          kill_slave_sms },     { NULL },     { { ev_SETTINGS,                     WAIT_FOR_GPS,        {     masterDevice_processOverrideCfg,
	                                                                                                                                               check_gps } } } },
    { WAIT_FOR_GPS,      { NULL },              { NULL },     { { ev_GPS_ACQUIRED,                 PREBOOT_STATE,       {                                    } },
	                                                            { ev_LocalDB_Error,                LOCAL_ISSUE,         {                                    } } } },
	{ PREBOOT_STATE,     { read_state },        { NULL },     { { ev_PREBOOT_NOT_PRESENT,          WAIT_FOR_MASTER,     {      db_discovery, checkMasterInfo } },
                                                                { ev_PREBOOT_PRESENT,              QUO,                 {                      process_state } },  
                                                                { ev_LocalDB_Error,                LOCAL_ISSUE,         {                                    } } } },
    { WAIT_FOR_MASTER,   { kill_master_sm , 
	                        kill_slave_sms},    { NULL },     { { ev_MASTER_DEFINED,               WAIT_FOR_SLAVES,     {                 checkSlavesPresent } },
	                                                            { ev_LocalDB_Error,                LOCAL_ISSUE,         {                                    } } } },
	{ WAIT_FOR_SLAVES,   { kill_slave_sms },    { NULL },     { { ev_MASTER_NOT_DEFINED,           WAIT_FOR_MASTER,     {                               NULL } },
                                                                { ev_SLAVE_DEFINED,                HANDLE_SPECTRUMS,    {                                    } },
																{ ev_LocalDB_Error,                LOCAL_ISSUE,         {                                    } } } },
	{ HANDLE_SPECTRUMS,  { create_master_sm },  { NULL },     { { ev_MASTER_NOT_DEFINED,           WAIT_FOR_MASTER,     {                               NULL } },
						 										{ ev_SLAVE_NOT_DEFINED,            WAIT_FOR_SLAVES,     {                               NULL } },
																{ ev_DlSpectrumAvailable,          DL_SPEC_AVAIL,       {                   create_slave_sms } },
	                                                            { ev_LocalDB_Error,                LOCAL_ISSUE,         {               masterDevice_disable } } } },
	{ DL_SPEC_AVAIL,    { setDLSpecState_Avail, 
	                      checkSOPSpectrums },  { NULL },     { { ev_MASTER_NOT_DEFINED,           WAIT_FOR_MASTER,     {                               NULL } },
																{ ev_SLAVE_NOT_DEFINED,            WAIT_FOR_SLAVES,     {                               NULL } },
																{ ev_DlSpectrumAvailable,          DL_SPEC_AVAIL,       {                  create_slave_sms,
																                                                                       setDLSpecState_Avail,
																                                                                           checkSOPSpectrums  } },
																{ ev_DlSpectrumNotAvailable,       HANDLE_SPECTRUMS,    {              masterDevice_disable,
 																                                                                     setDLSpecState_NotAvail } },
															    { ev_UlSOPSpectrumAvailable,       QUO,                 {                  checkSOPSpectrums } },
 						                                        { ev_AllSOPs_Available,            SOPS_SPEC_AVAIL,     {           SelectDLULSpectrumSOP_dl } },
																{ ev_NotAllSOPs_Available,         QUO,                 {              SelectDLULSpectrumGOP } },
																{ ev_UlSpectrumAvailable,          QUO,                 {              setULSpecState_Avail,
 																                                                                      SelectDLULSpectrumGOP,
                                                                                                                                        setULSpecState_Avail } },
																{ ev_UlSpectrumNotAvailable,       QUO,                 {              masterDevice_disable,
                                                                                                                                     setULSpecState_NotAvail } },
																{ ev_DlUlSpectrumAvailable,        QUO,                 { raise_masterDLULSpectrumAvailable,
																                                                            raise_slaveDLULSpectrumAvailable } },
																{ ev_DlUlSpectrumNotAvailable,     QUO,                 {               masterDevice_disable } },
                                                                { ev_ULNotificationSuccess,        QUO,                 {        setULSpecState_NotifSuccess } },
                                                                { ev_MasterNotificationSuccess,    QUO,                 {        setDLSpecState_NotifSuccess } },
	                                                            { ev_LocalDB_Error,                LOCAL_ISSUE,         {               masterDevice_disable } } } },
	{ SOPS_SPEC_AVAIL,  {setULSpecState_Avail},  {NULL},      { { ev_MASTER_NOT_DEFINED,           WAIT_FOR_MASTER,     {                               NULL } },
																{ ev_SLAVE_NOT_DEFINED,            WAIT_FOR_SLAVES,     {                               NULL } },
																{ ev_DlSpectrumAvailable,          QUO,                 {              setDLSpecState_Avail,
																                                                                    SelectDLULSpectrumSOP_dl } },
																{ ev_DlSpectrumNotAvailable,       HANDLE_SPECTRUMS,    {              masterDevice_disable,
																                                                                     setDLSpecState_NotAvail } },
															    { ev_UlSOPSpectrumAvailable,       QUO,                 {              setULSpecState_Avail,
																                                                                    SelectDLULSpectrumSOP_ul } },
																{ ev_UlSOPSpectrumNotAvailable,    DL_SPEC_AVAIL,       {              SelectDLULSpectrumGOP } },

																{ ev_UlSpectrumAvailable,			QUO,				 {		 	    setULSpecState_Avail,
                                                                                                                                       SelectDLULSpectrumGOP,
                                                                                                                                        setULSpecState_Avail } },

																{ ev_DlUlSpectrumAvailable,        QUO,                 { raise_masterDLULSpectrumAvailable,
																                                                            raise_slaveDLULSpectrumAvailable } },
																{ ev_DlUlSpectrumNotAvailable,     DL_SPEC_AVAIL,       {              SelectDLULSpectrumGOP } },
                                                                { ev_ULNotificationSuccess,        QUO,                 {        setULSpecState_NotifSuccess } },
                                                                { ev_MasterNotificationSuccess,    QUO,                 {        setDLSpecState_NotifSuccess } },
	                                                            { ev_LocalDB_Error,                LOCAL_ISSUE,         {               masterDevice_disable } } } },
	{ LOCAL_ISSUE, {Start_LocalIssueBackoff_Timer}, {NULL},   { { ev_LocalIssueBackOff_Expiry,     WAIT_FOR_SETTINGS,   {                               NULL } } } },
	{ SM_STATE_INVALID, { NULL },               { NULL },     { { SM_EVENT_INVALID,                SM_EVENT_INVALID,    {                               NULL } } } }
};


//#######################################################################################
static bool init_vars(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	if (!(PAWS_SM_FUNC(sm, Init)(sm_, PRIVATE_DATA(sm).paws_settings.min_dl_dbm_100k, PRIVATE_DATA(sm).paws_settings.db_retry_secs, PRIVATE_DATA(sm).paws_settings.max_polling_quick_secs)))
	{
		return false;
	}
	return true;
}

//#######################################################################################
static void Init_states(void* sm_)
{
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	// direct call
	PAWS_SM_FUNC(sm, Init_states)(sm);

	// local
	paws_combiner_private_data_free(sm, &(PRIVATE_DATA(sm)));

	// initialise variables
	if (!(init_vars(sm)))
	{
		LOG_PRINT(sm_, LOG_ERROR, "Problem in init_vars");
		return ;
	}

	// create the SOP SM list
	if (!(PRIVATE_DATA(sm).sop_sm_list = llist_new()))
	{
		LOG_PRINT(sm_, LOG_ERROR, "Problem in llist_new");
		return;
	}

	// create timers
	PRIVATE_DATA(sm).db_discovery_info.db_discovery_duration = DB_DISCOVERY_TIMER_INITIAL_DURATION;
	if ((timer_manager_create_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, DB_DISCOVERY_TIMER_ID, PRIVATE_DATA(sm).db_discovery_info.db_discovery_duration)) < 0)
	{
		LOG_PRINT(sm_, LOG_ERROR, "Problem in timer_manager_create_timer DB_DISCOVERY_TIMER_ID");
		return ;
	}

	PRIVATE_DATA(sm).gps_periodic_duration = PERIODIC_GPS_DEVICES_INITIAL_DURATION;
	if ((timer_manager_create_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, GPS_PERIODIC_TIMER_ID, PRIVATE_DATA(sm).gps_periodic_duration)) < 0)
	{
		LOG_PRINT(sm_, LOG_ERROR, "Problem in timer_manager_create_timer GPS_PERIODIC_TIMER_ID");
		return ;
	}

	PRIVATE_DATA(sm).paws_settings.setting_periodic_secs = DB_SETTINGS_INITIAL_DURATION;
	if ((timer_manager_create_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, READ_SETTINGS_TIMER_ID, PRIVATE_DATA(sm).paws_settings.setting_periodic_secs)) < 0)
	{
		LOG_PRINT(sm_, LOG_ERROR, "Problem in timer_manager_create_timer READ_SETTINGS_TIMER_ID");
		return ;
	}

	PRIVATE_DATA(sm).paws_settings.devices_periodic_secs = DB_DEVICES_INITIAL_DURATION;
	if ((timer_manager_create_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, READ_DEVICES_TIMER_ID, PRIVATE_DATA(sm).paws_settings.devices_periodic_secs)) < 0)
	{
		LOG_PRINT(sm_, LOG_ERROR, "Problem in timer_manager_create_timer READ_DEVICES_TIMER_ID");
		return ;
	}

	// create control_plane
	if (!(PRIVATE_DATA(sm).control_plane = paws_dal_control_plane_create(&sm->control_plane_cfg))) 
	{
		LOG_PRINT(sm_, LOG_ERROR, "Problem in paws_dal_control_plane_create");
		return;
	}

	if ((timer_manager_create_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, CONTROL_PLANE_TIMER_ID, sm->control_plane_cfg.status_periodicity)) < 0)
	{
		LOG_PRINT(sm_, LOG_ERROR, "Problem in timer_manager_create_timer CONTROL_PLANE_TIMER_ID");
		return ;
	}

	if ((timer_manager_create_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, LOCAL_ISSUE_BACKOFF_TIMER_ID, LOCAL_ISSUE_BACKOFF_TIMER_DURATION)) < 0)
	{
		LOG_PRINT(sm_, LOG_ERROR, "Problem in timer_manager_create_timer LOCAL_ISSUE_BACKOFF_TIMER_ID");
		return;
	}

	// first of all, read settings
	Read_Settings_Timer_Hdlr(sm_);

	// try to get devices
	Read_Devices_Timer_Hdlr(sm_);

	// call gps again to double check that device hasn't been moved whilst in reboot
	Gps_Periodic_Timer_Hdlr(sm_);

	// start sending control plane status
	Control_Plane_Timer_Hdlr(sm_);
}


//#######################################################################################
static bool Init(void* sm_, State* stl, uint32_t control_plane_status_port, uint32_t control_plane_status_periodicity)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	// generic Init
	if (!(LOCAL_FUNC(sm, stl_init)(sm_, stl, INIT, true)))		
	{
		return false;
	}

	// set up any control plane config
	strcpy(sm->control_plane_cfg.status_ipaddr, "127.0.0.1");
	sm->control_plane_cfg.status_port = control_plane_status_port;
	sm->control_plane_cfg.status_periodicity = control_plane_status_periodicity;

	return true;
}


//#######################################################################################
static void read_state(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	if ((PRIVATE_DATA(sm).state_info = paws_read_state()))
	{
		LOCAL_FUNC(sm, raise_)(sm_, ev_PREBOOT_PRESENT);

		// clear the state from the database
		paws_remove_state();
	}
	else
		LOCAL_FUNC(sm, raise_)(sm_, ev_PREBOOT_NOT_PRESENT);
}



//#######################################################################################
// must match llist data_callback_func2 prototype
static int state_info_device_name_compare(void* sm_entity_, anytype_u device_name)
{ 
	sm_state_info_t* sm_entity = (sm_state_info_t*)sm_entity_;
	if ((sm_entity) && (device_name.p))
	{
		int ret = strcmp(sm_entity->unique_id, device_name.p);
		if (ret < 0)
			return -1;
		else if (ret == 0)
			return 0;
		else
			return 1;
	}
	return -1;
}


//#######################################################################################
static void process_state(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	// handle pre-boot state
	// virtually copy to paws_sm->state_info, and then call the parent functions

	if (PRIVATE_DATA(sm).state_info)
	{
		// get the combiner state_info
		anytype_u device_name;
		static char* name = "combiner";
		device_name.p = name;
		sm_state_info_t* sm_state = NULL;
		if ((sm_state = llist_get_entry(PRIVATE_DATA(sm).state_info->state_info_ll, state_info_device_name_compare, device_name)))
		{
			// db discovery list
			memcpy(&PRIVATE_DATA(sm).db_discovery_info, &PRIVATE_DATA(sm).state_info->db_discovery_info, sizeof(paws_db_discovery_info_t));

			// set stl state
			LOCAL_FUNC(sm, stl_set_state_explicit)(sm_, sm_state->stl_current_state);
			// save state attributes
			LOCAL_FUNC(sm, process_state_attributes)(sm_, sm_state);
			// set explicit timers 
			LOCAL_FUNC(sm, process_state_timers)(sm_, sm_state->timer_info);

			// create master and slaves if needed
			process_state_sms(sm_);

			// handle spec states
			PRIVATE_DATA(sm).dl_spec_state = PRIVATE_DATA(sm).state_info->dl_spec_state;
			PRIVATE_DATA(sm).ul_spec_state = PRIVATE_DATA(sm).state_info->dl_spec_state;
			// handle spec
			PRIVATE_DATA(sm).ul_dl_spec = ul_dl_spec_cfg_vcopy(PRIVATE_DATA(sm).state_info->ul_dl_spec);

			checkSpecStates(sm_);
		}
	}
	// remove state_info    
	paws_sm_state_info_free(&PRIVATE_DATA(sm).state_info);

}


//#######################################################################################
static void process_state_sms(void* sm_)
{
	FUNC_DBG(sm_);
	
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	// loop through the list and create the SMs 
	if ((PRIVATE_DATA(sm).state_info) && (PRIVATE_DATA(sm).state_info->state_info_ll))
	{
		sm_state_info_t* head = llist_get_head(PRIVATE_DATA(sm).state_info->state_info_ll);
		sm_state_info_t* sm_state;
		for (sm_state = head; sm_state != NULL; sm_state = get_next_item_entry(sm_state->l_item))
		{
			if (strcmp(sm_state->unique_id, "combiner") == 0)
				continue;
			else if (strcmp(sm_state->unique_id, "master") == 0)
			{
				MASTER_SM(sm) = paws_master_sm_create(sm, "master", &PAWS_SM_DATA(sm_)->master_info, &PAWS_SM_DATA(sm_)->gps,
					PRIVATE_DATA(sm).paws_settings.min_dl_dbm_100k, PRIVATE_DATA(sm).paws_settings.db_retry_secs,
					&PRIVATE_DATA(sm).paws_settings.loginfo.msg_log, PRIVATE_DATA(sm).paws_settings.max_polling_quick_secs, sm_state);
			}
			else if (strcmp(sm_state->unique_id, "gop") == 0)
			{
				GOP_SM(sm) = paws_gop_slave_sm_create(sm, "gop", &PAWS_SM_DATA(sm_)->master_info, &PAWS_SM_DATA(sm_)->gps,
					PAWS_SM_DATA(sm_)->default_max_location_change, PAWS_SM_DATA(sm_)->default_max_polling_secs, &PRIVATE_DATA(sm).gop_slave_info, PRIVATE_DATA(sm).paws_settings.min_ul_dbm_100k,
					PRIVATE_DATA(sm).paws_settings.db_retry_secs, &PRIVATE_DATA(sm).paws_settings.loginfo.msg_log, PRIVATE_DATA(sm).paws_settings.max_polling_quick_secs, sm_state);
			}
			else
			{	// must be a SOP

				// get the device_info from the sop_slave_info
				int i = 0;
				paws_device_info_t* dev = NULL;
				for (i = 0; i < PRIVATE_DATA(sm).sop_slave_info.num_devices; i++)
				{
					dev = &PRIVATE_DATA(sm).sop_slave_info.device_info[i];
					if (strcmp(dev->unique_id, sm_state->unique_id) == 0)
					{
						// confirm it has a GPS
						if ((strcmp(dev->gps.device_name, dev->unique_id) == 0) && (dev->gps.fixed))
						{
							// call function to create SM and add to list
							create_sop_slave_sm(sm, dev, sm_state);
						}
						break;
					}
				}
			}
		}
	}
}


//#######################################################################################
static void setDLSpecState_NotAvail(void* sm_)
{
	FUNC_DBG(sm_);
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;
	PRIVATE_DATA(sm).dl_spec_state = SPEC_STATE_NOT_AVAIL;
	LOG_PRINT(sm, LOG_NOTICE, "dl_spec_state = SPEC_STATE_NOT_AVAIL");
}


//#######################################################################################
static void setULSpecState_NotAvail(void* sm_)
{
	FUNC_DBG(sm_);
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;
	PRIVATE_DATA(sm).ul_spec_state = SPEC_STATE_NOT_AVAIL;
	LOG_PRINT(sm, LOG_NOTICE, "ul_spec_state = SPEC_STATE_NOT_AVAIL");
}


//#######################################################################################
static void setDLSpecState_Avail(void* sm_)
{
	FUNC_DBG(sm_);
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;
	PRIVATE_DATA(sm).dl_spec_state = SPEC_STATE_AVAIL;
	LOG_PRINT(sm, LOG_NOTICE, "dl_spec_state = SPEC_STATE_AVAIL");
}


//#######################################################################################
static void setULSpecState_Avail(void* sm_)
{
	FUNC_DBG(sm_);
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;
	PRIVATE_DATA(sm).ul_spec_state = SPEC_STATE_AVAIL;
	LOG_PRINT(sm, LOG_NOTICE, "ul_spec_state = SPEC_STATE_AVAIL");
}


//#######################################################################################
static void setDLSpecState_NotifSuccess(void* sm_)
{
	FUNC_DBG(sm_);
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;
	if (PRIVATE_DATA(sm).dl_spec_state != SPEC_STATE_NOTIFICATION_SUCCESS)
	{
		PRIVATE_DATA(sm).dl_spec_state = SPEC_STATE_NOTIFICATION_SUCCESS;
		LOG_PRINT(sm, LOG_NOTICE, "dl_spec_state = SPEC_STATE_NOTIFICATION_SUCCESS");
		checkSpecStates(sm_);
	}
}


//#######################################################################################
static void setULSpecState_NotifSuccess(void* sm_)
{
	FUNC_DBG(sm_);
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;
	if (PRIVATE_DATA(sm).ul_spec_state != SPEC_STATE_NOTIFICATION_SUCCESS)
	{
		PRIVATE_DATA(sm).ul_spec_state = SPEC_STATE_NOTIFICATION_SUCCESS;
		LOG_PRINT(sm, LOG_NOTICE, "ul_spec_state = SPEC_STATE_NOTIFICATION_SUCCESS");
		checkSpecStates(sm_);
	}
}


//#######################################################################################
static void checkSpecStates(void* sm_)
{
	FUNC_DBG(sm_);
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;
	if ((PRIVATE_DATA(sm).dl_spec_state == SPEC_STATE_NOTIFICATION_SUCCESS) && (PRIVATE_DATA(sm).ul_spec_state == SPEC_STATE_NOTIFICATION_SUCCESS))
	{
		masterDevice_updateCfg(sm_);
	}
}

//#######################################################################################
static void enable_control_plane(void* sm_)
{
	FUNC_DBG(sm_);
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;
	LOG_PRINT(sm, LOG_NOTICE, "Enabling control-plane");
	PRIVATE_DATA(sm).control_plane_enabled = true;
	Control_Plane_Timer_Hdlr(sm_);
}



//#######################################################################################
static void disable_control_plane(void* sm_)
{
	FUNC_DBG(sm_);
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;
	if (!(PRIVATE_DATA(sm).paws_settings.spectrum_override.present))
	{
		LOG_PRINT(sm, LOG_NOTICE, "Disabling control-plane");
		PRIVATE_DATA(sm).control_plane_enabled = false;
	}
	else
	{
		LOG_PRINT(sm, LOG_NOTICE, "Override set, so control plane will not be disabled");
	}
	Control_Plane_Timer_Hdlr(sm_);
}


//#######################################################################################
static void check_gps(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	paws_gps_location_t new_gps;

	paws_read_gps(&new_gps, gDeviceName);

	if (!(new_gps.fixed))
	{
		LOG_PRINT(sm, LOG_ERROR, "GPS not acquired");
	}
	else
	{
		LOG_PRINT(sm, LOG_INFO, "GPS: fixed:   lat:%.6f long:%.6f height:%d heightType:%s",
			new_gps.latitude, new_gps.longitude, new_gps.height, new_gps.height_type);
	}

	memcpy(&PAWS_SM_DATA((paws_sm_t*)sm_)->gps, &new_gps, sizeof(paws_gps_location_t));

	if (PRIVATE_DATA(sm).paws_settings_read)
	{
		PRIVATE_DATA(sm).gps_periodic_duration = (new_gps.fixed) ? PRIVATE_DATA(sm).paws_settings.gps_periodic_slow_check_secs : PRIVATE_DATA(sm).paws_settings.gps_periodic_fast_check_secs;
	}
	timer_manager_set_duration(PAWS_SM_STL_DATA(sm_)->timer_manager, GPS_PERIODIC_TIMER_ID, PRIVATE_DATA(sm).gps_periodic_duration);

	// update master and slaves
	if (MASTER_SM(sm))
	{
		MASTER_LOCAL_FUNC(sm,set_gps)(MASTER_SM(sm), &(PAWS_SM_DATA((paws_sm_t*)sm_)->gps));
	}
	if (GOP_SM(sm))
	{
		MASTER_LOCAL_FUNC(sm, set_gps)(GOP_SM(sm), &(PAWS_SM_DATA((paws_sm_t*)sm_)->gps));
	}

	if (PAWS_SM_DATA((paws_sm_t*)sm_)->gps.fixed)
	{
		LOCAL_FUNC(sm, raise_)(sm_, ev_GPS_ACQUIRED);
	}
}


//#######################################################################################
static void kill_master_sm(void* sm_)
{
	FUNC_DBG(sm_);
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;
	if (MASTER_SM(sm))
	{
		paws_master_sm_free(&MASTER_SM(sm));
	}
}


//#######################################################################################
static void create_master_sm(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;
	if (!MASTER_SM(sm))
	{
		MASTER_SM(sm) = paws_master_sm_create(sm, "master", &PAWS_SM_DATA(sm_)->master_info, &PAWS_SM_DATA(sm_)->gps, PRIVATE_DATA(sm).paws_settings.min_dl_dbm_100k, 
			PRIVATE_DATA(sm).paws_settings.db_retry_secs, &PRIVATE_DATA(sm).paws_settings.loginfo.msg_log, PRIVATE_DATA(sm).paws_settings.max_polling_quick_secs, NULL);
		MASTER_LOCAL_FUNC(sm, Start)(MASTER_SM(sm));
	}
}


//#######################################################################################
static void kill_slave_sms(void* sm_)
{
	FUNC_DBG(sm_);
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;
	if (GOP_SM(sm))
	{
		paws_gop_slave_sm_free(&GOP_SM(sm));
	}
	
	// free the sop slaves sms
	if (PRIVATE_DATA(sm).sop_sm_list)
	{
		llist_clear(PRIVATE_DATA(sm).sop_sm_list, sop_sm_entity_free);
	}
}


//#######################################################################################
static void checkMasterInfo(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	if (PRIVATE_DATA(sm).master_info_read)
	{
		LOCAL_FUNC(sm, raise_)(sm_, ev_MASTER_DEFINED);
	}
	else
	{
		LOG_PRINT(sm, LOG_ERROR, "Master DeviceInfo not configued in PAWS database");
		LOCAL_FUNC(sm, raise_)(sm_, ev_MASTER_NOT_DEFINED);
	}
}


//#######################################################################################
static void checkSlavesPresent(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	if ((PRIVATE_DATA(sm).gop_slave_info.num_devices > 0) || (PRIVATE_DATA(sm).sop_slave_info.num_devices > 0))
	{
		LOCAL_FUNC(sm, raise_)(sm_, ev_SLAVE_DEFINED);
	}
	else
	{
		LOG_PRINT(sm, LOG_ERROR, "No slave DeviceInfo not configued in PAWS database");
		LOCAL_FUNC(sm, raise_)(sm_, ev_SLAVE_NOT_DEFINED);
	}
}
	


//#######################################################################################
// this function will determine if any SOPs need to be added or deleted
static void checkSlaveInfo(void* sm_, paws_slave_info_t* gops, paws_slave_info_t* sops)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	if (GOP_SM(sm))
	{
		// GOP is up and running so we can add/delete SMs as required
		// we walk through the "current" and "new" lists and determine which devices have been added/deleted.  This algorithm works as we always store the devices "by device-id" in alphabetical order
		uint16_t curr_idx = 0;
		uint16_t new_idx = 0;
		paws_device_info_t* curr_dev = (curr_idx == PRIVATE_DATA(sm).sop_slave_info.num_devices) ? NULL : &PRIVATE_DATA(sm).sop_slave_info.device_info[curr_idx];
		paws_device_info_t* new_dev = (new_idx == sops->num_devices) ? NULL : &sops->device_info[new_idx];

		while (new_dev || curr_dev)
		{
			if (!new_dev)
			{
				// curr_dev has to be deleted
				delete_sop_slave_sm(sm_, curr_dev->unique_id);
				curr_idx++;
				curr_dev = (curr_idx == PRIVATE_DATA(sm).sop_slave_info.num_devices) ? NULL : &PRIVATE_DATA(sm).sop_slave_info.device_info[curr_idx];
			}
			else if (!curr_dev)
			{
				// new_dev has to be added
				create_start_sop_slave_sm(sm_, new_dev);
				new_idx++;
				new_dev = (new_idx == sops->num_devices) ? NULL : &sops->device_info[new_idx];
			}
			else if (strcmp(new_dev->unique_id, curr_dev->unique_id) < 0)
			{
				// new_dev is lower than current dev so is new
				create_start_sop_slave_sm(sm_, new_dev);
				new_idx++;
				new_dev = (new_idx == sops->num_devices) ? NULL : &sops->device_info[new_idx];
			}
			else if (strcmp(new_dev->unique_id, curr_dev->unique_id) > 0)
			{
				// new_dev is greater than current dev, so current dev should be removed
				delete_sop_slave_sm(sm_, curr_dev->unique_id);
				curr_idx++;
				curr_dev = (curr_idx == PRIVATE_DATA(sm).sop_slave_info.num_devices) ? NULL : &PRIVATE_DATA(sm).sop_slave_info.device_info[curr_idx];
			}
			else
			{
				// they match so just move both pointers on
				curr_idx++;
				curr_dev = (curr_idx == PRIVATE_DATA(sm).sop_slave_info.num_devices) ? NULL : &PRIVATE_DATA(sm).sop_slave_info.device_info[curr_idx];
				new_idx++;
				new_dev = (new_idx == sops->num_devices) ? NULL : &sops->device_info[new_idx];
			}
		}

		// write new GOPs to GOP_SM
		GOP_FUNC(GOP_SM(sm), set_slaveInfo)(GOP_SM(sm), gops);
	}

	// copy data over to main storage
	memcpy(&PRIVATE_DATA(sm).gop_slave_info, gops, sizeof(paws_slave_info_t));
	memcpy(&PRIVATE_DATA(sm).sop_slave_info, sops, sizeof(paws_slave_info_t));
}



//#######################################################################################
static void write_all_state(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	paws_sm_state_info_t paws_sm_state_info;
	memset(&paws_sm_state_info, 0, sizeof(paws_sm_state_info));

	paws_sm_state_info.dl_spec_state = PRIVATE_DATA(sm).dl_spec_state;
	paws_sm_state_info.ul_spec_state = PRIVATE_DATA(sm).ul_spec_state;
	memcpy(&paws_sm_state_info.db_discovery_info, &PRIVATE_DATA(sm).db_discovery_info, sizeof(paws_db_discovery_info_t));
	paws_sm_state_info.ul_dl_spec = ul_dl_spec_cfg_vcopy(PRIVATE_DATA(sm).ul_dl_spec);

	// create the state_info_ll 
	if (!(paws_sm_state_info.state_info_ll = llist_new()))
		goto clean_up;

	sm_state_info_t* sm_state_info = NULL;
	
	// combiner
	if (!(sm_state_info = sm_state_info_new()))
		goto clean_up;
	LOCAL_FUNC(sm, store_state)(sm_, sm_state_info);
	if (!(sm_state_info->l_item = llist_append(paws_sm_state_info.state_info_ll, sm_state_info)))
		goto clean_up;
	sm_state_info = NULL;

	// master
	if (MASTER_SM(sm))
	{
		if (!(sm_state_info = sm_state_info_new()))
			goto clean_up;
		MASTER_LOCAL_FUNC(sm, store_state)(MASTER_SM(sm), sm_state_info);
		if (!(sm_state_info->l_item = llist_append(paws_sm_state_info.state_info_ll, sm_state_info)))
			goto clean_up;
		sm_state_info = NULL;
	}

	// GOP
	if (GOP_SM(sm))
	{
		if (!(sm_state_info = sm_state_info_new()))
			goto clean_up;
		GOP_LOCAL_FUNC(sm, store_state)(GOP_SM(sm), sm_state_info);
		if (!(sm_state_info->l_item = llist_append(paws_sm_state_info.state_info_ll, sm_state_info)))
			goto clean_up;
		sm_state_info = NULL;
	}

	// SOPs - loop through the sop_sm_list
	sop_slave_sm_entity_t* head = llist_get_head(PRIVATE_DATA(sm).sop_sm_list);
	sop_slave_sm_entity_t* sm_entity;
	for (sm_entity = head; sm_entity != NULL; sm_entity = get_next_item_entry(sm_entity->l_item))
	{
		if (!(sm_state_info = sm_state_info_new()))
			goto clean_up;
		LOCAL_FUNC(sm_entity->sm, store_state)(sm_entity->sm, sm_state_info);
		if (!(sm_state_info->l_item = llist_append(paws_sm_state_info.state_info_ll, sm_state_info)))
			goto clean_up;
		sm_state_info = NULL;
	}

	paws_write_state(&paws_sm_state_info);

clean_up:
	if (sm_state_info) sm_state_info_free(sm_state_info);
	if (paws_sm_state_info.ul_dl_spec) ul_dl_spec_cfg_free(&paws_sm_state_info.ul_dl_spec);
	if (paws_sm_state_info.state_info_ll) llist_free(&paws_sm_state_info.state_info_ll, sm_state_info_free);
}



//#######################################################################################
static ul_dl_spec_cfg_t* select_spaced_profiles(void* sm_, uint32_t bandwidth, spectrum_spec_t* dl_spec, spectrum_spec_t* ul_spec)
{
	FUNC_DBG(sm_);
	int spacing;
	uint32_t spaced_ul_start, spaced_ul_end;
	uint32_t band_start, band_end;
	uint32_t dl_start, dl_end, ul_start, ul_end;
	float dl_dbm, ul_dbm;
	spectrum_schedule_t* dl_sched = NULL;
	spectrum_schedule_t* ul_sched = NULL;
	spec_profile_type_t* dl_prof1 = NULL;
	spec_profile_type_t* dl_prof2 = NULL;
	spec_profile_type_t* ul_prof1 = NULL;
	spec_profile_type_t* ul_prof2 = NULL;

	if ((!ul_spec) || (!dl_spec))
		return NULL;

	// this function is a quick bodge for band 13

	// get DL info

	// get UL-DL spacing
	if (!(get_lte_band_channel(13, LTE_DL, &band_start, &band_end, &spacing)))
		return NULL;


	// get DL assignments
	if (!(dl_sched = dl_spec->spectrum_schedules))
		return NULL;
	if (!(dl_prof1 = dl_sched->profiles))
		return NULL;
	if (dl_prof1)
		dl_prof2 = dl_prof1->next;

	dl_start = dl_prof1->start_hz;
	dl_end = dl_prof1->end_hz;
	dl_dbm = dl_prof1->dbm;
	if (dl_prof2)
	{
		dl_end = dl_prof2->end_hz;
		if (dl_prof2->dbm < dl_dbm)
			dl_dbm = dl_prof2->dbm;
	}
	// trucate to band
	if (band_start > dl_start)
		dl_start = band_start;
	if (band_end < dl_end)
		dl_end = band_end;

	uint32_t avail_band = dl_end - dl_start;
	if (avail_band < bandwidth)
		return NULL;

	// get UL info
	if (!(get_lte_band_channel(13, LTE_UL, &band_start, &band_end, &spacing)))
		return NULL;

	// get UL assignments
	if (!(ul_sched = ul_spec->spectrum_schedules))
		return NULL;
	if (!(ul_prof1 = ul_sched->profiles))
		return NULL;
	if (ul_prof1)
		ul_prof2 = ul_prof1->next;

	ul_start = ul_prof1->start_hz;
	ul_end = ul_prof1->end_hz;
	ul_dbm = ul_prof1->dbm;
	if (ul_prof2)
	{
		ul_end = ul_prof2->end_hz;
		if (ul_prof2->dbm < ul_dbm)
			ul_dbm = ul_prof2->dbm;
	}
	// trucate to band
	if (band_start > ul_start)
		ul_start = band_start;
	if (band_end < ul_end)
		ul_end = band_end;

	avail_band = ul_end - ul_start;
	if (avail_band < bandwidth)
		return NULL;

	// if we reach here then we know we have enough for bandwidth
	spaced_ul_start = ul_start - spacing;
	spaced_ul_end = ul_end - spacing;

	uint32_t start = (dl_start >= spaced_ul_start) ? dl_start : spaced_ul_start;
	uint32_t end = (dl_end <= spaced_ul_end) ? dl_end : spaced_ul_end;
	uint32_t bw = end - start;
	if (bw < bandwidth)
		return NULL;
	end = start + bandwidth;

	ul_start = start + spacing;
	ul_end = end + spacing;

	// populate return param
	ul_dl_spec_cfg_t* ul_dl_spec = NULL;
	if (!(ul_dl_spec = ul_dl_spec_cfg_new()))
	{
		goto error_hdl;
	};
	if (!(ul_dl_spec->dl_cfg = spec_cfg_new()))
	{
		goto error_hdl;
	};
	if (!(ul_dl_spec->ul_cfg = spec_cfg_new()))
	{
		goto error_hdl;
	};
	ul_dl_spec->dl_cfg->spec = spectrum_spec_vcopy(dl_spec);
	ul_dl_spec->dl_cfg->sched = spectrum_sched_vcopy(dl_sched);
	ul_dl_spec->dl_cfg->start_hz = dl_start;
	ul_dl_spec->dl_cfg->dbm = dl_dbm;
	ul_dl_spec->dl_cfg->bandwidth = bandwidth;
	ul_dl_spec->ul_cfg->spec = spectrum_spec_vcopy(ul_spec);
	ul_dl_spec->ul_cfg->sched = spectrum_sched_vcopy(ul_sched);
	ul_dl_spec->ul_cfg->start_hz = ul_start;
	ul_dl_spec->ul_cfg->dbm = ul_dbm;
	ul_dl_spec->ul_cfg->bandwidth = bandwidth;

	return ul_dl_spec;
	
error_hdl:
	if (ul_dl_spec) ul_dl_spec_cfg_free(&ul_dl_spec);
	return NULL;
}



//#######################################################################################
static ul_dl_spec_cfg_t* select_profile(void* sm_, uint32_t bandwidth, avail_spectrum_t* dl_resp, avail_spectrum_t* ul_resp)
{
	FUNC_DBG(sm_);

	if ((!ul_resp) || (!dl_resp))
		return NULL;
	
	spectrum_spec_t* dl_spec = dl_resp->spectrum_specs;
	while (dl_spec)
	{
		// check if this DL spec allows enough contiguous bandwidth
		if (!(((dl_spec->max_contiguous_bw_hz > 0) && (dl_spec->max_contiguous_bw_hz < bandwidth)) ||		// configured max bandwidth is too small
			(dl_spec->max_contiguous_bw_hz_within_band < bandwidth)))									// allocated max bandwidth is too small
		{
			spectrum_spec_t* ul_spec = ul_resp->spectrum_specs;
			while (ul_spec)
			{
				// check if this UL spec allows enough contiguous bandwidth
				if (!(((ul_spec->max_contiguous_bw_hz > 0) && (ul_spec->max_contiguous_bw_hz < bandwidth)) ||		// configured max bandwidth is too small
					(ul_spec->max_contiguous_bw_hz_within_band < bandwidth)))									// allocated max bandwidth is too small
				{
					ul_dl_spec_cfg_t* ul_dl_spec;
					if ((ul_dl_spec = select_spaced_profiles(sm_, bandwidth, dl_spec, ul_spec)))
					{
						return ul_dl_spec;
					}
				}
				ul_spec = ul_spec->next;
			}
		}
		dl_spec = dl_spec->next;
	}
	return NULL;
}


//#######################################################################################
static ul_dl_spec_cfg_t* get_ul_dl_spectrum(void* sm_, avail_spectrum_t* dl_spectrum_resp, avail_spectrum_t* ul_spectrum_resp, int32_t max_bw)
{
	ul_dl_spec_cfg_t* ul_dl_spec = NULL;
	ul_dl_spec_cfg_t* _5MHz_ul_dl_spec = NULL;
	ul_dl_spec_cfg_t* _10MHz_ul_dl_spec = NULL;

	if (max_bw >= 5000000)	
		_5MHz_ul_dl_spec = select_profile(sm_, 5000000, dl_spectrum_resp, ul_spectrum_resp);

	if (!_5MHz_ul_dl_spec)
		return NULL;

	if (max_bw >= 10000000)
		_10MHz_ul_dl_spec = select_profile(sm_, 10000000, dl_spectrum_resp, ul_spectrum_resp);

	// select 10 if avail, else 5
	if (_10MHz_ul_dl_spec)
	{
		ul_dl_spec_cfg_free(&_5MHz_ul_dl_spec);
		ul_dl_spec = _10MHz_ul_dl_spec;
	}
	else
	{
		ul_dl_spec = _5MHz_ul_dl_spec;
	}

	// anything selected ?
	if (!ul_dl_spec)
		return NULL;
	
	// does the spectrum start immediately
	time_t now_ = time(NULL);
	if (!((ul_dl_spec->dl_cfg->sched->event_time_range.start_time <= now_) && (ul_dl_spec->ul_cfg->sched->event_time_range.start_time <= now_)))
	{
		// the spectrum doesnt start yet

		// Clear any selected.
		ul_dl_spec_cfg_free(&ul_dl_spec);

		return NULL;
	}

	return ul_dl_spec;
}



//#######################################################################################
static void checkSOPSpectrums(void* sm_)
{
	FUNC_DBG(sm_);

	avail_spectrum_t* ul_spectrum_resp = NULL;
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	if (PRIVATE_DATA(sm).gop_slave_info.num_devices > 0)
	{
		LOCAL_FUNC(sm_, raise_)(sm_, ev_NotAllSOPs_Available);
		return;
	}

	// loop through the SOPs, and get the spectrums
	if (!PRIVATE_DATA(sm).sop_sm_list)
	{
		LOCAL_FUNC(sm_, raise_)(sm_, ev_NotAllSOPs_Available);
		return;
	}


	// loop through SOPS and check each has a spectrum
	sop_slave_sm_entity_t* head = llist_get_head(PRIVATE_DATA(sm).sop_sm_list);
	sop_slave_sm_entity_t* sm_entity;
	for (sm_entity = head; sm_entity != NULL; sm_entity = get_next_item_entry(sm_entity->l_item))
	{
		// get the SPECTRUM RESP
		if (!(ul_spectrum_resp = LOCAL_FUNC(sm_entity->sm, GetAvailSpectrumResp)(sm_entity->sm)))
		{
			// no RESP available
			{
				LOCAL_FUNC(sm_, raise_)(sm_, ev_NotAllSOPs_Available);
				return;
			}
		}
	}

	// if we get here, they al had a spectrum
	LOCAL_FUNC(sm_, raise_)(sm_, ev_AllSOPs_Available);
}



//#######################################################################################
static void SelectDLULSpectrumGOP(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	avail_spectrum_t* dl_spectrum_resp = NULL;
	avail_spectrum_t* ul_spectrum_resp = NULL;
	int32_t max_bw = 5000000;		// this is a limitation at the moment

	ul_dl_spec_cfg_t* ul_dl_spec = NULL;

	if (!(dl_spectrum_resp = MASTER_LOCAL_FUNC(sm, GetAvailSpectrumResp)(MASTER_SM(sm))))
	{
		LOCAL_FUNC(sm_, raise_)(sm_, ev_DlSpectrumNotAvailable);
		goto no_spectrum;
	}

	// use GOP
	if (!GOP_SM(sm))
	{
		LOCAL_FUNC(sm_, raise_)(sm_, ev_DlUlSpectrumNotAvailable);
		goto no_spectrum;
	}
		
	if (!(ul_spectrum_resp = GOP_LOCAL_FUNC(sm, GetAvailSpectrumResp)(GOP_SM(sm))))
	{
		LOCAL_FUNC(sm_, raise_)(sm_, ev_UlSpectrumNotAvailable);
		goto no_spectrum;
	}

	ul_dl_spec = get_ul_dl_spectrum(sm_, dl_spectrum_resp, ul_spectrum_resp, max_bw);
	if (!ul_dl_spec)
	{
		LOCAL_FUNC(sm_, raise_)(sm_, ev_DlUlSpectrumNotAvailable);
		goto no_spectrum;
	}
	
	// set DL spectrum
	bool ok = MASTER_LOCAL_FUNC(sm, set_selected_spectrum)(MASTER_SM(sm), ul_dl_spec->dl_cfg);

	// set UL spectrum
	CALL_GOP_SLAVE(sm, set_selected_spectrum, ul_dl_spec->ul_cfg);

	// if GOP spectrum being used for SOPS
	if (ok && ((PRIVATE_DATA(sm).gop_slave_info.num_devices > 0) || (PRIVATE_DATA(sm).num_sop_min == 0)))
	{
		// check if its different from what stored
		if ((ul_dl_spec_cfg_freq_compare(ul_dl_spec, PRIVATE_DATA(sm).ul_dl_spec) != 0) || (ul_dl_spec_cfg_pwr_compare(ul_dl_spec, PRIVATE_DATA(sm).ul_dl_spec) != 0))
		{
			// set UL spectrum
			CALL_ALL_SOP_SLAVES(sm, set_selected_spectrum, ul_dl_spec->ul_cfg);

			// store it
			// free current one if stored
			if (PRIVATE_DATA(sm).ul_dl_spec) ul_dl_spec_cfg_free(&PRIVATE_DATA(sm).ul_dl_spec);
			PRIVATE_DATA(sm).ul_dl_spec = ul_dl_spec;
			ul_dl_spec = NULL;
		}
	}

	// raise the event
	LOCAL_FUNC(sm_, raise_)(sm_, ev_DlUlSpectrumAvailable);

	if (ul_dl_spec) ul_dl_spec_cfg_free(&ul_dl_spec);
	return;

no_spectrum:
	if (ul_dl_spec) ul_dl_spec_cfg_free(&ul_dl_spec);
	if (PRIVATE_DATA(sm).ul_dl_spec) ul_dl_spec_cfg_free(&PRIVATE_DATA(sm).ul_dl_spec);
}




//#######################################################################################
// In "dl" version, we always the "DlUlSpectrumAvailable" event, even if the spectrum is the same
static void SelectDLULSpectrumSOP_dl(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	avail_spectrum_t* dl_spectrum_resp = NULL;
	avail_spectrum_t* ul_spectrum_resp = NULL;
	ul_dl_spec_cfg_t* ul_dl_spec = NULL;

	// This is the logic to select the UP spectrum :-
	// if any GOPs, use GOP spectrum
	// if no GOPS, but one or more of the SOPs has no spectrum, use GOP
	// if no GOPS, and all SOPs have spectrum, use lowest SOP power.

	if (!(dl_spectrum_resp = MASTER_LOCAL_FUNC(sm, GetAvailSpectrumResp)(MASTER_SM(sm))))
	{
		LOCAL_FUNC(sm_, raise_)(sm_, ev_DlSpectrumNotAvailable);
		goto no_spectrum;;
	}

	// if no GOPs, try SOPs
	int32_t max_bw = 5000000;		// this is a limitation at the moment
	if (PRIVATE_DATA(sm).gop_slave_info.num_devices == 0)
	{
		// clear the sop_min info
		PRIVATE_DATA(sm).num_sop_min = 0;

		// loop through the SOPs, and calculate the dl-ul for each
		if (PRIVATE_DATA(sm).sop_sm_list)
		{
			sop_slave_sm_entity_t* head = llist_get_head(PRIVATE_DATA(sm).sop_sm_list);
			sop_slave_sm_entity_t* sm_entity;
			for (sm_entity=head; sm_entity != NULL; sm_entity = get_next_item_entry(sm_entity->l_item))
			{
				// get the SPECTRUM RESP
				if (!(ul_spectrum_resp = LOCAL_FUNC(sm_entity->sm, GetAvailSpectrumResp)(sm_entity->sm)))
				{
					// no RESP available
					// clear ul_dl_spec if one was previously selected
					if (ul_dl_spec)
					{
						ul_dl_spec_cfg_free(&ul_dl_spec);
						break;
					}
				}
				ul_dl_spec_cfg_t* slave_ul_dl_spec = get_ul_dl_spectrum(sm_, dl_spectrum_resp, ul_spectrum_resp, max_bw);

				// if nothing available for this SOP
				if (!slave_ul_dl_spec)
				{
					// Clear any selected.
					ul_dl_spec_cfg_free(&ul_dl_spec);
					break;
				}

				// use this if no ul-dl previously selected
				if (!ul_dl_spec)
				{
					ul_dl_spec = slave_ul_dl_spec;
					// set this SOP as the first sop_min
					PRIVATE_DATA(sm).num_sop_min = 0;
					memset(&PRIVATE_DATA(sm).sop_min, 0, sizeof(PRIVATE_DATA(sm).sop_min));
					PRIVATE_DATA(sm).sop_min[PRIVATE_DATA(sm).num_sop_min++] = sm_entity->sm;
					continue;
				}

				// check it has the same range as previously selected.  If it doesnt, we assume there is a SOP mismatch and therefore we cant use SOPs
				if (slave_ul_dl_spec->ul_cfg->start_hz != ul_dl_spec->ul_cfg->start_hz)
				{
					// there is a mismatch.
					// Clear any selected.
					ul_dl_spec_cfg_free(&slave_ul_dl_spec);
					ul_dl_spec_cfg_free(&ul_dl_spec);
					// clear previous SopMinFlags
					PRIVATE_DATA(sm).num_sop_min = 0;
					break;
				}

				// if this is lower power than current selected, use this one.
				if (slave_ul_dl_spec->ul_cfg->dbm < ul_dl_spec->ul_cfg->dbm)
				{
					// this is lower power so we must use it.

					// clear the previously selected dl_ul spectrum, and use this new one
					ul_dl_spec_cfg_free(&ul_dl_spec);
					ul_dl_spec = slave_ul_dl_spec;

					// set this SOP as the first sop_min
					PRIVATE_DATA(sm).num_sop_min = 0;
					PRIVATE_DATA(sm).sop_min[PRIVATE_DATA(sm).num_sop_min++] = sm_entity->sm;
					continue;
				}
				else if (slave_ul_dl_spec->ul_cfg->dbm == ul_dl_spec->ul_cfg->dbm)
				{
					// its the same Power, so set this as a MinSop too
					// set this SOP as a sop_min too
					PRIVATE_DATA(sm).sop_min[PRIVATE_DATA(sm).num_sop_min++] = sm_entity->sm;
				}

				// if we reach here, we wont use this slave spec, so delete it
				ul_dl_spec_cfg_free(&slave_ul_dl_spec);
			}
		}
	}

	// anything selected ?
	if (!ul_dl_spec)
	{
		LOCAL_FUNC(sm_, raise_)(sm_, ev_DlUlSpectrumNotAvailable);
		goto no_spectrum;;
	}

	// set DL spectrum
	bool ok = MASTER_LOCAL_FUNC(sm, set_selected_spectrum)(MASTER_SM(sm), ul_dl_spec->dl_cfg);

	// check if its different from what stored
	if (ok && ((ul_dl_spec_cfg_freq_compare(ul_dl_spec, PRIVATE_DATA(sm).ul_dl_spec) != 0) || (ul_dl_spec_cfg_pwr_compare(ul_dl_spec, PRIVATE_DATA(sm).ul_dl_spec) != 0)))
	{
		CALL_ALL_SLAVES(sm, set_selected_spectrum, ul_dl_spec->ul_cfg)

		// store it
		// free current one if stored
		if (PRIVATE_DATA(sm).ul_dl_spec) ul_dl_spec_cfg_free(&PRIVATE_DATA(sm).ul_dl_spec);
		PRIVATE_DATA(sm).ul_dl_spec = ul_dl_spec;
		ul_dl_spec = NULL;
	}

	// raise the event
	LOCAL_FUNC(sm_, raise_)(sm_, ev_DlUlSpectrumAvailable);

	if (ul_dl_spec) ul_dl_spec_cfg_free(&ul_dl_spec);
	return;

no_spectrum:
	if (ul_dl_spec) ul_dl_spec_cfg_free(&ul_dl_spec);
	if (PRIVATE_DATA(sm).ul_dl_spec) ul_dl_spec_cfg_free(&PRIVATE_DATA(sm).ul_dl_spec);
}



//#######################################################################################
// In "ul" version, we only raise the "DlUlSpectrumAvailable" event if the spectrum is different.  This is because the DlUlSpectrumAvailable is sent direct to the SOP sm in the handling of the "UlSOPSpectrumAvailable" func.
static void SelectDLULSpectrumSOP_ul(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	avail_spectrum_t* dl_spectrum_resp = NULL;
	avail_spectrum_t* ul_spectrum_resp = NULL;
	ul_dl_spec_cfg_t* ul_dl_spec = NULL;

	// This is the logic to select the UL spectrum :-
	// if any GOPs, use GOP spectrum
	// if no GOPS, but one or more of the SOPs has no spectrum, use GOP
	// if no GOPS, and all SOPs have spectrum, use lowest SOP power.

	if (!(dl_spectrum_resp = MASTER_LOCAL_FUNC(sm, GetAvailSpectrumResp)(MASTER_SM(sm))))
	{
		LOCAL_FUNC(sm_, raise_)(sm_, ev_DlSpectrumNotAvailable);
		goto no_spectrum;;
	}

	// if no GOPs, try SOPs
	int32_t max_bw = 5000000;		// this is a limitation at the moment
	if (PRIVATE_DATA(sm).gop_slave_info.num_devices == 0)
	{
		// clear the sop_min info
		PRIVATE_DATA(sm).num_sop_min = 0;

		// loop through the SOPs, and calculate the dl-ul for each
		if (PRIVATE_DATA(sm).sop_sm_list)
		{
			sop_slave_sm_entity_t* head = llist_get_head(PRIVATE_DATA(sm).sop_sm_list);
			sop_slave_sm_entity_t* sm_entity;
			for (sm_entity = head; sm_entity != NULL; sm_entity = get_next_item_entry(sm_entity->l_item))
			{
				// get the SPECTRUM RESP
				if (!(ul_spectrum_resp = LOCAL_FUNC(sm_entity->sm, GetAvailSpectrumResp)(sm_entity->sm)))
				{
					// no RESP available
					// clear ul_dl_spec if one was previously selected
					if (ul_dl_spec)
					{
						ul_dl_spec_cfg_free(&ul_dl_spec);
						break;
					}
				}
				ul_dl_spec_cfg_t* slave_ul_dl_spec = get_ul_dl_spectrum(sm_, dl_spectrum_resp, ul_spectrum_resp, max_bw);

				// if nothing available for this SOP
				if (!slave_ul_dl_spec)
				{
					// Clear any selected.
					ul_dl_spec_cfg_free(&ul_dl_spec);
					break;
				}

				// use this if no ul-dl previously selected
				if (!ul_dl_spec)
				{
					ul_dl_spec = slave_ul_dl_spec;
					// set this SOP as the first sop_min
					PRIVATE_DATA(sm).num_sop_min = 0;
					memset(&PRIVATE_DATA(sm).sop_min, 0, sizeof(PRIVATE_DATA(sm).sop_min));
					PRIVATE_DATA(sm).sop_min[PRIVATE_DATA(sm).num_sop_min++] = sm_entity->sm;
					continue;
				}

				// check it has the same range as previously selected.  If it doesnt, we assume there is a SOP mismatch and therefore we cant use SOPs
				if (slave_ul_dl_spec->ul_cfg->start_hz != ul_dl_spec->ul_cfg->start_hz)
				{
					// there is a mismatch.
					// Clear any selected.
					ul_dl_spec_cfg_free(&slave_ul_dl_spec);
					ul_dl_spec_cfg_free(&ul_dl_spec);
					// clear previous SopMinFlags
					PRIVATE_DATA(sm).num_sop_min = 0;
					break;
				}

				// if this is lower power than current selected, use this one.
				if (slave_ul_dl_spec->ul_cfg->dbm < ul_dl_spec->ul_cfg->dbm)
				{
					// this is lower power so we must use it.

					// clear the previously selected dl_ul spectrum, and use this new one
					ul_dl_spec_cfg_free(&ul_dl_spec);
					ul_dl_spec = slave_ul_dl_spec;

					// set this SOP as the first sop_min
					PRIVATE_DATA(sm).num_sop_min = 0;
					PRIVATE_DATA(sm).sop_min[PRIVATE_DATA(sm).num_sop_min++] = sm_entity->sm;
					continue;
				}
				else if (slave_ul_dl_spec->ul_cfg->dbm == ul_dl_spec->ul_cfg->dbm)
				{
					// its the same Power, so set this as a MinSop too
					// set this SOP as a sop_min too
					PRIVATE_DATA(sm).sop_min[PRIVATE_DATA(sm).num_sop_min++] = sm_entity->sm;
				}

				// if we reach here, we wont use this slave spec, so delete it
				ul_dl_spec_cfg_free(&slave_ul_dl_spec);
			}
		}
	}

	// anything selected ?
	if (!ul_dl_spec)
	{
		LOCAL_FUNC(sm_, raise_)(sm_, ev_DlUlSpectrumNotAvailable);
		goto no_spectrum;;
	}


	// check if its different from what stored
	if ((ul_dl_spec_cfg_freq_compare(ul_dl_spec, PRIVATE_DATA(sm).ul_dl_spec) != 0) || (ul_dl_spec_cfg_pwr_compare(ul_dl_spec, PRIVATE_DATA(sm).ul_dl_spec) != 0))
	{
		// set DL spectrum
		bool ok = MASTER_LOCAL_FUNC(sm, set_selected_spectrum)(MASTER_SM(sm), ul_dl_spec->dl_cfg);
		if (ok)
		{
			CALL_ALL_SLAVES(sm, set_selected_spectrum, ul_dl_spec->ul_cfg)
		}

		// raise the event
		LOCAL_FUNC(sm_, raise_)(sm_, ev_DlUlSpectrumAvailable);

		// store it
		// free current one if stored
		if (PRIVATE_DATA(sm).ul_dl_spec) ul_dl_spec_cfg_free(&PRIVATE_DATA(sm).ul_dl_spec);
		PRIVATE_DATA(sm).ul_dl_spec = ul_dl_spec;
		ul_dl_spec = NULL;
	}

	if (ul_dl_spec) ul_dl_spec_cfg_free(&ul_dl_spec);
	return;

no_spectrum:
	if (ul_dl_spec) ul_dl_spec_cfg_free(&ul_dl_spec);
	if (PRIVATE_DATA(sm).ul_dl_spec) ul_dl_spec_cfg_free(&PRIVATE_DATA(sm).ul_dl_spec);
}



//#######################################################################################
static void DlSpectrumNotAvailable(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, ev_DlSpectrumNotAvailable);
}


//#######################################################################################
static void DlSpectrumAvailable(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, ev_DlSpectrumAvailable);
}


//#######################################################################################
static void UlSpectrumNotAvailable(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, ev_UlSpectrumNotAvailable);
}


//#######################################################################################
static void UlSpectrumAvailable(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, ev_UlSpectrumAvailable);
}


//#######################################################################################
static void UlSOPSpectrumNotAvailable(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, ev_UlSOPSpectrumNotAvailable);
}


//#######################################################################################
static void UlSOPSpectrumAvailable(void* sm_, avail_spectrum_t* ul_spectrum_resp, void* caller)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;
	ul_dl_spec_cfg_t* ul_dl_spec=NULL;

	// if there is no ul_dl_spec already, raise it
	if (!PRIVATE_DATA(sm).ul_dl_spec)
	{
		LOCAL_FUNC(sm_, raise_)(sm_, ev_UlSOPSpectrumAvailable);
		goto cleanup;
	}

	// get the DL resp
	avail_spectrum_t* dl_spectrum_resp = NULL;
	if (!(dl_spectrum_resp = MASTER_LOCAL_FUNC(sm, GetAvailSpectrumResp)(MASTER_SM(sm))))
	{
		LOCAL_FUNC(sm_, raise_)(sm_, ev_DlSpectrumNotAvailable);
		goto cleanup;
	}

	// calculate the if the power has changed
	// if it has a different frequency, or a lower power, raise the event
	int ret = 0;
	int max_bw = 5000000; // this is a limit which needs to be removed at some point
	if ((ul_dl_spec = get_ul_dl_spectrum(sm_, dl_spectrum_resp, ul_spectrum_resp, max_bw)))
	{
		// if the frequency is different, raise the event
		ret = ul_dl_spec_cfg_freq_compare(ul_dl_spec, PRIVATE_DATA(sm).ul_dl_spec);
		if (ret != 0)
		{
			LOCAL_FUNC(sm_, raise_)(sm_, ev_UlSOPSpectrumAvailable);
			goto cleanup;
		}
		// now check the power
		ret = ul_dl_spec_cfg_pwr_compare(ul_dl_spec, PRIVATE_DATA(sm).ul_dl_spec);
	}

	// determine if the caller is a SOP min
	uint16_t idx = 0;
	for (idx = 0; idx < PRIVATE_DATA(sm).num_sop_min; idx++)
	{
		if (PRIVATE_DATA(sm).sop_min[idx] == (paws_sop_slave_sm_t*)caller)
		{
			break;
		}
	}

	// if sop_min, and the SOP now gives a different power, decrease the sop_min count.  If there are no sop-Mins left, raise the event
	if (idx < PRIVATE_DATA(sm).num_sop_min)
	{
		// it is a min_sop

		if (ret < 0)		// it its lower power, raise the event
		{
			LOCAL_FUNC(sm_, raise_)(sm_, ev_UlSOPSpectrumAvailable);
			goto cleanup;
		}
		else if (ret > 0)		// it its lower power, raise the event
		{	// it its higher power, remove it from min_sop list.  If there are no other min_sop, raise the event
		
			// remove it from the sop_min list
			PRIVATE_DATA(sm).num_sop_min--;
			uint16_t i;
			for (i = idx; i < PRIVATE_DATA(sm).num_sop_min; i++)
			{
				PRIVATE_DATA(sm).sop_min[i] = PRIVATE_DATA(sm).sop_min[i+1];
			}

			// If there are no other min_sop, raise the event
			if (PRIVATE_DATA(sm).num_sop_min == 0)
			{
				LOCAL_FUNC(sm_, raise_)(sm_, ev_UlSOPSpectrumAvailable);
				goto cleanup;
			}
		}
	}
	else
	{
		// if it wasnt a sop_min, but has lower power, raise the event
		if (ret < 0)
		{
			LOCAL_FUNC(sm_, raise_)(sm_, ev_UlSOPSpectrumAvailable);
			goto cleanup;
		}
	}

	// if we reach here, set the new sectrum, and just tell the slave that the DLUL spec is still available
	LOCAL_FUNC(caller, set_selected_spectrum)(caller, ul_dl_spec->ul_cfg);
	LOCAL_FUNC(caller, DLULSpectrumAvailable)(caller);

cleanup:
	if (ul_dl_spec) ul_dl_spec_cfg_free(&ul_dl_spec);
}



//#######################################################################################
static void DlNotificationSuccess(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, ev_MasterNotificationSuccess);
}


//#######################################################################################
static void UlNotificationSuccess(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, ev_ULNotificationSuccess);
}


//#######################################################################################
static void setDefaultInfo(void* sm_, float default_max_location_change, uint32_t default_max_polling_secs)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	PAWS_SM_DATA(sm_)->default_max_location_change = default_max_location_change;
	PAWS_SM_DATA(sm_)->default_max_polling_secs = default_max_polling_secs;

	// send to slaves
	CALL_ALL_SLAVES(sm, set_defaultInfo, PAWS_SM_DATA(sm_)->default_max_location_change, PAWS_SM_DATA(sm_)->default_max_polling_secs);
}


//#######################################################################################
static void timeout_handler(void* sm_, uint32_t id)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	switch (id) 
	{
		case DB_DISCOVERY_TIMER_ID :
			DBdiscovery_Timer_Hdlr(sm_);
			break;
		case GPS_PERIODIC_TIMER_ID :
			Gps_Periodic_Timer_Hdlr(sm_);
			break;
		case READ_SETTINGS_TIMER_ID :
			Read_Settings_Timer_Hdlr(sm_);
			break;
		case READ_DEVICES_TIMER_ID :
			Read_Devices_Timer_Hdlr(sm_);
			break;
		case CONTROL_PLANE_TIMER_ID :
			Control_Plane_Timer_Hdlr(sm_);
			break;
		case LOCAL_ISSUE_BACKOFF_TIMER_ID :
			LocalIssueBackoff_Timer_Hdlr(sm_);
			break;
		default:
			// direct call
			PAWS_SM_FUNC(sm, timeout_handler)(sm_, id);
			break;
	}
}


//#######################################################################################
static void Start_Gps_Periodic_Timer(void* sm_)
{
	FUNC_DBG(sm_);
	timer_manager_start_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, GPS_PERIODIC_TIMER_ID);
}



//#######################################################################################
static void Gps_Periodic_Timer_Hdlr(void* sm_)
{
	FUNC_DBG(sm_);
	check_gps(sm_);
	Start_Gps_Periodic_Timer(sm_);
}


//#######################################################################################
static void Start_Read_Settings_Timer(void* sm_)
{
	FUNC_DBG(sm_);
	timer_manager_start_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, READ_SETTINGS_TIMER_ID);
}

//#######################################################################################
static bool set_privateLogInfo(void* sm_, logger_cfg_t* cfg)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	if (!cfg)
	{
		return  false;
	}

	// DeviceCfg_Logger
	if ((!PRIVATE_DATA(sm).DeviceCfg_Logger) || (memcmp(&PRIVATE_DATA(sm).DeviceCfg_LogCfg, cfg, sizeof(logger_cfg_t)) != 0))	// if not already created, or config has changed
	{
		// copy the config
		memcpy(&PRIVATE_DATA(sm).DeviceCfg_LogCfg, cfg, sizeof(logger_cfg_t));

		// if already created free old one
		if (PRIVATE_DATA(sm).DeviceCfg_Logger)
			logger_free(&PRIVATE_DATA(sm).DeviceCfg_Logger);

		// if there is a filename configured
		if (strlen(cfg->logname))
		{
			PRIVATE_DATA(sm).DeviceCfg_Logger = logger_create(cfg);
		}
	}

	return true;
}


//#######################################################################################
static void Read_Settings_Timer_Hdlr(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	bool read_ok = paws_read_settings(&PRIVATE_DATA(sm).paws_settings);
	if (!read_ok)
	{
		fprintf(stderr, "Problem reading PAWS-Settings\n");
		LOG_PRINT(sm, LOG_ERROR, "Problem reading PAWS-Settings");
		LocalDBerror(sm_);
	}
	else 
	{
		if (!PRIVATE_DATA(sm).paws_settings_read)
		{

			PRIVATE_DATA(sm).paws_settings_read = true;
			LOG_PRINT(sm, LOG_INFO, "PAWS-Setting read OK");
			LOCAL_FUNC(sm, raise_)(sm_, ev_SETTINGS);
		}

		// update global config
		set_gPawsAppLoggerInfo(&PRIVATE_DATA(sm).paws_settings.loginfo.app_log);
		set_gPawsCloudLoggerInfo(&PRIVATE_DATA(sm).paws_settings.loginfo.cloud_log);
		// propogate settings to master and slave
		if (MASTER_SM(sm)) 
		{
			MASTER_LOCAL_FUNC(sm, set_msgLogInfo)(MASTER_SM(sm), &PRIVATE_DATA(sm).paws_settings.loginfo.msg_log);
			MASTER_LOCAL_FUNC(sm, set_maxPollingQuick)(MASTER_SM(sm), PRIVATE_DATA(sm).paws_settings.max_polling_quick_secs);
		}
		if (GOP_SM(sm))
		{
			CALL_ALL_SLAVES(sm, set_msgLogInfo, &PRIVATE_DATA(sm).paws_settings.loginfo.msg_log);
			CALL_ALL_SLAVES(sm, set_maxPollingQuick, PRIVATE_DATA(sm).paws_settings.max_polling_quick_secs);
		}
		// private loggers 
		set_privateLogInfo(sm_, &PRIVATE_DATA(sm).paws_settings.loginfo.app_log);

		// update timings
		timer_manager_set_duration(PAWS_SM_STL_DATA(sm_)->timer_manager, READ_SETTINGS_TIMER_ID, PRIVATE_DATA(sm).paws_settings.setting_periodic_secs);
		timer_manager_set_duration(PAWS_SM_STL_DATA(sm_)->timer_manager, READ_DEVICES_TIMER_ID, PRIVATE_DATA(sm).paws_settings.devices_periodic_secs);
		timer_manager_set_duration(PAWS_SM_STL_DATA(sm_)->timer_manager, RETRY_TIMER_ID, PRIVATE_DATA(sm).paws_settings.db_retry_secs);

		check_gps(sm_);
	}

	Start_Read_Settings_Timer(sm_);
}



//#######################################################################################
static void Start_Read_Devices_Timer(void* sm_)
{
	FUNC_DBG(sm_);
	timer_manager_start_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, READ_DEVICES_TIMER_ID);
}


//#######################################################################################
static void Read_Devices_Timer_Hdlr(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	// master
	bool read_ok = paws_read_master_info(((paws_sm_t*)sm_)->paws_sm_hdr.unique_id, &PAWS_SM_DATA(sm_)->master_info, &PRIVATE_DATA(sm).master_info_read);
	if (!read_ok)
	{
		fprintf(stderr, "Problem reading PAWS-MasterDevice\n");
		LOG_PRINT(sm, LOG_ERROR, "Problem reading PAWS-MasterDevice");
		LocalDBerror(sm_);
		return;
	}

	// slaves
	static paws_slave_info_t new_gops;
	static paws_slave_info_t new_sops;
	memset(&new_gops, 0, sizeof(paws_slave_info_t));
	memset(&new_sops, 0, sizeof(paws_slave_info_t));
	read_ok = paws_read_slave_info(((paws_sm_t*)sm_)->paws_sm_hdr.unique_id, &new_gops, &new_sops);
	if (!read_ok)
	{
		kill_slave_sms(sm_);
		PRIVATE_DATA(sm).gop_slave_info.num_devices = 0;
		PRIVATE_DATA(sm).sop_slave_info.num_devices = 0;		
		fprintf(stderr, "Problem reading PAWS-SlaveDevices\n");
		LOG_PRINT(sm, LOG_ERROR, "Problem reading PAWS-SlaveDevices");
		LocalDBerror(sm_);
		return;
	}

	checkMasterInfo(sm_);
	checkSlaveInfo(sm_, &new_gops, &new_sops);
	checkSlavesPresent(sm_);

	Start_Read_Devices_Timer(sm_);
}


//#######################################################################################
static void Control_Plane_Timer_Hdlr(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;
	
	if (!(paws_dal_control_plane_send_status(PRIVATE_DATA(sm).control_plane, PRIVATE_DATA(sm).control_plane_enabled)))
	{
		LOG_PRINT(sm, LOG_ERROR, "Problem sending control state %d to control-agent", PRIVATE_DATA(sm).control_plane_enabled)
	}
	else
	{
		log_level_e loglevel = (PRIVATE_DATA(sm).control_plane_enabled) ? LOG_INFO : LOG_ERROR;
		LOG_PRINT(sm, loglevel, "Sent control state %d to control-agent", PRIVATE_DATA(sm).control_plane_enabled)
	}

	Start_Control_Plane_Timer(sm_);
}


//#######################################################################################
static void Start_Control_Plane_Timer(void* sm_)
{
	FUNC_DBG(sm_);
	timer_manager_start_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, CONTROL_PLANE_TIMER_ID);
}



//#######################################################################################
static void Start_LocalIssueBackoff_Timer(void* sm_)
{
	FUNC_DBG(sm_);
	timer_manager_start_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, LOCAL_ISSUE_BACKOFF_TIMER_ID);
}


//#######################################################################################
static void LocalIssueBackoff_Timer_Hdlr(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, ev_LocalIssueBackOff_Expiry);
}


//#######################################################################################
static void raise_slaveDLULSpectrumAvailable(void* sm_)
{
	FUNC_DBG(sm_);
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;
	if (GOP_SM(sm))
	{
		CALL_ALL_SLAVES(sm, DLULSpectrumAvailable);
	}
}


//#######################################################################################
static void raise_masterDLULSpectrumAvailable(void* sm_)
{
	FUNC_DBG(sm_);
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;
	if (MASTER_SM(sm))
	{
		MASTER_LOCAL_FUNC(sm, DLULSpectrumAvailable)(MASTER_SM(sm));
	}
}


//#######################################################################################
static void masterDevice_processOverrideCfg(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	if (paws_config_master_device_override(PRIVATE_DATA(sm).DeviceCfg_Logger, &PRIVATE_DATA(sm).paws_settings.spectrum_override, &PAWS_SM_DATA(sm_)->master_info.antenna_info))
		enable_control_plane(sm_);
}


//#######################################################################################
static void masterDevice_updateCfg(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	if (!PRIVATE_DATA(sm).ul_dl_spec)
	{
		LOG_PRINT(sm, LOG_ERROR, "No UL-DL spectrum found");
		return;
	}

	bool device_enabled_changed = false;
	bool cfg_changed = false;
	device_cfg_t device_cfg;

	device_cfg.device_enabled = 1;
	// enable/disable the master
	device_cfg.bandwidth = PRIVATE_DATA(sm).ul_dl_spec->dl_cfg->bandwidth;
	device_cfg.dl_start_hz = PRIVATE_DATA(sm).ul_dl_spec->dl_cfg->start_hz;
	device_cfg.dl_dbm_per100k = PRIVATE_DATA(sm).ul_dl_spec->dl_cfg->dbm;
	device_cfg.ul_start_hz = PRIVATE_DATA(sm).ul_dl_spec->ul_cfg->start_hz;
	device_cfg.ul_dbm_per100k = PRIVATE_DATA(sm).ul_dl_spec->ul_cfg->dbm;

	paws_config_master_device(PRIVATE_DATA(sm).DeviceCfg_Logger, &device_cfg, &device_enabled_changed, &cfg_changed, 
		&PRIVATE_DATA(sm).reboot_needed, &PRIVATE_DATA(sm).paws_settings.spectrum_override, &PAWS_SM_DATA(sm_)->master_info.antenna_info);

	if ((!PRIVATE_DATA(sm).control_plane_enabled) && (!PRIVATE_DATA(sm).reboot_needed) && (device_cfg.device_enabled == 1))
		enable_control_plane(sm_);
}


//#######################################################################################
static void masterDevice_disable(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	bool device_enabled_changed = false;
	bool cfg_changed = false;
	device_cfg_t device_cfg;

	device_cfg.device_enabled = 0;
	paws_config_master_device(PRIVATE_DATA(sm).DeviceCfg_Logger, &device_cfg, &device_enabled_changed, &cfg_changed,
		&PRIVATE_DATA(sm).reboot_needed, &PRIVATE_DATA(sm).paws_settings.spectrum_override, &PAWS_SM_DATA(sm_)->master_info.antenna_info);

	// send new status to Control-Plane agent
	disable_control_plane(sm_);

	if (device_enabled_changed)
	{
		LOG_PRINT(sm_, LOG_NOTICE, "master-device-cfg: DISABLED");
	}
}


//#######################################################################################
static void create_gop_slave_sm(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	if (!GOP_SM(sm))
	{
		GOP_SM(sm) = paws_gop_slave_sm_create(sm, "gop", &PAWS_SM_DATA(sm_)->master_info, &PAWS_SM_DATA(sm_)->gps, PAWS_SM_DATA(sm_)->default_max_location_change, PAWS_SM_DATA(sm_)->default_max_polling_secs,
												&PRIVATE_DATA(sm).gop_slave_info, PRIVATE_DATA(sm).paws_settings.min_ul_dbm_100k, PRIVATE_DATA(sm).paws_settings.db_retry_secs, 
												&PRIVATE_DATA(sm).paws_settings.loginfo.msg_log, PRIVATE_DATA(sm).paws_settings.max_polling_quick_secs, NULL);
		GOP_LOCAL_FUNC(sm, Start)(GOP_SM(sm));
	}
}



//#######################################################################################
static paws_sop_slave_sm_t* create_sop_slave_sm(void* sm_, paws_device_info_t* device_info, sm_state_info_t* sm_state_info)
{
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	if (!PRIVATE_DATA(sm).sop_sm_list)
		return NULL;

	sop_slave_sm_entity_t* sm_entity = malloc(sizeof(sop_slave_sm_entity_t));

	if (sm_entity)
	{
		memset(sm_entity, 0, sizeof(sop_slave_sm_entity_t));

		// create SM itself
		if (!(sm_entity->sm = paws_sop_slave_sm_create(sm_, device_info->unique_id, &PAWS_SM_DATA(sm_)->master_info, &PAWS_SM_DATA(sm_)->gps, PAWS_SM_DATA(sm_)->default_max_location_change, PAWS_SM_DATA(sm_)->default_max_polling_secs,
			device_info, PRIVATE_DATA(sm).paws_settings.min_ul_dbm_100k, PRIVATE_DATA(sm).paws_settings.db_retry_secs,
			&PRIVATE_DATA(sm).paws_settings.loginfo.msg_log, PRIVATE_DATA(sm).paws_settings.max_polling_quick_secs, sm_state_info)))
		{
			goto error;
		}

		strcpy(sm_entity->device_name, device_info->unique_id);

		// add to llist
		if (!(sm_entity->l_item = llist_append(PRIVATE_DATA(sm).sop_sm_list, sm_entity)))
		{
			goto error;
		}

		return sm_entity->sm;
	}
error:
	if (sm_entity) sop_sm_entity_free(sm_entity);
	return NULL;
}



//#######################################################################################
static void create_start_sop_slave_sm(void* sm_, paws_device_info_t* device_info)
{
	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;
	paws_sop_slave_sm_t* slave_sm = NULL;

	if ((slave_sm = create_sop_slave_sm(sm, device_info, NULL)))
	{
		// start it
		LOCAL_FUNC(slave_sm, Start)(slave_sm);
		return;
	}
}


//#######################################################################################
// must match llist data_callback_func1 prototype
static int sop_sm_entity_free(void* sm_entity_)
{
	sop_slave_sm_entity_t* sm_entity = (sop_slave_sm_entity_t*)sm_entity_;
	if (sm_entity)
	{
		// free the SM
		paws_sop_slave_sm_free(&sm_entity->sm);

		// free the entity itself
		free(sm_entity);
	}
	return 0;
}


//#######################################################################################
// must match llist data_callback_func2 prototype
static int device_name_compare(void* sm_entity_, anytype_u device_name)
{
	sop_slave_sm_entity_t* sm_entity = (sop_slave_sm_entity_t*)sm_entity_;
	if ((sm_entity) && (device_name.p))
	{
		int ret = strcmp(sm_entity->device_name, device_name.p);
		if (ret < 0)
			return -1;
		else if (ret == 0)
			return 0;
		else
			return 1;
	}
	return -1;
}


//#######################################################################################
void delete_sop_slave_sm(void* sm_, device_name_t slave_id)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	if (!PRIVATE_DATA(sm).sop_sm_list)
		return;

	// delete from list.  This will also free the entry
	anytype_u device_name;
	device_name.p = slave_id;
	llist_delete_entries(PRIVATE_DATA(sm).sop_sm_list, device_name_compare, device_name, sop_sm_entity_free, true);
}


//#######################################################################################
static void create_slave_sms(void* sm_)
{
	FUNC_DBG(sm_); 

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	// create the GOP
	create_gop_slave_sm(sm_);   

	// create the SOPs
	if (!PRIVATE_DATA(sm).sop_sm_list)
		return;

	// Create the SOP slave SMs by looping through the SOP device list, and if the "SM" cannot be found in the sop_sm_list, create it
	int i = 0;
	for (i = 0; i < PRIVATE_DATA(sm).sop_slave_info.num_devices; i++)
	{
		paws_device_info_t* dev = &PRIVATE_DATA(sm).sop_slave_info.device_info[i];

		// is device already in sop_sm_list
		anytype_u device_name;
		device_name.p = dev->unique_id;
		if (!(llist_get_entry(PRIVATE_DATA(sm).sop_sm_list, device_name_compare, device_name)))
		{
			// its not there, so create it
			create_start_sop_slave_sm(sm_, dev);
		}
	}
}


//#######################################################################################
static void Start_DBdiscovery_Timer(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	int dur = PRIVATE_DATA(sm).db_discovery_info.db_discovery_duration - DB_DISCOVERY_TIMER_TOLERANCE;
	if (dur > 0)
	{
		timer_manager_set_duration(PAWS_SM_STL_DATA(sm_)->timer_manager, DB_DISCOVERY_TIMER_ID, dur);
		timer_manager_start_timer(PAWS_SM_STL_DATA(sm_)->timer_manager, DB_DISCOVERY_TIMER_ID);
	}
	else
	{
		db_discovery(sm_);
	}
}


//#######################################################################################
static void DBdiscovery_Timer_Hdlr(void* sm_)
{
	FUNC_DBG(sm_);
	db_discovery(sm_);
}


//#######################################################################################
static void db_discovery(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	paws_db_info_t db_info; 

	bool read_ok = paws_read_db(&db_info);
	if (!read_ok)
	{
		memset(&PRIVATE_DATA(sm).db_info, 0, sizeof(paws_db_info_t));
		fprintf(stderr, "Problem reading PAWS-DB-Info\n");
		LOG_PRINT(sm, LOG_ERROR, "Problem reading PAWS-DB-Info");
		LocalDBerror(sm_);
		return;
	}

	// if weblist present
	if (strlen(db_info.weblist_url.host))
	{
		paws_weblist_t* weblist = get_db_discovery_list(sm_, &db_info.weblist_url);
		if (process_db_discovery(sm_, weblist))
		{
			// update list
			weblist_free(&PRIVATE_DATA(sm).db_discovery_info.wsbd_weblist);
			PRIVATE_DATA(sm).db_discovery_info.wsbd_weblist = weblist;
		}
		else if (PRIVATE_DATA(sm).db_discovery_info.wsbd_weblist)				// if we had a weblist and have now lost it
		{
			PRIVATE_DATA(sm).db_discovery_info.db_discovery_duration = DB_DISCOVERY_TIMER_QUICK_DURATION;
			LOG_PRINT(sm, LOG_INFO, "setting db_discovery_duration=%ds", PRIVATE_DATA(sm).db_discovery_info.db_discovery_duration);
		}
		check_db_validity(sm_, &db_info);
	}

	Start_DBdiscovery_Timer(sm_);
	select_DB(sm_, NULL);
}



//#######################################################################################
static char* get_db_discovery_list_http(void* sm_, char* host, char* fname, int *body_offset)
{
	FUNC_DBG(sm_);

	int sockfd = -1;
	char *response = NULL;
	
	// check params
	if (!(host && strlen(host) && fname && strlen(fname)))
	{
		return NULL;
	}

	uint8_t db_attempts = MAX_DB_ACCESS_ATTEMPTS;
	while (db_attempts--)
	{
		// open socket
		char const *port = HTTP_PORT;
		sockfd = http_init(host, port);
		if (sockfd < 0)
		{
			LOG_PRINT(sm_, LOG_ERROR, "Could not socket to '%s'", host);
			goto error_hdl;
		}

		// send GET
		int status = http_send_get(sockfd, host, fname);
		if (status < 0)
		{
			LOG_PRINT(sm_, LOG_ERROR, "Sending request failed");
			goto error_hdl;
		}

		// get the response
		response = http_fetch_response(sockfd, body_offset);
		if (!response)
		{
			LOG_PRINT(sm_, LOG_ERROR, "Fetching response failed");
			goto error_hdl;
		}

		// free stuff up
		if (sockfd >= 0) http_close(sockfd);

		return response;

error_hdl:
		if (sockfd >= 0) http_close(sockfd);
		if (response) free(response);
	}

	return NULL;
}



//#######################################################################################
static char* get_db_discovery_list_https(void* sm_, char* host, char* fname, int *body_offset)
{
	FUNC_DBG(sm_);

	char *response = NULL;
	SSL* conn = NULL;
	int sockfd = -1;
	
	// check params
	if (!(host && strlen(host) && fname && strlen(fname) ))
	{
		return NULL;
	}

	uint8_t db_attempts = MAX_DB_ACCESS_ATTEMPTS;
	while (db_attempts--)
	{
		// open socket
		char const *port = HTTPS_PORT;
		sockfd = http_init(host, port);
		if (sockfd < 0)
		{
			LOG_PRINT(sm_, LOG_ERROR, "Could not make connection to '%s", host);
			goto error_hdl;
		}

		// make an ssl connection
		conn = https_connect(sockfd);
		if (!conn)
		{
			LOG_PRINT(sm_, LOG_ERROR, "Failure making ssl connection to %s", host);
			goto error_hdl;
		}

		// send the GET
		int status = https_send_get(conn, host, fname);
		if (status < 0)
		{
			LOG_PRINT(sm_, LOG_ERROR, "Sending request failed");
			goto error_hdl;
		}

		response = https_fetch_response(conn, body_offset);
		if (!response)
		{
			LOG_PRINT(sm_, LOG_ERROR, "Fetching response failed");
			goto error_hdl;
		}

		// puts(response);

		// free stuff up
		if (conn) https_disconnect(conn);
		if (sockfd >= 0) http_close(sockfd);
		return response;

error_hdl:
		if (conn) https_disconnect(conn);
		if (sockfd >= 0) http_close(sockfd);
		if (response) free(response);
	}

	return NULL;
}


//#######################################################################################
static paws_weblist_t* get_db_discovery_list(void* sm_, paws_weblist_url_t* discovery_list_url)
{
	FUNC_DBG(sm_);

	char *response = NULL;
	json_value *value = NULL;
	int body_offset = 0;

	// determine if this is http or https
	const char* HTTP_HEADER = "http://";
	const char* HTTPS_HEADER = "https://";
	char *host = NULL;
	// move past http header
	if ((host = strstr(discovery_list_url->host, HTTP_HEADER)))
	{
		host += strlen(HTTP_HEADER);
		response = get_db_discovery_list_http(sm_, host, discovery_list_url->fname, &body_offset);
	}
	else if ((host = strstr(discovery_list_url->host, HTTPS_HEADER)))
	{
		host += strlen(HTTPS_HEADER);
		response = get_db_discovery_list_https(sm_, host, discovery_list_url->fname, &body_offset);
	}
	else
	{
		LOG_PRINT(sm_, LOG_ERROR, "URL must start with \"http://\" or \"https://\"");
		goto error_hdl;
	}

	if (!response)
	{
		LOG_PRINT(sm_, LOG_ERROR, "Failure in http/https GET");
		goto error_hdl;
	}

	// get the HTTP response code
	int code = http_get_status_code(response);
	if (code >= 400)
	{
		// these are HTTP error codes
		LOG_PRINT(sm_, LOG_ERROR, "HTTP ERROR RESP %d", code);
		goto error_hdl;
	}

	// log the info
	APPLOG_TVWSDB_MSG(sm_, response + body_offset);

	// convert to json
	json_char *json = (json_char*)response + body_offset;
	int len = strlen(response) - body_offset;
	value = json_parse(json, len);
	if (value == NULL) {
		LOG_PRINT(sm_, LOG_ERROR, "Unable to parse data");
		goto error_hdl;
	}

	// convert json to a paws_weblist_t
	paws_weblist_t* weblist = json_2_weblist(value);

	// free stuff up
	if (response) free(response);
	if (value) json_value_free(value);

	return weblist;

error_hdl:
	if (response) free(response);
	if (value) json_value_free(value);
	return NULL;
}


//#######################################################################################
static bool process_db_discovery(void* sm_, paws_weblist_t* weblist)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	if (weblist) 
	{
		if ((weblist->num_items) && (weblist->refresh_rate > 0))
		{
			PRIVATE_DATA(sm).db_discovery_info.db_discovery_duration = weblist->refresh_rate * 60;
			LOG_PRINT(sm, LOG_INFO, "setting db_discovery_duration=%ds", PRIVATE_DATA(sm).db_discovery_info.db_discovery_duration);
			return true;
		}
	}
	return false;
}



//#######################################################################################
static bool host_in_weblist(void* sm_, paws_url_t host)
{
	FUNC_DBG(sm_); 

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	if (!PRIVATE_DATA(sm).db_discovery_info.wsbd_weblist)
		return false;

	if (!host)
		return false;

	for (int i = 0; i < PRIVATE_DATA(sm).db_discovery_info.wsbd_weblist->num_items; i++)
	{
		weblist_item_t* item = &PRIVATE_DATA(sm).db_discovery_info.wsbd_weblist->items[i];
		if ((strcmp(host, item->url) == 0) && (item->mcwsd_support))
			return true;
	}
	return false;
}

//#######################################################################################
static void check_db_validity(void* sm_, paws_db_info_t* db_info)
{
	FUNC_DBG(sm_);

	bool db_writeback = false;

	if (!db_info)
		return;

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	if ( (!(PRIVATE_DATA(sm).db_discovery_info.wsbd_weblist)) ||
		 (!(PRIVATE_DATA(sm).db_discovery_info.wsbd_weblist->num_items > 0)) ||
		 (!(db_info) ))
	{
		invalidate_all_db(sm_, db_info);
		paws_write_db(db_info);
		return;
	}

	// check that the DBs in our list are validated in the wsbd_weblist
	for (uint32_t i = 0; i < db_info->num_db; i++)
	{
		paws_db_item_t* db = &db_info->db_list[i];
		
		// is host in weblist;
		if ((host_in_weblist(sm_, db->db_url.host)))
		{
			if (!(db->valid))
				db_writeback = true;
			db->valid = true;
		}
		else
		{
			if (db->valid)
				db_writeback = true;
			db->valid = false;
		}
	}

	if (db_writeback)
		paws_write_db(db_info);
}


//#######################################################################################
static void invalidate_all_db(void* sm_, paws_db_info_t* db_info)
{
	FUNC_DBG(sm_);

	if (!db_info)
		return;

	// Invalidate all databases
	// check that the DBs in our list are validated in the wsbd_weblist
	for (uint32_t i = 0; i < db_info->num_db; i++)
	{
		paws_db_item_t* db = &db_info->db_list[i];
		db->valid = false;
	}
}



//#######################################################################################
static void select_DB(void* sm_, void* caller)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	// if we get here, select a new DB
	// unBarr DBs for those who'd Barred timer has expired
	unBarr_DBs(sm_);

	paws_db_info_t db_info;

	bool read_ok = paws_read_db(&db_info);
	if (!read_ok)
	{
		memset(&PRIVATE_DATA(sm).db_info, 0, sizeof(paws_db_info_t));
		fprintf(stderr, "Problem reading PAWS-DB-Info\n");
		LOG_PRINT(sm, LOG_ERROR, "Problem reading PAWS-DB-Info");
		LocalDBerror(sm_);
		return;
	}

	memcpy(&PRIVATE_DATA(sm).db_info, &db_info, sizeof(paws_db_info_t));

	check_db_validity(sm_, &PRIVATE_DATA(sm).db_info);

	// walk all DBs and see if selected DB is still there and present 
	for (uint32_t i = 0; i < PRIVATE_DATA(sm).db_info.num_db; i++)
	{
		paws_db_item_t* db = &PRIVATE_DATA(sm).db_info.db_list[i];
		if ((db->valid) && (db->barred_utc == 0))
		{
			// if this is the current selected_db 
			if (strcmp(PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db.name, db->name) == 0)
			{
				// use currently selected db
				// is caller is set call that SM, else call master and all slaves
				if (caller)
				{
					LOCAL_FUNC(caller, set_db)(caller, &PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db);
				}
				else
				{
					// update master slaves
					if (MASTER_SM(sm))
					{
						MASTER_LOCAL_FUNC(sm, set_db)(MASTER_SM(sm), &PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db);
					}
					if (GOP_SM(sm))
					{
						CALL_ALL_SLAVES(sm, set_db, &PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db);
					}
				}
				return;
			}
		}
	}

	// if we get here, selected_db was not present/valid, so clear it 
	memset(&PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db, 0, sizeof(paws_db_item_t));

	// walk the list and select a new valid one
	for (uint32_t i = 0; i < PRIVATE_DATA(sm).db_info.num_db; i++)
	{
		paws_db_item_t* db = &PRIVATE_DATA(sm).db_info.db_list[i];
		if ((db->valid) && (db->barred_utc == 0))
		{
			// use currently selected db
			memcpy(&PAWS_SM_DATA(sm)->selected_db, db, sizeof(paws_db_item_t));
			break;
		}
	}

	// is caller is set call that SM, else call master and all slaves
	if (caller)
	{
		LOCAL_FUNC(caller, set_db)(caller, &PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db);
	}
	else
	{
		// update master slaves
		if (MASTER_SM(sm))
		{
			MASTER_LOCAL_FUNC(sm, set_db)(MASTER_SM(sm), &PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db);
		}
		if (GOP_SM(sm))
		{
			CALL_ALL_SLAVES(sm, set_db, &PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db);
		}
	}
}


//#######################################################################################
static void unBarr_DBs(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	paws_db_info_t db_info;

	bool read_ok = paws_read_db(&db_info);
	if (!read_ok)
	{
		memset(&PRIVATE_DATA(sm).db_info, 0, sizeof(paws_db_info_t));
		fprintf(stderr, "Problem reading PAWS-DB-Info\n");
		LOG_PRINT(sm, LOG_ERROR, "Problem reading PAWS-DB-Info");
		LocalDBerror(sm_);
		return;
	}

	memcpy(&PRIVATE_DATA(sm).db_info, &db_info, sizeof(paws_db_info_t));

	bool db_writeback = false;
	time_t now_ = time(NULL);	// get utc

	paws_db_item_t* db = NULL;
	for (uint32_t i = 0; i < db_info.num_db; i++)
	{
		// if DB is barred, can it now be unbarred ?
		db = &db_info.db_list[i];
		if ((db->barred_utc != 0) && (now_ >= (db->barred_utc + PRIVATE_DATA(sm).paws_settings.db_barred_secs)))
		{
			db->barred_utc = 0;
			db_writeback = true;
		}
	}
	// if something was unbarred, write it back
	if (db_writeback)
	{
		paws_write_db(&db_info);
	}
}


//#######################################################################################
static void Barr_DB(void* sm_)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	// 	mark selected_db as Barred
	if ((strlen(PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db.db_url.host) > 0))
	{
		paws_db_info_t db_info;

		bool read_ok = paws_read_db(&db_info);
		if (!read_ok)
		{
			memset(&PRIVATE_DATA(sm).db_info, 0, sizeof(paws_db_info_t));
			fprintf(stderr, "Problem reading PAWS-DB-Info\n");
			LOG_PRINT(sm, LOG_ERROR, "Problem reading PAWS-DB-Info");
			LocalDBerror(sm_);
			return;
		}

		memcpy(&PRIVATE_DATA(sm).db_info, &db_info, sizeof(paws_db_info_t));

		// find selected_db in db_info.
		paws_db_item_t* db = NULL;
		for (uint32_t i = 0; i < db_info.num_db; i++)
		{
			db = &db_info.db_list[i];
			if ((strcmp(db->db_url.host, PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db.db_url.host) == 0))
			{
				// this matches
				break;
			}
		}

		// if someting selected
		if (db)
		{
			// set it as barred
			db->barred_utc = time(NULL);
			// write it back to database
			paws_write_db(&db_info);
		}
	}
}


//#######################################################################################
static void DBerror(void* sm_, void* caller)
{
	FUNC_DBG(sm_);
	Barr_DB(sm_);
	select_DB(sm_, caller);
}


//#######################################################################################
static void LocalDBerror(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, ev_LocalDB_Error);
}


//#######################################################################################
static void ProcessDbUpdate(void* sm_, json_value* new_db_spec)
{
	FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	paws_db_info_t* db_info = &PRIVATE_DATA(sm).db_info;

	// this will update the DB list.

	// Copy the token from the current head of the list. 
	paws_db_token_t token="";

	if (db_info->num_db)
	{
		memcpy(token, db_info->db_list[0].db_url.token, sizeof(paws_db_token_t));
	}
	
	if (json_dbUpdate_2_dblist(new_db_spec, token, &db_info->num_db, &db_info->db_list[0]))
	{
		paws_write_db(db_info);
	}

	select_DB(sm_, NULL);
}


//#######################################################################################
static void Run(void* sm_)
{
	//FUNC_DBG(sm_);

	paws_combiner_sm_t* sm = (paws_combiner_sm_t*)sm_;

	int iter = 0;

	LOCAL_FUNC(sm_, raise_)(sm_, ev_Start);

	while (1)
	{
		// printf("tick: iter = %d\n", iter);
		// printf("state = %s\n", state_id_2_str(sm, PAWS_SM_STL_DATA(sm_)->stl->current_state_id));

		int num_events_processed;
		do
		{
			// run the timer
			LOCAL_FUNC(sm, process_next_timer_tick)(sm_);

			// process any triggered events
			num_events_processed = LOCAL_FUNC(sm, process_events)(sm_);

			// run the master tick
			if (MASTER_SM(sm))
			{
				num_events_processed += MASTER_PUBLIC_FUNC(MASTER_SM(sm), run_tick)(MASTER_SM(sm));
			}

			// run the GOP and SOP slave ticks
			if (GOP_SM(sm))
			{
				num_events_processed += GOP_PUBLIC_FUNC(GOP_SM(sm), run_tick)(GOP_SM(sm));

				if (PRIVATE_DATA(sm).sop_sm_list)
				{
					sop_slave_sm_entity_t* head = llist_get_head(PRIVATE_DATA(sm).sop_sm_list);
					sop_slave_sm_entity_t* sm_entity;
					for (sm_entity = head; sm_entity != NULL; sm_entity = get_next_item_entry(sm_entity->l_item))
					{
						PUBLIC_FUNC(sm_entity->sm, run_tick)(sm_entity->sm);
					} 
				}
			}

		} while (num_events_processed > 0);

		if (PRIVATE_DATA(sm).reboot_needed)
		{
			if (!(PRIVATE_DATA(sm).paws_settings.spectrum_override.present))
			{
				write_all_state(sm_);
				paws_reboot_master_device();
				break;
			}
		}

		// sleep for a second
		sleep(1);

		iter++;
	}
}


//#######################################################################################
paws_combiner_sm_t* paws_combiner_sm_create(const char* sm_name, uint32_t control_plane_status_port, uint32_t control_plane_status_periodicity)
{
//	FUNC_DBG(sm_);

	paws_combiner_sm_t* paws_combiner_sm = NULL;
	if (!(paws_combiner_sm = malloc(sizeof(paws_combiner_sm_t))))
	{
		goto error_hdl;
	}

	memset(paws_combiner_sm, 0, sizeof(paws_combiner_sm_t));

	// "inherit" the paws_sm
	if (!(paws_combiner_sm->paws_sm = paws_sm_create((void*)paws_combiner_sm, &paws_combiner_sm->paws_sm_func_store, sm_name)))
	{
		goto error_hdl;
	}
	
	// copy the header from the paws_sm
	memcpy(&paws_combiner_sm->paws_sm_hdr, &paws_combiner_sm->paws_sm->paws_sm_hdr, sizeof(paws_sm_header_t));

	//  ---- called by paws_sm
	POPULATE_PAWS_SM_FUNC(paws_combiner_sm, Init_states);
	POPULATE_PAWS_SM_FUNC(paws_combiner_sm, timeout_handler);
	POPULATE_PAWS_SM_FUNC(paws_combiner_sm, event_id_2_str);
	POPULATE_PAWS_SM_FUNC(paws_combiner_sm, state_id_2_str);
	//  ---- called publicly
	POPULATE_PUBLIC_FUNC(paws_combiner_sm, Run);
	POPULATE_PUBLIC_FUNC(paws_combiner_sm, select_DB);
	POPULATE_PUBLIC_FUNC(paws_combiner_sm, DBerror);
	POPULATE_PUBLIC_FUNC(paws_combiner_sm, LocalDBerror);
	POPULATE_PUBLIC_FUNC(paws_combiner_sm, ProcessDbUpdate);
	POPULATE_PUBLIC_FUNC(paws_combiner_sm, DlSpectrumNotAvailable);
	POPULATE_PUBLIC_FUNC(paws_combiner_sm, DlSpectrumAvailable);
	POPULATE_PUBLIC_FUNC(paws_combiner_sm, UlSpectrumNotAvailable);
	POPULATE_PUBLIC_FUNC(paws_combiner_sm, UlSpectrumAvailable);
	POPULATE_PUBLIC_FUNC(paws_combiner_sm, UlSOPSpectrumNotAvailable);
	POPULATE_PUBLIC_FUNC(paws_combiner_sm, UlSOPSpectrumAvailable);
	POPULATE_PUBLIC_FUNC(paws_combiner_sm, DlNotificationSuccess);
	POPULATE_PUBLIC_FUNC(paws_combiner_sm, UlNotificationSuccess);
	POPULATE_PUBLIC_FUNC(paws_combiner_sm, setDefaultInfo);

	// Initialise
	if (!(Init(paws_combiner_sm, state_transition_table, control_plane_status_port, control_plane_status_periodicity)))
	{
		goto error_hdl;
	}
	
	return paws_combiner_sm;

error_hdl:
	if (paws_combiner_sm)  paws_combiner_sm_free(&paws_combiner_sm);
	return NULL;
}


//#######################################################################################
static void paws_combiner_private_data_free(paws_combiner_sm_t* sm_, paws_combiner_private_data_t* data)
{
	FUNC_DBG(sm_);

	if (data)
	{
		// free the master_sm
		if (data->paws_master_sm)
		{
			paws_master_sm_free(&data->paws_master_sm);
		}

		// free the gop_slave_sm
		if (data->paws_gop_slave_sm)
		{
			paws_gop_slave_sm_free(&data->paws_gop_slave_sm);
		}

		// free the sop slaves sms
		if (data->sop_sm_list)
		{
			llist_free(&data->sop_sm_list, sop_sm_entity_free);
		}

		// discovery list
		if (data->db_discovery_info.wsbd_weblist)
		{
			weblist_free(&data->db_discovery_info.wsbd_weblist);
		}

		// state_info
		if (data->state_info)
		{
			paws_sm_state_info_free(&data->state_info);
		}

		// ul_dl_spec
		if (data->ul_dl_spec)
		{
			ul_dl_spec_cfg_free(&data->ul_dl_spec);
		}

		// clear down any control plane resources
		if (data->control_plane)
		{
			paws_dal_control_plane_free(&data->control_plane);
		}

		// private loggers
        if (PRIVATE_DATA(sm_).DeviceCfg_Logger)
			logger_free(&PRIVATE_DATA(sm_).DeviceCfg_Logger);

		// zero it
		memset(data, 0, sizeof(paws_combiner_private_data_t));
	}
}


//#######################################################################################
void paws_combiner_sm_free(paws_combiner_sm_t** paws_combiner_sm)
{
	FUNC_DBG(*paws_combiner_sm);

	// check that state machine exists
	if ((paws_combiner_sm) && (*paws_combiner_sm))
	{
		// free the private data attributes
		paws_combiner_private_data_free(*paws_combiner_sm, &(PRIVATE_DATA(*paws_combiner_sm)));

		// free the paws_sm.
		// This also free the paws_sm data
		paws_sm_free(&(**paws_combiner_sm).paws_sm);
		 
		// free the struct
		free_and_null((void**)paws_combiner_sm);
	}
}






