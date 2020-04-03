/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "utils/utils.h"

#include "paws_utils.h"
#include "paws_slave_sm.h"
#include "paws_sm.h"
#include "paws_common.h"
#include "paws_combiner_sm_types.h"
#include "paws_messages.h"



// ##################### Function prototypes 
// definition of all static class functions called from STL
//  ---- called privately
PAWS_SM_FUNC_PROTO(checkAvailableSpectrumUl);
PAWS_SM_FUNC_PROTO(combiner_UlSpectrumAvailable);
PAWS_SM_FUNC_PROTO(combiner_UlSpectrumNotAvailable);
PAWS_SM_FUNC_PROTO(combiner_UlNotificationSuccess);

static int run_tick(void* sm_);
// GOP funcs
static void gop_Send_AvailableSpectrumReq(void* sm_);
static void gop_Send_Notifications(void* sm_, paws_slave_info_t* slave_info);
static void gop_Send_Spectrum_Use_Notify(void* sm_);
static void gop_Send_Spectrum_Use_Notify_New(void* sm_);
static bool gop_Process_Notify_Use_Resp(void* sm_, json_value* resp);
static void gop_set_slaveInfo(void* sm_, paws_slave_info_t* slave_info);
static paws_slave_info_t* gop_get_slaveInfo(void* sm_);

// SOP funcs
static void sop_Send_AvailableSpectrumReq(void* sm_);
static void sop_Send_Spectrum_Use_Notify(void* sm_);
static void sop_Send_Spectrum_Use_Notify_New(void* sm_);
static bool sop_Process_Notify_Use_Resp(void* sm_, json_value* resp);
static void sop_set_deviceInfo(void* sm_, paws_device_info_t* device_info);
static paws_device_info_t* sop_get_deviceInfo(void* sm_);
static bool sop_Init(void* sm_, State* stl, paws_device_info_t* master_info, paws_gps_location_t* gps, float default_max_location_change, uint32_t default_max_polling_secs,
	paws_device_info_t *device_info, float min_dbm_100k, uint16_t db_retry_secs, logger_cfg_t* msg_log_cfg, uint32_t max_polling_quick_secs, sm_state_info_t* sm_state_info);


#define SLAVE_FUNC(sm, fnc)						((paws_slave_sm_t*)sm)->slave_hdr.funcs->fnc
#define PARENT_SLAVE_FUNC(sm, fnc)				(sm)->slave_sm->local_slave_funcs.fnc
#define PARENT_SLAVE_SM_FUNC(sm, fnc)			(sm)->slave_sm->local_paws_sm_funcs.fnc
#define SLAVE_SM_DATA(sm)						((paws_slave_sm_t*)sm)->slave_hdr.data





// #######################################################################################################################
// #######################################################################################################################
// #######################################################################################################################
// #######################################################################################################################
//   slave_sm
// #######################################################################################################################
// #######################################################################################################################
// #######################################################################################################################
// #######################################################################################################################


#define PAWSF(fnc) \
static void _##fnc(void* sm) \
{\
	LOCAL_FUNC((paws_slave_sm_t*)sm, fnc)(sm); \
}

#define SLAVEF(fnc) \
static void _##fnc(void* sm) \
{\
	SLAVE_FUNC((paws_slave_sm_t*)sm, fnc)(sm); \
}
PAWSF(Clear_Spectrums)
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

SLAVEF(Send_AvailableSpectrumReq)
SLAVEF(Send_Spectrum_Use_Notify)
SLAVEF(Send_Spectrum_Use_Notify_New)
SLAVEF(combiner_UlSpectrumAvailable)
SLAVEF(combiner_UlSpectrumNotAvailable)



#define QUO		SM_STATE_NO_CHANGE

// ######## STL states
#define FOREACH_STATE(STATE) \
    STATE(INIT) \
	STATE(REQUEST_NEW_SPECTRUM) \
	STATE(WAITING_SPECTRUM_RESP) \
	STATE(SELECTING_NEW_SPECTRUM) \
	STATE(SELECTING_NEXT_SPECTRUM) \
	STATE(SPECTRUM_PENDING) \
	STATE(SPECTRUM_AVAILABLE) \
	STATE(SPECTRUM_NOTIFICATION) \
	STATE(SPECTRUM_ACTIVE) \
	STATE(NO_SPECTRUM_AVAILABLE) \
	STATE(SLAVE_STATE_END_MARKER) 

// ######## STL events.  Note also there are common events also defined in paws_sm.h
#define FOREACH_M_EV(EV) \
    EV(New_Slave_Info) \
	EV(SLAVE_EV_END_MARKER)


#define GEN_STATE_ENUM(ENUM) ENUM,
#define GEN_STATE_STR(STRING) #STRING,

enum slave_state_e {
	sDummyState = SM_STATE_START_OF_USER_IDS,
	FOREACH_STATE(GEN_STATE_ENUM)
};

static const char *slave_state_str[] = {
	"dummy",
	FOREACH_STATE(GEN_STATE_STR)
};


#define GEN_M_EV_ENUM(ENUM) ev_##ENUM,
#define GEN_M_EV_STR(STRING) #STRING,

enum slave_event_e {
	ev_sDummy = ev_PAWS_SM_END_EV_MARKER,
	FOREACH_M_EV(GEN_M_EV_ENUM)
};

