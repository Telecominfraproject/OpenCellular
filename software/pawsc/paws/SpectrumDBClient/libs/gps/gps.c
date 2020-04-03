/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

// Own header
#include "gps.h"

// Standard headers
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>



static int starts_with(char const *candidate, char const *pattern)
{
    while (1)
    {
        if (*candidate == '\0')
            return 0;
        if (*pattern == '\0')
            return 1;
        if (*candidate != *pattern)
            return 0;
        candidate++;
        pattern++;
    }
}

// From http://aprs.gids.nl/nmea
//
// 1           2        3      4    5       6 7 8  9     10    11 12  13 14 15
// $GPGGA,194530.000,3051.8007,N,10035.9989,W,1,4 ,2.18 ,746.4,M,-22.2,M,,*6B
// $GPGGA,          ,         , ,          , ,0,00,99.99,     , ,     , ,,*48
// 
// 1. Sentence type.
// 2. UTC.
// 3 & 4. Latitude.
// 5 & 6. Longitude.
// 7. 1 means we have a fix.
// 8. The fix is goodness '4'.
// 9. 2.18 is horizontal dilution of position.
// 10. 746.4 is altitude
// 11. Altitude units
// 12. Height of mean sea level where you are.
//                                    
// $GPRMC,194530.000,A,3051.8007,N,10035.9989,W,1.49,111.67,310714,,,A*74
// $GPRMC,          ,V,         , ,          , ,    ,      ,      ,,,N*53


static HoursMinutesSeconds parse_time(const char *s)
{
    // 194530.000 -> 19h 45m 30.000s
    HoursMinutesSeconds time;
    char buf[3] = "00";
    buf[0] = s[0];
    buf[1] = s[1];
    time.hours = atoi(buf);

    buf[0] = s[2];
    buf[1] = s[3];
    time.minutes = atoi(buf);

    time.seconds = (float)atof(s + 4);

    return time;
}


static float parse_angle(const char *s)
{
    //  3051.8007,N -> 30 deg 51.8007 minutes North -> 30.863345 deg North -> 30.863345 deg
    // 10035.9989,W -> 100 deg 35.9989 minutes West -> 100.599982 deg West -> 259.400018 deg

    char const *dot = strchr(s, '.');
    if (!dot) return -1000.0f;

    char const *comma = strchr(s, ',');
    if (!comma) return -2000.0f;

    float minutes = (float)atof(dot - 2);

    char deg_str[] = "000";
    deg_str[1] = *(dot - 4);
    deg_str[2] = *(dot - 3);
    if ((dot - 5) >= s)
        deg_str[0] = *(dot - 5);
    float degrees = (float)atof(deg_str);

    degrees += minutes / 60.0f;

    if (comma[1] == 'S' || comma[1] == 'W')
        degrees = 360.0f - degrees;

    return degrees;
}


static char const *get_next_field(char const *str)
{
    char const *rv = strchr(str, ',');
    if (!rv)
        return NULL;
    rv++;
    while (*rv == ' ')
        rv++;
    return rv;
}


static int parse_gpgga_sentence(char const *buf, GpsFixData *fix_data)
{
    char const *utc = get_next_field(buf);
    if (!utc) return -1;
    fix_data->utc_time = parse_time(utc);

    char const *latitude = get_next_field(utc);
    if (!latitude) return -1;
    char const *latitude_hemisphere = get_next_field(latitude);
    if (!latitude_hemisphere) return -1;
    fix_data->latitude = parse_angle(latitude);

    char const *longitude = get_next_field(latitude_hemisphere);
    if (!longitude) return -1;
    char const *longitude_hemisphere = get_next_field(longitude);
    if (!longitude_hemisphere) return -1;
    fix_data->longitude = parse_angle(longitude);

    char const *fix_type = get_next_field(longitude_hemisphere);
    if (!fix_type) return -1;
    int fix_type_int = atoi(fix_type);
    if (fix_type_int != 1 && fix_type_int != 2) return -1;

    char const *num_satellites = get_next_field(fix_type);
    if (!num_satellites) return -1;
    fix_data->num_satellites = atoi(num_satellites);

    char const *horizontal_accuracy = get_next_field(num_satellites);
    if (!horizontal_accuracy) return -1;
    fix_data->horizontal_accuracy = (float)atof(horizontal_accuracy);

    char const *altitude = get_next_field(horizontal_accuracy);
    if (!altitude) return -1;
    fix_data->altitude = (float)atof(altitude);

    char const *unit = get_next_field(altitude);
    if (!unit) return -1;
    if (*unit != 'M')
        return -1;

    return 0;
}


int gps_get_fix(GpsFixData *fix)
{
	int f = open("/dev/ttyS1", O_RDONLY);
	if (f == -1)
	{
		return -1;
	}

    clock_t give_up_time = clock() + CLOCKS_PER_SEC * 2;
    char buf[150];
    int got_fix = 0;
    while (clock() < give_up_time) {

		int n = read(f, buf, sizeof(buf));
		if (n < 0)
		{
			// try again
			continue;
		}

        if (starts_with(buf, "$GPGGA")) {
            if (parse_gpgga_sentence(buf, fix) == 0) {
                got_fix = 1;
                break;
            }
        }
    }

    close(f);

//     process_line("$GPGGA, 144858.00, 5211.69438, N, 00008.09551, E, 2, 10, 1.11, 43.0, M, 45.7, M, , 0000 * 6D");
//     process_line("$GPGGA, 144859.00, 5211.69438, N, 00008.09547, E, 2, 10, 0.84, 43.0, M, 45.7, M, , 0000 * 66");

//    process_line("$GPGGA,194530.000,3051.8007,N,10035.9989,W,1,4 ,2.18 ,746.4,M,-22.2,M,,*6B");
//     process_line("$GPGGA,          ,         , ,          , ,0,00,99.99,     , ,     , ,,*48");
//     process_line("$GPGGA,,,,,,0,00,99.99,,,,,,*48");

    return got_fix - 1;
}


float gps_distance_between_two_locations(float lat1, float lon1, float lat2, float lon2)
{
    // Convert from degrees to radians
    float deg_to_rad = 3.14159265f / 180.0f;
    lon1 *= deg_to_rad;
    lat1 *= deg_to_rad;
    lon2 *= deg_to_rad;
    lat2 *= deg_to_rad;

    float earth_radius_in_metres = 6371e3;
    float x = (lon2 - lon1) * cosf(0.5f * (lat2 + lat1));
    float y = lat2 - lat1;
    float distance_in_metres = earth_radius_in_metres * sqrtf(x*x + y*y);

    return (float)distance_in_metres;
}