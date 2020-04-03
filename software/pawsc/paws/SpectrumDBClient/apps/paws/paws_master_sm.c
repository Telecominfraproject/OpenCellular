/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "json-parser/json.h"
#include "json-parser/json_utils.h"
#include "utils/utils.h"

#include "paws_utils.h"
#include "paws_master_sm.h"
#include "paws_sm.h"
#include "paws_common.h"
#include "paws_combiner_sm_types.h"
#include "paws_messages.h"
#include "paws_dal_types.h"


// ##################### Function prototypes 
// definition of all static class functions called from STL
//  ---- called privately
PAWS_SM_FUNC_PROTO(Init_states);
PAWS_SM_FUNC_PROTO(Send_Init_Req);
PAWS_SM_FUNC_PROTO(Send_DeviceRegistration);
PAWS_SM_FUNC_PROTO(checkAvailableSpectrumDl);
PAWS_SM_FUNC_PROTO(Send_AvailableSpectrumReq);
PAWS_SM_FUNC_PROTO(Send_Spectrum_Use_Notify);
PAWS_SM_FUNC_PROTO(combiner_setDefaultInfo);
PAWS_SM_FUNC_PROTO(raiseRegistrationComplete);
PAWS_SM_FUNC_PROTO(combiner_DlNotificationSuccess);
PAWS_SM_FUNC_PROTO(combiner_DlSpectrumAvailable);
PAWS_SM_FUNC_PROTO(combiner_DlSpectrumNotAvailable);
// definition of static class functions which are not called directly by STL
static void Process_Init_Resp(void* sm_, json_value* resp);
static bool init_vars(void* sm_, paws_device_info_t* master_info, paws_gps_location_t* gps, float min_dbm_100k, uint16_t db_retry_secs, uint32_t max_polling_quick_secs);
static bool Init(void* sm_, State* stl, paws_device_info_t* master_info, paws_gps_location_t* gps, float min_dbm_100k, uint16_t db_retry_secs,
	logger_cfg_t* msg_log_cfg, uint32_t max_polling_quick_secs, sm_state_info_t* sm_state_info);
// public funcs
static int run_tick(void* sm_);
static void paws_master_private_data_free(paws_master_sm_t* sm_, paws_master_private_data_t* data);



// #############################################################################################################################
// #############################################################################################################################
// ###   STL definition

#define PAWSF(fnc) \
static void _##fnc(void* sm) \
{\
	LOCAL_FUNC((paws_master_sm_t*)sm, fnc)(sm); \
}

PAWSF(Start_Retry_Timer)
PAWSF(Stop_Retry_Timer)
PAWSF(Start_maxPolling_Timer)
PAWSF(Start_SpectrumPending_Timer)
PAWSF(Stop_SpectrumPending_Timer)
PAWSF(Start_SpectrumExpiry_Timer)
PAWSF(Stop_SpectrumExpiry_Timer)
PAWSF(combiner_selectDB)
PAWSF(combiner_DBerror)
PAWSF(set_spectrum_InUse)
PAWSF(check_spectrum_InUse)
PAWSF(clear_spectrum_InUse)
PAWSF(activatePendingSpectrum)
PAWSF(ActivateSpectrumRuleset)
PAWSF(check_Notify_Required)


#define QUO		SM_STATE_NO_CHANGE	


// ######## STL states
#define FOREACH_STATE(STATE) \
    STATE(INIT) \
	STATE(DB_SELECT) \
	STATE(PAWS_INIT) \
	STATE(DEVICE_REG) \
	STATE(REQUEST_NEW_SPECTRUM) \
	STATE(SELECTING_NEW_SPECTRUM) \
	STATE(SELECTING_NEXT_SPECTRUM) \
	STATE(SPECTRUM_PENDING) \
	STATE(SPECTRUM_AVAILABLE) \
	STATE(SPECTRUM_NOTIFICATION) \
	STATE(SPECTRUM_ACTIVE) \
	STATE(NO_SPECTRUM_AVAILABLE) \
	STATE(MASTER_STATE_END_MARKER) 

// ######## STL events.  Note also there are common events also defined in paws_sm.h
#define FOREACH_M_EV(EV) \
    EV(Init_Resp_Received) \
	EV(Device_reg_Resp_Received) \
	EV(MASTER_EV_END_MARKER)



#define GEN_STATE_ENUM(ENUM) ENUM,
#define GEN_STATE_STR(STRING) #STRING,

