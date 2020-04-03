/*
Copyright(c) Microsoft Corporation.All rights reserved.
Licensed under the MIT License.
*/

#ifndef MME_ADDRESS_INFO_H_
#define MME_ADDRESS_INFO_H_

#include "utils/types.h"

typedef struct mme_address {
	host_addr_t mme_addr;
	struct mme_address *next;
} mme_address_t;

typedef struct
{
	mme_address_t *head;
} mme_address_list_t;

extern bool mme_address_list_add(mme_address_t* addr);
extern void mme_address_list_free(void);
extern char* mme_address_list_get_curr(void);
extern bool mme_address_list_goto_next(void);
extern int mme_address_list_set_head(char* in_use);
extern char* mme_address_get_head_addr(void);
extern int32_t mme_address_make_outgoing_socket(const char *addr_str, uint16_t port, int protocol);

#endif // SCTP_AGENT_DB_INFO_H_