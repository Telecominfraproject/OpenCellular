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
#include "paws_utils.h"
#include "paws_dal_utils.h"


//#######################################################################################
#define MAX_SQL_LEN		(200)
static char sql_str[MAX_SQL_LEN];
static char tmp[200];
#define SQL_STRCAT(s)     strncat(sql_str, s, MAX_SQL_LEN - strlen(sql_str) - 1);



//#######################################################################################
static bool sql_get_weblist(sqlite3 *sql_hdl, paws_weblist_url_t* weblist_url)
{
	// get the TVWSDBInfo info
	sqlite3_stmt *stmt=NULL;
	char sql[100];
	sprintf(sql, "SELECT weblist_host, weblist_filename FROM TVWSDBInfo");
	int rc = sqlite3_prepare_v2(sql_hdl, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}

	// clear host to start 
	weblist_url->host[0] = '\0';

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		char *tmp;
		tmp = (char*)sqlite3_column_text(stmt, 0);   strcpy(weblist_url->host, tmp);
		tmp = (char*)sqlite3_column_text(stmt, 1);   strcpy(weblist_url->fname, tmp);
	}

	if (rc != SQLITE_DONE)
	{
		goto error_hdl;
	}

	sqlite3_finalize(stmt);

	return true;

error_hdl:
	if (stmt) sqlite3_finalize(stmt);
	return false;
}



//#######################################################################################
bool paws_read_db(paws_db_info_t* db)
{
	sqlite3 *sql_hdl = NULL;
	sqlite3_stmt *stmt = NULL;

	if (!db)
	{
		goto error_hdl;
	}
	memset(db, 0, sizeof(paws_db_info_t));
	
	// get datafile location and open it
	char* db_sql_file = get_paws_db_location();
	if (!db_sql_file)
	{
		goto error_hdl;
	}
	sqlite3_open(db_sql_file, &sql_hdl);
	if (!sql_hdl)
	{
		goto error_hdl;
	}

	// get the weblist info
	if (!(sql_get_weblist(sql_hdl, &db->weblist_url)))
	{
		goto error_hdl;
	}

	// now get the database entries
	const char *sql = "SELECT name, host, token, valid, barred_utc FROM TVWSDBInfoDB";

	int rc = sqlite3_prepare_v2(sql_hdl, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		paws_db_item_t* db_item = &db->db_list[db->num_db] ;
		char *tmp;
		tmp = (char*)sqlite3_column_text(stmt, 0);   strcpy(db_item->name, tmp);
		tmp = (char*)sqlite3_column_text(stmt, 1);   strcpy(db_item->db_url.host, tmp);
		tmp = (char*)sqlite3_column_text(stmt, 2);   strcpy(db_item->db_url.token, tmp);		
		db_item->valid = sqlite3_column_int(stmt, 3);
		tmp = (char*)sqlite3_column_text(stmt, 4);   
		if ((tmp) && (strlen(tmp) > 0))
		{
			if ((db_item->barred_utc = timestamp_to_timet(tmp)) == -1)
			{
				goto error_hdl;
			}
		}
		db->num_db++;
	}

	if (rc != SQLITE_DONE)
	{
		goto error_hdl;
	}

	sqlite3_finalize(stmt);
	stmt = NULL;

	// free up resources
	if (sql_hdl)
		sqlite3_close(sql_hdl);

	return true;

error_hdl:
	if (stmt) sqlite3_finalize(stmt);
	if (sql_hdl) sqlite3_close(sql_hdl);
	db->num_db = 0;
	return false;
}


//#######################################################################################
static paws_db_item_t* get_db_from_dblist(paws_db_item_t* db1, int num_db_in_list, paws_db_item_t* db_list)
{
	for (int i = 0; i < num_db_in_list; i++)
	{
		paws_db_item_t* db2 = &db_list[i];

		// they are the same if the url matches
		if (strcmp(db1->db_url.host, db2->db_url.host) == 0)
			return db2;

	}
	return NULL;
}


//#######################################################################################
static bool sql_add_db_item(paws_db_item_t* db_item)
{
	sqlite3 *sql_hdl = NULL;
	char* error_msg = NULL;

	if (!db_item)
	{
		goto error_hdl;
	}

	// get datafile location and open it
	char* db_sql_file = get_paws_db_location();
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
	sql_str[0] = '\0';
	SQL_STRCAT("INSERT INTO TVWSDBInfoDB (name, host, token, valid");
	if (db_item->barred_utc)
	{
		SQL_STRCAT(", barred_utc ");
	}
	SQL_STRCAT(") VALUES ( ");
	sprintf(tmp, "'%s', '%s', '%s', %d", db_item->name, db_item->db_url.host, db_item->db_url.token, db_item->valid);
	SQL_STRCAT(tmp);
	if (db_item->barred_utc)
	{
		SQL_STRCAT(", '");
		strftime(tmp, sizeof(tmp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&db_item->barred_utc));
		SQL_STRCAT(tmp);
		SQL_STRCAT("'");
	}
	SQL_STRCAT(");");

	// run command
	int rc = sqlite3_exec(sql_hdl, sql_str, NULL, NULL, &error_msg);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}

	// free up resources
	if (error_msg) sqlite3_free(error_msg);
	if (sql_hdl) sqlite3_close(sql_hdl);

	return true;

