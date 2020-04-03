/*
Copyright(c) Microsoft Corporation.All rights reserved.
Licensed under the MIT License.
*/

#pragma once

#include "utils/types.h"

typedef void(*socket_send_func)(uint8_t* payload_, uint32_t payload_len);				// function to send S1AP message
typedef void(*event_callback_func)(void);												// generic callback 1 param

extern void tcps1_config(
	event_callback_func peer_ready_func,			// called by tcps1 when the MME is ready to receive data
	socket_send_func passthru_to_enb_func,			// called by tcps1 to send downlink data to eNB software.  SCTP-AGENT will check PAWS status to determine if tunnel is "open".
	socket_send_func passthru_to_mme_func,			// called by tcps1 to send uplink data to MME.  SCTP-AGENT will check PAWS status to determine if tunnel is "open".
	socket_send_func send_to_mme_func,				// called by tcps1 to send uplink data to MME.  SCTP-AGENT will NOT check PAWS status to determine if tunnel is "open".
	event_callback_func drop_enb_socket_func,		// called by tcps1 to tell SCTP_AGENT that the eNB<-->SCTPAGENT sockets (listener and connection) should be closed
	event_callback_func drop_mme_socket_func,		// called by tcps1 to tell SCTP_AGENT that the SCTP_AGENT<-->MME socket should be closed
	event_callback_func reset_mme_socket_func,		// called by tcps1 to tell SCTP_AGENT that the SCTP_AGENT<-->MME socket should be reset (i.e. closed followed immediately by open)
	void* logger,									// Pointer to the "logger" instance which tcps1 shall use 
	void* cloud_logger,								// Pointer to the "cloud_logger" instance which tcps1 shall use 
	device_name_t device_name,						// Device name
	device_id_t device_id,							// Device ID
	uint32_t pkt_ack_guard_ms,						// How long tcps1 will wait for an acknowledgement (from the MME) for a UL data packet,  before retransmitting it.
	uint32_t pkt_retx_max_attempts);				// Numnber of retransmissions attempts for UL data before interface is classed as dropped.

// free up all TCPS1 resources
extern void tcps1_free(void);

// Perform TCPS1 reset immediately
extern void tcps1_reset(void);

// Trigger a TCPS1-reset at next event
extern void tcps1_trigger_reset(void);

// Handle an uplink message
extern void tcps1_process_ul(uint8_t* pkt, uint32_t pkt_len);

// Handle an downlink message
extern void tcps1_process_dl(uint8_t* pkt, uint32_t pkt_len);

// Process a timer expiry
extern void tcps1_process_timer_expiry(void);

// Tell tcps1 that MME socket has been disconnected
extern void tcps1_mme_disconnected(void);

// Tell tcps1 that MME socket has been connected
extern void tcps1_mme_connected(void);

// Query tcps1 to determine if MME is ready for data
extern bool tcps1_is_mme_ready(void);