enum master_state_e {
	mDummyState = SM_STATE_START_OF_USER_IDS,
	FOREACH_STATE(GEN_STATE_ENUM)
};

static const char *master_state_str[] = {
	"dummy",
	FOREACH_STATE(GEN_STATE_STR)
};


#define GEN_M_EV_ENUM(ENUM) ev_##ENUM,
#define GEN_M_EV_STR(STRING) #STRING,

enum master_event_e {
	ev_mDummy = ev_PAWS_SM_END_EV_MARKER,
	FOREACH_M_EV(GEN_M_EV_ENUM)
};

const char *master_ev_str[] = {
	"dummy",
	FOREACH_M_EV(GEN_M_EV_STR)
};



//#######################################################################################
// STL debug functions
static const char* state_id_2_str(void* sm_, int id)
{
	UNUSED_PARAM(sm_);

	if ((id > mDummyState) && (id < MASTER_STATE_END_MARKER))
	{
		return master_state_str[(id - mDummyState)];
	}
	// otherwise its unknown
	return "Unknown";
}

static const char* event_id_2_str(void* sm_, int id)
{
	paws_master_sm_t* sm = (paws_master_sm_t*)sm_;

	if ((id > ev_mDummy) && (id < ev_MASTER_EV_END_MARKER))
	{
		return master_ev_str[(id - ev_mDummy)];
	}
	// otherwise call paws_sm 
	return (PAWS_SM_FUNC(sm, event_id_2_str)(sm_, id));
}



//################################################################################################################################################################################################


