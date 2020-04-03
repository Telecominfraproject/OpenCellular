/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <stdio.h>

#include <sqlite3.h>
#include "sctp_agent_db_info.h"

#define MAX_SCTP_AGENT_DB_NAME	(100)
static char g_sctp_agent_db[MAX_SCTP_AGENT_DB_NAME] = "\0";



//#######################################################################################
bool set_sctp_agent_db_location(char* db)
{
	if (!db)
		return false;

	int slen = snprintf(g_sctp_agent_db, MAX_SCTP_AGENT_DB_NAME, "%s", db);
	if ((slen <= 0) || (slen >= MAX_SCTP_AGENT_DB_NAME))
		return false;

	return true;
}



//#######################################################################################
char* get_sctp_agent_db_location(void)
{
	return &g_sctp_agent_db[0];
}



//#######################################################################################
bool get_sctp_agent_info(uint32_t* status_port, uint32_t* status_periodicity)
{
	sqlite3 *sql_hdl = NULL;
	sqlite3_stmt *stmt = NULL;

	int port = -1;
	int periodicity = -1;

	// get datafile location and open it
	char* db_sql_file = get_sctp_agent_db_location();
	if (!db_sql_file)
	{
		goto error_hdl;
	}
	sqlite3_open(db_sql_file, &sql_hdl);
	if (!sql_hdl)
	{
		goto error_hdl;
	}

	// get the port
	const char *sql = "SELECT passthrough_ctl_port FROM SocketInfo";
	int rc = sqlite3_prepare_v2(sql_hdl, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}	
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		port = sqlite3_column_int(stmt, 0);
	}
	if (rc != SQLITE_DONE)
	{
		goto error_hdl;
	}
	sqlite3_finalize(stmt); stmt = NULL;
	if (port == -1)
	{
		goto error_hdl;
	}


	// get the paws status periodicity i.e. how often the status must be sent to the sctp-agent
	const char *sql2 = "SELECT passthrough_ctl_periodicity FROM Settings";
	rc = sqlite3_prepare_v2(sql_hdl, sql2, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		periodicity = sqlite3_column_int(stmt, 0);
	}
	if (rc != SQLITE_DONE)
	{
		goto error_hdl;
	}
	sqlite3_finalize(stmt); stmt = NULL;
	if (periodicity == -1)
	{
		goto error_hdl;
	}

	// release database
	if (sql_hdl) sqlite3_close(sql_hdl);

	*status_port = port;
	*status_periodicity = periodicity;

	return true;

error_hdl:
	if (sql_hdl) sqlite3_close(sql_hdl);
	if (stmt) sqlite3_finalize(stmt);
	return false;
}





