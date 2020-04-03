/*
Copyright(c) Microsoft Corporation.All rights reserved.
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
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#endif

// Standard headers
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "utils/utils.h"
#include "logger/logger.h"

#include "sctp_agent_db_info.h"
#include "sctp_agent.h"
#include "lte_db_info.h"
#include "mme_address.h"

#include "sctp_globals.h"


#include "tcps1_api.h"


// static func prototypes
static char* get_status_str(void);
static char* get_status_json_str(void);






// loggers
void* g_logger = NULL;
void* g_cloud_logger = NULL;

#define LOG_EVENT(ev) { \
	if (g_cloud_logger) \
	{ \
		int slen = snprintf(logline_, MAX_LOG_LINE_LENGTH, "{ \"event\":\"%s\", %s }", \
								ev, get_status_json_str()); \
		if (!((slen <= 0) || (slen >= MAX_LOG_LINE_LENGTH))) \
		{ \
			cloud_logger_log(g_cloud_logger, SCTP_LOGGER_TYPE, gDeviceName, "SCTP-EVENT", SCTP_LOGGER_TYPE, logline_); \
		} \
	} \
}

// config
sctp_agent_cfg_t g_cfg;

// Sockets
static int32_t g_mme_sock = -1;
static uint16_t g_paws_app_listener_port = 0;
static int32_t g_paws_app_listener_sock = -1;
static int32_t g_paws_app_sock = -1;
static uint16_t g_enodeb_listener_port = 0;
static int32_t g_enodeb_listener_sock = -1;
static int32_t g_enodeb_sock = -1;
static fd_set g_active_sock_set;

// timers
#define PAWS_TUNNEL_PERIODIC_TOLERANCE	(15)
static uint32_t g_passthrough_ctl_periodicity = 0;
static time_t g_tunnel_end_time = 0;
static time_t g_status_time = 0;
#define SHOW_STATUS_PERIODICITY			(30)

// Management of the MME
static time_t g_mme_lost_time = 0;			// when the connection to the mme was lost
static time_t g_mme_swap_time = 0;			// when the MME was last swapped
static bool g_mme_ready = true;				// is the far end ready.   For sctp, we always set it to true

// static prototypes


// #########################################################################################################
static void close_socket(int32_t* sock);
static void close_all_sockets(void);
static int32_t make_listener_tcp_socket(uint16_t port);
static int32_t make_listener_sctp_socket(uint16_t port);
static int32_t make_outgoing_sctp_socket(const char *ip_addr_str, uint16_t port);
static void make_mme_outgoing_tcp_socket(void);
static void drop_mme_outgoing_tcp_socket(void);
static void reset_mme_outgoing_tcp_socket(void);
static void make_mme_outgoing_sctp_socket(void);
static void make_paws_listener_socket(void);
static void make_enodeb_listener_sctp_socket(void);
static void close_enodeb_listener_sctp_socket(void);
static void close_enodeb_sctp_socket(void);
static void close_enodeb_sctp_sockets(void);
static int32_t accept_connection(int32_t listener_sock, const char* sock_name);
static bool control_plane_enabled(bool* changed);
static uint8_t* read_sock_or_close(int32_t *sock, int32_t *num_bytes_read);
static bool send_sock_or_close(int32_t *sock, uint8_t* buf, uint32_t buflen);
static void mme_connected_sctp(void);
static void mme_disconnected_sctp(void);
static void mme_connected_tcp(void);
static void mme_disconnected_tcp(void);
static void enb_connected_sctp(void);
static void enb_disconnected_sctp(void);
static void enb_connected_tcp(void);
static void enb_disconnected_tcp(void);
static void disconnect_enb_sockets(void);
static void mme_ready(void);
static void process_ul_msg(uint8_t* payload_, uint32_t payload_len);
static void process_ul_msg_tcps1(uint8_t* payload_, uint32_t payload_len);
static void process_dl_msg(uint8_t* payload_, uint32_t payload_len);
static void process_dl_msg_tcps1(uint8_t* payload_, uint32_t payload_len);
static void passthru_to_enb(uint8_t* payload_, uint32_t payload_len);
static void passthru_to_mme(uint8_t* payload_, uint32_t payload_len);
static void send_to_mme(uint8_t* payload_, uint32_t payload_len);
static void handle_enodeb_sock(void);
static void handle_mme_sock(void);
static void handle_paws_app_sock(void);
static void handle_paws_app_listener_sock(void);
static void handle_enodeb_listener_sock(void);
static char* get_status_str(void);
static char* get_status_json_str(void);
static void show_status(void);

// functions to for mme socket management.  Details will change depending on whether tcp or sctp is used.  This is because tcp will call into tcps1 code. 
typedef void(*make_mme_outgoing_socket_func)(void);
typedef void(*socket_event_func)(void);
typedef void(*process_msg_func)(uint8_t* payload_, uint32_t payload_len);
typedef void(*timer_expiry_func)(void);
static make_mme_outgoing_socket_func make_mme_outgoing_socket_f = make_mme_outgoing_sctp_socket;
static socket_event_func mme_connected_f = mme_connected_sctp;
static socket_event_func mme_disconnected_f = mme_disconnected_sctp;
static socket_event_func enb_connected_f = enb_connected_sctp;
static socket_event_func enb_disconnected_f = enb_disconnected_sctp;
static process_msg_func process_ul_msg_f = process_ul_msg;
static process_msg_func process_dl_msg_f = process_dl_msg;
static timer_expiry_func timer_expiry_f = NULL;
static void system_reboot(void);
static void lte_reboot_if_needed(void);




// #########################################################################################################
static void system_reboot(void)
{
	system("reboot");
}


// #########################################################################################################
static void lte_reboot_if_needed(void)
{
	static bool rebooted = true;

#ifdef REBOOT_AFTER_RFTXSTATUS_DROP
	// if this is not post-reboot, and RfTxStatus has been turned off
	if (!rebooted)
	{
		int tx_status;
		bool ok = lte_db_get_RxTxStatus(&tx_status);
		if ((!ok) || (!tx_status))			// if we could not read database, or RfTxStatus=0, we need to reboot
		{
			LOG_PRINT(LOG_ERROR, "Rebooting due to RxTxStatus=0\n");
			sleep(5);
			system_reboot();
		}
	}
#endif
	rebooted = false;
}


// #########################################################################################################
static void close_socket(int32_t* sock)
{
	if (*sock != -1)
	{
		close(*sock);
		FD_CLR(*sock, &g_active_sock_set);
		*sock = -1;
	}
}


// #########################################################################################################
static void close_all_sockets(void)
{
	close_socket(&g_mme_sock);
	close_socket(&g_paws_app_listener_sock);
	close_socket(&g_paws_app_sock);
	close_socket(&g_enodeb_listener_sock);
	close_socket(&g_enodeb_sock);
}


// #########################################################################################################
static int32_t make_outgoing_socket(const char *addr_str, uint16_t port, int protocol)
{
	LOG_PRINT(LOG_FUNC, "addr=%s port=%d\n", addr_str, port);

	int32_t sockfd;
	struct addrinfo *servinfo = NULL;
	struct addrinfo *p = NULL;

	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = protocol;

	// convert port to a string
	char snum[7];
	snprintf(snum, sizeof(snum), "%d", port);

	LOG_PRINT(LOG_NOTICE, "Doing dns lookup for [addr=%s, port=%d]", addr_str, port);
	if ((getaddrinfo(addr_str, snum, &hints, &servinfo)) != 0)
	{
		LOG_PRINT(LOG_ERROR, "Failure from getaddrinfo [addr=%s, port=%d]", addr_str, port);
		goto error_hdl;
	}

	// loop through serverinfo
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			continue;
		}

		if (protocol == IPPROTO_TCP)
		{
			int flag = 1;
			setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag)); 
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			continue;
		}

		break; // if we get here, we must have connected successfully
	}

	if (p == NULL)
	{
		// looped off the end of the list with no connection
		goto error_hdl;
	}

	freeaddrinfo(servinfo); // all done with this structure

	return sockfd;

error_hdl:
	if (servinfo) freeaddrinfo(servinfo);
	return -1;
}


// #########################################################################################################
static int32_t make_listener_tcp_socket(uint16_t port)
{
	LOG_PRINT(LOG_FUNC, "port=%d", port);
	
	int32_t sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		sctp_fatal_error("SCTP-Agent:  Socket failure");
	}

    // Make the socket non-blocking to avoid a race-condition that could cause 
    // us to stop responding. Just google "select accept race condition" for more info.
	int32_t flags = fcntl(sock, F_GETFL, 0);
	if (flags < 0)
	{
		sctp_fatal_error("fcntl 1");
	}
	flags |= O_NONBLOCK;
	if (fcntl(sock, F_SETFL, flags) != 0)
	{
		sctp_fatal_error("fcntl 2");
	}

    struct sockaddr_in serv_addr = { 0 };
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	int32_t reuse = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1)
	{
		sctp_fatal_error("setsockopt SO_REUSEADDR failed");
	}

	if (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		sctp_fatal_error("bind");
	}
	if (listen(sock, 1) < 0)
	{
		sctp_fatal_error("listen");
	}
    return sock;
}

// #########################################################################################################
static int32_t make_listener_sctp_socket(uint16_t port)
{
	LOG_PRINT(LOG_FUNC, "port=%d", port);

	int32_t sock = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sock == -1)
	{
		sctp_fatal_error("socket");
	}

	// Make the socket non-blocking to avoid a race-condition that could cause 
	// us to stop responding. Just google "select accept race condition" for more info.
	int32_t flags = fcntl(sock, F_GETFL, 0);
	if (flags < 0)
	{
		close(sock); 
		sctp_fatal_error("fcntl 1");
	}
	flags |= O_NONBLOCK;
	if (fcntl(sock, F_SETFL, flags) != 0)
	{
		close(sock);
		sctp_fatal_error("fcntl 2");
	}

	struct sockaddr_in serv_addr = { 0 };
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	int32_t reuse = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1)
	{
		close(sock);
		sctp_fatal_error("setsockopt SO_REUSEADDR failed");
	}
	
	if (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		close(sock);
		sctp_fatal_error("bind");
	}

	if (listen(sock, 1) < 0)
	{
		close(sock);
		sctp_fatal_error("listen");
	}

	return sock;
}



// #########################################################################################################
static int32_t make_outgoing_sctp_socket(const char *addr_str, uint16_t port)
{
	return make_outgoing_socket(addr_str, port, IPPROTO_SCTP);
}



// #########################################################################################################
static void make_mme_outgoing_sctp_socket(void)
{
	if (g_mme_sock == -1)
	{
		// make MME socket
		char* mme_addr = mme_address_list_get_curr();
		g_mme_sock = make_outgoing_sctp_socket(mme_addr, g_cfg.socket_info.mme_sctp_port);
		if (g_mme_sock != -1)
		{
			mme_connected_f();
		}
	}
}




// #########################################################################################################
static void make_mme_outgoing_tcp_socket(void)
{
	if (g_mme_sock == -1)
	{
		bool swapped = false;
		char* mme_addr = NULL;
		// is it time to swap the MME
		if ((g_mme_swap_time + (time_t)g_cfg.settings.azure_mme_socket_swap_mme_s) < time(NULL))
		{
			swapped = mme_address_list_goto_next();
			if (swapped)
			{
				mme_addr = mme_address_list_get_curr();
				LOG_PRINT(LOG_NOTICE, "MME swapped to %s\n", ((mme_addr == NULL) ? "--" : mme_addr));
				disconnect_enb_sockets();
			}
			g_mme_swap_time = time(NULL);
		}

		// if the MME has been down for long enough, drop the eNB socket
		if ((g_mme_lost_time + (time_t)g_cfg.settings.azure_mme_socket_drop_enb_s) < time(NULL))
		{
			if ((g_enodeb_listener_sock != -1) || (g_enodeb_sock != -1))
			{
				LOG_PRINT(LOG_NOTICE, "MME down for %d secs, so closing eNB sockets", g_cfg.settings.azure_mme_socket_drop_enb_s);
				disconnect_enb_sockets();
			}
		}

		mme_addr = mme_address_list_get_curr();
		if (!mme_addr)
		{
			LOG_PRINT(LOG_ERROR, "No MME address defied\n");
		}
		LOG_PRINT(LOG_DEBUG, "Trying to connect MME socket (%s)\n", mme_addr);

		// make MME socket
		g_mme_sock = mme_address_make_outgoing_socket(mme_addr, g_cfg.socket_info.mme_sctp_port, IPPROTO_TCP);

		if (g_mme_sock != -1)
		{
			mme_connected_f();
		}

	}
}


// #########################################################################################################
static void drop_mme_outgoing_tcp_socket(void)
{
	if (g_mme_sock != -1)
	{
		// close the socket
		LOG_PRINT(LOG_NOTICE, "dropping MME socket");
		close_socket(&g_mme_sock);
		mme_disconnected_tcp();
	}
}


// #########################################################################################################
static void reset_mme_outgoing_tcp_socket(void)
{
	LOG_PRINT(LOG_NOTICE, "resetting MME socket");
	// drop it
	drop_mme_outgoing_tcp_socket();
	// reopen it
	make_mme_outgoing_tcp_socket();
}



// #########################################################################################################
static void make_paws_listener_socket(void)
{
	if (!g_cfg.settings.use_passthrough_ctl)
		return;

	if (g_paws_app_listener_sock == -1)
	{
		g_paws_app_listener_sock = make_listener_tcp_socket(g_paws_app_listener_port);
		if (g_paws_app_listener_sock != -1)
		{
			FD_SET(g_paws_app_listener_sock, &g_active_sock_set);
			LOG_PRINT(LOG_NOTICE, "Created PAWS listener socket [port=%d]", g_paws_app_listener_port);
		}
	}
}


// #########################################################################################################
static void make_enodeb_listener_sctp_socket(void)
{
	if (g_enodeb_listener_sock == -1)
	{
		// if mme is up, and ready, and tunnel is active
		bool tunnel_up = (!g_cfg.settings.use_passthrough_ctl) ? true : ((g_paws_app_sock != -1) && (g_tunnel_end_time > time(NULL)));
		if ((g_mme_sock != -1) && (g_mme_ready) && tunnel_up)
		{
			g_enodeb_listener_sock = make_listener_sctp_socket(g_enodeb_listener_port);
			if (g_enodeb_listener_sock != -1)
			{
				FD_SET(g_enodeb_listener_sock, &g_active_sock_set);
				LOG_PRINT(LOG_NOTICE, "Created eNode listener socket [port=%d]", g_enodeb_listener_port);
			}
		}
	}
}


// #########################################################################################################
static void close_enodeb_listener_sctp_socket(void)
{
	if (g_enodeb_listener_sock != -1)
	{
		close_socket(&g_enodeb_listener_sock);
	}
}


// #########################################################################################################
static void close_enodeb_sctp_socket(void)
{
	if (g_enodeb_sock != -1)
	{
		close_socket(&g_enodeb_sock);
	}
}


// #########################################################################################################
static void close_enodeb_sctp_sockets(void)
{
	if (g_enodeb_listener_sock != -1)
	{
		LOG_PRINT(LOG_NOTICE, "Closing all eNode sockets");
		close_enodeb_listener_sctp_socket();
		close_enodeb_sctp_socket();
	}
}


// #########################################################################################################
static int32_t accept_connection(int32_t listener_sock, const char* sock_name )
{
	LOG_PRINT(LOG_FUNC, " ");
	
	struct sockaddr_in remote_address;
    socklen_t sock_len = sizeof(remote_address);
	int32_t new_sock = accept(listener_sock, (struct sockaddr *)&remote_address, &sock_len);
    if (new_sock < 0)
	{
		LOG_PRINT(LOG_ERROR, "accept [%s]", sock_name);
		goto error_hdl;
	}

    char remote_ip_addr_str[INET_ADDRSTRLEN];
    const char *ret = inet_ntop(AF_INET, (const void*)&remote_address.sin_addr, remote_ip_addr_str, INET_ADDRSTRLEN); 
	if (!ret)
	{
		LOG_PRINT(LOG_ERROR, "inet_ntop [%s]", sock_name);
		goto error_hdl;
	}

	LOG_PRINT(LOG_NOTICE, "[%s] Accepted connection from %s", sock_name, remote_ip_addr_str);

    return new_sock;

error_hdl:
	if (new_sock != -1) close(new_sock);
	return -1;
}


// #########################################################################################################
static bool control_plane_enabled(bool *changed)
{
	LOG_PRINT(LOG_FUNC, " ");
	static bool enabled = false;

	bool tunnel_up = (!g_cfg.settings.use_passthrough_ctl) ? true : ((g_paws_app_sock != -1) && (g_tunnel_end_time > time(NULL)));
	if ((g_mme_sock != -1) && (g_enodeb_sock != -1) && tunnel_up)
	{
		*changed = (enabled == false);
		enabled = true;
		
	}
	else
	{
		*changed = (enabled == true);
		enabled = false;
	}

	return enabled;
}



// #########################################################################################################
static uint8_t* read_sock_or_close(int32_t *sock, int32_t *num_bytes_read)
{
	LOG_PRINT(LOG_FUNC, " ");

	// Read it.
    static uint8_t buffer[1550];
	int32_t nbytes = read(*sock, buffer, sizeof(buffer));
	if (nbytes <= 0)
	{
        // Socket has been closed from the remote end.
		close_socket(sock);
    }

    *num_bytes_read = nbytes;

    return buffer;
}


// #########################################################################################################
static bool send_sock_or_close(int32_t *sock, uint8_t* buf, uint32_t buflen)
{
	LOG_PRINT(LOG_FUNC, " ");

	if (*sock == -1)
		return false;

	ssize_t num_sent = send(*sock, buf, buflen, MSG_DONTWAIT | MSG_NOSIGNAL);
	if (num_sent != (ssize_t)buflen)
	{
		// Socket has been closed from the remote end.
		close_socket(sock); 
		return false;
	}

	return true;
}



// #########################################################################################################
static void mme_connected_sctp(void)
{
	char* mme_addr = mme_address_list_get_curr();
	LOG_PRINT(LOG_NOTICE, "MME connected [addr=%s port=%d] Status=[%s] ", mme_addr, g_cfg.socket_info.mme_sctp_port, get_status_str());
	LOG_EVENT("MME connected");
	FD_SET(g_mme_sock, &g_active_sock_set);
	make_enodeb_listener_sctp_socket();
}



// #########################################################################################################
static void mme_disconnected_sctp(void)
{
	LOG_PRINT(LOG_ERROR, "Lost connection to MME.  Status=[%s]", get_status_str());
	LOG_EVENT("MME lost");
	disconnect_enb_sockets();
}


// #########################################################################################################
static void mme_connected_tcp(void)
{
	char* mme_addr = mme_address_list_get_curr();
	LOG_PRINT(LOG_NOTICE, "MME connected [addr=%s port=%d] Status=[%s] ", mme_addr, g_cfg.socket_info.mme_sctp_port, get_status_str());
	LOG_EVENT("MME connected");
	FD_SET(g_mme_sock, &g_active_sock_set);
	tcps1_mme_connected();
}



// #########################################################################################################
static void mme_disconnected_tcp(void)
{
	LOG_PRINT(LOG_ERROR, "Lost connection to MME.  Status=[%s]", get_status_str());
	LOG_EVENT("MME lost");
	g_mme_lost_time = time(NULL);
	g_mme_swap_time = time(NULL);
	close_socket(&g_mme_sock);			// do this for safety, but shouldnt be necessary
	tcps1_mme_disconnected();
}


// #########################################################################################################
static void enb_connected_sctp(void)
{
	FD_SET(g_enodeb_sock, &g_active_sock_set);
	LOG_PRINT(LOG_NOTICE, "eNB connected. Status=[%s]", get_status_str());
	LOG_EVENT("eNB connected");

	lte_reboot_if_needed();
}

// #########################################################################################################
static void enb_disconnected_sctp(void)
{
	LOG_PRINT(LOG_ERROR, "Lost connection to eNodeB. Status=[%s]", get_status_str());
	LOG_EVENT("eNB lost");
}

// #########################################################################################################
static void enb_connected_tcp(void)
{
	FD_SET(g_enodeb_sock, &g_active_sock_set);
	LOG_PRINT(LOG_NOTICE, "eNB connected. Status=[%s]", get_status_str());
	LOG_EVENT("eNB connected");
	
	lte_reboot_if_needed();

	if (g_mme_sock != -1)
		tcps1_reset();			// if MME is up, do a reset immediately
	else
		tcps1_trigger_reset();	// if MME is down, trigger a reset to occur when MME socket comes up
}

// #########################################################################################################
static void enb_disconnected_tcp(void)
{
	LOG_PRINT(LOG_ERROR, "Lost connection to eNodeB. Status=[%s]", get_status_str());
	LOG_EVENT("eNB lost");
	close_socket(&g_enodeb_sock);		// do this for safety

	if (g_mme_sock != -1)
		tcps1_reset();			// if MME is up, do a reset immediately
	else
		tcps1_trigger_reset();	// if MME is down, trigger a reset to occur when MME socket comes up
}

// #########################################################################################################
static void mme_ready(void)
{
	g_mme_ready = true;
	make_enodeb_listener_sctp_socket();
}



// #########################################################################################################
static void disconnect_enb_sockets(void)
{
	if ((g_enodeb_listener_sock != -1) || (g_enodeb_sock != -1))
	{
		close_enodeb_sctp_sockets();
		enb_disconnected_f();
	}
}


// #########################################################################################################
static void process_ul_msg(uint8_t* payload_, uint32_t payload_len)
{
	passthru_to_mme(payload_, payload_len);
}


// #########################################################################################################
static void process_ul_msg_tcps1(uint8_t* payload_, uint32_t payload_len)
{
	tcps1_process_ul(payload_, payload_len);
}


// #########################################################################################################
static void process_dl_msg(uint8_t* payload_, uint32_t payload_len)
{
	passthru_to_enb(payload_, payload_len);
}


// #########################################################################################################
static void process_dl_msg_tcps1(uint8_t* payload_, uint32_t payload_len)
{
	tcps1_process_dl(payload_, payload_len);
}


// #########################################################################################################
static void passthru_to_enb(uint8_t* payload_, uint32_t payload_len)
{
	bool status_changed = false;
	if (control_plane_enabled(&status_changed))
	{
		LOG_PRINT(LOG_DEBUG, "sending %d bytes: %s to enb", payload_len, payload_);
		if (!(send_sock_or_close(&g_enodeb_sock, payload_, payload_len)))
		{
			enb_disconnected_f();
		}
	}
}


// #########################################################################################################
static void send_to_mme(uint8_t* payload_, uint32_t payload_len)
{
	LOG_PRINT(LOG_DEBUG, "sending %d bytes: %s to mme\n", payload_len, payload_);
	if (!(send_sock_or_close(&g_mme_sock, payload_, payload_len)))
	{
		mme_disconnected_f();
	}
}


// #########################################################################################################
static void passthru_to_mme(uint8_t* payload_, uint32_t payload_len)
{
	bool status_changed = false;
	if (control_plane_enabled(&status_changed))
	{
		LOG_PRINT(LOG_DEBUG, "sending %d bytes: %s to mme\n", payload_len, payload_);
		if (!(send_sock_or_close(&g_mme_sock, payload_, payload_len)))
		{
			mme_disconnected_f();
		}
	}
}



// #########################################################################################################
static void handle_enodeb_sock(void)
{
	LOG_PRINT(LOG_FUNC, " ");
	
	if (g_enodeb_sock != -1)
	{
		int32_t nbytes = 0;
		uint8_t* buf = read_sock_or_close(&g_enodeb_sock, &nbytes);

		LOG_PRINT(LOG_DEBUG, "%d bytes received", nbytes);
		if (nbytes > 0)
		{
			process_ul_msg_f(buf, nbytes);
		}
		else
		{
			enb_disconnected_f();
		}
	}
}


// #########################################################################################################
static void handle_mme_sock(void)
{
	LOG_PRINT(LOG_FUNC, " ");

	int32_t nbytes = 0;
	uint8_t* buf = read_sock_or_close(&g_mme_sock, &nbytes);

	LOG_PRINT(LOG_DEBUG, "%d bytes received", nbytes);
	if (nbytes > 0) 
	{
		process_dl_msg_f(buf, nbytes);
    }
	else if (g_mme_sock == -1)			// was it closed
	{
		mme_disconnected_f();
	}
}


// #########################################################################################################
static void handle_paws_app_sock(void)
{
	LOG_PRINT(LOG_FUNC, " ");
	
	if (!g_cfg.settings.use_passthrough_ctl)
		return;

	int32_t nbytes = 0;
	uint8_t* buf = read_sock_or_close(&g_paws_app_sock, &nbytes);

    if (nbytes > 0)
	{
		// The data the PAWS app sends us is always a single byte, that is 
		// either a 0 or 1. We expect to receive one byte every minute or
		// so. If we've received more than one byte, that would be surprising
		// but it is fine, we can just ignore everything other than the last
		// byte.
		char c = buf[nbytes - 1];
		if (c == 0) 
		{
			g_tunnel_end_time = 0;
			LOG_PRINT(LOG_NOTICE, "PAWS app disabled tunnel. Status=[%s]", get_status_str());
			LOG_EVENT("tunnel down");
			enb_disconnected_f();
		}
		else if (c == 1) 
		{
			g_tunnel_end_time = time(NULL) + g_passthrough_ctl_periodicity;
			LOG_PRINT(LOG_NOTICE, "PAWS app enabled tunnel. Status=[%s]", get_status_str());
			LOG_EVENT("tunnel up");
			make_enodeb_listener_sctp_socket();
		}
		else
		{
			LOG_PRINT(LOG_ERROR, "PAWS sent invalid byte of 0x%02x\n", c);
		}
	}
	else
	{
		g_tunnel_end_time = 0;
		LOG_PRINT(LOG_NOTICE, "PAWS app disabled tunnel. Status=[%s]", get_status_str());
		LOG_EVENT("tunnel down");
		enb_disconnected_f();
	}
}


// #########################################################################################################
static void handle_paws_app_listener_sock(void)
{
	LOG_PRINT(LOG_FUNC, " ");

	if (!g_cfg.settings.use_passthrough_ctl)
		return;

    g_paws_app_sock = accept_connection(g_paws_app_listener_sock, "paws_listener");
	if (g_paws_app_sock != -1)
	{
		FD_SET(g_paws_app_sock, &g_active_sock_set);
		LOG_PRINT(LOG_NOTICE, "PAWS connected. Status=[%s]", get_status_str());
		LOG_EVENT("PAWS connected");
	}
}


// #########################################################################################################
static void handle_enodeb_listener_sock(void)
{
	LOG_PRINT(LOG_FUNC, " ");

	g_enodeb_sock = accept_connection(g_enodeb_listener_sock, "end_listener");
	if (g_enodeb_sock != -1)
	{
		enb_connected_f();
	}
}



// #########################################################################################################
static char* get_status_str(void)
{
	static char status_str[100];
	if (!g_cfg.settings.use_passthrough_ctl)
		sprintf(status_str, "MME=%d eNB=%d UsingPaws:0", (g_mme_sock != -1), (g_enodeb_sock != -1));
	else
		sprintf(status_str, "MME=%d eNB=%d UsingPaws:1 PAWS=%d Tunnel=%d", (g_mme_sock != -1), (g_enodeb_sock != -1), (g_paws_app_sock != -1), (g_tunnel_end_time != 0));
	g_status_time = time(NULL);;	
	return status_str;
}

static char* get_status_json_str(void)
{
	static char status_str[100];
	if (!g_cfg.settings.use_passthrough_ctl)
		sprintf(status_str, "\"enb_connected\":%d, \"mme_connected\":%d, \"using_paws\":0, \"paws_connected\":%d, \"tunnel_up\":%d", (g_enodeb_sock != -1), (g_mme_sock != -1), (g_paws_app_sock != -1), (g_tunnel_end_time != 0));
	else
		sprintf(status_str, "\"enb_connected\":%d, \"mme_connected\":%d, \"using_paws\":1, \"paws_connected\":%d, \"tunnel_up\":%d", (g_enodeb_sock != -1), (g_mme_sock != -1), (g_paws_app_sock != -1), (g_tunnel_end_time != 0));
	return status_str;
}


// #########################################################################################################
static void show_status(void)
{
	time_t now_ = time(NULL);
	if (now_ > g_status_time + SHOW_STATUS_PERIODICITY)
	{
		char status_str[110];
		sprintf(status_str, "Status=[%s]", get_status_str());
		LOG_PRINT(LOG_INFO, "%s", status_str);
		LOG_EVENT("Status");
	}
}


// #########################################################################################################
static void timer_handler(int signo)
{
	(void)signo;
}

// #########################################################################################################
void sctp_agent_run(sctp_agent_cfg_t* cfg)
{
	if (!cfg)
	{
		sctp_fatal_error("SCTP-Agent:  cfg is NULL")
		return;
	}

	memcpy(&g_cfg, cfg, sizeof(sctp_agent_cfg_t));

	// create logger
	if (strlen(cfg->log_info.logname))
	{
		if (!(g_logger = logger_create(&cfg->log_info)))		
		{
			sctp_fatal_error("SCTP-Agent:  Unable to create logger")
			return; 
		}
	}

	// create cloud logger
	if (strlen(cfg->cloud_log_info.cloud_addr))
	{
		if (!(g_cloud_logger = cloud_logger_create(&cfg->cloud_log_info)))		
		{
			sctp_fatal_error("SCTP-Agent:  Unable to create cloud logger")
			return;
		}
	}

	// init times
	g_mme_lost_time = time(NULL);
	g_mme_swap_time = time(NULL);

	// Do configs for tcps1
	if (cfg->socket_info.mme_conn_type == CONN_TCP)
	{
		make_mme_outgoing_socket_f = make_mme_outgoing_tcp_socket;
		mme_connected_f = mme_connected_tcp;
		mme_disconnected_f = mme_disconnected_tcp;
		enb_connected_f = enb_connected_tcp;
		enb_disconnected_f = enb_disconnected_tcp;
		process_ul_msg_f = process_ul_msg_tcps1;
		process_dl_msg_f = process_dl_msg_tcps1;
		timer_expiry_f = tcps1_process_timer_expiry;
		g_mme_ready = false;
		tcps1_config(mme_ready, passthru_to_enb, passthru_to_mme, send_to_mme, disconnect_enb_sockets, drop_mme_outgoing_tcp_socket, reset_mme_outgoing_tcp_socket, g_logger, g_cloud_logger, gDeviceName, gDeviceId, 
			cfg->settings.azure_mme_pkt_retx_guard_ms, cfg->settings.azure_mme_pkt_retx_max_attempts);
	}

	// Initialize the set of active sockets.
	FD_ZERO(&g_active_sock_set);

	// make MME socket
	make_mme_outgoing_socket_f();

	// store the config info
	g_passthrough_ctl_periodicity = cfg->settings.passthrough_ctl_periodicity + PAWS_TUNNEL_PERIODIC_TOLERANCE;
	g_paws_app_listener_port = cfg->socket_info.passthrough_ctl_port;
	g_enodeb_listener_port = cfg->socket_info.enb_sctp_port;

	// create the PAWS socket
	make_paws_listener_socket();

	// block SIGALRM so that we can use a linux timer
	// the following code has the effect that the timer is only triggered within the select i.e. if the SIGALRM is triggered whilst other code is being processed 
	// it is delayed until the select is executed.
	struct sigaction s;
	s.sa_handler = timer_handler;
	sigemptyset(&s.sa_mask);
	s.sa_flags = 0;
	sigaction(SIGALRM, &s, NULL);
	// Block SIGTERM.
	sigset_t sigset, oldset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigprocmask(SIG_BLOCK, &sigset, &oldset);

	struct timespec timeout; 
	timeout.tv_sec = cfg->settings.azure_mme_socket_retry_s;
	timeout.tv_nsec = 0;

	// loop and handle sockets
    while (1) {
        // Block until a socket does something.   
        fd_set read_fd_set = g_active_sock_set;

		int32_t ret = pselect(FD_SETSIZE, &read_fd_set, NULL, NULL, &timeout, &oldset);
		if (ret > 0)
		{
			// something was received on one of the socks
			// Loop through all the sockets we've registered...
			for (int32_t i = 0; i < FD_SETSIZE; ++i)
			{
				// Continue unless this socket has got something for us.
				if (FD_ISSET(i, &read_fd_set))
				{
					if (i == g_enodeb_sock)
						handle_enodeb_sock();
					else if (i == g_mme_sock)
						handle_mme_sock();
					else if (i == g_paws_app_sock)
						handle_paws_app_sock();
					else if (i == g_paws_app_listener_sock)
						handle_paws_app_listener_sock();
					else if (i == g_enodeb_listener_sock)
						handle_enodeb_listener_sock();
					else
					{
						close_all_sockets();
						sctp_fatal_error("select() Unknown socket (%d)", i);
						break;
					}
				}
			}
		}
		else if (ret == 0)
		{
			// timeout
			LOG_PRINT(LOG_DEBUG, "timeout");
		}
		else // (ret < 0)
		{
			if (errno == EINTR)
			{
				if (timer_expiry_f)
				{
					timer_expiry_f();
				}
			}
			else
			{
				// error 
				close_all_sockets();
				sctp_fatal_error("select() error");
				break;
			}
		}

		// if lost, recreate mme socket
		if (g_mme_sock == -1)
		{
			make_mme_outgoing_socket_f();
		}

		// show status
		show_status();
    }

	// free up resoures
	close_all_sockets();
	if (cfg->socket_info.mme_conn_type == CONN_TCP)
		tcps1_free();
	if (g_logger)
		logger_free(&g_logger);
	if (g_cloud_logger)
		cloud_logger_free(&g_cloud_logger);

	sleep(1);

}



