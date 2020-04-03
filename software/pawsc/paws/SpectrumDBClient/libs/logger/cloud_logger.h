/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/


#ifndef MSRC_CLOUD_LOGGER_H_
#define MSRC_CLOUD_LOGGER_H_


#ifdef _MSC_VER
#include <winsock2.h>
typedef unsigned socklen_t;
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger_common.h"
#include "json-parser/json.h"
#include "utils/utils.h"


typedef struct {					
	log_level_e			level;						// what levels to log
	filename_t			logname;					// log file for general logging info
	host_addr_t			cloud_addr;					// could be a DNS address
	struct addrinfo*	cloud_addrinfo;				// the actual address IP info - populated by getaddrinfo, so leave unset when calling cloud_logger_create
	uint16_t			port;
} cloud_logger_cfg_t;


extern void* cloud_logger_create(cloud_logger_cfg_t* cfg);

extern void cloud_logger_log(void* logger_, log_app_t app, device_name_t device_name, cloud_buffer_t cloud_buffer, char *type, char* log_j_str);

extern void cloud_logger_free(void** cloud_logger_);



#endif

