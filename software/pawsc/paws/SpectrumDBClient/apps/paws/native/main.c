/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "paws_globals.h"

#include "lte_db_info.h"
#include "paws_db_info.h"
#include "sctp_agent_db_info.h"
#include "paws_common.h"
#include "paws_combiner_sm.h"

#include "utils/types.h"
#include "utils/utils.h"


int main()
{
	paws_combiner_sm_t* comb = NULL;
	uint32_t sctp_agent_port;
	uint32_t sctp_agent_status_perdiocity;

	if (!(set_paws_db_location("/media/sf_4Gclient/apps/paws/sql_db/paws_native.db")))
		fatal_error("Cant set PAWS-DB location");

	if (!(set_sctp_agent_db_location("/media/sf_4Gclient/apps/sctp-agent/sql_db/sctp_agent_native.db")))
		fatal_error("Cant set SCTP-AGENT-DB location");

	if (!(set_lte_db_location("/media/sf_4Gclient/apps/paws/sql_db/femto.db")))
		fatal_error("Cant set Device-DB location");

	// create the ssl context.
	if (!(https_init()))
		fatal_error("Problem in https_init");

	// get the CloudLog settings
	get_gPawsLoggerSettings();

	// get the DeviceId
	populate_gDeviceName();

	// loop until the SCTP settings are read correctly
	while (1)
	{
		if ((get_sctp_agent_info(&sctp_agent_port, &sctp_agent_status_perdiocity)))
			break;
		sleep(1);	// try in a second
	}

	if (!(comb = paws_combiner_sm_create("combiner", sctp_agent_port, sctp_agent_status_perdiocity)))
		fatal_error("Cant initialise sytem");

	PUBLIC_FUNC(comb, Run)(comb);

	paws_combiner_sm_free(&comb);

	PawsGlobalsFree();

	// free ssl txt.
	https_free();

	return 0;
}

