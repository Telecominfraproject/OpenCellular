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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#include "mme_address.h"
#include "sctp_globals.h"
#include "sctp_agent_db_info.h"


// MME address info
static mme_address_list_t* mme_address_list = NULL;			// address list
// current MME DNS info
static struct addrinfo *mme_servinfo = NULL;
static bool dns_triggered = false;			// do DNS next time we try and open socket



static bool mme_address_dns_lookup(int protocol, int port)
{
	char* mme_addr = NULL;

	// release current servinfo
	if (mme_servinfo)
	{
		freeaddrinfo(mme_servinfo);
		mme_servinfo = NULL;
	}

	if (!(mme_addr = mme_address_get_head_addr()))
		goto error_hdl;

	LOG_PRINT(LOG_NOTICE, "Doing dns lookup for [addr=%s]", mme_addr);

	// init the hints
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = protocol;

	// convert port to a string
	char snum[7];
	snprintf(snum, sizeof(snum), "%d", port);

	if ((getaddrinfo(mme_addr, snum, &hints, &mme_servinfo)) != 0)
	{
		LOG_PRINT(LOG_ERROR, "Failure from getaddrinfo [addr=%s]", mme_addr);
		goto error_hdl;
	}

	LOG_PRINT(LOG_NOTICE, "Successful dns-lookup for [addr=%s]", mme_addr);

	return true;

error_hdl:
	if (mme_servinfo) 
		freeaddrinfo(mme_servinfo);
	mme_servinfo = NULL;
	return false;
}



//#######################################################################################
// add as circular list
bool mme_address_list_add(mme_address_t* addr)
{
	if (!addr)
		return false;

	if (!mme_address_list)
	{
		if (!(mme_address_list = calloc(1, sizeof(mme_address_list_t))))
			return false;
	}

	mme_address_t* tail = NULL;

	if (mme_address_list->head == NULL)
	{
		mme_address_list->head = addr;
	}
	else
	{
		tail = mme_address_list->head;
		while (tail->next != mme_address_list->head)
			tail = tail->next;
		tail->next = addr;
	}
	addr->next = mme_address_list->head;

	return true;
}


//#######################################################################################
void mme_address_list_free(void)
{
	if (!mme_address_list)
		return;

	mme_address_t *head = mme_address_list->head;
	mme_address_t *curr = mme_address_list->head;
	if (!curr)
		return;		// nothing in list

	while (mme_address_list->head != head)
	{
		curr = mme_address_list->head;
		mme_address_list->head = curr->next;
		free(curr);
	}
	mme_address_list->head = NULL;
}



//#######################################################################################
char* mme_address_list_get_curr(void)
{
	if ((!mme_address_list) || (!mme_address_list->head))
		return NULL;
	mme_address_t* head = mme_address_list->head;
	return head->mme_addr;
}


//#######################################################################################
// returns True if address actually changes
bool mme_address_list_goto_next(void)
{
	bool ret = false;
	if (!mme_address_list)
		return ret;

	if (mme_address_list->head)
	{
		ret = (mme_address_list->head != mme_address_list->head->next);
		mme_address_list->head = mme_address_list->head->next;
		if (ret)
			mme_address_write_head_to_db(mme_address_list->head->mme_addr);
	}

	// trigger DNS
	dns_triggered = true;

	return ret;
}




//#######################################################################################
// Set the head to the address in use.  Retunr TRUE is present
int mme_address_list_set_head(char* in_use)
{
	if (!mme_address_list)
		return 0;

	if ((!in_use) || (strlen(in_use) == 0))
		return 0;

	mme_address_t *head = mme_address_list->head;
	mme_address_t *curr = mme_address_list->head;
	if (!curr)
		return 0;		// nothing in list

    // move to position after head, and then walk until we're back to head
	curr = curr->next;
	while (curr != head)
	{
		if (strcmp(in_use, curr->mme_addr) == 0)
		{
			mme_address_list->head = curr;
			break;
		}
		curr = curr->next;
	}

	mme_address_write_head_to_db(mme_address_list->head->mme_addr);
	return 1;

}


//#######################################################################################
char* mme_address_get_head_addr()
{
	if ((!mme_address_list) || (!mme_address_list->head) || (!mme_address_list->head->mme_addr) || (strlen(mme_address_list->head->mme_addr) == 0))
	{
		LOG_PRINT(LOG_ERROR, "No MME address head defined\n");
		return NULL;
	}
	return mme_address_list->head->mme_addr;
}


// #########################################################################################################
int32_t mme_address_make_outgoing_socket(const char *addr_str, uint16_t port, int protocol)
{
	LOG_PRINT(LOG_FUNC, "addr=%s port=%d\n", addr_str, port);

	int32_t sockfd = -1;

	// do DNS if not done yet
	if ((!mme_servinfo) || (dns_triggered))
	{
		dns_triggered = false;
		if (!mme_address_dns_lookup(protocol, port))
			goto error_hdl;
	}

	// loop through all the results and connect to the first we can
	struct addrinfo *p = NULL;
	for (p = mme_servinfo; p != NULL; p = p->ai_next)
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
			sockfd = -1;
			continue;
		}

		break; // if we get here, we must have connected successfully
	}

	if (p == NULL)
	{
		// looped off the end of the list with no connection
		goto error_hdl;
	}

	return sockfd;

error_hdl:
	return -1;
}
