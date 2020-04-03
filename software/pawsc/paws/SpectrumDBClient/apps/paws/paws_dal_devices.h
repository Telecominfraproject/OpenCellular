/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_DAL_DEVICES_H_
#define PAWS_DAL_DEVICES_H_

// This is the PAWS Data Abstraction layer.
// It is responsible for reading and writing all data to stroage.
// It is expected that developers will implment individual source for this entity, on a per-platform basis.

#include <stdint.h>
#include <stdbool.h>

#include "utils/types.h"
#include "paws_dal_settings.h"
#include "paws_dal_gps.h"

// *********************************************************************
// device info
// *********************************************************************


#define PAWS_MAX_DEVICE_TECHNOLOGY_LEN		(30)
#define PAWS_MAX_DEVICE_MANUFACTURER_LEN		(30)
#define PAWS_MAX_DEVICE_MODEL_LEN			(20)
#define PAWS_MAX_DEVICE_TYPE_LEN				(2)
#define PAWS_MAX_DEVICE_CATEGORY_LEN			(7)
typedef char paws_device_tech_t[PAWS_MAX_DEVICE_TECHNOLOGY_LEN];
typedef char paws_device_manufacturer_t[PAWS_MAX_DEVICE_MANUFACTURER_LEN];
typedef char paws_device_model_t[PAWS_MAX_DEVICE_MODEL_LEN];
typedef char paws_device_type_t[PAWS_MAX_DEVICE_TYPE_LEN];
typedef char paws_device_category_t[PAWS_MAX_DEVICE_CATEGORY_LEN];


typedef struct {
	int								gain;				// Stored in units of 0.1dB.   SQL schema stores it as a float in unts of dB
} paws_antenna_info_t;

typedef struct {
	paws_device_manufacturer_t		manufacturer;
	paws_device_model_t				model;
} paws_device_identity_t;

typedef struct {
	paws_device_type_t				type;
	paws_device_category_t			cat;
	int								emission_class;
	paws_device_tech_t				technology_id;
} paws_device_characteristics_t;

typedef struct {
	device_name_t					unique_id;
	paws_antenna_info_t				antenna_info;
	paws_device_identity_t			device_identity;
	paws_device_characteristics_t	device_characteristics;
	paws_gps_location_t				gps;
} paws_device_info_t;

#define MAX_SLAVE_DEVICES			(256)

typedef struct {
	uint16_t						num_devices;
	paws_device_info_t				device_info[MAX_SLAVE_DEVICES];
} paws_slave_info_t;


typedef struct
{
	bool		device_enabled;				// enable/disable the master
	int			bandwidth;
	int			dl_start_hz;
	float		dl_dbm_per100k;
	int			ul_start_hz;
	float		ul_dbm_per100k;
} device_cfg_t;


// *********************************************************************
// Read/write functons
// *********************************************************************

// Return True if read OK.
extern bool paws_read_master_info(log_app_t app, paws_device_info_t* dev, bool* master_device_cfg_read);

// Return True if read OK.
extern bool paws_read_slave_info(log_app_t app, paws_slave_info_t* gop_slaves, paws_slave_info_t* sop_slaves);

// get the device Id
extern bool paws_get_device_name(device_name_t device_name);

// Return True if configured OK.
// Sets *reboot_needed to True if cfg changes require a reboot
extern bool paws_config_master_device(void* logger, device_cfg_t* paws_cfg, bool* admin_state_changed, bool* cfg_changed, bool* reboot_needed, paws_setting_override_t* spectrum_override, paws_antenna_info_t* antenna_info);

// trigger to reboot the device
extern void paws_reboot_master_device(void);

// Return True if configured OK.
// Sets *reboot_needed to True if cfg changes require a reboot
extern bool paws_config_master_device_override(void* logger, paws_setting_override_t* spectrum_override, paws_antenna_info_t* antenna_info);


#endif // PAWS_DAL_DEVICES_H_