/*                      State-wide            State-wide                                                                                                    Event specific */
/* State                'pre' funcs           'post' funcs                                 Event                             Next state                     'post' funcs    */
static State state_transition_table[] = {
    { INIT ,                        { Init_states },                      { NULL },       { { ev_Start,                      DB_SELECT,                     { _combiner_selectDB } } } },
    { DB_SELECT ,                   { _Start_Retry_Timer},    {_Stop_Retry_Timer },       { { ev_Retry_Timeout,              QUO,                           { _combiner_selectDB,
                                                                                                                                                              _Start_Retry_Timer               } },
                                                                                            { ev_DB_Found,                   PAWS_INIT,                     { Send_Init_Req                    } },
                                                                                            { ev_DB_Updated,                 PAWS_INIT,                     { Send_Init_Req                    } },
                                                                                            { ev_DB_NotFound,                QUO,                           { _Start_Retry_Timer               } } } },
    { PAWS_INIT ,                   { NULL} ,                             { NULL },       { { ev_DB_Error,                   DB_SELECT,                     { _combiner_DBerror,
                                                                                                                                                              combiner_DlSpectrumNotAvailable  } },
                                                                                            { ev_DB_Response_Failure,        DB_SELECT,                     { _combiner_DBerror,
                                                                                                                                                              combiner_DlSpectrumNotAvailable  } },
                                                                                            { ev_Init_Resp_Received,         DEVICE_REG,                    { _Start_maxPolling_Timer,
                                                                                                                                                              combiner_setDefaultInfo,
                                                                                                                                                              Send_DeviceRegistration          } } } },
    { DEVICE_REG ,                   { NULL } ,                           { NULL },       { { ev_DB_Error,                   DB_SELECT,                     { _combiner_DBerror,
                                                                                                                                                              combiner_DlSpectrumNotAvailable  } },
                                                                                            { ev_DB_Response_Failure,        DB_SELECT,                     { _combiner_DBerror,
                                                                                                                                                              combiner_DlSpectrumNotAvailable  } },
                                                                                            { ev_Device_reg_Resp_Received,   REQUEST_NEW_SPECTRUM,          { raiseRegistrationComplete        } } } },
    { REQUEST_NEW_SPECTRUM ,         { _combiner_selectDB } ,             { NULL },       { { ev_DB_Error,                   QUO,                           { _combiner_DBerror,
                                                                                                                                                              _check_spectrum_InUse            } },
                                                                                            { ev_DB_Response_Failure,        DB_SELECT,                     { _combiner_DBerror,
                                                                                                                                                              _check_spectrum_InUse            } },
                                                                                            { ev_DB_NotFound,                QUO,                           { _check_spectrum_InUse            } },
                                                                                            { ev_DB_Found,                   QUO,                           { Send_AvailableSpectrumReq        } },
                                                                                            { ev_SpectrumActive,             SPECTRUM_ACTIVE,               { NULL                             } },
                                                                                            { ev_SpectrumNotActive,          DB_SELECT,                     { combiner_DlSpectrumNotAvailable  } },
                                                                                            { ev_maxPolling_Timeout,         QUO,                           { _combiner_selectDB               } },
																							{ ev_maxPollingQuick_Timeout,    QUO,                           { _combiner_selectDB               } },
																							{ ev_Available_Spectrum_Resp,    SELECTING_NEW_SPECTRUM,        { checkAvailableSpectrumDl         } } } },
    { SELECTING_NEW_SPECTRUM ,      {NULL } ,                            { NULL },        { { ev_GPS_location_changed,       REQUEST_NEW_SPECTRUM,          { _clear_spectrum_InUse            } },
                                                                                            { ev_maxPolling_Timeout,         REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_maxPollingQuick_Timeout,    REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_SpectrumAvailable,          SPECTRUM_AVAILABLE,            { NULL                             } },
                                                                                            { ev_SpectrumPending,            SPECTRUM_PENDING,              { NULL                             } },
                                                                                            { ev_SpectrumNotAvailable,       NO_SPECTRUM_AVAILABLE,         { NULL                             } } } },
    { SELECTING_NEXT_SPECTRUM   ,  { _clear_spectrum_InUse,
                                     checkAvailableSpectrumDl },         { NULL },        { { ev_GPS_location_changed,       REQUEST_NEW_SPECTRUM,          { _clear_spectrum_InUse            } },
                                                                                            { ev_maxPolling_Timeout,         REQUEST_NEW_SPECTRUM,          { NULL                             } },
                                                                                            { ev_maxPollingQuick_Timeout,    REQUEST_NEW_SPECTRUM,          { NULL                             } },
                                                                                            { ev_SpectrumAvailable,          SPECTRUM_AVAILABLE,            { NULL                             } },
                                                                                            { ev_SpectrumPending,            SPECTRUM_PENDING,              { NULL                             } },
                                                                                            { ev_SpectrumNotAvailable,       REQUEST_NEW_SPECTRUM,          { NULL                             } } } },
    { SPECTRUM_PENDING,    { _Start_SpectrumPending_Timer,
                             _check_spectrum_InUse},      {_Stop_SpectrumPending_Timer }, { { ev_GPS_location_changed,       REQUEST_NEW_SPECTRUM,          { _clear_spectrum_InUse            } },
                                                                                            { ev_maxPolling_Timeout,         REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_maxPollingQuick_Timeout,    REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_SpectrumExpiry_Timeout,     QUO,                           { _clear_spectrum_InUse,
                                                                                                                                                              combiner_DlSpectrumNotAvailable  } },
                                                                                            { ev_SpectrumActive,             QUO,                           { NULL                             } },
                                                                                            { ev_SpectrumNotActive,          QUO,                           { combiner_DlSpectrumNotAvailable  } },
                                                                                            { ev_SpectrumPending_Timeout,    SPECTRUM_AVAILABLE,            { _activatePendingSpectrum         } } } },
    { SPECTRUM_AVAILABLE,           { _ActivateSpectrumRuleset,
                                      _Start_SpectrumExpiry_Timer,
                                      combiner_DlSpectrumAvailable },     { NULL },       { { ev_GPS_location_changed,       REQUEST_NEW_SPECTRUM,          { _clear_spectrum_InUse            } },
                                                                                            { ev_maxPolling_Timeout,         REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_maxPollingQuick_Timeout,    REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_SpectrumExpiry_Timeout,     SELECTING_NEXT_SPECTRUM,       { NULL                             } },
                                                                                            { ev_DlUlSpectrumAvailable,      SPECTRUM_NOTIFICATION,         { NULL                             } } } },
    { SPECTRUM_NOTIFICATION ,       { _combiner_selectDB },               { NULL },       { { ev_DB_Error,                   QUO,                           { _check_spectrum_InUse,
	                                                                                                                                                          _combiner_DBerror                } },
                                                                                            { ev_DB_Response_Failure,        QUO,                           { _check_spectrum_InUse,
	                                                                                                                                                          _combiner_DBerror                } },
                                                                                            { ev_DB_NotFound,                QUO,                           { _check_spectrum_InUse            } },
                                                                                            { ev_DB_Found,                   QUO,                           { _check_Notify_Required           } },
                                                                                            { ev_SpectrumActive,             SPECTRUM_ACTIVE,               { NULL                             } },
                                                                                            { ev_SpectrumNotActive,          DB_SELECT,                     { combiner_DlSpectrumNotAvailable  } },
                                                                                            { ev_GPS_location_changed,       REQUEST_NEW_SPECTRUM,          { _clear_spectrum_InUse            } },
                                                                                            { ev_maxPolling_Timeout,         REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_maxPollingQuick_Timeout,    REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_SpectrumExpiry_Timeout,     SELECTING_NEXT_SPECTRUM,       { NULL                             } },
                                                                                            { ev_Notification_Required,      QUO,                           { Send_Spectrum_Use_Notify         } },
                                                                                            { ev_Notification_Not_Required,  SPECTRUM_ACTIVE,               { _set_spectrum_InUse,
	                                                                                                                                                          combiner_DlNotificationSuccess   } },
                                                                                            { ev_Notification_Success,       SPECTRUM_ACTIVE,               { _set_spectrum_InUse,
	                                                                                                                                                          combiner_DlNotificationSuccess   } },
																							{ ev_Notification_Failure,       QUO,                           { _check_spectrum_InUse            } } } },
    { SPECTRUM_ACTIVE ,             { NULL },                             { NULL },       { { ev_GPS_location_changed,       REQUEST_NEW_SPECTRUM,          { _clear_spectrum_InUse            } },
                                                                                            { ev_maxPolling_Timeout,         REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_maxPollingQuick_Timeout,    REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_SpectrumExpiry_Timeout,     SELECTING_NEXT_SPECTRUM,       { NULL                             } } } },
    { NO_SPECTRUM_AVAILABLE ,       { _Stop_SpectrumExpiry_Timer,
	                                  combiner_DlSpectrumNotAvailable},   { NULL },       {	{ ev_maxPolling_Timeout,         REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_maxPollingQuick_Timeout,    REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_GPS_location_changed,       REQUEST_NEW_SPECTRUM,          { _clear_spectrum_InUse            } } } }
};


