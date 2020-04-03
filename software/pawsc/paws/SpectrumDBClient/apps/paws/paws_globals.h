/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_GLOBALS_H_
#define PAWS_GLOBALS_H_

#include "utils/types.h"
#include "logger/logger.h"
#include "logger/cloud_logger.h"

typedef enum {
	PAWS_REGDOMAIN_GENERIC=0,				// generic PAWS rfc 7545
	PAWS_REGDOMAIN_ETSI						// ETSI 301 598
} paws_reg_domain_e;


// global vars
extern device_name_t gDeviceName;
extern logger_cfg_t gPawsAppLoggerCfg;
extern void* gPawsAppLogger;
extern void* gPawsCloudLogger;
extern paws_reg_domain_e g_paws_reg_domain;

// prototypes
extern void get_gPawsLoggerSettings(void);
extern bool set_gPawsAppLoggerInfo(logger_cfg_t* cfg);
extern bool set_gPawsCloudLoggerInfo(cloud_logger_cfg_t* cfg);


extern void populate_gDeviceName(void);	 

extern void PawsGlobalsFree(void);

#endif

