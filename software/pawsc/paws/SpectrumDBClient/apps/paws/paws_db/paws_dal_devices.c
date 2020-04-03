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

#include "paws_globals.h"
#include "paws_common.h"
#include "paws_db_info.h"
#include "paws_types.h"
#include "paws_dal_utils.h"



//#######################################################################################
static bool sql_get_DeviceInfoAntenna(sqlite3 *sql_hdl, char* deviceid, paws_antenna_info_t* device_ant, bool* antenna_info_read)
{
	*antenna_info_read = false;

	// get the Antenna info
	sqlite3_stmt *stmt=NULL;
	char sql[200];
	sprintf(sql, "SELECT gain FROM DeviceInfoAntenna where deviceid='%s'", deviceid);
	int rc = sqlite3_prepare_v2(sql_hdl, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		device_ant->gain = (int)((float)sqlite3_column_double(stmt, 0) * 10.0);		// Stored in units of 0.1dB.   SQL schema stores it as a float in uints of dB
		*antenna_info_read = true;
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
static bool sql_get_DeviceInfoId(sqlite3 *sql_hdl, char* deviceid, paws_device_identity_t* device_info_id, bool* device_name_read)
{
	*device_name_read = false;

	// get the Antenna info
	sqlite3_stmt *stmt=NULL;
	char sql[100];
	sprintf(sql, "SELECT manufacturer, model FROM DeviceInfo where deviceid='%s'", deviceid);
	int rc = sqlite3_prepare_v2(sql_hdl, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		char *tmp;
		tmp = (char*)sqlite3_column_text(stmt, 0);   strcpy(device_info_id->manufacturer, tmp);
		tmp = (char*)sqlite3_column_text(stmt, 1);   strcpy(device_info_id->model, tmp);
		*device_name_read = true;
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
bool paws_read_master_info(log_app_t app, paws_device_info_t* dev, bool* master_device_cfg_read)
{
	sqlite3 *sql_hdl = NULL;
	sqlite3_stmt *stmt=NULL;
	bool device_characteristics_read = false;
	bool antenna_info_read = false;
	bool device_name_read = false;

	*master_device_cfg_read = false;

	if (!dev)
	{
		goto error_hdl;
	}
	memset(dev, 0, sizeof(paws_device_info_t));
	
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

	// get the Device Characteristcs for the master device
	const char *sql = "SELECT deviceid, type, cat, emission_class, technology_id FROM DeviceInfoCharacteristics where cat='master'";

	int rc = sqlite3_prepare_v2(sql_hdl, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}

	paws_device_characteristics_t*	device_characteristics = &dev->device_characteristics;
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		char *tmp;

		tmp = (char*)sqlite3_column_text(stmt, 0);   
		if (strcmp(gDeviceName, tmp) != 0)
		{
			char log[100];
			snprintf(log, sizeof(log), "Device mismatch: femto_db='%s' paws_db='%s'", gDeviceName, tmp);
			logger_log(gPawsAppLogger, log, LOG_ERROR, app, true, true, __FILENAME__, __LINE__, __func__, gPawsCloudLogger, gDeviceName, PAWS_LOG_TYPE);
			goto error_hdl;
		}
		strcpy(dev->unique_id, tmp);
		tmp = (char*)sqlite3_column_text(stmt, 1);   strcpy(device_characteristics->type, tmp);
		tmp = (char*)sqlite3_column_text(stmt, 2);   strcpy(device_characteristics->cat, tmp);
		device_characteristics->emission_class = sqlite3_column_int(stmt, 3);
		tmp = (char*)sqlite3_column_text(stmt, 4);   strcpy(device_characteristics->technology_id, tmp);
		device_characteristics_read = true;
	}

	if (rc != SQLITE_DONE)
	{
		goto error_hdl;
	}
	
	sqlite3_finalize(stmt);
	stmt = NULL;

	// get the Antenna info
	if (device_characteristics_read)
	{
		if (!(sql_get_DeviceInfoAntenna(sql_hdl, dev->unique_id, &dev->antenna_info, &antenna_info_read)))
		{
			goto error_hdl;
		}
	}

	// get the Device info
	if (antenna_info_read)
	{
		if (!(sql_get_DeviceInfoId(sql_hdl, dev->unique_id, &dev->device_identity, &device_name_read)))
		{
			goto error_hdl;
		}
	}

	// free up resources
	if (sql_hdl)
		sqlite3_close(sql_hdl);

	*master_device_cfg_read = (device_characteristics_read && antenna_info_read && device_name_read);

	return true;

error_hdl:
	if (stmt) sqlite3_finalize(stmt);
	if (sql_hdl) sqlite3_close(sql_hdl);
	*master_device_cfg_read = false;
	return false;
}




//#######################################################################################
bool paws_read_slave_info(log_app_t app, paws_slave_info_t* gop_slaves, paws_slave_info_t* sop_slaves)
{
	sqlite3 *sql_hdl = NULL;
	sqlite3_stmt *stmt=NULL;

	(void)app; // currently unused

	if ((!gop_slaves) || (!sop_slaves))
	{
		goto error_hdl;
	}
	memset(gop_slaves, 0, sizeof(paws_slave_info_t));
	memset(sop_slaves, 0, sizeof(paws_slave_info_t));

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

	// get the Device Characteristcs for the all slave devices
	const char *sql = "SELECT deviceid, type, cat, emission_class, technology_id FROM DeviceInfoCharacteristics where cat='slave'  ORDER BY deviceid";

	int rc = sqlite3_prepare_v2(sql_hdl, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		paws_device_info_t device;
		paws_device_info_t* dev = &device;
		memset(dev, 0, sizeof(paws_device_info_t));

		paws_device_characteristics_t*	device_characteristics = &dev->device_characteristics;
		char *tmp;
		tmp = (char*)sqlite3_column_text(stmt, 0);   strcpy(dev->unique_id, tmp);
		tmp = (char*)sqlite3_column_text(stmt, 1);   strcpy(device_characteristics->type, tmp);
		tmp = (char*)sqlite3_column_text(stmt, 2);   strcpy(device_characteristics->cat, tmp);
		device_characteristics->emission_class = sqlite3_column_int(stmt, 3);
		tmp = (char*)sqlite3_column_text(stmt, 4);   strcpy(device_characteristics->technology_id, tmp);

		bool device_name_read = false;

		// get the Device info
		if (!(sql_get_DeviceInfoId(sql_hdl, dev->unique_id, &dev->device_identity, &device_name_read)))
		{
			goto error_hdl;
		}

		if (device_name_read)
		{
			// is this SOP - if a GPS is present and fixed it is a SOP slave, else it is a GOP slave
			bool sop = ((paws_read_gps_from_db(&dev->gps, dev->unique_id)) && (dev->gps.fixed));

			// if this is a GOP but there were previously SOPs counted, move all these SOPS to GOPs
			if ((!sop) && (sop_slaves->num_devices > 0))
			{
				memcpy(&gop_slaves->device_info[gop_slaves->num_devices], &sop_slaves->device_info[0],
					sizeof(paws_device_info_t) * sop_slaves->num_devices);
				gop_slaves->num_devices += sop_slaves->num_devices;
				sop_slaves->num_devices = 0;
			}

			// if there are any GOPs, we will not run any SOPs, so count SOP as GOP in this case
			if (sop && (gop_slaves->num_devices == 0))
			{
				memcpy(&sop_slaves->device_info[sop_slaves->num_devices], dev, sizeof(paws_device_info_t));
				sop_slaves->num_devices++;
			}
			else
			{
				memcpy(&gop_slaves->device_info[gop_slaves->num_devices], dev, sizeof(paws_device_info_t));
				gop_slaves->num_devices++;
			}
		}
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
	sop_slaves->num_devices = 0;
	gop_slaves->num_devices = 0;

	return false;
}