const char *slave_ev_str[] = {
	"dummy",
	FOREACH_M_EV(GEN_M_EV_STR)
};




//#######################################################################################
// STL debug functions
static const char* state_id_2_str(void* sm_, int id)
{
	UNUSED_PARAM(sm_);

	if ((id > sDummyState) && (id < SLAVE_STATE_END_MARKER))
	{
		return slave_state_str[(id - sDummyState)];
	}
	// otherwise its unknown
	return "Unknown";
}

static const char* event_id_2_str(void* sm_, int id)
{
	paws_slave_sm_t* sm = (paws_slave_sm_t*)sm_;

	if ((id > ev_sDummy) && (id < ev_SLAVE_EV_END_MARKER))
	{
		return slave_ev_str[(id - ev_sDummy)];
	}
	// otherwise call paws_sm 
	return (PAWS_SM_FUNC(sm, event_id_2_str)(sm_, id));
}



//################################################################################################################################################################################################


/*                      State-wide            State-wide                                                                                                    Event specific */
/* State                'pre' funcs           'post' funcs                                 Event                             Next state                     'post' funcs    */
static State state_transition_table[] = {
	{ INIT ,                         { _Clear_Spectrums },   { _Stop_Retry_Timer },       { { ev_Start,                      REQUEST_NEW_SPECTRUM,          { _Start_maxPolling_Timer          } },
																							{ ev_Retry_Timeout,              REQUEST_NEW_SPECTRUM,          { NULL                             } } } },
    { REQUEST_NEW_SPECTRUM ,         { _combiner_selectDB } ,             { NULL },       { { ev_DB_Error,                   QUO,                           { _combiner_DBerror,
                                                                                                                                                              _check_spectrum_InUse            } },
                                                                                            { ev_DB_Response_Failure,        QUO,                           { _combiner_DBerror,
                                                                                                                                                              _check_spectrum_InUse            } },
                                                                                            { ev_DB_NotFound,                QUO,                           { _combiner_DBerror,
                                                                                                                                                              _check_spectrum_InUse            } },
                                                                                            { ev_DB_Found,                   QUO,                           { _Send_AvailableSpectrumReq       } },
																							{ ev_DB_Updated,                 QUO,			                { _Send_AvailableSpectrumReq       } },
																							{ ev_SpectrumActive,             SPECTRUM_ACTIVE,               { NULL                             } },
                                                                                            { ev_SpectrumNotActive,          INIT,                          { _Start_Retry_Timer,
          	                                                                                                                                                  _combiner_UlSpectrumNotAvailable } },
	                                                                                        { ev_Retry_Timeout,              QUO,                           { _combiner_selectDB               } },
																							{ ev_maxPolling_Timeout,         QUO,                           { _combiner_selectDB               } },
																							{ ev_maxPollingQuick_Timeout,    QUO,                           { _combiner_selectDB               } },
																							{ ev_Available_Spectrum_Resp,    SELECTING_NEW_SPECTRUM,        { checkAvailableSpectrumUl         } } } },
    { WAITING_SPECTRUM_RESP ,         { NULL  } ,                         { NULL },       { { ev_DB_Error,                   REQUEST_NEW_SPECTRUM,          { _check_spectrum_InUse            } },
                                                                                            { ev_DB_Response_Failure,        REQUEST_NEW_SPECTRUM,          { _check_spectrum_InUse            } },
                                                                                            { ev_DB_NotFound,                REQUEST_NEW_SPECTRUM,          { _check_spectrum_InUse            } },
																							{ ev_SpectrumActive,             SPECTRUM_ACTIVE,               { NULL                             } },
                                                                                            { ev_SpectrumNotActive,          INIT,                          { _Start_Retry_Timer,
          	                                                                                                                                                  _combiner_UlSpectrumNotAvailable } },
	                                                                                        { ev_Retry_Timeout,              REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_maxPolling_Timeout,         REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_maxPollingQuick_Timeout,    REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_Available_Spectrum_Resp,    SELECTING_NEW_SPECTRUM,        { checkAvailableSpectrumUl         } } } },
    { SELECTING_NEW_SPECTRUM,       {NULL } ,                            { NULL },        { { ev_GPS_location_changed,       REQUEST_NEW_SPECTRUM,          { _clear_spectrum_InUse            } },
                                                                                            { ev_maxPolling_Timeout,         REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_maxPollingQuick_Timeout,    REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_SpectrumAvailable,          SPECTRUM_AVAILABLE,            { NULL                             } },
                                                                                            { ev_SpectrumPending,            SPECTRUM_PENDING,              { NULL                             } },
                                                                                            { ev_SpectrumNotAvailable,       NO_SPECTRUM_AVAILABLE,         { NULL                             } } } },
    { SELECTING_NEXT_SPECTRUM ,     { _clear_spectrum_InUse,
                                      checkAvailableSpectrumUl },        { NULL },        { { ev_GPS_location_changed,       REQUEST_NEW_SPECTRUM,          { _clear_spectrum_InUse            } },
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
                                                                                                                                                              _combiner_UlSpectrumNotAvailable } },
                                                                                            { ev_SpectrumActive,             QUO,                           { NULL                             } },
                                                                                            { ev_SpectrumNotActive,          QUO,                           { _combiner_UlSpectrumNotAvailable } },
                                                                                            { ev_SpectrumPending_Timeout,    SPECTRUM_AVAILABLE,            { _activatePendingSpectrum         } } } },
    { SPECTRUM_AVAILABLE,           { _ActivateSpectrumRuleset,
                                      _Start_SpectrumExpiry_Timer,
                                      _combiner_UlSpectrumAvailable },    { NULL },       { { ev_GPS_location_changed,       REQUEST_NEW_SPECTRUM,          { _clear_spectrum_InUse            } },
                                                                                            { ev_maxPolling_Timeout,         REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_maxPollingQuick_Timeout,    REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_SpectrumExpiry_Timeout,     SELECTING_NEXT_SPECTRUM,       { NULL                             } },
                                                                                            { ev_DlUlSpectrumAvailable,      SPECTRUM_NOTIFICATION,         { NULL                             } } } },
    { SPECTRUM_NOTIFICATION ,       { _check_Notify_Required },               { NULL },   { { ev_DB_Error,                   INIT,                          { _combiner_DBerror,
                                                                                                                                                              _check_spectrum_InUse            } },
                                                                                            { ev_DB_Response_Failure,        QUO,                           { _combiner_DBerror,
                                                                                                                                                              _check_spectrum_InUse            } },
                                                                                            { ev_DB_NotFound,                QUO,                           { _combiner_DBerror,
	                                                                                                                                                          _check_spectrum_InUse            } },
                                                                                            { ev_SpectrumActive,             SPECTRUM_ACTIVE,               { NULL                             } },
                                                                                            { ev_SpectrumNotActive,          INIT,                          { _Start_Retry_Timer,
	                                                                                                                                                          _combiner_UlSpectrumNotAvailable } },
                                                                                            { ev_GPS_location_changed,       REQUEST_NEW_SPECTRUM,          { _clear_spectrum_InUse            } },
                                                                                            { ev_maxPolling_Timeout,         REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_maxPollingQuick_Timeout,    REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_SpectrumExpiry_Timeout,     SELECTING_NEXT_SPECTRUM,       { NULL                             } },

                                                                                            { ev_Notification_Required,      QUO,                           { _Send_Spectrum_Use_Notify        } },
                                                                                            { ev_Notification_Not_Required,  SPECTRUM_ACTIVE,               { _set_spectrum_InUse,
	                                                                                                                                                          combiner_UlNotificationSuccess   } },
                                                                                            { ev_Notification_Success,       SPECTRUM_ACTIVE,               { _set_spectrum_InUse,
	                                                                                                                                                          combiner_UlNotificationSuccess   } },
																						    { ev_Notification_Failure,       QUO,                           { _check_spectrum_InUse            } } } },
    { SPECTRUM_ACTIVE ,             { NULL },                             { NULL },       { { ev_GPS_location_changed,       REQUEST_NEW_SPECTRUM,          { _clear_spectrum_InUse            } },
                                                                                            { ev_maxPolling_Timeout,         REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_maxPollingQuick_Timeout,    REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_SpectrumExpiry_Timeout,     SELECTING_NEXT_SPECTRUM,       { NULL                             } },
																							{ ev_New_Slave_Info,             QUO,                           { _check_Notify_Required           } },
																							{ ev_Notification_Required,      QUO,                           { _Send_Spectrum_Use_Notify_New    } } } },
	{ NO_SPECTRUM_AVAILABLE ,       { _Stop_SpectrumExpiry_Timer,
	                                 _combiner_UlSpectrumNotAvailable},   { NULL },       { { ev_maxPolling_Timeout,         REQUEST_NEW_SPECTRUM,			{ NULL                             } },
																							{ ev_maxPollingQuick_Timeout,    REQUEST_NEW_SPECTRUM,          { NULL                             } },
																							{ ev_GPS_location_changed,       REQUEST_NEW_SPECTRUM,          { _clear_spectrum_InUse            } } } }
};






