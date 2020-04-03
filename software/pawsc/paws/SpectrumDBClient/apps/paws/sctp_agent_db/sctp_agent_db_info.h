/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <stdint.h>
#include <stdbool.h>

#ifndef SCTP_AGENT_DB_INFO_H_
#define SCTP_AGENT_DB_INFO_H_

extern bool set_sctp_agent_db_location(char* db);

extern char* get_sctp_agent_db_location(void);

extern bool get_sctp_agent_info(uint32_t* port, uint32_t* status_periodicity);


#endif // SCTP_AGENT_DB_INFO_H_