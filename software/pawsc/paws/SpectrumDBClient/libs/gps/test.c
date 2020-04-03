/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include "gps.h"

#include <stdio.h>


int main()
{
    GpsFixData fix;
    int rv = gps_get_fix(&fix);
    printf("gps_get_fix returned %d\n", rv);

    if (rv == 0) {
        printf("Fix was:\n");
        printf("  utc time: %02d:%02d:%05.2f\n", fix.utc_time.hours, fix.utc_time.minutes, fix.utc_time.seconds);
        printf("  lat: %.4f\n", fix.latitude);
        printf("  lon: %.4f\n", fix.longitude);
        printf("  num sats: %d\n", fix.num_satellites);
        printf("  hori accuracy: %.2f\n", fix.horizontal_accuracy);
        printf("  altitude: %.0f metres above mean sea level\n", fix.altitude);
    }
    
    float pintshop_lat = 52.204248f;
    float pintshop_lon = 0.119082f;
    float msrc_lat = 52.194903f;
    float msrc_lon = 0.134992f;
    float dist = gps_distance_between_two_locations(pintshop_lat, pintshop_lon, msrc_lat, msrc_lon);
    printf("Distance between MSRC and Pint Shop is %.0f metres", dist);

    return 0;
}