//#######################################################################################
static void checkAvailableSpectrumUl(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, checkAvailableSpectrum)(sm_);
}


//#######################################################################################
static void combiner_UlSpectrumAvailable(void* sm_)
{
	FUNC_DBG(sm_);
	COMBINER_PUBLIC_FUNC(CREATOR_SM(sm_), UlSpectrumAvailable)(CREATOR_SM(sm_));
}

//#######################################################################################
static void combiner_UlSpectrumNotAvailable(void* sm_)
{
	FUNC_DBG(sm_);
	COMBINER_PUBLIC_FUNC(CREATOR_SM(sm_), UlSpectrumNotAvailable)(CREATOR_SM(sm_));
}

//#######################################################################################
static void combiner_UlNotificationSuccess(void* sm_)
{
	FUNC_DBG(sm_);
	COMBINER_PUBLIC_FUNC(CREATOR_SM(sm_), UlNotificationSuccess)(CREATOR_SM(sm_));
}

//#######################################################################################
static void Start(void* sm_)
{
	FUNC_DBG(sm_);
	LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_Start);
}


//#######################################################################################
static int run_tick(void* sm_)
{
//	FUNC_DBG(sm_);

	paws_slave_sm_t* sm = (paws_slave_sm_t*)sm_;

	// run the timer
	LOCAL_FUNC(sm, process_next_timer_tick)(sm_);

	// process any triggered events
	return LOCAL_FUNC(sm, process_events)(sm_);
}



