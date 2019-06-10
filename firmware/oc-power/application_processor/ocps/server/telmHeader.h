/*
 * telemetry_header.h
 *
 *  Created on: Aug 3, 2018
 *      Author: vthakur
 */

#ifndef telm_header
#define telm_header

#include "alarmHeader.h"
#include "customActionHeader.h"

typedef enum {
   OCP_TELEMETRY_NONE = 0,
   OCP_TELEMETRY_GET = 1,
}OCP_TelemetryFuncmap;   

typedef union {
	uint8_t portStatus;
    struct {
    	/*!< Flags set to true if power I/O ports have been enabled over the last telemetry
        		interval. More space efficient to use bitfields in an int32 */
		uint8_t batt : 1;
		uint8_t pv   : 1;
		uint8_t adp  : 1;
		uint8_t l1   : 1;
		uint8_t l2   : 1;
		uint8_t l3   : 1;
		uint8_t l4   : 1;
		uint8_t l5   : 1;
    } bits;
}OCPPowerPortStatus;

typedef struct {
	uint16_t bv;		/*!< battery voltage */
	uint16_t bc;		/*!< battery current */
	/*!<
        	...
        	Include other battery configuration data as optional
	 */
}__attribute__((packed, aligned(1))) OCPBatteryTelemetry;

typedef struct {
    int16_t temp[4];
}__attribute__((packed, aligned(1))) OCPTempTelemetry;

typedef struct {
	uint32_t timestamp;   /*!< unix timestamp */
	uint32_t timeinterval;   /*!< average window (seconds) */
	OCPPowerPortStatus powerPortStatus;
	OCPBatteryTelemetry batteryTelemetry;
	uint16_t p1v;		/*!< solar input */
	uint16_t p1c;		/*!< solar input */
	uint16_t adp1v;		/*!< DC input (ADP) */
	uint16_t adp1c;		/*!< DC input (ADP) */
	uint16_t l1v;		/*!< output */
	uint16_t l1c;		/*!< output */
	uint16_t l2v;		/*!< output */
	uint16_t l2c;		/*!< output */
	uint16_t l3v;		/*!< output */
	uint16_t l3c;		/*!< output */
	uint16_t l4v;		/*!< output */
	uint16_t l4c;		/*!< output */
	uint16_t l5v;		/*!< output */
	uint16_t l5c;		/*!< output */
    OCPTempTelemetry ocpTempData;
	AlarmData alarmStatus[MAX_ALERT_BYTES];
	CustomAction customActions[MAX_CUSTOM_ACTION];
}__attribute__((packed, aligned(1))) OCPTelemetryData;

typedef struct {
	uint32_t from_ts;
	uint32_t to_ts;
}__attribute__((packed, aligned(1))) OCPTelemetryRequest;

typedef struct {
	uint32_t numberOfEntries;
	OCPTelemetryData *telemetryData;
} __attribute__((packed, aligned(1))) OCPTelemetryResponse;

//typedef OCPTelemetryData TelemetryData;
#endif
