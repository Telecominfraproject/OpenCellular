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


// ##############################################################
uint16_t map_mme_sctp_port(uint16_t enb_sctp_port)
{
	// use the same port
	return enb_sctp_port;

//	return enb_sctp_port + 1;			// Use a different port if testing on the local VirtualBox VM. 
										// This allows a test mme to be run on the VM at the same time
}