//#######################################################################################
static bool init_vars(void* sm_, paws_device_info_t* set_master_info, paws_gps_location_t* gps, float default_max_location_change, uint32_t default_max_polling_secs, 
	float min_dbm_100k, uint16_t db_retry_secs, uint32_t max_polling_quick_secs)
{
    FUNC_DBG(sm_);

    paws_slave_sm_t* sm = (paws_slave_sm_t*)sm_;

    if (!(PAWS_SM_FUNC(sm, Init)(sm_, min_dbm_100k, db_retry_secs, max_polling_quick_secs)))
    {
        return false;
    }

	// store gps
	LOCAL_FUNC(sm, set_gps)(sm, gps);

	// store fap info
	LOCAL_FUNC(sm, set_master_info)(sm, set_master_info);

	// default info
	LOCAL_FUNC(sm, set_defaultInfo)(sm_, default_max_location_change, default_max_polling_secs);

    return true;
}


//#######################################################################################
static bool slave_Init(void* sm_, State* stl, paws_device_info_t* master_info, paws_gps_location_t* gps, float default_max_location_change, uint32_t default_max_polling_secs, 
	float min_dbm_100k, uint16_t db_retry_secs, logger_cfg_t* msg_log_cfg, uint32_t max_polling_quick_secs, sm_state_info_t* sm_state_info)
{
    FUNC_DBG(sm_);

    paws_slave_sm_t* sm = (paws_slave_sm_t*)sm_;

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
    if (!(init_vars(sm, master_info, gps, default_max_location_change, default_max_polling_secs, min_dbm_100k, db_retry_secs, max_polling_quick_secs)))
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
static void paws_slave_sm_data_free(paws_slave_sm_t* sm_, paws_slave_sm_data_t* data)
{
	FUNC_DBG(sm_);

	if (data)
	{
		// nothing to free
	}
}


//#######################################################################################
static void paws_slave_sm_free(paws_slave_sm_t** paws_slave_sm)
{
	FUNC_DBG(*paws_slave_sm);

	// check that state machine exists
	if ((paws_slave_sm) && (*paws_slave_sm))
	{
		// free the private data attributes
		paws_slave_sm_data_free(*paws_slave_sm, SLAVE_SM_DATA(*paws_slave_sm));

		// free the paws_sm
		// This also free the paws_sm data
		paws_sm_free(&(**paws_slave_sm).paws_sm);

		// free the struct
		free_and_null((void**)paws_slave_sm);
	}
}

//#######################################################################################

#define S_POPULATE_PAWS_SM_FUNC(sm,fnc) \
	sm->paws_sm_hdr.funcs->fnc = fnc; \
	sm->local_paws_sm_funcs.fnc = fnc;

#define POPULATE_SLAVE_FUNC(sm,fnc) \
	sm->slave_hdr.funcs->fnc = fnc; \
	sm->local_slave_funcs.fnc = fnc;

#define POPULATE_SLAVE_FUNC2(sm,fnc,fname) \
	sm->slave_hdr.funcs->fnc = fname; \
	sm->local_slave_funcs.fnc = fname;

#define S_POPULATE_PUBLIC_FUNC(sm,fnc) \
	sm->slave_hdr.public_funcs->fnc = fnc; \
	sm->local_public_funcs.fnc = fnc;

static paws_slave_sm_t* paws_slave_sm_create(void* creator, paws_sm_funcs_t* paws_sm_funcs, paws_slave_sm_funcs_t* slave_funcs, paws_slave_public_funcs_t* public_funcs, char* sm_name)
{
//	FUNC_DBG(sm_);

	paws_slave_sm_t* paws_slave_sm = NULL;
	if (!(paws_slave_sm = malloc(sizeof(paws_slave_sm_t))))
	{
		goto error_hdl;
	}
	memset(paws_slave_sm, 0, sizeof(paws_slave_sm_t));

	paws_slave_sm->slave_hdr.funcs = slave_funcs;
	paws_slave_sm->slave_hdr.public_funcs = public_funcs;
	paws_slave_sm->slave_hdr.data = &paws_slave_sm->slave_data_store;

	// "inherit" the paws_sm
	if (!(paws_slave_sm->paws_sm = paws_sm_create(creator, paws_sm_funcs, sm_name)))
	{
		goto error_hdl;
	}
	
	// copy the header from the paws_sm
	memcpy(&paws_slave_sm->paws_sm_hdr, &paws_slave_sm->paws_sm->paws_sm_hdr, sizeof(paws_sm_header_t));

	//  ---- called by paws_sm
	// for safety, copy the parent paws sm first, then overwrite
	memcpy(&paws_slave_sm->local_paws_sm_funcs, paws_slave_sm->paws_sm_hdr.funcs, sizeof(paws_sm_funcs_t));
	S_POPULATE_PAWS_SM_FUNC(paws_slave_sm, event_id_2_str);
	S_POPULATE_PAWS_SM_FUNC(paws_slave_sm, state_id_2_str);
	//  ---- slave funcs
	POPULATE_SLAVE_FUNC2(paws_slave_sm, Init, slave_Init);
	POPULATE_SLAVE_FUNC(paws_slave_sm, init_vars);
	POPULATE_SLAVE_FUNC(paws_slave_sm, checkAvailableSpectrumUl);
	POPULATE_SLAVE_FUNC(paws_slave_sm, combiner_UlSpectrumAvailable);
	POPULATE_SLAVE_FUNC(paws_slave_sm, combiner_UlSpectrumNotAvailable);
	POPULATE_SLAVE_FUNC(paws_slave_sm, combiner_UlNotificationSuccess);
	//  ---- public funcs
	S_POPULATE_PUBLIC_FUNC(paws_slave_sm, Start);
	S_POPULATE_PUBLIC_FUNC(paws_slave_sm, run_tick);

	return paws_slave_sm;

error_hdl:
	paws_slave_sm_free(&paws_slave_sm);
	return NULL;
}





// #######################################################################################################################
// #######################################################################################################################
// #######################################################################################################################
//   GOP
// #######################################################################################################################
// #######################################################################################################################
// #######################################################################################################################



//#######################################################################################
static void gop_Send_AvailableSpectrumReq(void* sm_)
{
	FUNC_DBG(sm_);

	LOG_PRINT(sm_, LOG_NOTICE, "----> AVAIL-SPEC-REQ GOP");

	json_value* resp = post_Slave_GOP_Available_Spectrum_Request(sm_, &(PAWS_SM_DATA((paws_sm_t*)sm_)->master_info), &(PAWS_SM_DATA((paws_sm_t*)sm_)->gps),
		&(PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db.db_url));
	LOCAL_FUNC(sm_, Process_Available_Spectrum_Resp)(sm_, resp, LTE_UL, time(NULL));
	json_value_free(resp);
}


//#######################################################################################
static void gop_Send_Notifications(void* sm_, paws_slave_info_t* slave_info)
{
	FUNC_DBG(sm_);
	paws_device_info_t* device = NULL;
	int i = 0;

	for (i = 0; i < slave_info->num_devices; i++)
	{
		device = &slave_info->device_info[i];

		LOG_PRINT(sm_, LOG_NOTICE, "----> NOTIFY-REQ GOP(device = %s)", device->unique_id);

		json_value* resp = post_GOP_slave_Notify_Request(sm_, &(PAWS_SM_DATA((paws_sm_t*)sm_)->master_info), device, &(PAWS_SM_DATA((paws_sm_t*)sm_)->gps), 
			&(PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db.db_url), PAWS_SM_DATA((paws_sm_t*)sm_)->selected_spectrum, 13);

		if (!(SLAVE_FUNC(sm_, Process_Notify_Use_Resp(sm_,resp))))
		{
			// Nothing defined

			// possible enhancement is to block slave

		}
		json_value_free(resp);
	}
	LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_Notification_Success);
}


