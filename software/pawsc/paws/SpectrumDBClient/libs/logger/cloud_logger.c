/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

// Platform headers
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "json-parser/json_utils.h"

#include "utils/types.h"
#include "cloud_logger.h"
#include "logger.h"

#define SOCKET_BACKOFF_SECS		(60)

typedef struct
{
	// config
	cloud_logger_cfg_t		cfg;

	// dynamic
	int						sock;
	time_t					socket_backoff;		// if the socket is down, backoff and recreate  once this time is reached
	uint32_t				msgid;

	// local logger
	void*					local_logger;
} cloud_logger_t;


// macros for local logging
#define CLOUD_LOGGER_TYPE				 "cloud_logger"
#define MAX_LOG_LINE_LENGTH				(200)
static char logline_[MAX_LOG_LINE_LENGTH];
#define LOCAL_LOG(local_logger, loglevel, fmt, args...) { \
	if (local_logger) \
	{ \
		int slen = snprintf(logline_, MAX_LOG_LINE_LENGTH, fmt, ##args); \
		if (!((slen <= 0) || (slen >= MAX_LOG_LINE_LENGTH))) \
		{ \
			logger_log(local_logger, logline_, loglevel, CLOUD_LOGGER_TYPE, true, true, __FILENAME__, __LINE__, __func__, NULL, "", ""); \
		} \
	} \
}


//#######################################################################################
// statuc func prototypes
typedef int(*make_outgoing_socket_func_t)(const char *ip_addr_str, unsigned port, struct addrinfo **addrinfo, time_t* socket_retry, void* local_logger);
int clog_make_outgoing_udp_socket(const char *ip_addr_str, unsigned port, struct addrinfo **addrinfo, time_t* socket_retry, void* local_logger);
int clog_make_outgoing_tcp_socket(const char *ip_addr_str, unsigned port, struct addrinfo **addrinfo, time_t* socket_retry, void* local_logger);
typedef bool(*send_on_socket_func_t)(cloud_logger_t* logger, char* log);
bool clog_send_on_tcp_socket(cloud_logger_t* logger, char* log);
bool clog_send_on_udp_socket(cloud_logger_t* logger, char* log);
make_outgoing_socket_func_t make_outgoing_socket = clog_make_outgoing_udp_socket;       // use UDP 
send_on_socket_func_t send_on_socket = clog_send_on_udp_socket;							// use UDP 


//#######################################################################################
// ## limit needed as UDP
#define CLOUD_LOGGER_MAX_PACKET		(1000)
static char pkg_too_big[] = "{\"msg\":\"Log message discarded as > 1000 bytes\"}";




//#######################################################################################
int clog_make_outgoing_udp_socket(const char *addr_str, unsigned port, struct addrinfo **addr_info, time_t* socket_retry, void* local_logger)
{
	(void)port; // unused

	time_t now = time(NULL);
	if (now < *socket_retry)			// dont try to create socket if still in backoff period
	{
		return -1;
	}

	int32_t sockfd=-1;
	struct addrinfo *servinfo = NULL;
	struct addrinfo *p = NULL;

	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	// convert port to a string
	char snum[7];
	snprintf(snum, sizeof(snum), "%d", port);

	if ((getaddrinfo(addr_str, snum, &hints, &servinfo)) != 0)
	{
		LOCAL_LOG(local_logger, LOG_ERROR, "[addr:%s] Failure in getaddr_info", addr_str);
		goto error_hdl;
	}

	// loop through all the results 
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			LOCAL_LOG(local_logger, LOG_ERROR, "[addr:%s] Failure to open socket", addr_str);
			continue;
		}

		// free previous addr_info
		if (*addr_info)
		{
			free(*addr_info);
			*addr_info = NULL;
		}

		// make a deep copy of p
		if (!(*addr_info = malloc(sizeof(struct addrinfo))))
			goto error_hdl;
		**addr_info = *p;
		(**addr_info).ai_addr=0;
		(**addr_info).ai_canonname=0;
		if (!((**addr_info).ai_addr = malloc(p->ai_addrlen)))
			goto error_hdl;
		memcpy((**addr_info).ai_addr, p->ai_addr, p->ai_addrlen);
		if (p->ai_canonname) 
		{
			if (!((**addr_info).ai_canonname = malloc(strlen(p->ai_canonname))))
				goto error_hdl;
			memcpy((**addr_info).ai_canonname, p->ai_canonname, strlen(p->ai_canonname));
		}

		break; // if we get here, we must have connected successfully
	}

	if (p == NULL)
	{
		LOCAL_LOG(local_logger, LOG_ERROR, "[addr:%s] No sockets found ", addr_str);
		// looped off the end of the list with no connection
		goto error_hdl;
	}

	freeaddrinfo(servinfo); // all done with this structure

	*socket_retry = 0;
	LOCAL_LOG(local_logger, LOG_NOTICE, "[addr:%s] Socket sucessfully opened ", addr_str);
	return sockfd;

