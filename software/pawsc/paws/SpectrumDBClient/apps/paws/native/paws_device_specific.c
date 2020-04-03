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
	printf("%s NATIVE \n", __func__);
}

//#######################################################################################
bool reboot_device_on_admin_disable(void)
{
	return true;	
}
