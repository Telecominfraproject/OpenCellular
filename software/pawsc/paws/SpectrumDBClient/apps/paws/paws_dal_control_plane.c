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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "utils/utils.h"
#include "paws_dal_control_plane.h"



typedef struct {
	control_plane_cfg_t cfg;
	int sock;
} control_plane_t;






//#######################################################################################
static int make_outgoing_socket(const char *ip_addr_str, unsigned port)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		goto error_hdl;
	}

	struct sockaddr_in ip_addr = { 0 };
	ip_addr.sin_family = AF_INET;
	ip_addr.sin_port = htons(port);
	if (inet_pton(AF_INET, ip_addr_str, &(ip_addr.sin_addr)) < 0)
	{
		goto error_hdl;
	}

	if (connect(sock, &ip_addr, sizeof(ip_addr)) < 0)
	{
		goto error_hdl;
	}

	return sock;

error_hdl:
	if (sock >= 0) close(sock);
	return -1;
}


//#######################################################################################
void* paws_dal_control_plane_create(control_plane_cfg_t* cfg)
{
	control_plane_t* control_plane = NULL;

	if (!cfg)
	{
		return NULL;
	}

	if (((strlen(cfg->status_ipaddr) == 0)) || (cfg->status_port == 0))
	{
		return NULL;
	}

	// malloc a new control_plane_t
	if (!(control_plane = malloc(sizeof(control_plane_t))))
	{
		return NULL;
	}
	memset(control_plane, 0, sizeof(control_plane_t));

	// copy the config
	memcpy(&control_plane->cfg, cfg, sizeof(control_plane_cfg_t));

	// open the socket
	control_plane->sock = -1;

	// if this fails, still return control_plane, and attempt to open it when we send data,
	// This way we can start PAWS before control-plane-agent without any catastrophies
	control_plane->sock = make_outgoing_socket(cfg->status_ipaddr, cfg->status_port);

	return (void*)control_plane;
}



//#######################################################################################
void paws_dal_control_plane_free(void** control_plane_)
{
	if (!((control_plane_) && (*control_plane_)))
		return;

	control_plane_t** control_plane = (control_plane_t**)control_plane_;

	// free the socket
	if ((*control_plane)->sock >= 0)
	{
		close((*control_plane)->sock);
		(*control_plane)->sock = -1;
	}

	// free the struct
	free_and_null(control_plane_);
}


//#######################################################################################
bool paws_dal_control_plane_send_status(void* control_plane_, bool status)
{
	if (!control_plane_)
	{
		return false;
	}

	control_plane_t* control_plane = (control_plane_t*)control_plane_;

	if (((strlen(control_plane->cfg.status_ipaddr) == 0)) || (control_plane->cfg.status_port == 0))
	{
		return false;
	}

	if (control_plane->sock == -1)
	{ 
		// open the socket
		if ((control_plane->sock = make_outgoing_socket(control_plane->cfg.status_ipaddr, control_plane->cfg.status_port)) == -1)
		{
			return false;
		}
	}
		
	// try to send a packet
	size_t num_to_send = sizeof(status);
	ssize_t num_sent = send(control_plane->sock, &status, num_to_send, MSG_DONTWAIT | MSG_NOSIGNAL);
	if (num_sent != (ssize_t)num_to_send)
	{
		// try again

		// delete and recreate the socket
		if (control_plane->sock >= 0)
		{
			close(control_plane->sock);
			control_plane->sock = -1;
		}
		if ((control_plane->sock = make_outgoing_socket(control_plane->cfg.status_ipaddr, control_plane->cfg.status_port)) == -1)
		{
			return false;
		}
		
		// re-send the data
		num_sent = send(control_plane->sock, &status, num_to_send, MSG_DONTWAIT | MSG_NOSIGNAL);
		if (num_sent != (ssize_t)num_to_send)
		{
			return false;
		}
	}
		
	return true;
}