//#######################################################################################
static void Init_states(void* sm_)
{
	paws_master_sm_t* sm = (paws_master_sm_t*)sm_;

	paws_master_private_data_free(sm, &(PRIVATE_DATA(sm)));

	// direct call
	PAWS_SM_FUNC(sm, Init_states)(sm);
}



//#######################################################################################
static void Send_Init_Req(void* sm_)
{
    FUNC_DBG(sm_);

	paws_master_sm_t* sm = (paws_master_sm_t*)sm_;

	LOG_PRINT(sm_, LOG_NOTICE, "----> INIT-REQ MASTER");

	json_value* resp = post_Init_Request(sm_, &(PAWS_SM_DATA((paws_sm_t*)sm_)->master_info), &(PAWS_SM_DATA((paws_sm_t*)sm_)->gps),
		&(PAWS_SM_DATA((paws_sm_t*)sm)->selected_db.db_url));
	Process_Init_Resp(sm_, resp);
	json_value_free(resp);
}

//#######################################################################################
static void Send_DeviceRegistration(void* sm_)
{
    FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_Device_reg_Resp_Received);
}

//#######################################################################################
static void checkAvailableSpectrumDl(void* sm_)
{
    FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, checkAvailableSpectrum)(sm_);
}

//#######################################################################################
static void Send_AvailableSpectrumReq(void* sm_)
{
    FUNC_DBG(sm_);

	LOG_PRINT(sm_, LOG_NOTICE, "----> AVAIL-SPEC-REQ MASTER");

	json_value* resp = post_Avail_Spectrum_Request(sm_, &(PAWS_SM_DATA((paws_sm_t*)sm_)->master_info), &(PAWS_SM_DATA((paws_sm_t*)sm_)->gps),
		&(PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db.db_url));
	// make a note of reception time.  This is used to compare time phase with TVWSDB server
	LOCAL_FUNC(sm_, Process_Available_Spectrum_Resp)(sm_, resp, LTE_DL, time(NULL));
	json_value_free(resp);
}

//#######################################################################################
static void Send_Spectrum_Use_Notify(void* sm_)
{
    FUNC_DBG(sm_);

	LOG_PRINT(sm_, LOG_NOTICE, "----> NOTIFY-REQ MASTER");

	json_value* resp = post_Notify_Request(sm_, &(PAWS_SM_DATA((paws_sm_t*)sm_)->master_info), &(PAWS_SM_DATA((paws_sm_t*)sm_)->gps),
		&(PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db.db_url), PAWS_SM_DATA((paws_sm_t*)sm_)->selected_spectrum, 13);
	LOCAL_FUNC(sm_, Process_Notify_Use_Resp)(sm_, resp);
	json_value_free(resp);
}

