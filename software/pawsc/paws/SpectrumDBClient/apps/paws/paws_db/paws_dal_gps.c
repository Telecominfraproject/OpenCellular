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
#include "gps/gps.h"

#include "paws_db_info.h"
#include "paws_types.h"
#include "paws_dal_utils.h"


//#######################################################################################
bool paws_read_gps_from_db(paws_gps_location_t* gps, device_name_t device_name)
{
	char **pazResult = NULL; 
	sqlite3 *sql_hdl = NULL;

	gps->fixed = false;

	if (!gps)
	{
		goto error_hdl;
	}
	memset(gps, 0, sizeof(paws_gps_location_t));

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

	char zSql[100];
	sprintf(zSql, "SELECT * FROM Gps where deviceid='%s'", device_name);
	int pnRow;           /* Number of result rows written here */
	int pnColumn;       /* Number of result columns written here */
	char *pzErrmsg = NULL;
	int rc = sqlite3_get_table(
		sql_hdl,          /* An open database */
		zSql,			/* SQL to be evaluated */
		&pazResult,    /* Results of the query */
		&pnRow,           /* Number of result rows written here */
		&pnColumn,        /* Number of result columns written here */
		&pzErrmsg       /* Error msg written here */
		);

	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}

	if (!((pnRow == 1) && (pnColumn == 6)))
	{
		goto error_hdl;
	}
	
	for (int row = 0; row < pnRow; row++)
	{
		for (int col = 0; col < pnColumn; col++)
		{
			int index = (row * pnColumn) + pnColumn + col;
			if (strcmp(pazResult[col], "deviceid") == 0)
				strcpy(gps->device_name, pazResult[index]);
			else if (strcmp(pazResult[col], "fixed") == 0)
				gps->fixed =atoi(pazResult[index]) != 0;
			else if (strcmp(pazResult[col], "latitude") == 0)
				gps->latitude = (float)atof(pazResult[index]);
			else if (strcmp(pazResult[col], "longitude") == 0)
				gps->longitude = (float)atof(pazResult[index]);
			else if (strcmp(pazResult[col], "height") == 0)
				gps->height = atoi(pazResult[index]);
			else if (strcmp(pazResult[col], "heightType") == 0)
				strcpy(gps->height_type, pazResult[index]);
			else
			{
				goto error_hdl;
			}
		}
	}
	
	// free up resources
	if (pazResult) 
		sqlite3_free_table(pazResult);
	if (sql_hdl)
		sqlite3_close(sql_hdl);

	return gps->fixed;

error_hdl:
	if (pazResult) sqlite3_free_table(pazResult);
	if (sql_hdl) sqlite3_close(sql_hdl);
	return false;
}



//#######################################################################################
static void paws_read_gps_from_device(paws_gps_location_t* gps, device_name_t device_name)
{
	GpsFixData fix;
	if (gps_get_fix(&fix) == -1)
	{
		gps->fixed = false;
		return;
	}

	// map from GpsFixData to paws_gps_location_t
	strcpy(gps->device_name, device_name);
	gps->fixed = true;
	gps->latitude = fix.latitude;
	gps->longitude = fix.longitude;
	gps->height = fix.altitude;
	snprintf(gps->height_type, sizeof(paws_antenna_height_type_t), "AMSL");
}



//#######################################################################################
void paws_read_gps(paws_gps_location_t* gps, device_name_t device_name)
{
	bool ret = paws_read_gps_from_db(gps, device_name);
	if (!ret)
	{
		paws_read_gps_from_device(gps, device_name);
	}
}