error_hdl:
	if (servinfo) freeaddrinfo(servinfo);
	if (sockfd >= 0) close(sockfd);
	*socket_retry = now + SOCKET_BACKOFF_SECS;		// set new backoff period
	if (*addr_info)
	{
		free(*addr_info);
		*addr_info = NULL;
	}
	return -1;
}

//#######################################################################################
int clog_make_outgoing_tcp_socket(const char *addr_str, unsigned port, struct addrinfo **addr_info, time_t* socket_retry, void* local_logger)
{
	(void)addr_info;

	time_t now = time(NULL);
	if (now < *socket_retry)			// dont try to create socket if still in backoff period
	{
		return -1;
	}

	int32_t sockfd=-1;
	struct addrinfo *servinfo = NULL;
	struct addrinfo *p = NULL;

	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// convert port to a string
	char snum[7];
	snprintf(snum, sizeof(snum), "%d", port);

	if ((getaddrinfo(addr_str, snum, &hints, &servinfo)) != 0)
	{
		LOCAL_LOG(local_logger, LOG_ERROR, "[addr:%s] Failure in getaddr_info", addr_str);
		goto error_hdl;
	}

	// loop through all the results 
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			LOCAL_LOG(local_logger, LOG_ERROR, "[addr:%s] Failure to open socket", addr_str);
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			LOCAL_LOG(local_logger, LOG_ERROR, "[addr:%s] Failure to connect socket", addr_str);
			close(sockfd);
			continue;
		}
		break; // if we get here, we must have connected successfully
	}

	if (p == NULL)
	{
		LOCAL_LOG(local_logger, LOG_ERROR, "[addr:%s] No sockets found ", addr_str);
		// looped off the end of the list with no connection
		goto error_hdl;
	}

	freeaddrinfo(servinfo); // all done with this structure

	*socket_retry = 0;

	LOCAL_LOG(local_logger, LOG_ERROR, "[addr:%s] Socket sucessfully opened ", addr_str);
	LOCAL_LOG(local_logger, LOG_NOTICE, "[addr:%s] Socket sucessfully opened ", addr_str);
	return sockfd;


error_hdl:
	if (servinfo) freeaddrinfo(servinfo);
	if (sockfd >= 0) close(sockfd);
	*socket_retry = now + SOCKET_BACKOFF_SECS;		// set new backoff period
	return -1;
}


//#######################################################################################
bool clog_send_on_udp_socket(cloud_logger_t* logger, char* log)
{
	if ((!logger) || (!log))
	{
		return false;
	}

	// create socket if not yet created
	if (logger->sock == -1)
	{
		if ((logger->sock = make_outgoing_socket(logger->cfg.cloud_addr, logger->cfg.port, &logger->cfg.cloud_addrinfo,  &logger->socket_backoff, logger->local_logger)) == -1)
		{
			// still no socket
			return false;
		}
	}
	
	// send it on sock
	size_t num_to_send = strlen(log);
	ssize_t num_sent = sendto(logger->sock, log, num_to_send, 0, logger->cfg.cloud_addrinfo->ai_addr, logger->cfg.cloud_addrinfo->ai_addrlen);
	if (num_sent != (ssize_t)num_to_send)
	{
		// try again
		// delete and recreate the socket
		if (logger->sock >= 0)
		{
			close(logger->sock);
			logger->sock = -1;
		}

		if ((logger->sock = make_outgoing_socket(logger->cfg.cloud_addr, logger->cfg.port, &logger->cfg.cloud_addrinfo, &logger->socket_backoff, logger->local_logger)) == -1)
		{
			return false;
		}

		// re-send the data
		ssize_t num_sent = sendto(logger->sock, log, num_to_send, 0, logger->cfg.cloud_addrinfo->ai_addr, logger->cfg.cloud_addrinfo->ai_addrlen);
		if (num_sent != (ssize_t)num_to_send)
		{
			LOCAL_LOG(logger->local_logger, LOG_ERROR, "[addr:%s] Problem sending to socket", logger->cfg.cloud_addr);
			return false;
		}
	}

	return true;
}

//#######################################################################################
bool clog_send_on_tcp_socket(cloud_logger_t* logger, char* log)
{
	if ((!logger) || (!log))
	{
		return false;
	}

	// create socket if not yet created
	if (logger->sock == -1)
	{
		if ((logger->sock = make_outgoing_socket(logger->cfg.cloud_addr, logger->cfg.port, &logger->cfg.cloud_addrinfo, &logger->socket_backoff, logger->local_logger)) == -1)
		{
			// still no socket
			return false;
		}
	}

	// send it on sock
	size_t num_to_send = strlen(log);
	ssize_t num_sent = send(logger->sock, log, num_to_send, MSG_DONTWAIT | MSG_NOSIGNAL);
	if (num_sent != (ssize_t)num_to_send)
	{
		// try agai
		// delete and recreate the socket
		if (logger->sock >= 0)
		{
			close(logger->sock);
			logger->sock = -1;
		}

		if ((logger->sock = make_outgoing_socket(logger->cfg.cloud_addr, logger->cfg.port, &logger->cfg.cloud_addrinfo, &logger->socket_backoff, logger->local_logger)) == -1)
		{
			return false;
		}

		// re-send the data
		num_sent = send(logger->sock, log, num_to_send, MSG_DONTWAIT | MSG_NOSIGNAL);
		if (num_sent != (ssize_t)num_to_send)
		{
			LOCAL_LOG(logger->local_logger, LOG_ERROR, "[addr:%s] Failure to open socket", logger->cfg.cloud_addr);
			return false;
		}
	}

	return true;
}



