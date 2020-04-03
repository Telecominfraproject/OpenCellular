/*
Copyright(c) Microsoft Corporation.All rights reserved.
Licensed under the MIT License.
*/

#ifndef SCTP_AGENT_DB_INFO_H_
#define SCTP_AGENT_DB_INFO_H_

#include <stdint.h>
#include <stdbool.h>

#include "utils/types.h"
#include "logger/logger.h"

#include "mme_address.h"
#include "sctp_agent_db_info.h"

typedef enum
{
	CONN_SCTP = 0,
	CONN_TCP
} mme_conn_type_e;

#define AZURE_MME_SOCKET_RETRY_S_MIN			(1)
#define AZURE_MME_PKT_GUARD_MS_MIN				(20)

typedef struct {
	uint16_t passthrough_ctl_port;
	uint16_t enb_sctp_port;
	uint16_t mme_sctp_port;
	mme_conn_type_e	mme_conn_type;
} sctp_agent_socket_cfg_t;

typedef struct {
	bool			use_passthrough_ctl; 
	uint32_t		passthrough_ctl_periodicity;
	uint32_t		azure_mme_socket_retry_s;					// how often to try to re-open a socket to MME
	uint32_t		azure_mme_socket_drop_enb_s;	// duration of a socket being disconnected which will cause an eNB socket drop and RESET
	uint32_t		azure_mme_socket_swap_mme_s;	// duration of a socket being disconnected which will cause an attempt to swap MME
	uint32_t		azure_mme_pkt_retx_guard_ms;				// how long to wait for packet acknowledgement , before doing a retx
	uint32_t		azure_mme_pkt_retx_max_attempts;			// max number of retx (not including initial tx). At max attempts, MME socket is dropped
} sctp_agent_settings_cfg_t;

typedef struct {
	sctp_agent_socket_cfg_t		socket_info;
	sctp_agent_settings_cfg_t	settings;
	logger_cfg_t				log_info;
	cloud_logger_cfg_t			cloud_log_info;
} sctp_agent_cfg_t;


extern bool set_sctp_agent_db_location(char* db);

extern char* get_sctp_agent_db_location(void);

extern bool get_sctp_agent_cfg(sctp_agent_cfg_t* cfg);			// note this does not populate "enb_sctp_port"

extern void mme_address_write_head_to_db(char* mme_addr);

#endif // SCTP_AGENT_DB_INFO_H_

