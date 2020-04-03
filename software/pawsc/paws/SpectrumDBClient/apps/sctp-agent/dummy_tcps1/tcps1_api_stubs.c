/*
Copyright(c) Microsoft Corporation.All rights reserved.
Licensed under the MIT License.
*/

// Standard headers
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tcps1_api.h"

void tcps1_config(event_callback_func peer_ready_func,
	socket_send_func passthru_to_enb_func, socket_send_func passthru_to_mme_func, socket_send_func send_to_mme_func,
	event_callback_func drop_enb_socket_func, event_callback_func drop_mme_socket_func, event_callback_func reset_mme_socket_func,
	void* logger, void* cloud_logger,
	device_name_t device_name, device_id_t device_id,
	uint32_t pkt_ack_guard_ms, uint32_t pkt_retx_max_attempts)
{
	printf("%s: peer_ready_func=%p, passthru_to_enb_func=%p, passthru_to_mme_func=%p, send_to_mme_func=%p, drop_enb_socket_func=%p, drop_mme_socket_func=%p, reset_mme_socket_func=%p, logger=%p, cloud_logger=%p, device_name=%p, device_id=%d, pkt_ack_guard_ms=%d, pkt_retx_max_attempts=%d \n", 
		__func__, peer_ready_func, passthru_to_enb_func, passthru_to_mme_func, send_to_mme_func, drop_enb_socket_func, drop_mme_socket_func, reset_mme_socket_func, logger, cloud_logger, device_name, device_id, pkt_ack_guard_ms,  pkt_retx_max_attempts);
}

void tcps1_reset(void)
{
	printf("%s\n", __func__);
}

void tcps1_trigger_reset(void)
{
	printf("%s\n", __func__);
}

void tcps1_free(void)
{
	printf("%s\n", __func__);
}

void tcps1_process_ul(uint8_t* pkt, uint32_t pkt_len)
{
	printf("%s: pkt=%p len=%d\n", __func__, pkt, pkt_len);
}

void tcps1_process_dl(uint8_t* pkt, uint32_t pkt_len)
{
	printf("%s: pkt=%p len=%d\n", __func__, pkt, pkt_len);
}

void tcps1_process_timer_expiry(void)
{
	printf("%s\n", __func__);
}

void tcps1_mme_disconnected(void)
{
	printf("%s\n", __func__);
}

void tcps1_mme_connected(void)
{
	printf("%s\n", __func__);
}

bool tcps1_is_mme_ready(void)
{
	printf("%s\n", __func__);
	return false;
}
