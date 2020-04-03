/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_TIMER_INFO_H_
#define PAWS_TIMER_INFO_H_

//####### Timer tolerance
// some timers experiencing max duration between actions.  These tolerances specify how close to the end of max-duration to reach before expiring the timer
#define DB_DISCOVERY_TIMER_TOLERANCE			(60)
#define SPECTRUM_EXPIRY_TIMER_TOLERANCE			(15)
#define MAX_POLLING_TIMER_TOLERANCE				(15)

// ####### Timer durations
#define DB_SETTINGS_INITIAL_DURATION		    (1 * 60)        // start off every 1 mins, but this will get overwritten 
#define DB_DEVICES_INITIAL_DURATION				(1 * 60)        // start off every 1 mins, but this will get overwritten 
#define PERIODIC_GPS_DEVICES_INITIAL_DURATION	(5)				// quick check until we have SETIINGS defined
#define DB_DISCOVERY_TIMER_INITIAL_DURATION		(DB_DISCOVERY_TIMER_TOLERANCE + 60)     // start off every 1 min, but this will get overwritten by the received refreshRate.
#define DB_DISCOVERY_TIMER_QUICK_DURATION		(75 * 60)		// ## 75 minutes         ## as per ETSI EN 301 598 v1.1.1
#define LOCAL_ISSUE_BACKOFF_TIMER_DURATION		(10)			// There has been a local problem.  How long to wait before we re-init the whole system

#endif // #define PAWS_TIMER_INFO_H_

