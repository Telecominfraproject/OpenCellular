/*
Copyright(c) Microsoft Corporation.All rights reserved.
Licensed under the MIT License.
*/

#ifndef SCTP_GLOBALS_H_
#define SCTP_GLOBALS_H_

#include "utils/types.h"
#include "utils/utils.h"
#include "logger/logger.h"

#include "sctp_cfg_options.h"

// reference to globals
extern device_name_t gDeviceName;
extern device_id_t gDeviceId;
extern void* g_logger;
extern void* g_cloud_logger;

#define SCTP_LOGGER_TYPE	"SCTPAGENT"

#define MAX_LOG_LINE_LENGTH				(200)
static char logline_[MAX_LOG_LINE_LENGTH];
#define LOG_PRINT(loglevel, fmt, args...) { \
	if (g_logger) \
		{ \
		int slen = snprintf(logline_, MAX_LOG_LINE_LENGTH, fmt, ##args); \
		if (!((slen <= 0) || (slen >= MAX_LOG_LINE_LENGTH))) \
				{ \
			logger_log(g_logger, logline_, loglevel, SCTP_LOGGER_TYPE, true, true, __FILENAME__, __LINE__, __func__, g_cloud_logger, gDeviceName, SCTP_LOGGER_TYPE); \
				} \
		} \
}


#define sctp_fatal_error(fmt, args...) { \
	LOG_PRINT(LOG_CRITICAL, fmt, ##args); \
	fatal_error("%s %d   " fmt, __FILE__, __LINE__, ##args) \
}




#endif // SCTP_GLOBALS_H_