error_hdl:
	if (error_msg) sqlite3_free(error_msg);
	if (sql_hdl) sqlite3_close(sql_hdl);
	return false;	
}


//#######################################################################################
static bool sql_update_db_item(paws_db_item_t* db_item)
{
	sqlite3 *sql_hdl = NULL;
	char* error_msg = NULL;

	if (!db_item)
	{
		goto error_hdl;
	}

	// get datafile location and open it
	char* db_sql_file = get_paws_db_location();
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
	sql_str[0] = '\0';
	sprintf(tmp, "UPDATE TVWSDBInfoDB SET ");  SQL_STRCAT(tmp);
	sprintf(tmp, "name = '%s'", db_item->name);  SQL_STRCAT(tmp);
	sprintf(tmp, ", token = '%s'", db_item->db_url.token);  SQL_STRCAT(tmp);
	sprintf(tmp, ", valid = %d", db_item->valid);  SQL_STRCAT(tmp);
	SQL_STRCAT(", barred_utc='"); 
	if (db_item->barred_utc)
	{
		strftime(tmp, sizeof(tmp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&db_item->barred_utc));
		SQL_STRCAT(tmp);
	}
	SQL_STRCAT("'");
	sprintf(tmp, " WHERE host = '%s' ;", db_item->db_url.host);  SQL_STRCAT(tmp);

	// run command
	int rc = sqlite3_exec(sql_hdl, sql_str, NULL, NULL, &error_msg);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}

	// free up resources
	if (error_msg) sqlite3_free(error_msg);
	if (sql_hdl) sqlite3_close(sql_hdl);

	return true;

error_hdl:
	if (error_msg) sqlite3_free(error_msg);
	if (sql_hdl) sqlite3_close(sql_hdl);
	return false;
}


//#######################################################################################
static bool sql_delete_db_item(paws_db_item_t* db_item)
{
	sqlite3 *sql_hdl = NULL;
	char* error_msg = NULL;

	if (!db_item)
	{
		goto error_hdl;
	}

	// get datafile location and open it
	char* db_sql_file = get_paws_db_location();
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
	sprintf(sql_str, "DELETE FROM TVWSDBInfoDB WHERE host = '%s' ;", db_item->db_url.host); 
	// run command
	int rc = sqlite3_exec(sql_hdl, sql_str, NULL, NULL, &error_msg);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}

	// free up resources
	if (error_msg) sqlite3_free(error_msg);
	if (sql_hdl) sqlite3_close(sql_hdl);

	return true;

error_hdl:
	if (error_msg) sqlite3_free(error_msg);
	if (sql_hdl) sqlite3_close(sql_hdl);
	return false;
}



//#######################################################################################
bool paws_write_db(paws_db_info_t* db)
{
	uint32_t i = 0;

	if (!db)
	{
		goto error_hdl;
	}

	// read the current DB
	paws_db_info_t curr_db;
	if (!(paws_read_db(&curr_db)))
	{
		goto error_hdl;
	}

	// add or update DB
	for (i = 0; i < db->num_db; i++)				// walk new db
	{
		paws_db_item_t* db_item = &db->db_list[i];
		paws_db_item_t* curr_db_item = NULL;
		if (!(curr_db_item = get_db_from_dblist(db_item, curr_db.num_db, &curr_db.db_list[0])))		// if new db is not in curr db list, add it 
		{
			// it is not there, so add it
			if (!(sql_add_db_item(db_item)))
			{
				goto error_hdl;
			}
		}
		else
		{																			// if new db is in curr db list, check if it is different, and if so update it
			// it is there, so update it if different
			if (memcmp(db_item, curr_db_item, sizeof(paws_db_item_t)) != 0)
			{
				// update it
				if (!(sql_update_db_item(db_item)))
				{
					goto error_hdl;
				}
			}
		}
	}

	// delete any which have been removed
	for (i = 0; i < curr_db.num_db; i++)					// walk curr db.  If any DB from curr_db are not in new db list, delete it from the sql
	{
		paws_db_item_t* curr_db_item = &curr_db.db_list[i];
		paws_db_item_t* new_db_item = NULL;
		if (!(new_db_item = get_db_from_dblist(curr_db_item, db->num_db, &db->db_list[0])))		// if curr db is not in new db list, delete it
		{
			// delete it 
			if (!(sql_delete_db_item(curr_db_item)))
			{
				goto error_hdl;
			}
		}
	}

	return true;

error_hdl:
	return false;
}