//#######################################################################################
static void combiner_setDefaultInfo(void* sm_)
{
    FUNC_DBG(sm_);
	COMBINER_PUBLIC_FUNC(CREATOR_SM(sm_), setDefaultInfo)(CREATOR_SM(sm_), PAWS_SM_DATA(sm_)->default_max_location_change, PAWS_SM_DATA(sm_)->default_max_polling_secs);
}

//#######################################################################################
static void raiseRegistrationComplete(void* sm_)
{
    FUNC_DBG(sm_);
}

//#######################################################################################
static void combiner_DlNotificationSuccess(void* sm_)
{
    FUNC_DBG(sm_);
    COMBINER_PUBLIC_FUNC(CREATOR_SM(sm_), DlNotificationSuccess)(CREATOR_SM(sm_));
}

//#######################################################################################
static void combiner_DlSpectrumAvailable(void* sm_)
{
    FUNC_DBG(sm_);
    COMBINER_PUBLIC_FUNC(CREATOR_SM(sm_), DlSpectrumAvailable)(CREATOR_SM(sm_));
}

//#######################################################################################
static void combiner_DlSpectrumNotAvailable(void* sm_)
{
    FUNC_DBG(sm_);
    COMBINER_PUBLIC_FUNC(CREATOR_SM(sm_), DlSpectrumNotAvailable)(CREATOR_SM(sm_));
}

//#######################################################################################
static void Process_Init_Resp(void* sm_, json_value* resp)
{
    FUNC_DBG(sm_);

	paws_master_sm_t* sm = (paws_master_sm_t*)sm_;

	if (LOCAL_FUNC(sm, Check_HTTP_Result)(sm_, resp, true))
	{
		if (LOCAL_FUNC(sm, Check_Valid_Resp)(sm_, resp, true))
		{
			//  parse the ruleset info
			json_value* rulesetInfos = NULL;
			if ((rulesetInfos = json_get(resp, "result/rulesetInfos")))
			{
				// walk list of rulesetInfo until we have processed a maxlocationChange param
				if (rulesetInfos->type == json_array)
				{
					int64_t max_polling_secs = 0;
					double max_location_change  = 0;
					for (uint32_t i=0; i < (rulesetInfos->u.array.length && ((max_location_change==0) || (max_polling_secs == 0))) ; i++)
					{
						json_value* ruleset_info = rulesetInfos->u.array.values[i];
						if (ruleset_info->type == json_object)
						{
							if ((json_get_int(ruleset_info, "maxPollingSecs", &max_polling_secs)))
							{
								PAWS_SM_DATA(sm)->default_max_polling_secs = max_polling_secs;
							}
							if ((json_get_double(ruleset_info, "maxLocationChange", (double*)&max_location_change)))
							{
                                PAWS_SM_DATA(sm)->default_max_location_change = (float)max_location_change;
							}
						}
					}
				}
			}	
			LOG_PRINT(sm_, LOG_NOTICE, "<---- INIT-RESP MASTER");
			LOCAL_FUNC(sm, raise_)(sm_, (int)ev_Init_Resp_Received);
		}
	}
}



//#######################################################################################
static bool init_vars(void* sm_, paws_device_info_t* master_info, paws_gps_location_t* gps, float min_dbm_100k, uint16_t db_retry_secs, uint32_t max_polling_quick_secs)
{
    FUNC_DBG(sm_);

    paws_master_sm_t* sm = (paws_master_sm_t*)sm_;

    if (!(PAWS_SM_FUNC(sm, Init)(sm_, min_dbm_100k, db_retry_secs, max_polling_quick_secs)))
    {
        return false;
    }

	// store gps
	LOCAL_FUNC(sm, set_gps)(sm, gps);

	// store fap info
	LOCAL_FUNC(sm, set_master_info)(sm, master_info);

    return true;
}


