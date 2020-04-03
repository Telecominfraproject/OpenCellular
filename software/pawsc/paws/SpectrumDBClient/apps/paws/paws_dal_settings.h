/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_DAL_SETTINGS_H_
#define PAWS_DAL_SETTINGS_H_

// This is the PAWS Data Abstraction layer.
// It is responsible for reading and writing all data to stroage.
// It is expected that developers will implment individual source for this entity, on a per-platform basis.

#include <stdint.h>
#include <stdbool.h>

#include "utils/types.h"
#include "logger/logger.h"

// *********************************************************************
// Config
// *********************************************************************

typedef struct {	
	logger_cfg_t			app_log;
	logger_cfg_t			msg_log;
	cloud_logger_cfg_t		cloud_log;
} paws_setting_loginfo_t;

typedef struct {
	bool		present;
	uint16_t	bandwidth_rb;
	uint16_t	earfcndl;
	uint16_t	earfcnul;
	int16_t		ref_sig_pwr;			// stored in units of 0.1dB
	int16_t		pmax;					// stored in units of 0.1dB
} paws_setting_override_t;

typedef struct
{
	// spectrum selected criteria
	float					min_dl_dbm_100k;				// min DL dbm per 100k to qualify for selection
	float					min_ul_dbm_100k;				// min UL dbm per 100k to qualify for selection
	// timer info
	uint16_t				db_retry_secs;					// how often to retry events (default=15)
	uint16_t				db_barred_secs;					// how long to barr a specific DB following failure (default=45)
	uint16_t				setting_periodic_secs;			// how often to re-read paws-settings (default=300).
	uint16_t				devices_periodic_secs;			// how often to re-read device info (default=60).
	uint16_t				gps_periodic_fast_check_secs;	// how often to re-read GPS (default=15).  Used when GPS is "NOT" acquired
	uint16_t				gps_periodic_slow_check_secs;	// how often to re-read GPS (default=600). Used when GPS "IS" acquired
	uint32_t                max_polling_quick_secs;			// quici maxPollingSecs.  0=disable
	// logging
	paws_setting_loginfo_t	loginfo;
	// override
	paws_setting_override_t	spectrum_override;				// can override the spectrum which the TVWSDB provides in the AVAIL_SPEC_RESP.
} paws_settings_t;


// *********************************************************************
// Read/write functons
// *********************************************************************

extern bool paws_read_settings(paws_settings_t* settings);



#endif // PAWS_DAL_SETTINGS_H_

