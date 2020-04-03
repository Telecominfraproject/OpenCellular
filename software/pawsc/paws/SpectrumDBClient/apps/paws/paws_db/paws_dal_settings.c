/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#include <sqlite3.h>

#include "utils/utils.h"
#include "json-parser/json.h"
#include "json-parser/json_utils.h"

#include "paws_db_info.h"
#include "paws_types.h"
#include "paws_dal_utils.h"



// ########################################################################
static bool paws_read_settingsCloudLogInfo(sqlite3 *sql_hdl, cloud_logger_cfg_t* log_cfg)
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
static bool paws_read_settingsLogInfo(sqlite3 *sql_hdl, paws_setting_loginfo_t* loginfo)
{
	if ((!sql_hdl) || (!loginfo))
	{
		return false;
	}
	memset(loginfo, 0, sizeof(paws_setting_loginfo_t));

	// first read the Cloud log
	if (!(paws_read_settingsCloudLogInfo(sql_hdl, &loginfo->cloud_log)))
	{
		goto error_hdl;
	}


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

	if (!((rc == SQLITE_OK) && (pnRow == 1) && (pnColumn == 6)))
	{
		goto error_hdl;
	}

	for (int row = 0; row < pnRow; row++)
	{
		for (int col = 0; col < pnColumn; col++)
		{
			int index = (row * pnColumn) + pnColumn + col;
			if (strcmp(pazResult[col], "level") == 0)
				loginfo->app_log.level = atoi(pazResult[index]);
			else if (strcmp(pazResult[col], "logname") == 0)
				strcpy(loginfo->app_log.logname, pazResult[index]);
			else if (strcmp(pazResult[col], "tvwsdb_messages_logname") == 0)
				strcpy(loginfo->msg_log.logname, pazResult[index]);
			else if (strcmp(pazResult[col], "file_size") == 0)
				strcpy(loginfo->app_log.size, pazResult[index]);
			else if (strcmp(pazResult[col], "max_files") == 0)
				loginfo->app_log.max = atoi(pazResult[index]);
			else if (strcmp(pazResult[col], "compress") == 0)
				loginfo->app_log.compress = atoi(pazResult[index]);
			else
			{
				goto error_hdl;
			}
		}
	}

	loginfo->msg_log.level = loginfo->app_log.level;
	loginfo->msg_log.level = loginfo->app_log.level;
	strcpy(loginfo->msg_log.size, loginfo->app_log.size);
	loginfo->msg_log.max = loginfo->app_log.max;
	loginfo->msg_log.compress = loginfo->app_log.compress;
	loginfo->app_log.cloud_level = loginfo->cloud_log.level;
	loginfo->msg_log.cloud_level = loginfo->cloud_log.level;

	// free up resources
	if (pazResult)
		sqlite3_free_table(pazResult);

	return true;

error_hdl:
	if (pazResult) sqlite3_free_table(pazResult);
	return false;
}