//#######################################################################################
static bool Init(void* sm_, State* stl, paws_device_info_t* master_info, paws_gps_location_t* gps, float min_dbm_100k, uint16_t db_retry_secs, 
	logger_cfg_t* msg_log_cfg, uint32_t max_polling_quick_secs, sm_state_info_t* sm_state_info)
{
	FUNC_DBG(sm_);

	paws_master_sm_t* sm = (paws_master_sm_t*)sm_;

	// determine initial state and whether we should run the "pre" functions
	int initial_state = (sm_state_info) ? sm_state_info->stl_current_state : INIT;
	bool run_pre = (sm_state_info == NULL);
	LOG_PRINT(sm_, LOG_INFO, "Initialising STL.  state = %s", LOCAL_FUNC(sm_, state_id_2_str)(sm_, initial_state));
	// generic Init
	if (!(LOCAL_FUNC(sm, stl_init)(sm_, stl, initial_state, run_pre)))
	{
		return false;
	}

	// initialise variables
	if (!(init_vars(sm, master_info, gps, min_dbm_100k, db_retry_secs, max_polling_quick_secs)))
	{
		return false;
	}

	// create logger
	LOCAL_FUNC(sm, set_msgLogInfo)(sm_, msg_log_cfg);

	// process pre-boot state
	if (sm_state_info)
	{
		LOCAL_FUNC(sm, process_state_attributes)(sm_, sm_state_info);
		LOCAL_FUNC(sm, process_state_timers)(sm_, sm_state_info->timer_info);
	}

	return true;
}


//#######################################################################################
static int run_tick(void* sm_)
{
//	FUNC_DBG(sm_);

	paws_master_sm_t* sm = (paws_master_sm_t*)sm_;

	// run the timer
	LOCAL_FUNC(sm, process_next_timer_tick)(sm_);

	// process any triggered events
	return LOCAL_FUNC(sm, process_events)(sm_);
}



//#######################################################################################
paws_master_sm_t* paws_master_sm_create(void* creator, const char* sm_name, paws_device_info_t* master_info, paws_gps_location_t* gps, float min_dbm_100k, uint16_t db_retry_secs,
	logger_cfg_t* msg_log_cfg, uint32_t max_polling_quick_secs, sm_state_info_t* sm_state_info)
{
  //  FUNC_DBG(sm_);

    paws_master_sm_t* paws_master_sm = NULL;
    if (!(paws_master_sm = malloc(sizeof(paws_master_sm_t))))
    {
        goto error_hdl;
    }
    memset(paws_master_sm, 0, sizeof(paws_master_sm_t));
    
	// "inherit" the paws_sm
    if (!(paws_master_sm->paws_sm = paws_sm_create(creator, &paws_master_sm->paws_sm_func_store, sm_name)))
    {
        goto error_hdl;
    }

	LOG_PRINT(creator, LOG_NOTICE, "Creating MASTER SM");

    // copy the header from the paws_sm
    memcpy(&paws_master_sm->paws_sm_hdr, &paws_master_sm->paws_sm->paws_sm_hdr, sizeof(paws_sm_header_t));

    //  ---- called by paws_sm
	POPULATE_PAWS_SM_FUNC(paws_master_sm, Init_states);
	POPULATE_PAWS_SM_FUNC(paws_master_sm, state_id_2_str);
	POPULATE_PAWS_SM_FUNC(paws_master_sm, event_id_2_str);

	//  ---- called publicly
	POPULATE_PUBLIC_FUNC(paws_master_sm, run_tick);

    // Initialise
    if (!(Init(paws_master_sm, state_transition_table, master_info, gps, min_dbm_100k, db_retry_secs, msg_log_cfg, max_polling_quick_secs, sm_state_info)))
    {
        goto error_hdl;
    }

    return paws_master_sm;

error_hdl:
    paws_master_sm_free(&paws_master_sm);
    return NULL;
}


static void paws_master_private_data_free(paws_master_sm_t* sm_, paws_master_private_data_t* data)
{
    FUNC_DBG(sm_);

    if (data)
    {
        // nothing to free
    }
}


void paws_master_sm_free(paws_master_sm_t** paws_master_sm)
{
    FUNC_DBG(*paws_master_sm);

    // check that state machine exists
    if ((paws_master_sm) && (*paws_master_sm))
    {
		LOG_PRINT(*paws_master_sm, LOG_NOTICE, "Deleting MASTER SM");

        // free the private data attributes
		paws_master_private_data_free(*paws_master_sm, &(PRIVATE_DATA(*paws_master_sm)));

        // free the paws_sm
        // This also free the paws_sm data
        paws_sm_free(&(**paws_master_sm).paws_sm);

        // free the struct
        free_and_null((void**)paws_master_sm);
    }
}

//#######################################################################################


