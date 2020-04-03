/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_MESSAGES_H_
#define PAWS_MESSAGES_H_

#include "gps/gps.h"
#include "json-parser/json.h"

#include "paws_types.h"


// PAWS error codes
#define PAWS_ERROR_RESERVED				(-100)
#define PAWS_ERROR_VERSION				(-101)
#define PAWS_ERROR_UNSUPPORTED			(-102)
#define PAWS_ERROR_UNIMPLEMENTED		(-103)
#define PAWS_ERROR_OUTSIDE_COVERAGE		(-104)
#define PAWS_ERROR_DATABASE_CHANGE		(-105)
#define PAWS_ERROR_COMPATIBILITY_MAX	PAWS_ERROR_RESERVED
#define PAWS_ERROR_COMPATIBILITY_MIN 	PAWS_ERROR_DATABASE_CHANGE
#define PAWS_ERROR_RESERVED2 			(-200)
#define PAWS_ERROR_MISSING				(-201)
#define PAWS_ERROR_INVALID_VALUE		(-202)
#define PAWS_ERROR_REQERROR_MAX			PAWS_ERROR_RESERVED2
#define PAWS_ERROR_REQERROR_MIN			PAWS_ERROR_INVALID_VALUE
#define PAWS_ERROR_RESERVED3			(-300)
#define PAWS_ERROR_UNAUTHORIZED			(-301)
#define PAWS_ERROR_NOT_REGISTERED		(-302)
#define PAWS_ERROR_AUTHORISATION_MAX	PAWS_ERROR_RESERVED3
#define PAWS_ERROR_AUTHORISATION_MIN	PAWS_ERROR_NOT_REGISTERED


#define MAX_DB_ACCESS_ATTEMPTS			(5)		/* how many to retry a connection before giving up */

extern json_value* post_Init_Request(void* sm_, paws_device_info_t *device_info, paws_gps_location_t* gps, paws_db_url_t* url);
extern json_value* post_Avail_Spectrum_Request(void* sm_, paws_device_info_t *device_info, paws_gps_location_t* gps, paws_db_url_t* url);
extern json_value* post_Slave_GOP_Available_Spectrum_Request(void* sm_, paws_device_info_t *device_info, paws_gps_location_t* gps, paws_db_url_t* url);
extern json_value* post_Slave_SOP_Available_Spectrum_Request(void* sm_, paws_device_info_t* master_info, paws_device_info_t* slave_device_info, paws_gps_location_t* gps, paws_db_url_t* url);
extern json_value* post_Notify_Request(void* sm_, paws_device_info_t *device_info, paws_gps_location_t* gps, paws_db_url_t* url, spec_cfg_t*	cfg, uint8_t band_id);
extern json_value* post_GOP_slave_Notify_Request(void* sm_, paws_device_info_t* master_info, paws_device_info_t* slave_device_info, paws_gps_location_t* master_gps, paws_db_url_t* url, spec_cfg_t* cfg, uint8_t band_id);
extern json_value* post_SOP_slave_Notify_Request(void* sm_, paws_device_info_t* master_info, paws_device_info_t* slave_device_info, paws_gps_location_t* master_gps, paws_db_url_t* url, spec_cfg_t* cfg, uint8_t band_id);

extern void paws_messages_ssl_free(void* sm_);

#endif // PAWS_MESSAGES_H_
