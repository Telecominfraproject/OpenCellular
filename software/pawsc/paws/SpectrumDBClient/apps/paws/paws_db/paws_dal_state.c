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
paws_sm_state_info_t* paws_read_state(void)
{
	sqlite3 *sql_hdl = NULL;
	char *json_str=NULL;
	sqlite3_stmt *stmt=NULL;
	paws_sm_state_info_t* sm_info = NULL;
	json_value *jval = NULL;

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

	// get the state
	const char *sql = "SELECT state_str FROM PreBootState";

	int rc = sqlite3_prepare_v2(sql_hdl, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		json_str = (char*)sqlite3_column_text(stmt, 0);  
		if (json_str)		// if something was read
		{
			int len = strlen(json_str);
			// now convert the json string to a paws_sm_state_info_t
			// we have the contents, now convert to json
			if (!(jval = json_parse((json_char*)json_str, len)))
			{
				goto error_hdl;
			}
			if (!(sm_info = json2_paws_sm_state_info(jval)))
			{
				goto error_hdl;
			}
		}
	}

	if (rc != SQLITE_DONE)
	{
		goto error_hdl;
	}

	// free up resources
	if (stmt)
		sqlite3_finalize(stmt);
	stmt = NULL;
	if (sql_hdl)
		sqlite3_close(sql_hdl);
	if (jval) 
		json_value_free(jval);
	
	// return the sm info
	return sm_info;

error_hdl:
	if (stmt) sqlite3_finalize(stmt);
	if (sql_hdl) sqlite3_close(sql_hdl);
	if (jval) json_value_free(jval);
	return NULL;
}




//#######################################################################################
void paws_remove_state(void)
{
	sqlite3 *sql_hdl = NULL;
	char* error_msg = NULL;

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
	char sql_str[100];
	sprintf(sql_str, "DELETE FROM PreBootState ;");
	// run command
	int rc = sqlite3_exec(sql_hdl, sql_str, NULL, NULL, &error_msg);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}

error_hdl:
	// free up resources
	if (error_msg) sqlite3_free(error_msg);
	if (sql_hdl) sqlite3_close(sql_hdl);
}



//#######################################################################################
bool paws_write_state(paws_sm_state_info_t* sm_state_info)
{
	sqlite3 *sql_hdl = NULL;
	char* error_msg = NULL;
	char* sql_str = NULL;
	char* p_sql_str = NULL;
	json_value* jval = NULL;

	sql_str = get_init_paws_dal_encode_str();

	// build SQL command
	sprintf(sql_str, "INSERT INTO PreBootState (state_str) VALUES ( '");

	// point to where the json is to be written
	p_sql_str = sql_str + strlen(sql_str);
	if (!(paws_sm_state_info_2_jsonstr(sm_state_info)))
		return false;
	// we have the contents, now convert to json to check it is valid
	int slen = strlen(p_sql_str);
	if (!(jval = json_parse((json_char*)p_sql_str, slen)))
	{
		return false;
	}

	// append the end 
	p_sql_str += slen;
	sprintf(p_sql_str, "' ) ;");

	// j contains a valid json string, so write it to the SQL
	paws_remove_state();				// for safety remove any current state, however there shouldnt actually be any

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

	// run command
	int rc = sqlite3_exec(sql_hdl, sql_str, NULL, NULL, &error_msg);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}

	// free up resources
	if (error_msg) sqlite3_free(error_msg);
	if (sql_hdl) sqlite3_close(sql_hdl);
	if (jval) json_value_free(jval);

	return true;

error_hdl:
	if (error_msg) sqlite3_free(error_msg);
	if (sql_hdl) sqlite3_close(sql_hdl);
	if (jval) json_value_free(jval);
	return false;
}
