/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <stdio.h>


#include "paws_db_info.h"

#define MAX_PAWS_DB_NAME	(100)
static char g_paws_db[MAX_PAWS_DB_NAME] = "\0";


//#######################################################################################
bool set_paws_db_location(char* db)
{
	if (!db)
		return false;

	int slen = snprintf(g_paws_db, MAX_PAWS_DB_NAME, "%s", db);
	if ((slen <= 0) || (slen >= MAX_PAWS_DB_NAME))
		return false;

	return true;
}



//#######################################################################################
char* get_paws_db_location(void)
{
	return &g_paws_db[0];
}