//#######################################################################################
static void gop_Send_Spectrum_Use_Notify(void* sm_)
{
	FUNC_DBG(sm_);

	paws_gop_slave_sm_t* sm = (paws_gop_slave_sm_t*)sm_;

	if ((!PAWS_SM_DATA((paws_sm_t*)sm_)->selected_spectrum))
	{
		LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_DB_NotFound);
		return;
	}

	SLAVE_FUNC(sm_, Send_Notifications(sm_, &PRIVATE_DATA(sm).slave_info));
}


//#######################################################################################
static void gop_Send_Spectrum_Use_Notify_New(void* sm_)
{
	FUNC_DBG(sm_);
}

//#######################################################################################
static bool gop_Process_Notify_Use_Resp(void* sm_, json_value* resp)
{
	FUNC_DBG(sm_);

	if (LOCAL_FUNC(sm_, Check_HTTP_Result)(sm_, resp, false))
	{
		if (LOCAL_FUNC(sm_, Check_Valid_Resp)(sm_, resp, false))
		{
			// raise event to show a valid RESP was received
			LOG_PRINT(sm_, LOG_NOTICE, "<---- NOTIFY-RESP");
			return true;
		}
	}
	return false;
}


//#######################################################################################
static void gop_set_slaveInfo(void* sm_, paws_slave_info_t* slave_info)
{
	FUNC_DBG(sm_);
	paws_gop_slave_sm_t* sm = (paws_gop_slave_sm_t*)sm_;
	memcpy(&(PRIVATE_DATA(sm).slave_info), slave_info, sizeof(paws_slave_info_t));
}


//#######################################################################################
static paws_slave_info_t* gop_get_slaveInfo(void* sm_)
{
	FUNC_DBG(sm_);
	paws_gop_slave_sm_t* sm = (paws_gop_slave_sm_t*)sm_;
	return &PRIVATE_DATA(sm).slave_info;
}


//#######################################################################################
static bool gop_Init(void* sm_, State* stl, paws_device_info_t* master_info, paws_gps_location_t* gps, float default_max_location_change, uint32_t default_max_polling_secs, paws_slave_info_t* slave_info, 
	float min_dbm_100k, uint16_t db_retry_secs, logger_cfg_t* msg_log_cfg, uint32_t max_polling_quick_secs, sm_state_info_t* sm_state_info)
{
	paws_gop_slave_sm_t* sm = (paws_gop_slave_sm_t*)sm_;

	// Initialise
	if (!(PARENT_SLAVE_FUNC(sm, Init)(sm_, stl, master_info, gps, default_max_location_change, default_max_polling_secs, min_dbm_100k, db_retry_secs, msg_log_cfg, max_polling_quick_secs, sm_state_info)))
	{
		return false;
	}

	// GOP specific
	GOP_FUNC(sm, set_slaveInfo)(sm, slave_info);

	return true;
}



