/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_DAL_GPS_H_
#define PAWS_DAL_GPS_H_

// This is the PAWS Data Abstraction layer.
// It is responsible for reading and writing all data to stroage.
// It is expected that developers will implment individual source for this entity, on a per-platform basis.

#include <stdint.h>
#include <stdbool.h>

// *********************************************************************
// GPS
// *********************************************************************

#define MAX_ANT_HEIGHT_TYPE_LEN			(20)
typedef char paws_antenna_height_type_t[MAX_ANT_HEIGHT_TYPE_LEN];

typedef struct
{
	device_name_t					device_name;
	bool						fixed;				// location has been acquired
	float						latitude;
	float						longitude;
	uint16_t					height;
	paws_antenna_height_type_t	height_type;
} paws_gps_location_t;


// *********************************************************************
// Read/write functons
// *********************************************************************

// Return True if read OK.
extern bool paws_read_gps_from_db(paws_gps_location_t* gps, device_name_t device_name);
extern void paws_read_gps(paws_gps_location_t* data, device_name_t device_name);

#endif // PAWS_DAL_GPS_H_


