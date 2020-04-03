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
#include "sctp_agent_db_info.h"

#include "mme_address.h"


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




// ########################################################################
static bool read_CloudLogInfo(sqlite3 *sql_hdl, cloud_logger_cfg_t* log_cfg)
{
	if ((!sql_hdl) || (!log_cfg))
	{
		return false;
	}
	memset(log_cfg, 0, sizeof(cloud_logger_cfg_t));

	const char *zSql = "SELECT * FROM SettingsCloudLogInfo";
	int pnRow;           /* Number of result rows written here */
	int pnColumn;       /* Number of result columns written here */
	char *pzErrmsg = NULL;
	char **pazResult = NULL;
	int rc = sqlite3_get_table(
		sql_hdl,          /* An open database */
		zSql,			/* SQL to be evaluated */
		&pazResult,    /* Results of the query */
		&pnRow,           /* Number of result rows written here */
		&pnColumn,        /* Number of result columns written here */
		&pzErrmsg       /* Error msg written here */
		);

	if (!((rc == SQLITE_OK) && (pnRow == 1) && (pnColumn == 4)))
	{
		goto error_hdl;
	}

	for (int row = 0; row < pnRow; row++)
	{
		for (int col = 0; col < pnColumn; col++)
		{
			int index = (row * pnColumn) + pnColumn + col;
			if (strcmp(pazResult[col], "level") == 0)
				log_cfg->level = atoi(pazResult[index]);
			else if (strcmp(pazResult[col], "logname") == 0)
			{
				if (pazResult[index])
					strcpy(log_cfg->logname, pazResult[index]);
			}
			else if (strcmp(pazResult[col], "port") == 0)
				log_cfg->port = atoi(pazResult[index]);
			else if (strcmp(pazResult[col], "cloud_ip") == 0)
			{
				if (pazResult[index])
					strcpy(log_cfg->cloud_addr, pazResult[index]);
			}
			else
			{
				goto error_hdl;
			}
		}
	}

	// free up resources
	if (pazResult)
		sqlite3_free_table(pazResult);

	return true;

error_hdl:
	if (pazResult) sqlite3_free_table(pazResult);
	return false;
}



// ########################################################################
static bool read_LogInfo(sqlite3 *sql_hdl, logger_cfg_t* loginfo)
{
	if ((!sql_hdl) || (!loginfo))
	{
		return false;
	}
	memset(loginfo, 0, sizeof(logger_cfg_t));

	const char *zSql = "SELECT * FROM SettingsLogInfo";
	int pnRow;           /* Number of result rows written here */
	int pnColumn;       /* Number of result columns written here */
	char *pzErrmsg = NULL;
	char **pazResult = NULL;
	int rc = sqlite3_get_table(
		sql_hdl,          /* An open database */
		zSql,			/* SQL to be evaluated */
		&pazResult,    /* Results of the query */
		&pnRow,           /* Number of result rows written here */
		&pnColumn,        /* Number of result columns written here */
		&pzErrmsg       /* Error msg written here */
		);

	if (!((rc == 0) && (pnRow == 1) && (pnColumn == 5)))
	{
		goto error_hdl;
	}

	for (int row = 0; row < pnRow; row++)
	{
		for (int col = 0; col < pnColumn; col++)
		{
			int index = (row * pnColumn) + pnColumn + col;
			if (strcmp(pazResult[col], "level") == 0)
				loginfo->level = atoi(pazResult[index]);
			else if (strcmp(pazResult[col], "logname") == 0)
				strcpy(loginfo->logname, pazResult[index]);
			else if (strcmp(pazResult[col], "file_size") == 0)
				strcpy(loginfo->size, pazResult[index]);
			else if (strcmp(pazResult[col], "max_files") == 0)
				loginfo->max = atoi(pazResult[index]);
			else if (strcmp(pazResult[col], "compress") == 0)
				loginfo->compress = (bool)(pazResult[index]);
			else
			{
				goto error_hdl;
			}
		}
	}

	// free up resources
	if (pazResult)
		sqlite3_free_table(pazResult);

	return true;

error_hdl:
	if (pazResult) sqlite3_free_table(pazResult);
	return false;
}




//#######################################################################################
void mme_address_write_head_to_db(char* mme_addr)
{
	sqlite3 *sql_hdl = NULL;
	char sql_str[200];

	if ((!mme_addr) || (strlen(mme_addr) == 0))
		return;

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

	// build SQL command
	sprintf(sql_str, "UPDATE SocketInfo set SocketInfoMmeCurrent='%s'", mme_addr);
	char* error_msg = NULL;
	int rc = sqlite3_exec(sql_hdl, sql_str, NULL, NULL, &error_msg);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}

error_hdl:
	// free up resources
	if (sql_hdl) sqlite3_close(sql_hdl);
}




