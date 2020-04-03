/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#include "paws_globals.h"
#include "paws_common.h"
#include "paws_dal_settings.h"
#include "paws_dal_devices.h"

// global vars are denoted by a leading 'g'
device_name_t gDeviceName = { 0 };
void* gPawsAppLogger = NULL;
logger_cfg_t gPawsAppLoggerCfg = { 0 };
void* gPawsCloudLogger = NULL;
static cloud_logger_cfg_t gPawsCloudLoggerCfg = { 0, { 0 }, { 0 }, 0, 0 };
paws_reg_domain_e g_paws_reg_domain = PAWS_REGDOMAIN_ETSI;


//#######################################################################################
void get_gPawsLoggerSettings(void)
{
	paws_settings_t settings;
	
	if (!(paws_read_settings(&settings)))
	{
		fprintf(stderr,"Unable to read settings");
		return;
	}

	set_gPawsAppLoggerInfo(&settings.loginfo.app_log);
	set_gPawsCloudLoggerInfo(&settings.loginfo.cloud_log);
}


//#######################################################################################
bool set_gPawsAppLoggerInfo(logger_cfg_t* cfg)
{
	if (!cfg)
	{
		return  false;
	}

	if ((!gPawsAppLogger) || (memcmp(&gPawsAppLoggerCfg, cfg, sizeof(logger_cfg_t)) != 0))		// if not already created, or config has changed
	{
		// copy the new config
		memcpy(&gPawsAppLoggerCfg, cfg, sizeof(logger_cfg_t));

		// if already created free old one
		if (gPawsAppLogger)
			logger_free(&gPawsAppLogger);

		// if there is a filename configured
		if (strlen(gPawsAppLoggerCfg.logname))
		{
			if (!(gPawsAppLogger = logger_create(cfg)))
				return false;
		}
	}

	return true;
}


//#######################################################################################
bool set_gPawsCloudLoggerInfo(cloud_logger_cfg_t* cfg)
{
	if ((!cfg))
	{
		return  false;
	}

	if ((!gPawsCloudLogger) || (memcmp(&gPawsCloudLoggerCfg, cfg, sizeof(cloud_logger_cfg_t)) != 0))		// if not already created, or config has changed
	{
		// copy the new config
		memcpy(&gPawsCloudLoggerCfg, cfg, sizeof(cloud_logger_cfg_t));

		// if already created free old one
		if (gPawsCloudLogger)
			cloud_logger_free(&gPawsCloudLogger);

		// if there is an IP configured
		if (strlen(gPawsCloudLoggerCfg.cloud_addr))
		{
			if (!(gPawsCloudLogger = cloud_logger_create(cfg)))
				return false;
		}
	}

	return true;
}


//#######################################################################################
extern void populate_gDeviceName(void)
{
	if ((!(paws_get_device_name(gDeviceName))) || (strlen(gDeviceName) < 1))
	{
		char log[50];
		sprintf(log, "%s", "Unable to read DeviceId");
		logger_log(gPawsAppLogger, log, LOG_ERROR, "Init", true, true, __FILENAME__, __LINE__, __func__, gPawsCloudLogger, gDeviceName, PAWS_LOG_TYPE);
	}

}


//#######################################################################################
void PawsGlobalsFree(void)
{
	if (gPawsAppLogger)
		logger_free(&gPawsAppLogger);

	if (gPawsCloudLogger)
		cloud_logger_free(&gPawsCloudLogger);
}
