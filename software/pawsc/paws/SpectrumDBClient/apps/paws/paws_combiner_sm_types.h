/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_COMBINER_SM_TYPES_H_
#define PAWS_COMBINER_SM_TYPES_H_

#include "state-machine/state-machine.h"
#include "https-client/https.h"
#include "lists/lists.h"

#include "paws_types.h"
#include "paws_sm_types.h"
#include "paws_master_sm_types.h"
#include "paws_slave_sm_types.h"

#include "paws_dal_settings.h"
#include "paws_dal_state.h"
#include "paws_dal_control_plane.h"


typedef struct {
	paws_sm_func	Run;
	void			(*select_DB)(void* sm_, void* caller);
	void			(*DBerror)(void* sm_, void* caller);
	paws_sm_func	LocalDBerror;
	void			(*ProcessDbUpdate)(void* sm_, json_value* new_db_spec);
	paws_sm_func	DlSpectrumNotAvailable;
	paws_sm_func	DlSpectrumAvailable;
	paws_sm_func	UlSpectrumNotAvailable;
	paws_sm_func	UlSpectrumAvailable;
	paws_sm_func	UlSOPSpectrumNotAvailable;
	void            (*UlSOPSpectrumAvailable)(void* sm_, avail_spectrum_t* spectrum_resp, void* caller);
	paws_sm_func	DlNotificationSuccess;
	paws_sm_func	UlNotificationSuccess;
	void			(*setDefaultInfo)(void* sm_, float default_max_location_change, uint32_t default_max_polling_secs);
} paws_combiner_public_funcs_t;


typedef struct {
	device_name_t					device_name;
	paws_sop_slave_sm_t*			sm; 
	llist_item_t*					l_item;			// pointer back to entity on list for quicker deletion
} sop_slave_sm_entity_t;

typedef struct {

	// slave device info
	paws_slave_info_t				gop_slave_info;
	paws_slave_info_t				sop_slave_info;

	// SMs
	paws_master_sm_t*				paws_master_sm;
	paws_gop_slave_sm_t*			paws_gop_slave_sm;
	llist_t*						sop_sm_list;					// list of sop_slave_sm_entity_t 

	// SOP min info
	// the SopMin indicates if a SOP is one of the ones which is used to limit the DLUl powers.
	// It must be tracked to determine if future processing is needed on a subsequent UlSpectrumAvailable call from that SOP.
	uint16_t						num_sop_min;
	paws_sop_slave_sm_t*			sop_min[MAX_SLAVE_DEVICES];		// list of SM pointers whcih are sop_min
	
	// DB and DB Discovery stuff
	paws_db_info_t					db_info;
	paws_db_discovery_info_t		db_discovery_info;
	
	// paws selection config
	bool							paws_settings_read;
	paws_settings_t					paws_settings;

	// pre-boot state info
	paws_sm_state_info_t*			state_info;

	// spectrum states
	paws_spectrum_state_e			dl_spec_state;
	paws_spectrum_state_e			ul_spec_state;
	ul_dl_spec_cfg_t*				ul_dl_spec;					 // current paired spectrum in use

	// gps
	uint32_t						gps_periodic_duration;
	paws_gps_location_t				gps;

	// master device has been read
	bool							master_info_read;

	// control plane - do we pass Control signalling.  When this is false, it is assumed no control is given to the device so it will not be radiating.
	void*							control_plane;
	bool							control_plane_enabled;

	// has a reboot been triggered
	bool							reboot_needed;		

	// logger for the DeviceCfg
	logger_cfg_t					DeviceCfg_LogCfg;
	void*							DeviceCfg_Logger;			// this is the config for the log messages

} paws_combiner_private_data_t;


typedef struct {
	paws_sm_header_t				paws_sm_hdr;			// THIS MUST BE FIRST ENTRY IN ANY SM WHICH HAS A PAWS_SM.    
	paws_sm_funcs_t					paws_sm_func_store;
	paws_combiner_private_data_t	private_data_store;
	paws_combiner_public_funcs_t	public_func_store;
	control_plane_cfg_t				control_plane_cfg;

	paws_sm_t*						paws_sm;
} paws_combiner_sm_t;



#endif // #define PAWS_COMBINER_SM_TYPES_H_


