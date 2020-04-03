/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/


#ifndef MSRC_LOGGER_COMMON_H_
#define MSRC_LOGGER_COMMON_H_


#define CLOUD_LOGGER_STDOUT			"STDOUT"

#define MAX_CLOUD_BUFFER_LEN		(20)
typedef char cloud_buffer_t[MAX_CLOUD_BUFFER_LEN];

#define				LOGGER_MAX_APP_LEN			(20)
typedef char log_app_t[LOGGER_MAX_APP_LEN];

typedef enum {
	LOG_FUNC,				// functions
	LOG_DEBUG,				// gory details
	LOG_INFO,				// informational
	LOG_NOTICE,				// more important information
	LOG_WARNING,			// more important information
	LOG_ERROR,				// error
	LOG_CRITICAL				// fatal - app will probably exit
} log_level_e;



int logger_get_datetimeUTC_ms(char* dst, int dstlen);			// dst points to char array at least 




#endif