//#######################################################################################

#define POPULATE_GOP_SLAVE_FUNC(sm,fnc)				sm->slave_hdr.funcs->fnc = gop_##fnc;

#define POPULATE_GOP_PUBLIC_FUNC(sm,fnc)			sm->slave_hdr.public_funcs->fnc = gop_##fnc;

#define POPULATE_GOP_SPECIFIC_FUNC(sm,fnc)			sm->gop_slave_hdr.funcs->fnc = gop_##fnc;

paws_gop_slave_sm_t* paws_gop_slave_sm_create(void* creator, char* sm_name, paws_device_info_t* master_info, paws_gps_location_t* gps, float default_max_location_change, uint32_t default_max_polling_secs,
	paws_slave_info_t *slave_info, float min_dbm_100k, uint16_t db_retry_secs, logger_cfg_t* msg_log_cfg, uint32_t max_polling_quick_secs, sm_state_info_t* sm_state_info)
{
	//	FUNC_DBG(sm_);

	paws_gop_slave_sm_t* paws_gop_slave_sm = NULL;

	if (!(paws_gop_slave_sm = malloc(sizeof(paws_gop_slave_sm_t))))
	{
		goto error_hdl;
	}
	memset(paws_gop_slave_sm, 0, sizeof(paws_gop_slave_sm_t));

	// "inherit" the slave_sm
	if (!(paws_gop_slave_sm->slave_sm = paws_slave_sm_create(creator, &paws_gop_slave_sm->paws_sm_func_store, &paws_gop_slave_sm->slave_func_store, &paws_gop_slave_sm->public_func_store, sm_name )))
	{
		goto error_hdl;
	}

	// copy the paws_header from the slave_sm
	memcpy(&paws_gop_slave_sm->paws_sm_hdr, &paws_gop_slave_sm->slave_sm->paws_sm_hdr, sizeof(paws_sm_header_t));
	// copy the slave header
	memcpy(&paws_gop_slave_sm->slave_hdr, &paws_gop_slave_sm->slave_sm->slave_hdr, sizeof(slave_sm_header_t));
	// copy the paws_sm
	paws_gop_slave_sm->paws_sm = paws_gop_slave_sm->slave_sm->paws_sm;

	LOG_PRINT(creator, LOG_NOTICE, "Creating GOP SM");

	
	// populate functions
	// ---- paws_sm funcs
	// --------none
	//  ---- slave funcs
	POPULATE_GOP_SLAVE_FUNC(paws_gop_slave_sm, Send_AvailableSpectrumReq);
	POPULATE_GOP_SLAVE_FUNC(paws_gop_slave_sm, Send_AvailableSpectrumReq);
	POPULATE_GOP_SLAVE_FUNC(paws_gop_slave_sm, Send_Spectrum_Use_Notify);
	POPULATE_GOP_SLAVE_FUNC(paws_gop_slave_sm, Send_Spectrum_Use_Notify_New);
	POPULATE_GOP_SLAVE_FUNC(paws_gop_slave_sm, Send_Notifications);
	POPULATE_GOP_SLAVE_FUNC(paws_gop_slave_sm, Process_Notify_Use_Resp);
	// ---- public funcs
	// --------none
	// GOP-specific funcs
	paws_gop_slave_sm->gop_slave_hdr.funcs = &paws_gop_slave_sm->gop_func_store;
	POPULATE_GOP_SPECIFIC_FUNC(paws_gop_slave_sm, set_slaveInfo);
	POPULATE_GOP_SPECIFIC_FUNC(paws_gop_slave_sm, get_slaveInfo);

	// Initialise
	if (!(gop_Init(paws_gop_slave_sm, state_transition_table, master_info, gps, default_max_location_change, default_max_polling_secs, slave_info, min_dbm_100k, db_retry_secs, 
		msg_log_cfg, max_polling_quick_secs, sm_state_info)))
	{
		goto error_hdl;
	}

	return paws_gop_slave_sm;

error_hdl:
	paws_gop_slave_sm_free(&paws_gop_slave_sm);
	return NULL;
}


//#######################################################################################
static void paws_gop_sm_data_free(paws_gop_slave_sm_t* sm_, paws_gop_private_data_t* data)
{
	FUNC_DBG(sm_);

	if (data)
	{
		//  nothing to free
	}
}


//#######################################################################################
void paws_gop_slave_sm_free(paws_gop_slave_sm_t** paws_gop_slave_sm)
{
	FUNC_DBG(*paws_gop_slave_sm);

	// check that state machine exists
	if ((paws_gop_slave_sm) && (*paws_gop_slave_sm))
	{
		LOG_PRINT(*paws_gop_slave_sm, LOG_NOTICE, "Deleting GOP SM");

		// free the private data attributes
		paws_gop_sm_data_free(*paws_gop_slave_sm, &(PRIVATE_DATA(*paws_gop_slave_sm)));

		// free the slace_sm
		paws_slave_sm_free(&(**paws_gop_slave_sm).slave_sm);

		// free the struct
		free_and_null((void**)paws_gop_slave_sm);
	}
}





// #######################################################################################################################
// #######################################################################################################################
// #######################################################################################################################
//   SOP
// #######################################################################################################################
// #######################################################################################################################
// #######################################################################################################################