//#######################################################################################
// note this does not populate "enb_sctp_port".  This parameter is read from the LTE DB
//
static bool get_mme_address_list(void)
{
	sqlite3 *sql_hdl = NULL;
	sqlite3_stmt *stmt = NULL;
	mme_address_t* mme_address = NULL;

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

	// get the MME addresses
	const char *sql = "SELECT ip_address FROM SocketInfoMmeList";
	int rc = sqlite3_prepare_v2(sql_hdl, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{ 
		if (!(mme_address = calloc(1,sizeof(mme_address_t))))
			goto error_hdl;
		char *tmp;
		tmp = (char*)sqlite3_column_text(stmt, 0);   strcpy(mme_address->mme_addr, tmp);

		// check that mme address is actually populated
		if (strlen(mme_address->mme_addr) == 0)
		{
			goto error_hdl;
		}

		// add to list
		if (!(mme_address_list_add(mme_address)))
		{
			goto error_hdl;
		}
		mme_address = NULL;
	}

	// get the Current address in use
	char in_use[200];
	in_use[0] = 0;
	const char *sql2 = "SELECT SocketInfoMmeCurrent FROM SocketInfo";
	rc = sqlite3_prepare_v2(sql_hdl, sql2, -1, &stmt, NULL);
	if (rc == SQLITE_OK)
	{
		while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
		{
			char* tmp = (char*)sqlite3_column_text(stmt, 0);
			if ((tmp) && (strlen(tmp) > 0))
			{
				strcpy(in_use, tmp);
			}
		}
	}
	
	// free up resources
	if (stmt) sqlite3_finalize(stmt);
	if (sql_hdl) sqlite3_close(sql_hdl);

	// update database
	int curr_written = 0;
	if (strlen(in_use) > 0)
	{
		curr_written = mme_address_list_set_head(in_use);
	}

	if (!curr_written)
	{
		// use head as "current"
		char *mme_addr = mme_address_get_head_addr();
		if (!mme_addr)
		{
			goto error_hdl;
		}
		mme_address_write_head_to_db(mme_addr);
	}

	return true; 

error_hdl:
	if (mme_address) free(mme_address);
	mme_address_list_free();
	if (stmt) sqlite3_finalize(stmt);
	if (sql_hdl) sqlite3_close(sql_hdl);
	return false;
}



//#######################################################################################
// note this does not populate "enb_sctp_port".  This parameter is read from the LTE DB
//
bool get_sctp_agent_cfg(sctp_agent_cfg_t* cfg)
{
	sqlite3 *sql_hdl = NULL;
	sqlite3_stmt *stmt = NULL;

	if (!cfg)
	{
		goto error_hdl;
	}

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

	// #################################################################################################################
	// get the socket info
	const char *sql = "SELECT passthrough_ctl_port, mme_connection_type FROM SocketInfo";
	int rc = sqlite3_prepare_v2(sql_hdl, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		char *tmp;
		cfg->socket_info.passthrough_ctl_port = sqlite3_column_int(stmt, 0);
		tmp = (char*)sqlite3_column_text(stmt, 1);  cfg->socket_info.mme_conn_type = (strcmp(tmp, "sctp") == 0) ? CONN_SCTP : CONN_TCP;
	}
	if (rc != SQLITE_DONE)
	{
		goto error_hdl;
	}
	sqlite3_finalize(stmt); stmt = NULL;


	// release database - release here as we might write to the dateabase in get_mme_address_list
	if (sql_hdl) sqlite3_close(sql_hdl);
	sql_hdl = NULL;
	if (!(get_mme_address_list()))
	{
		goto error_hdl;
	}

	sqlite3_open(db_sql_file, &sql_hdl);
	if (!sql_hdl)
	{
		goto error_hdl;
	}

	// #################################################################################################################
	// get the log info
	if (!(read_LogInfo(sql_hdl, &cfg->log_info)))
	{
		goto error_hdl;
	}


	// #################################################################################################################
	// get the log info
	if (!(read_CloudLogInfo(sql_hdl, &cfg->cloud_log_info)))
	{
		goto error_hdl;
	}

	// copy cloud level
	cfg->log_info.cloud_level = cfg->cloud_log_info.level;


	// #################################################################################################################
	// get the paws status periodicity i.e. how often the status must be sent to the sctp-agent
	const char *sql2 = "SELECT use_passthrough_ctl, passthrough_ctl_periodicity, azure_mme_socket_retry_s, azure_mme_socket_drop_enb_s, azure_mme_socket_swap_mme_s, azure_mme_pkt_retx_guard_ms, azure_mme_pkt_retx_max_attempts FROM Settings";
	rc = sqlite3_prepare_v2(sql_hdl, sql2, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		cfg->settings.use_passthrough_ctl = (bool)sqlite3_column_int(stmt, 0);
		cfg->settings.passthrough_ctl_periodicity = sqlite3_column_int(stmt, 1);
		cfg->settings.azure_mme_socket_retry_s = sqlite3_column_int(stmt, 2);
		cfg->settings.azure_mme_socket_drop_enb_s = sqlite3_column_int(stmt, 3);
		cfg->settings.azure_mme_socket_swap_mme_s = sqlite3_column_int(stmt, 4);
		cfg->settings.azure_mme_pkt_retx_guard_ms = sqlite3_column_int(stmt, 5);
		cfg->settings.azure_mme_pkt_retx_max_attempts = sqlite3_column_int(stmt, 6);
	}
	if (rc != SQLITE_DONE)
	{
		goto error_hdl;
	}

	sqlite3_finalize(stmt); stmt = NULL;

	// validate settings
	if (cfg->settings.azure_mme_socket_retry_s < AZURE_MME_SOCKET_RETRY_S_MIN)
		cfg->settings.azure_mme_socket_retry_s = AZURE_MME_SOCKET_RETRY_S_MIN;
	if (cfg->settings.azure_mme_pkt_retx_guard_ms < AZURE_MME_PKT_GUARD_MS_MIN)
		cfg->settings.azure_mme_pkt_retx_guard_ms = AZURE_MME_PKT_GUARD_MS_MIN;

	// release database
	if (sql_hdl) sqlite3_close(sql_hdl);

	return true;

error_hdl:
	if (sql_hdl) sqlite3_close(sql_hdl);
	if (stmt) sqlite3_finalize(stmt);
	return false;
}





