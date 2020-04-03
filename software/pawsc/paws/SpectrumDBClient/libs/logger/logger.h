/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/


#ifndef MSRC_LOGGER_H_
#define MSRC_LOGGER_H_

#include "utils/types.h"

#include "logger_common.h"
#include "cloud_logger.h"


#define MAX_LOGROTATE_LEN	    		(10)		// e.g.	"100k", "1M", "10M", "1G" etc
typedef char logsize_t[MAX_LOGROTATE_LEN];

typedef struct {										
	log_level_e				level;						// what levels to log
	log_level_e				cloud_level;				// what levels to pass to cloug-logger
	filename_t				logname;					// log file for general logging info
	logsize_t				size;						// how big a logfile should get before rotating
	int						max;						// how many logfiles to keep
	uint8_t					compress;					// should old file be compressed
} logger_cfg_t;

#define LOGGER_MAX_LENGTH				(10000)			// maximum length of a line to log

extern char* loglevel_2_str(log_level_e level);

extern void* logger_create(logger_cfg_t* cfg);

extern void logger_log(void *logger_, const char *log, log_level_e level, log_app_t app, bool add_level, bool add_timestamp, const char* file_, int line_, const char* func_, void *cloud_logger, device_name_t device_name, char *type);

extern void logger_free(void** logger);



#endif