//#######################################################################################
static void sop_Send_AvailableSpectrumReq(void* sm_)
{
	FUNC_DBG(sm_);

	paws_sop_slave_sm_t* sm = (paws_sop_slave_sm_t*)sm_;

	LOG_PRINT(sm_, LOG_NOTICE, "----> AVAIL-SPEC-REQ SOP (device=%s)", PRIVATE_DATA(sm).device_info.unique_id);

	json_value* resp = post_Slave_SOP_Available_Spectrum_Request(sm_, &(PAWS_SM_DATA((paws_sm_t*)sm_)->master_info), &PRIVATE_DATA(sm).device_info, &PRIVATE_DATA(sm).device_info.gps,
		&(PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db.db_url));
	LOCAL_FUNC(sm_, Process_Available_Spectrum_Resp)(sm_, resp, LTE_UL, time(NULL));
	json_value_free(resp);
}


//#######################################################################################
static void sop_Send_Spectrum_Use_Notify(void* sm_)
{
	FUNC_DBG(sm_);

	paws_sop_slave_sm_t* sm = (paws_sop_slave_sm_t*)sm_;

	if ((!PAWS_SM_DATA((paws_sm_t*)sm_)->selected_spectrum))
	{
		LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_DB_NotFound);
		return;
	}

	LOG_PRINT(sm_, LOG_NOTICE, "----> NOTIFY-REQ SOP (device=%s)", PRIVATE_DATA(sm).device_info.unique_id);

	json_value* resp = post_SOP_slave_Notify_Request(sm_, &(PAWS_SM_DATA((paws_sm_t*)sm_)->master_info), &PRIVATE_DATA(sm).device_info, &(PAWS_SM_DATA((paws_sm_t*)sm_)->gps),
		&(PAWS_SM_DATA((paws_sm_t*)sm_)->selected_db.db_url), PAWS_SM_DATA((paws_sm_t*)sm_)->selected_spectrum, 13);

	if (!(SLAVE_FUNC(sm_, Process_Notify_Use_Resp(sm_, resp))))
	{
		LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_Notification_Failure);
	}
	else
	{
		LOCAL_FUNC(sm_, raise_)(sm_, (int)ev_Notification_Success);
	}
	json_value_free(resp);
}




//#######################################################################################
static void sop_Send_Spectrum_Use_Notify_New(void* sm_)
{
	FUNC_DBG(sm_);
}

//#######################################################################################
static bool sop_Process_Notify_Use_Resp(void* sm_, json_value* resp)
{
	FUNC_DBG(sm_);

	if (LOCAL_FUNC(sm_, Check_HTTP_Result)(sm_, resp, false))
	{
		if (LOCAL_FUNC(sm_, Check_Valid_Resp)(sm_, resp, false))
		{
			// raise event to show a valid RESP was received
			LOG_PRINT(sm_, LOG_NOTICE, "<---- NOTIFY-RESP");
			return true;
		}
	}
	return false;
}


//#######################################################################################
static void sop_set_deviceInfo(void* sm_, paws_device_info_t* device_info)
{
	FUNC_DBG(sm_);
	paws_sop_slave_sm_t* sm = (paws_sop_slave_sm_t*)sm_;
	memcpy(&(PRIVATE_DATA(sm).device_info), device_info, sizeof(paws_device_info_t));
}


//#######################################################################################
static paws_device_info_t* sop_get_deviceInfo(void* sm_)
{
	FUNC_DBG(sm_);
	paws_sop_slave_sm_t* sm = (paws_sop_slave_sm_t*)sm_;
	return &PRIVATE_DATA(sm).device_info;
}

//#######################################################################################
static void sop_combiner_UlSpectrumAvailable(void* sm_)
{
	FUNC_DBG(sm_);
	paws_sop_slave_sm_t* sm = (paws_sop_slave_sm_t*)sm_;

	avail_spectrum_t* spectrum_resp = LOCAL_FUNC(sm, GetAvailSpectrumResp)(sm);

	COMBINER_PUBLIC_FUNC(CREATOR_SM(sm_), UlSOPSpectrumAvailable)(CREATOR_SM(sm_), spectrum_resp, sm_);
}

//#######################################################################################
static void sop_combiner_UlSpectrumNotAvailable(void* sm_)
{
	FUNC_DBG(sm_);
	COMBINER_PUBLIC_FUNC(CREATOR_SM(sm_), UlSOPSpectrumNotAvailable)(CREATOR_SM(sm_));
}


//#######################################################################################
static bool sop_Init(void* sm_, State* stl, paws_device_info_t* master_info, paws_gps_location_t* gps, float default_max_location_change, uint32_t default_max_polling_secs, 
	paws_device_info_t *device_info, float min_dbm_100k, uint16_t db_retry_secs, logger_cfg_t* msg_log_cfg, uint32_t max_polling_quick_secs, sm_state_info_t* sm_state_info)
{
	paws_sop_slave_sm_t* sm = (paws_sop_slave_sm_t*)sm_;

	// Initialise
	if (!(PARENT_SLAVE_FUNC(sm, Init)(sm_, stl, master_info, gps, default_max_location_change, default_max_polling_secs, min_dbm_100k, db_retry_secs, msg_log_cfg, max_polling_quick_secs, sm_state_info)))
	{
		return false;
	}

	// SOP specific
	SOP_FUNC(sm, set_deviceInfo)(sm, device_info);

	return true;
}