//#######################################################################################
void* cloud_logger_create(cloud_logger_cfg_t* cfg)
{
	cloud_logger_t *logger = NULL;
	void* local_logger = NULL;

	if (!cfg)
	{
		goto error_hdl;
	}

	// make a "local" logger just for errors and critical info
	logger_cfg_t logger_cfg = { LOG_NOTICE, LOG_NOTICE, { 0 }, "50k", 2, 1 };
	memcpy(logger_cfg.logname, cfg->logname, sizeof(filename_t));
	if (!(local_logger = logger_create(&logger_cfg)))
	{
		goto error_hdl;
	}

	//################# validate the config
	// is the name defined
	if (strlen(cfg->cloud_addr) <= 0)
	{
		LOCAL_LOG(local_logger, LOG_ERROR, "No cloud_addr defined in database");
		goto error_hdl;
	}

	// validation is OK, so create entity
	if (!(logger = malloc(sizeof(cloud_logger_t))))
	{
		LOCAL_LOG(local_logger, LOG_ERROR, "Unable to malloc the cloud_logger_t");
		goto error_hdl;
	}

	memset(logger, 0, sizeof(cloud_logger_t));
	logger->sock = -1;

	memcpy(&logger->cfg, cfg, sizeof(cloud_logger_cfg_t));

	logger->sock = make_outgoing_socket(cfg->cloud_addr, cfg->port, &logger->cfg.cloud_addrinfo, &logger->socket_backoff, local_logger);

	logger->local_logger = local_logger;
	local_logger = NULL;

	return logger;

error_hdl:
	if ((logger) && (logger->sock >= 0))
	{
		close(logger->sock);
		logger->sock = -1;
	}
	if (local_logger)
	{
		logger_free(&local_logger);
	}
	if (logger->local_logger)
	{
		logger_free(&logger->local_logger);
	}
	if (logger)
	{
		free_and_null((void**)&logger);
	}
	return NULL;
}


//######################################################################################################################
void cloud_logger_log(void* logger_, log_app_t app, device_name_t device_name, cloud_buffer_t cloud_buffer, char *type, char* log_j_str)
{
	if ((!logger_) || (!log_j_str) || (!type))
	{
		return;
	}

	char* pkt = log_j_str;

	if (strlen(log_j_str) > CLOUD_LOGGER_MAX_PACKET)
	{
		// its too big, use standard Truncate warning message
		pkt = pkg_too_big;
	}


	cloud_logger_t* logger = (cloud_logger_t*)logger_;

	// convert the json to string
	json_value* log_j = NULL;
	char* log = NULL;

	// we have the contents, now convert to json
	if (!(log_j = json_parse((json_char*)pkt, strlen(pkt))))
	{
		return;
	}

	// add the common metadata
	char utc_ms[30];
	logger_get_datetimeUTC_ms(utc_ms, sizeof(utc_ms));
	if (!(json_set_string(log_j, "utc", utc_ms)))
		return;
	if (!(json_set_string(log_j, "node", (device_name && (strlen(device_name) > 0)) ? device_name : "DeviceIdNotSet")))
		return;
	if (!(json_set_string(log_j, "type", type)))
		return;
	if (!(json_set_string(log_j, "sub-type", app)))
		return;
	if (!(json_set_string(log_j, "bufferid", cloud_buffer)))
		return;
	if (!(json_set_int(log_j, "msgid", logger->msgid++)))
		return;



	// check that the JSON is actually valid by converting it to a string
	if (!(log = json_value_2_string(log_j)))
	{
		return;
	}

	// send the messages
	if (!(send_on_socket(logger, log)))
	{
		goto error_hdl;
	}

	logger->msgid++;

	if (log) free(log);
	if (log_j) json_value_free(log_j);

	return;

error_hdl:
	if (log) free(log);
	if (log_j) json_value_free(log_j);
}



//#######################################################################################
extern void cloud_logger_free(void** logger)
{
	if ((logger) && (*logger))
	{
		cloud_logger_t* cloud_logger = (cloud_logger_t*)*logger;
		if (cloud_logger->sock)
		{
			close(cloud_logger->sock);
		}
		if (cloud_logger->local_logger)
		{
			logger_free(&cloud_logger->local_logger);
		}
		free_and_null((void**)logger);
	}
}