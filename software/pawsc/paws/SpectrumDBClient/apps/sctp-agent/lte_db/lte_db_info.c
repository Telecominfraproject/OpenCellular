/*
Copyright(c) Microsoft Corporation.All rights reserved.
Licensed under the MIT License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#include <sqlite3.h>

#include "lte_db_info.h"

#define MAX_LTE_DB_NAME	(100)
static char g_lte_db[MAX_LTE_DB_NAME] = "\0";


//#######################################################################################
bool lte_db_set_location(char* db)
{
	if (!db)
		return false;

	int slen = snprintf(g_lte_db, MAX_LTE_DB_NAME, "%s", db);
	if ((slen <= 0) || (slen >= MAX_LTE_DB_NAME))
		return false;

	return true;
}


//#######################################################################################
char* lte_db_get_location(void)
{
	return &g_lte_db[0];
}


//#######################################################################################
static bool get_device_name(device_name_t device_name)
{
	sqlite3 *sql_hdl = NULL;
	sqlite3_stmt *stmt = NULL;
	int rc;

	memset(device_name, 0, sizeof(device_name_t));

	// get datafile location and open it
	char* db_sql_file = lte_db_get_location();
	if (!db_sql_file)
	{
		goto error_hdl;
	}
	sqlite3_open(db_sql_file, &sql_hdl);
	if (!sql_hdl)
	{
		goto error_hdl;
	}

	// get the DeviceId.  We use FAPServiceAccessMgmtLTE.HNBName
	const char *sql = "SELECT HNBName FROM FAPServiceAccessMgmtLTE";
	rc = sqlite3_prepare_v2(sql_hdl, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		char* tmp = (char*)sqlite3_column_text(stmt, 0);
		if (tmp)
		{
			snprintf(device_name, sizeof(device_name_t), "%s", tmp);
		}
	}
	if (rc != SQLITE_DONE)
	{
		goto error_hdl;
	}
	sqlite3_finalize(stmt); stmt = NULL;

	// release database
	if (sql_hdl) sqlite3_close(sql_hdl);

	return true;

error_hdl:
	if (sql_hdl) sqlite3_close(sql_hdl);
	if (stmt) sqlite3_finalize(stmt);
	return false;
}



//#######################################################################################
static bool get_device_id(device_id_t* device_id)
{
	sqlite3 *sql_hdl = NULL;
	sqlite3_stmt *stmt = NULL;
	int rc;

	*device_id = 0;

	// get datafile location and open it
	char* db_sql_file = lte_db_get_location();
	if (!db_sql_file)
	{
		goto error_hdl;
	}
	sqlite3_open(db_sql_file, &sql_hdl);
	if (!sql_hdl)
	{
		goto error_hdl;
	}

	// get the DeviceId.  We use FAPServiceAccessMgmtLTE.HNBName
	const char *sql = "SELECT CellIdentity FROM FAPServiceCellConfigLTERANCommon";
	rc = sqlite3_prepare_v2(sql_hdl, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		*device_id  = sqlite3_column_int(stmt, 0);
	}
	if (rc != SQLITE_DONE)
	{
		goto error_hdl;
	}
	sqlite3_finalize(stmt); stmt = NULL;

	// release database
	if (sql_hdl) sqlite3_close(sql_hdl);

	return true;

error_hdl:
	if (sql_hdl) sqlite3_close(sql_hdl);
	if (stmt) sqlite3_finalize(stmt);
	return false;
}


//#######################################################################################
bool lte_db_get_enb_info(int *port, device_name_t device_name, device_id_t *device_id)
{
	sqlite3 *sql_hdl = NULL;
	sqlite3_stmt *stmt = NULL;
	*port = -1;

	if (!(get_device_name(device_name)))
	{
		goto error_hdl;
	}

	if (!(get_device_id(device_id)))
	{
		goto error_hdl;
	}

	// get datafile location and open it
	char* db_sql_file = lte_db_get_location();
	if (!db_sql_file)
	{
		goto error_hdl;
	}
	sqlite3_open(db_sql_file, &sql_hdl);
	if (!sql_hdl)
	{
		goto error_hdl;
	}

	// #################################################################################################################
	// get the sctp port used for the eNB software to connect to the SCTP database
	const char *sql = "SELECT S1SigLinkPort FROM FAPServiceFAPControlLTEGateway";
	int rc = sqlite3_prepare_v2(sql_hdl, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		*port = sqlite3_column_int(stmt, 0);
	}
	if (rc != SQLITE_DONE)
	{
		goto error_hdl;
	}
	sqlite3_finalize(stmt); stmt = NULL;

	// release database
	if (sql_hdl) sqlite3_close(sql_hdl);

	return true;

error_hdl:
	if (sql_hdl) sqlite3_close(sql_hdl);
	if (stmt) sqlite3_finalize(stmt);
	return false;
}



//#######################################################################################
bool lte_db_get_RxTxStatus(int *tx_status)
{
	sqlite3 *sql_hdl = NULL;
	sqlite3_stmt *stmt = NULL;

	// get datafile location and open it
	char* db_sql_file = lte_db_get_location();
	if (!db_sql_file)
	{
		goto error_hdl;
	}
	sqlite3_open(db_sql_file, &sql_hdl);
	if (!sql_hdl)
	{
		goto error_hdl;
	}

	// #################################################################################################################
	// get the sctp port used for the eNB software to connect to the SCTP database
	const char *sql = "SELECT RFTxStatus FROM FAPServiceFAPControlLTE";
	int rc = sqlite3_prepare_v2(sql_hdl, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		*tx_status = sqlite3_column_int(stmt, 0);
	}
	if (rc != SQLITE_DONE)
	{
		goto error_hdl;
	}
	sqlite3_finalize(stmt); stmt = NULL;

	// release database
	if (sql_hdl) sqlite3_close(sql_hdl);

	return true;

error_hdl:
	if (sql_hdl) sqlite3_close(sql_hdl);
	if (stmt) sqlite3_finalize(stmt);
	return false;
}
