/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <stdio.h>

#include "lte_db_info.h"

#define MAX_LTE_DB_NAME	(100)
static char g_lte_db[MAX_LTE_DB_NAME] = "\0";


//#######################################################################################
bool set_lte_db_location(char* db)
{
	if (!db)
		return false;

	int slen = snprintf(g_lte_db, MAX_LTE_DB_NAME, "%s", db);
	if ((slen <= 0) || (slen >= MAX_LTE_DB_NAME))
		return false;

	return true;
}



//#######################################################################################
char* get_lte_db_location(void)
{
	return &g_lte_db[0];
}