//#######################################################################################

#define POPULATE_SOP_SLAVE_FUNC(sm,fnc)				sm->slave_hdr.funcs->fnc = sop_##fnc;

#define POPULATE_SOP_PUBLIC_FUNC(sm,fnc)			sm->slave_hdr.public_funcs->fnc = sop_##fnc;

#define POPULATE_SOP_SPECIFIC_FUNC(sm,fnc)			sm->sop_slave_hdr.funcs->fnc = sop_##fnc;

paws_sop_slave_sm_t* paws_sop_slave_sm_create(void* creator, char* sm_name, paws_device_info_t* master_info, paws_gps_location_t* gps, float default_max_location_change, uint32_t default_max_polling_secs,
	paws_device_info_t *device_info, float min_dbm_100k, uint16_t db_retry_secs, logger_cfg_t* msg_log_cfg, uint32_t max_polling_quick_secs, sm_state_info_t* sm_state_info)
{
	//	FUNC_DBG(sm_);

	paws_sop_slave_sm_t* paws_sop_slave_sm = NULL;

	if (!(paws_sop_slave_sm = malloc(sizeof(paws_sop_slave_sm_t))))
	{
		goto error_hdl;
	}
	memset(paws_sop_slave_sm, 0, sizeof(paws_sop_slave_sm_t));

	// "inherit" the slave_sm
	if (!(paws_sop_slave_sm->slave_sm = paws_slave_sm_create(creator, &paws_sop_slave_sm->paws_sm_func_store, &paws_sop_slave_sm->slave_func_store, &paws_sop_slave_sm->public_func_store, sm_name)))
	{
		goto error_hdl;
	}

	// copy the paws_header from the slave_sm
	memcpy(&paws_sop_slave_sm->paws_sm_hdr, &paws_sop_slave_sm->slave_sm->paws_sm_hdr, sizeof(paws_sm_header_t));
	// copy the slave header
	memcpy(&paws_sop_slave_sm->slave_hdr, &paws_sop_slave_sm->slave_sm->slave_hdr, sizeof(slave_sm_header_t));
	// copy the paws_sm
	paws_sop_slave_sm->paws_sm = paws_sop_slave_sm->slave_sm->paws_sm;

	LOG_PRINT(creator, LOG_NOTICE, "Creating SOP SM [%s]", sm_name);

	// populate functions
	// ---- paws_sm funcs
	// --------none
	//  ---- slave funcs
	POPULATE_SOP_SLAVE_FUNC(paws_sop_slave_sm, Send_AvailableSpectrumReq);
	POPULATE_SOP_SLAVE_FUNC(paws_sop_slave_sm, Send_AvailableSpectrumReq);
	POPULATE_SOP_SLAVE_FUNC(paws_sop_slave_sm, Send_Spectrum_Use_Notify);
	POPULATE_SOP_SLAVE_FUNC(paws_sop_slave_sm, Send_Spectrum_Use_Notify_New);
	POPULATE_SOP_SLAVE_FUNC(paws_sop_slave_sm, Process_Notify_Use_Resp);
	POPULATE_SOP_SLAVE_FUNC(paws_sop_slave_sm, combiner_UlSpectrumAvailable);
	POPULATE_SOP_SLAVE_FUNC(paws_sop_slave_sm, combiner_UlSpectrumNotAvailable);

	// ---- public funcs
	
	// --------none
	// GOP-specific funcs
	paws_sop_slave_sm->sop_slave_hdr.funcs = &paws_sop_slave_sm->sop_func_store;
	POPULATE_SOP_SPECIFIC_FUNC(paws_sop_slave_sm, set_deviceInfo);
	POPULATE_SOP_SPECIFIC_FUNC(paws_sop_slave_sm, get_deviceInfo);

	// Initialise
	if (!(sop_Init(paws_sop_slave_sm, state_transition_table, master_info, gps, default_max_location_change, default_max_polling_secs, device_info, min_dbm_100k, db_retry_secs,
		msg_log_cfg, max_polling_quick_secs, sm_state_info)))
	{
		goto error_hdl;
	}

	return paws_sop_slave_sm;

error_hdl:
	paws_sop_slave_sm_free(&paws_sop_slave_sm);
	return NULL;
}


//#######################################################################################
static void paws_sop_sm_data_free(paws_sop_slave_sm_t* sm_, paws_sop_private_data_t* data)
{
	FUNC_DBG(sm_);

	if (data)
	{
		//  nothing to free
	}
}


//#######################################################################################
void paws_sop_slave_sm_free(paws_sop_slave_sm_t** paws_sop_slave_sm)
{
	FUNC_DBG(*paws_sop_slave_sm);

	// check that state machine exists
	if ((paws_sop_slave_sm) && (*paws_sop_slave_sm))
	{
		LOG_PRINT(*paws_sop_slave_sm, LOG_NOTICE, "Deleting SOP SM [%s]", (*paws_sop_slave_sm)->paws_sm_hdr.unique_id);

		// free the private data attributes
		paws_sop_sm_data_free(*paws_sop_slave_sm, &(PRIVATE_DATA(*paws_sop_slave_sm)));

		// free the slace_sm
		paws_slave_sm_free(&(**paws_sop_slave_sm).slave_sm);

		// free the struct
		free_and_null((void**)paws_sop_slave_sm);
	}
}


