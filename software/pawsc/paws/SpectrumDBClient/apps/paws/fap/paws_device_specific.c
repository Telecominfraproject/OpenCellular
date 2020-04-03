/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "paws_device_specific.h"

//#######################################################################################
void system_reboot(void)
{
	system("reboot");
}


//#######################################################################################
bool reboot_device_on_admin_disable(void)
{
	return false;		// for the FAP, the PAWS will not reboot the FAP upon setting Admin=False.
						// Instead, status=0 will be sent to the SCTP-AGENT agent, which will indirectly cause the eNB sw to disable the radio.
}
