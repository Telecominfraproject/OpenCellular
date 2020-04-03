/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_DAL_STATE_H_
#define PAWS_DAL_STATE_H_

#include "paws_dal_gps.h"
#include "paws_dal_database.h"

#include "lists/lists.h"


// This is the PAWS Data Abstraction layer.
// It is responsible for reading and writing all data to stroage.
// It is expected that developers will implment individual source for this entity, on a per-platform basis.


// *********************************************************************
// PAWS state info.
// This is stored prior to reboot, and read post-boot.
// *********************************************************************

typedef struct
{
	uint32_t				maxPollingSecs;
	float					maxLocationChange;
} ruleset_info_t;

typedef struct
{
	time_t					start_time;
	time_t					stop_time;
} time_range_t;

typedef struct spec_profile_type
{
	uint32_t					start_hz;
	uint32_t					end_hz;
	float						dbm;
	struct spec_profile_type*	next;
} spec_profile_type_t;

typedef struct spectrum_schedule
{
	time_range_t				event_time_range;
	spec_profile_type_t*		profiles;
	struct spectrum_schedule*	next;
	uint8_t						refcnt;								// there can be multiple references, so use this to control the free
} spectrum_schedule_t;

typedef struct spectrum_spec
{
	spectrum_schedule_t*	spectrum_schedules;
	float					max_contiguous_bw_hz;				// 0 if not present. 0 will mean no constraint
	float					max_contiguous_bw_hz_within_band;	// max contiguous actually within band
	bool					needsSpectrumReport;				// false if not preset
	ruleset_info_t			ruleset_info;
	struct spectrum_spec*	next;
	uint8_t					refcnt;								// there can be multiple references, so use this to control the free
} spectrum_spec_t;

typedef struct
{
	spectrum_spec_t*		spectrum_specs;
	uint8_t					refcnt;								// there can be multiple references, so use this to control the free
} avail_spectrum_t;


typedef struct {
	spectrum_spec_t*		spec;
	spectrum_schedule_t*	sched;
	uint32_t				start_hz;
	float					dbm;
	uint32_t				bandwidth;
	uint8_t					refcnt;								// there can be multiple references, so use this to control the free
} spec_cfg_t;

typedef struct
{
	spec_cfg_t*				dl_cfg;
	spec_cfg_t*				ul_cfg;
	uint8_t					refcnt;								// there can be multiple references, so use this to control the free
} ul_dl_spec_cfg_t;

typedef struct
{
	device_name_t			unique_id;
	int						stl_current_state;
	char*					timer_info;
	float					default_max_location_change;
	uint32_t				default_max_polling_secs;
	float					specific_max_location_change;
	uint32_t				specific_max_polling_secs;
	paws_gps_location_t		gps;
	paws_db_item_t			selected_db;
	avail_spectrum_t*		avail_spectrum_resp;				
	spec_cfg_t*				available_spectrum;					
	spec_cfg_t*				pending_spectrum;					
	spec_cfg_t*				selected_spectrum;					
	spec_cfg_t*				spectrum_in_use;	
	llist_item_t*			l_item;
} sm_state_info_t;

typedef enum {
	SPEC_STATE_NOT_AVAIL = 0,
	SPEC_STATE_AVAIL,
	SPEC_STATE_NOTIFICATION_SUCCESS
} paws_spectrum_state_e;

typedef struct
{
	paws_weblist_t		*wsbd_weblist;
	int					db_discovery_duration;
} paws_db_discovery_info_t;

typedef struct
{
	paws_spectrum_state_e		dl_spec_state;
	paws_spectrum_state_e		ul_spec_state;
	paws_db_discovery_info_t	db_discovery_info;
	ul_dl_spec_cfg_t*			ul_dl_spec;

	llist_t*					state_info_ll;			// list of sm_state_info_t
} paws_sm_state_info_t;


// *********************************************************************
// Read/write functons
// *********************************************************************

// Return NULL if empty.
extern paws_sm_state_info_t* paws_read_state(void);
extern void paws_remove_state(void);

// Return True if read OK.
// state is not to be freed inside function
extern bool paws_write_state(paws_sm_state_info_t* state);


#endif // PAWS_DAL_STATE_H_


