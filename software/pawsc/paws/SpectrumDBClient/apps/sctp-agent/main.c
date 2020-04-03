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
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

// Standard headers
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "device_db_info.h"
#include "lte_db_info.h"
#include "sctp_agent_db_info.h"
#include "sctp_agent.h"


#define sctp_fatal_error(msg, ...) \
{ \
    fprintf(stderr, "FATAL ERROR in %s, line %d: ", __FILE__, __LINE__); \
    printf(msg, ##__VA_ARGS__); \
    putchar('\n'); \
    exit(-1); \
}

#define sctp_fatal_perror(msg) \
{ \
    fprintf(stderr, "FATAL ERROR in %s, line %d: ", __FILE__, __LINE__); \
    perror(msg); \
    exit(-1); \
}

// Cfg
sctp_agent_cfg_t g_sctp_agent_cfg;

// globals
device_name_t gDeviceName = { 0 };
device_id_t gDeviceId = 0;


// #########################################################################################################
static void get_database_configs(void)
{
	if (!(set_sctp_agent_db_location(SCTP_AGENT_DB_LOCATION)))
		sctp_fatal_error("Cant set SCTP-AGENT-DB location");

	if (!(lte_db_set_location(LTE_DB_LOCATION)))
		sctp_fatal_error("Cant set Device-DB location");

	// loop until the SCTP settings are read correctly
	while (1)
	{
		if ((get_sctp_agent_cfg(&g_sctp_agent_cfg)))
			break;
		sleep(1);	// try in a second
	}

	// loop until the LTE-DB settings are read correctly
	while (1)
	{
		int port = -1;
		if (lte_db_get_enb_info(&port, gDeviceName, &gDeviceId))
		{
			g_sctp_agent_cfg.socket_info.enb_sctp_port = port;
			break;
		}
		sleep(1);	// try in a second
	}

	// use the same SCTP port as the destination MME SCTP port
	g_sctp_agent_cfg.socket_info.mme_sctp_port = map_mme_sctp_port(g_sctp_agent_cfg.socket_info.enb_sctp_port);
}


// #########################################################################################################
int main()
{
	// wait a while for other services to come up
	sleep(10);

	memset(&g_sctp_agent_cfg, 0, sizeof(sctp_agent_cfg_t));

	// this will loop until the databases are read
	get_database_configs();

	// it wont return from the function unless there is a failure
	sctp_agent_run(&g_sctp_agent_cfg);

	return 0;
}
