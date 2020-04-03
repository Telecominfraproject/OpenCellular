/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_SLAVE_SM_TYPES_H_
#define PAWS_SLAVE_SM_TYPES_H_

#include "paws_sm_types.h"

typedef struct {
	bool			(*Init)(void* sm_, State* stl, paws_device_info_t* master_info, paws_gps_location_t* gps, float default_max_location_change, uint32_t default_max_polling_secs, float min_dbm_100k, uint16_t db_retry_secs, logger_cfg_t* msg_log_cfg, uint32_t max_polling_quick_secs, sm_state_info_t* sm_state_info);
	bool			(*init_vars)(void* sm_, paws_device_info_t* master_info, paws_gps_location_t* gps, float default_max_location_change, uint32_t default_max_polling_secs, float min_dbm_100k, uint16_t db_retry_secs, uint32_t max_polling_quick_secs);
	paws_sm_func	checkAvailableSpectrumUl;
	paws_sm_func	combiner_UlSpectrumAvailable;
	paws_sm_func	combiner_UlSpectrumNotAvailable;
	paws_sm_func	combiner_UlNotificationSuccess;
	//The following are all pure virtual
	paws_sm_func	Send_AvailableSpectrumReq;
	paws_sm_func	Send_Spectrum_Use_Notify;
	paws_sm_func	Send_Spectrum_Use_Notify_New;
	void			(*Send_Notifications)(void* sm_, paws_slave_info_t* slave_info);
	bool			(*Process_Notify_Use_Resp)(void* sm_, json_value* resp);
	void			(*update_slave_info)(void* sm_, paws_slave_info_t* slave_info);
} paws_slave_sm_funcs_t;

typedef struct {
	int				dummy1;			// nothing
} paws_slave_sm_data_t;

typedef struct {
	paws_sm_func	Start;
	int				(*run_tick)(void* sm_);
} paws_slave_public_funcs_t;

typedef struct {
	paws_slave_sm_funcs_t*		funcs;					// THIS MUST BE FIRST.    
	paws_slave_public_funcs_t*	public_funcs;
	paws_slave_sm_data_t*		data;
} slave_sm_header_t;

typedef struct {
	paws_sm_header_t			paws_sm_hdr;			// THIS MUST BE FIRST IN ANY SM WHICH HAS A PAWS_SM.    
	paws_sm_t*					paws_sm;
	slave_sm_header_t			slave_hdr;
	paws_sm_funcs_t				local_paws_sm_funcs;
	paws_slave_sm_funcs_t		local_slave_funcs;
	paws_slave_public_funcs_t	local_public_funcs;
	paws_slave_sm_data_t		slave_data_store;
} paws_slave_sm_t;


typedef struct {
	paws_slave_info_t			slave_info;
} paws_gop_private_data_t;


typedef struct {
	void						(*set_slaveInfo)(void* sm_, paws_slave_info_t* slave_info);
	paws_slave_info_t*			(*get_slaveInfo)(void* sm_);
} paws_gop_slave_sm_funcs_t;


typedef struct {
	paws_gop_slave_sm_funcs_t*		funcs;
} gop_slave_sm_header_t;


typedef struct {
	paws_sm_header_t			paws_sm_hdr;			// THIS MUST BE FIRST IN ANY SM WHICH HAS A PAWS_SM.    
	paws_sm_t*					paws_sm;				//
	slave_sm_header_t			slave_hdr;				//
	paws_sm_funcs_t				paws_sm_func_store;		//
	paws_slave_sm_funcs_t		slave_func_store;		//
	paws_slave_public_funcs_t	public_func_store;		//   down to here must match paws_slave_sm_t

	gop_slave_sm_header_t		gop_slave_hdr;
	paws_gop_private_data_t		private_data_store;
	paws_gop_slave_sm_funcs_t	gop_func_store;			// these are functions specific to GOP

	paws_slave_sm_t*			slave_sm;
} paws_gop_slave_sm_t;



typedef struct {
	paws_device_info_t			device_info;
} paws_sop_private_data_t;


typedef struct {
	void						(*set_deviceInfo)(void* sm_, paws_device_info_t* device_info);
	paws_device_info_t*			(*get_deviceInfo)(void* sm_);
} paws_sop_slave_sm_funcs_t;

typedef struct {
	paws_sop_slave_sm_funcs_t*		funcs;
} sop_slave_sm_header_t;


typedef struct {
	paws_sm_header_t			paws_sm_hdr;			// THIS MUST BE FIRST IN ANY SM WHICH HAS A PAWS_SM.    
	paws_sm_t*					paws_sm;				//
	slave_sm_header_t			slave_hdr;				//
	paws_sm_funcs_t				paws_sm_func_store;		//
	paws_slave_sm_funcs_t		slave_func_store;		//
	paws_slave_public_funcs_t	public_func_store;		//   down to here must match paws_slave_sm_t

	sop_slave_sm_header_t		sop_slave_hdr;
	paws_sop_private_data_t		private_data_store;
	paws_sop_slave_sm_funcs_t	sop_func_store;			// these are functions specific to SOP

	paws_slave_sm_t*			slave_sm;
} paws_sop_slave_sm_t;



#endif // #define PAWS_SLAVE_SM_TYPES_H_
