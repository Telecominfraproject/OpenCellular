/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#pragma once


typedef struct
{
    int hours, minutes;
    float seconds;
} HoursMinutesSeconds;


typedef struct
{
    HoursMinutesSeconds utc_time;
    float latitude;             // Degrees North. Range is 0 to 360.
    float longitude;            // Degrees East. Range is 0 to 360.
    int num_satellites;
    float horizontal_accuracy;  // Dilution of Precision: <1 ideal, 1-2 excellent, 2-5 good, 5-10 moderate, 10-20 Fair, >20 Poor.
    float altitude;             // Meters above mean sea level (geoid).
} GpsFixData;


// Read the raw data stream from the GPS device, until a fix is obtained.
// If no fix is found after 2 seconds, -1 will be returned and the passed
// in GpsFixData structure will not be updated.
//
// Returns 0 on success, -1 otherwise.
int gps_get_fix(GpsFixData *fix);

// Returns the distance in meters between two locations specified as longitude
// and latitude in degrees.
float gps_distance_between_two_locations(float lat1, float lon1, float lat2, float lon2);