// ########################################################################
static bool paws_read_settingsSpectrumOverride(sqlite3 *sql_hdl, paws_setting_override_t* override)
{
	if ((!sql_hdl) || (!override))
	{
		return false;
	}
	memset(override, 0, sizeof(paws_setting_override_t));

	const char *zSql = "SELECT * FROM SettingsSpectrumOverride";
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

	if (!(rc == SQLITE_OK))
	{
		goto error_hdl;
	}

	if ((pnRow == 1) && (pnColumn == 6))
	{
		for (int row = 0; row < pnRow; row++)
		{
			for (int col = 0; col < pnColumn; col++)
			{
				int index = (row * pnColumn) + pnColumn + col;
				if (strcmp(pazResult[col], "present") == 0)
					override->present = atoi(pazResult[index]) != 0;
				else if (strcmp(pazResult[col], "bandwidth_rb") == 0)
					override->bandwidth_rb = atoi(pazResult[index]);
				else if (strcmp(pazResult[col], "earfcndl") == 0)
					override->earfcndl = atoi(pazResult[index]);
				else if (strcmp(pazResult[col], "earfcnul") == 0)
					override->earfcnul = atoi(pazResult[index]);
				else if (strcmp(pazResult[col], "RefSigPower") == 0)
					override->ref_sig_pwr = atoi(pazResult[index]) * 10;		// convert to units of 0.1dB
				else if (strcmp(pazResult[col], "PMax") == 0)
					override->pmax = atoi(pazResult[index]) * 10;				// convert to units of 0.1dB
				else
				{
					goto error_hdl;
				}
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
bool paws_read_settings(paws_settings_t* settings)
{
	sqlite3 *sql_hdl = NULL;

	if (!settings)
	{
		return false;
	}
	memset(settings, 0, sizeof(paws_settings_t));
	
	// get datafile location and open it
	char* db_sql_file = get_paws_db_location();
	if (!db_sql_file)
	{
		goto error_hdl;
	}
	sqlite3_open(db_sql_file, &sql_hdl);
	if (!sql_hdl)
	{
		return false;
	}

	const char *zSql = "SELECT * FROM Settings";
	int pnRow;           /* Number of result rows written here */
	int pnColumn;       /* Number of result columns written here */
	char *pzErrmsg=NULL;
	char **pazResult=NULL;
	int rc = sqlite3_get_table(
		sql_hdl,          /* An open database */
		zSql,			/* SQL to be evaluated */
		&pazResult,    /* Results of the query */
		&pnRow,           /* Number of result rows written here */
		&pnColumn,        /* Number of result columns written here */
		&pzErrmsg       /* Error msg written here */
		);
	
	if (!((rc == SQLITE_OK) && (pnRow == 1) && (pnColumn == 9)))
	{
		goto error_hdl;
	}

	for (int row = 0; row < pnRow; row++)
	{
		for (int col = 0; col < pnColumn; col++)
		{
			int index = (row * pnColumn) + pnColumn + col;
			if (strcmp(pazResult[col], "min_dl_dbm_100k") == 0)
				settings->min_dl_dbm_100k = (float)atof(pazResult[index]);
			else if (strcmp(pazResult[col], "min_ul_dbm_100k") == 0)
				settings->min_ul_dbm_100k = (float)atof(pazResult[index]);
			else if (strcmp(pazResult[col], "db_retry_secs") == 0)
				settings->db_retry_secs = atoi(pazResult[index]);
			else if (strcmp(pazResult[col], "db_barred_secs") == 0)
				settings->db_barred_secs = atoi(pazResult[index]);
			else if (strcmp(pazResult[col], "setting_periodic_secs") == 0)
				settings->setting_periodic_secs = atoi(pazResult[index]);
			else if (strcmp(pazResult[col], "devices_periodic_secs") == 0)
				settings->devices_periodic_secs = atoi(pazResult[index]);
			else if (strcmp(pazResult[col], "gps_periodic_fast_check_secs") == 0)
				settings->gps_periodic_fast_check_secs = atoi(pazResult[index]);
			else if (strcmp(pazResult[col], "gps_periodic_slow_check_secs") == 0)
				settings->gps_periodic_slow_check_secs = atoi(pazResult[index]);
			else if (strcmp(pazResult[col], "max_polling_quick_secs") == 0)
				settings->max_polling_quick_secs = atoi(pazResult[index]);
			else
			{
				goto error_hdl;
			}
		}
	}

	// free up resources
	if (pazResult) 
		sqlite3_free_table(pazResult);

	if (!(paws_read_settingsLogInfo(sql_hdl, &settings->loginfo)))
	{
		goto error_hdl;
	}

	if (!(paws_read_settingsSpectrumOverride(sql_hdl, &settings->spectrum_override)))
	{
		goto error_hdl;
	}

	// free up resources
	if (sql_hdl)
		sqlite3_close(sql_hdl);

	return true;

error_hdl:
	if (pazResult) sqlite3_free_table(pazResult);
	if (sql_hdl) sqlite3_close(sql_hdl);
	return false;
}

