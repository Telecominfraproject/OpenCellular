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

#include "lte_db_info.h"
#include "lte_utils.h"

#include "paws_types.h"
#include "paws_common.h"
#include "paws_globals.h"
#include "paws_dal_device_cfg.h"
#include "paws_dal_utils.h"
#include "paws_device_specific.h"


//#######################################################################################
#define MAX_SQL_LEN		(200)
static char sql_str[MAX_SQL_LEN];
static char tmp[200];
#define SQL_STRCAT(s)     strncat(sql_str, s, MAX_SQL_LEN - strlen(sql_str) - 1);


static int current_adminState = -1;		// we don't write adminState=0.  To disable we issue command to SCTP-Agent which will cause eNB software to disable the radio.
										// -1 means not set


static device_cfg_params_t current_device_cfg = { 0 };


#define MAX_RADIATED_POWER_DBM			(36)



//#######################################################################################
bool paws_get_device_name(device_name_t device_name)
{
	sqlite3 *sql_hdl = NULL;
	sqlite3_stmt *stmt = NULL;
	int rc;

	memset(device_name, 0, sizeof(device_name_t));

	// get datafile location and open it
	char* db_sql_file = get_lte_db_location();
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
static bool sql_get_master_cfg(device_cfg_params_t* cfg, bool use_override)
{
	sqlite3 *sql_hdl = NULL;
	sqlite3_stmt *stmt = NULL;
	int rc;

	if (!cfg)
	{
		goto error_hdl;
	}
	memset(cfg, 0, sizeof(device_cfg_params_t));
	
	// get datafile location and open it
	char* db_sql_file = get_lte_db_location();
	if (!db_sql_file)
	{
		goto error_hdl;
	}
	sqlite3_open(db_sql_file, &sql_hdl);
	if (!sql_hdl)
	{
		goto error_hdl;
	}

	if (current_adminState == -1)			// only read AdminState if it hasn't been read previously
	{
		// get the Admin State
		const char *sql = "SELECT AdminState FROM FAPServiceFAPControlLTE";
		rc = sqlite3_prepare_v2(sql_hdl, sql, -1, &stmt, NULL);
		if (rc != SQLITE_OK)
		{
			goto error_hdl;
		}
		while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
		{
			current_adminState = sqlite3_column_int(stmt, 0);
		}
		if (rc != SQLITE_DONE)
		{
			goto error_hdl;
		}
		sqlite3_finalize(stmt); stmt = NULL;
	}
	cfg->admin_state = current_adminState;

	// get x_000295_external_pa_gain
	const char *sql4 = "SELECT X_000295_ExternalPAGain FROM FAPServiceCellConfigLTERANRF";
	rc = sqlite3_prepare_v2(sql_hdl, sql4, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		cfg->x_000295_external_pa_gain = sqlite3_column_int(stmt, 0);
	}
	if (rc != SQLITE_DONE)
	{
		goto error_hdl;
	}
	sqlite3_finalize(stmt); stmt = NULL;

	// get MaxPower - stored in dB as units of dBM, so we'll multiply by 10 and store it as units of 0.1db, as per other fields.
	const char *sql2 = "SELECT MaxTxPower FROM FAPServiceCapabilities";
	rc = sqlite3_prepare_v2(sql_hdl, sql2, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		cfg->capabilities_max_tx_power = sqlite3_column_int(stmt, 0) * 10;
	}
	if (rc != SQLITE_DONE)
	{
		goto error_hdl;
	}
	sqlite3_finalize(stmt); stmt = NULL;

	// get MinPower - stored in dB as units of 0.1dB, so we'll keep it like that.
	const char *sql3 = "SELECT X_000295_MinTxPower FROM FAPServiceCapabilitiesLTE";
	rc = sqlite3_prepare_v2(sql_hdl, sql3, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		goto error_hdl;
	}
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		cfg->capabilities_min_tx_power = sqlite3_column_int(stmt, 0);
	}
	if (rc != SQLITE_DONE)
	{
		goto error_hdl;
	}
	sqlite3_finalize(stmt); stmt = NULL;

	if (use_override)
	{
		cfg->bandwidth = current_device_cfg.bandwidth;
		cfg->earfcndl = current_device_cfg.earfcndl;
		cfg->earfcnul = current_device_cfg.earfcnul;
		cfg->ref_sig_power = current_device_cfg.ref_sig_power;
		cfg->pmax = current_device_cfg.pmax;
	}
	else
	{
		// get bandwidth, ref_sig_power, earfcndl, earfcnul
		const char *sql4 = "SELECT ULBandwidth, ReferenceSignalPower, EARFCNDL, EARFCNUL  FROM FAPServiceCellConfigLTERANRF";
		rc = sqlite3_prepare_v2(sql_hdl, sql4, -1, &stmt, NULL);
		if (rc != SQLITE_OK)
		{
			goto error_hdl;
		}
		while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
		{
			cfg->bandwidth = sqlite3_column_int(stmt, 0);
			cfg->ref_sig_power = sqlite3_column_int(stmt, 1) * 10;
			cfg->earfcndl = sqlite3_column_int(stmt, 2);
			cfg->earfcnul = sqlite3_column_int(stmt, 3);
		}
		if (rc != SQLITE_DONE)
		{
			goto error_hdl;
		}
		sqlite3_finalize(stmt); stmt = NULL;

		// get Pmax
		const char *sql5 = "SELECT PMax FROM FAPServiceCellConfigLTERANMobilityIdleModeIntraFreq";
		rc = sqlite3_prepare_v2(sql_hdl, sql5, -1, &stmt, NULL);
		if (rc != SQLITE_OK)
		{
			goto error_hdl;
		}
		while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
		{
			cfg->pmax = sqlite3_column_int(stmt, 0) * 10;
		}
		if (rc != SQLITE_DONE)
		{
			goto error_hdl;
		}
		sqlite3_finalize(stmt); stmt = NULL;
	}

	// release database
	if (sql_hdl) sqlite3_close(sql_hdl);

	return true;

error_hdl:
	if (sql_hdl) sqlite3_close(sql_hdl);
	if (stmt) sqlite3_finalize(stmt);
	return false;
}



//#######################################################################################
//
//
//        +---------+                         +---------+                        +------------+
//        |  FAP    |------------------------>|   LRE   |----------*------------>|   Antenna  |------------*-------------->
//        +---------+                         +---------+          |             +------------+            |
//			                                                       |                                       |
//		  	                                                       |                                       |
//			                                               total_ref_sig_power                          total_power
//                                                          (max_pa_power)                             (max_tx_power)
//             
//        		total_power = that specified in the AVAIL_SPEC_RESP (or by Ofcom for Override)
//              min_pa_power <= total_reg_sig_power <= max_pa_power
//              total_ref_sig_power + antenna_gain <= max_tx_power
//
static bool limit_and_validate_cfg(void* logger, device_cfg_params_t* dst, int min_pa_power, int max_pa_power, int master_antenna_gain, int max_tx_power, int lre)
{
	float _10log_numRBelements = 0;

	// total power = 10xlog(num_100k_blocks) + dbm_per100k;
	// fix the 10xlog(num_100k_blocks) based on bandwdith
	if (dst->bandwidth == 25)
	{
		_10log_numRBelements = 24.77;
	}
	else if (dst->bandwidth == 50)
	{
		_10log_numRBelements = 27.78;
	}
	else if (dst->bandwidth == 100)
	{
		_10log_numRBelements = 30.79;
	}
	else
		return false;

	dst->capabilities_min_tx_power = min_pa_power;
	dst->capabilities_max_tx_power = max_pa_power;
	dst->antenna_gain = master_antenna_gain;
	dst->x_000295_external_pa_gain = lre;
	dst->total_ref_sig_power = dst->ref_sig_power + (_10log_numRBelements*10) + dst->antenna_gain;

	// check the max radiated power is not exceeded
	if (dst->total_ref_sig_power > max_tx_power)
	{
		char dbg[200];
		sprintf(dbg, "Limiting DL power to maximum allowed to be radiated [%.2fdBm]", max_tx_power / 10.0);
		logger_log(logger, dbg, LOG_NOTICE, "Device-Cfg", true, true, NULL, 0, NULL, gPawsCloudLogger, gDeviceName, PAWS_LOG_TYPE);
		dst->total_ref_sig_power = max_tx_power;
		dst->limited = true;
	}

	// take off the antenna gain to check the max_pa and min_pa
	dst->total_ref_sig_power -= dst->antenna_gain;
	
	// check min
	if (dst->total_ref_sig_power < min_pa_power)
	{
		char dbg[200];
		sprintf(dbg, "Invalid cfg :  Allowed DL power is below Minimum Power of the device.  Ref_sig_power=%.2fdBm Minimum_Ref_sig_power=%.2fdBm \n",
			(float)dst->total_ref_sig_power/10.0, (float)min_pa_power/10.0);
		logger_log(logger, dbg, LOG_ERROR, "Device-Cfg", true, true, NULL, 0, NULL, gPawsCloudLogger, gDeviceName, PAWS_LOG_TYPE);
		return false;
	}
	// check max
	if (dst->total_ref_sig_power > max_pa_power)
	{
		char dbg[200];
		sprintf(dbg, "Limiting DL power to maximum of the device [%.2fdBm]", dst->capabilities_max_tx_power/10.0);
		logger_log(logger, dbg, LOG_NOTICE, "Device-Cfg", true, true, NULL, 0, NULL, gPawsCloudLogger, gDeviceName, PAWS_LOG_TYPE);
		dst->total_ref_sig_power = max_pa_power;
		dst->limited = true;
	}

	dst->ref_sig_power = dst->total_ref_sig_power - (_10log_numRBelements * 10);


	// check Pmax min
	if (dst->pmax < (LTE_PMAX_MIN * 10))
	{
		char dbg[200];
		sprintf(dbg, "Invalid cfg :  Allowed UL power is below Minimum Power of an LTE UE.  PMax=%.2fdBm PMax(min)=%d.0dBm \n",
			(float)dst->pmax / 10.0, LTE_PMAX_MIN);
		logger_log(logger, dbg, LOG_ERROR, "Device-Cfg", true, true, NULL, 0, NULL, gPawsCloudLogger, gDeviceName, PAWS_LOG_TYPE);
		return false;
	}

	// check Pmax max
	if (dst->pmax > (LTE_PMAX_MAX * 10))
	{
		char dbg[200];
		sprintf(dbg, "Limiting UL power to maximum of an LTE UE [%d.0dBm]", LTE_PMAX_MAX);
		logger_log(logger, dbg, LOG_NOTICE, "Device-Cfg", true, true, NULL, 0, NULL, gPawsCloudLogger, gDeviceName, PAWS_LOG_TYPE);
		dst->pmax = (LTE_PMAX_MAX * 10);
		dst->limited = true;
	}

	// fail safe - just to double check the valiues are consistent
	if (!(validate_lte_cfg(13, dst->bandwidth, dst->ref_sig_power, dst->earfcndl, dst->earfcnul, dst->pmax)))
	{
		char dbg[200];
		sprintf(dbg, "Invalid cfg :  Bandwidth=%d(RB)  earfcndl=%d  earfcnul=%d  ref_sig_power=%.2fdBm  Pmax=%.2fdBm",
			dst->bandwidth, dst->earfcndl, dst->earfcnul, (float)dst->ref_sig_power/10.0, (float)dst->pmax/10.0);
		logger_log(logger, dbg, LOG_ERROR, "Device-Cfg", true, true, NULL, 0, NULL, gPawsCloudLogger, gDeviceName, PAWS_LOG_TYPE);
		return false;
	}

	return true;
}


//#######################################################################################
bool create_device_cfg_params(void* logger, device_cfg_params_t* dst, int min_tx_power, int max_tx_power, device_cfg_t* paws_cfg, int master_antenna_gain, int lre)
{
	float _10log_numRBelements = 0;
	float _10log_num100kblocks = 0;

	if ((!dst) || (!paws_cfg))
		return false;

	memset(dst, 0, sizeof(device_cfg_params_t));

	// set the capabilities
	dst->capabilities_min_tx_power = min_tx_power;
	dst->capabilities_max_tx_power = max_tx_power;
	dst->antenna_gain = master_antenna_gain;
	dst->x_000295_external_pa_gain = lre;

	// adminState
	dst->admin_state = paws_cfg->device_enabled;

	if (dst->admin_state)
	{
		// bandwidth
		if (paws_cfg->bandwidth == 5000000)
			dst->bandwidth = 25;
		else if (paws_cfg->bandwidth == 10000000)
			dst->bandwidth = 50;
		else if (paws_cfg->bandwidth == 20000000)
			dst->bandwidth = 100;
		else
			return false;

		// uarcfn
		if ((dst->earfcndl = dl_hz_to_earfcn(13, paws_cfg->dl_start_hz + (paws_cfg->bandwidth / 2))) == 0)
			return false;
		if ((dst->earfcnul = ul_hz_to_earfcn(13, paws_cfg->ul_start_hz + (paws_cfg->bandwidth / 2))) == 0)
			return false;

		// total power = 10xlog(num_100k_blocks) + dbm_per100k;
		// fix the 10xlog(num_100k_blocks) based on bandwdith
		if (dst->bandwidth == 25)
		{
			_10log_numRBelements = 24.77;
			_10log_num100kblocks = 16.98;
		}
		else if (dst->bandwidth == 50)
		{
			_10log_numRBelements = 27.78;
			_10log_num100kblocks = 20.00;
		}
		else if (dst->bandwidth == 100)
		{
			_10log_numRBelements = 30.79;
			_10log_num100kblocks = 23.01;
		}
		else
			return false;

		dst->pmax = (int)((_10log_num100kblocks + paws_cfg->ul_dbm_per100k) * 10.0);

		dst->limited = false;

		// Ref power must fall between MinTxPower and MaxTxPower
		uint32_t max_total_power = (_10log_num100kblocks + paws_cfg->dl_dbm_per100k) * 10;
		dst->total_ref_sig_power = max_total_power - dst->antenna_gain;
		// create power per 100k
		dst->ref_sig_power = dst->total_ref_sig_power - (_10log_numRBelements * 10);

		if (!(limit_and_validate_cfg(logger, dst, dst->capabilities_min_tx_power, dst->capabilities_max_tx_power, master_antenna_gain, max_total_power, lre)))
			return false;
	}

	return true;
}


//#######################################################################################
bool process_device_cfg_params(void* logger, device_cfg_params_t* new_, device_cfg_params_t* curr, bool* admin_state_changed, bool *cfg_changed, bool *reboot_needed, bool spectrum_override_present)
{
	sqlite3 *sql_hdl = NULL;
	char* error_msg = NULL;

	sql_str[0] = '\0';
	tmp[0] = '\0';

	if ((!new_) || (!curr) || (!cfg_changed) || (!reboot_needed))
	{
		goto error_hdl;
	}

	*cfg_changed = false;
	*reboot_needed = false;
	*admin_state_changed = false;

	// get datafile location and open it
	char* db_sql_file = get_lte_db_location();
	if (!db_sql_file)
	{
		goto error_hdl;
	}
	sqlite3_open(db_sql_file, &sql_hdl);
	if (!sql_hdl)
	{
		goto error_hdl;
	}

	// write params 
	if (new_->admin_state)
	{
		// ULBandwidth, ReferenceSignalPower, EARFCNDL, EARFCNUL
		// build SQL command

		sprintf(tmp, "UPDATE FAPServiceCellConfigLTERANRF SET ");  SQL_STRCAT(tmp);
		// Bandwidth
		if (new_->bandwidth != curr->bandwidth)
		{
			sprintf(tmp, "ULBandwidth = %d, DLBandwidth = %d", new_->bandwidth, new_->bandwidth);  SQL_STRCAT(tmp);
			*cfg_changed = true;
			current_device_cfg.bandwidth = new_->bandwidth;
		}
		// ReferenceSignalPower
		if ((new_->ref_sig_power/10) != (curr->ref_sig_power/10))			// divide by 10 as the femto,db does not have granularity to 0.1db
		{
			if (*cfg_changed) SQL_STRCAT(",");
			sprintf(tmp, "ReferenceSignalPower = %d", new_->ref_sig_power / 10);  SQL_STRCAT(tmp);
			*cfg_changed = true;
			current_device_cfg.ref_sig_power = new_->ref_sig_power;
		}
	
		// EARFCNDL
		if (new_->earfcndl != curr->earfcndl)
		{
			if (*cfg_changed) SQL_STRCAT(",");
			sprintf(tmp, "EARFCNDL = %d", new_->earfcndl);  SQL_STRCAT(tmp);
			*cfg_changed = true;
			current_device_cfg.earfcndl = new_->earfcndl;
		}
		// EARFCNUL
		if (new_->earfcnul != curr->earfcnul)
		{
			if (*cfg_changed) SQL_STRCAT(",");
			sprintf(tmp, "EARFCNUL = %d", new_->earfcnul);  SQL_STRCAT(tmp);
			*cfg_changed = true;
			current_device_cfg.earfcnul = new_->earfcnul;
		}

		if (*cfg_changed)
		{
			logger_log(logger, sql_str, LOG_NOTICE, "Device-Cfg", true, true, NULL, 0, NULL, gPawsCloudLogger, gDeviceName, PAWS_LOG_TYPE);
			if (!spectrum_override_present)
			{
				// run command
				int rc = sqlite3_exec(sql_hdl, sql_str, NULL, NULL, &error_msg);
				if (rc != SQLITE_OK)
				{
					goto error_hdl;
				}
			}
		}

		// Pmax
		if ((new_->pmax/10) != (curr->pmax/10))				// divide by 10 as the femto.db does not have granularity to 0.1db
		{
			*cfg_changed = true;
			current_device_cfg.pmax = new_->pmax;
			// build SQL command
			sprintf(sql_str, "UPDATE FAPServiceCellConfigLTERANMobilityIdleModeIntraFreq SET Pmax=%d", new_->pmax / 10);
			logger_log(logger, sql_str, LOG_NOTICE, "Device-Cfg", true, true, NULL, 0, NULL, gPawsCloudLogger, gDeviceName, PAWS_LOG_TYPE);
			if (!spectrum_override_present)
			{
				// run command
				int rc = sqlite3_exec(sql_hdl, sql_str, NULL, NULL, &error_msg);
				if (rc != SQLITE_OK)
				{
					goto error_hdl;
				}
			}
		}
	}

	// AdminState
	if (new_->admin_state != current_adminState)
	{
		current_adminState = new_->admin_state;
		*admin_state_changed = true;
	}

	// free up resources
	if (sql_hdl)
		sqlite3_close(sql_hdl);

	if ((*cfg_changed) && (new_->admin_state == true))
		*reboot_needed = true;

	// per device, check if device should always be rebooted when Admin is set to False)
	if ((*admin_state_changed) && (new_->admin_state == false) && (reboot_device_on_admin_disable()))
	{
		*reboot_needed = true;
	}

	// log it
	if (new_->admin_state)
	{
		float dl_mhz = (float)dl_earfcn_to_hz(13, new_->earfcndl) / 1000000;
		float ul_mhz = (float)ul_earfcn_to_hz(13, new_->earfcnul) / 1000000;
		float ref_sig_pwr = (new_->admin_state) ? ((float)new_->ref_sig_power / 10.0) : -99.9;
		float total_ref_sig_pwr = (new_->admin_state) ? ((float)new_->total_ref_sig_power / 10.0) : -99.9;
		float ul_pmax = (new_->admin_state) ? ((float)new_->pmax / 10.0) : -99.9;

		char log_j_str[300];
		sprintf(log_j_str, "{\"reboot\":%d, \"CfgChanged\":%d, \"device_enabled\":%d, \"Limited\":%d, \"Bandwidth\":%d, \"EARFCNDL\":%d, \"DL_MHz\":%.2f, \"EARFCNUL\":%d, \"UL_MHz\":%.2f, \"RefSigPower\":%.2f, \"TotalRefSigPower\":%.2f, \"Pmax\":%.2f, \"DeviceAntennaGain\":%.2f, \"LRE\":%.2f, \"DeviceMinPower\":%.2f, \"DeviceMaxPower\":%.2f }",
			*reboot_needed, *cfg_changed, new_->admin_state,
			new_->limited, new_->bandwidth, new_->earfcndl, dl_mhz, new_->earfcnul, ul_mhz, ref_sig_pwr, total_ref_sig_pwr, ul_pmax,
			(float)new_->antenna_gain / 10.0, (float)new_->x_000295_external_pa_gain / 10.0, (float)new_->capabilities_min_tx_power / 10.0, (float)new_->capabilities_max_tx_power / 10.0);
		logger_log(logger, log_j_str, LOG_NOTICE, "Device-Cfg", true, true, NULL, 0, NULL, gPawsCloudLogger, gDeviceName, PAWS_LOG_TYPE);
		// DeviceCfg Cloud log
		if (gPawsCloudLogger)
		{
			cloud_logger_log(gPawsCloudLogger, "Device-Cfg", gDeviceName, CLOUD_LOGGER_DEVICE_CFG, PAWS_LOG_TYPE, log_j_str);
		}
	}
	else
	{
		char log_j_str[300];
		sprintf(log_j_str, "{\"reboot\":%d, \"CfgChanged\":%d, \"device_enabled\":%d, \"RefSigPower\":-99.9, \"TotalRefSigPower\":-99.9, \"Pmax\":-99.9, \"DeviceAntennaGain\":%.2f, \"LRE\":%.2f, \"DeviceMinPower\":%.2f, \"DeviceMaxPower\":%.2f }",
			*reboot_needed, *cfg_changed, new_->admin_state,
			(float)new_->antenna_gain / 10.0, (float)new_->x_000295_external_pa_gain / 10.0, (float)new_->capabilities_min_tx_power / 10.0, (float)new_->capabilities_max_tx_power / 10.0);
		logger_log(logger, log_j_str, LOG_NOTICE, "Device-Cfg", true, true, NULL, 0, NULL, gPawsCloudLogger, gDeviceName, PAWS_LOG_TYPE);
		// DeviceCfg Cloud log
		if (gPawsCloudLogger)
		{
			cloud_logger_log(gPawsCloudLogger, "Device-Cfg", gDeviceName, CLOUD_LOGGER_DEVICE_CFG, PAWS_LOG_TYPE, log_j_str);
		}
	}

	return true;

error_hdl:
	*cfg_changed = false;
	*reboot_needed = false;
	*admin_state_changed = false;
	if (sql_hdl) sqlite3_close(sql_hdl);
	return false;
}


//#######################################################################################
bool paws_config_master_device(void* logger, device_cfg_t* paws_cfg, bool* admin_state_changed, bool* cfg_changed, bool* reboot_needed, paws_setting_override_t* spectrum_override, paws_antenna_info_t* antenna_info)
{
	device_cfg_params_t curr_device_cfg;
	device_cfg_params_t new_device_cfg;

	if (!(sql_get_master_cfg(&curr_device_cfg, spectrum_override->present)))
		return false;

	// create new config
	if (!(create_device_cfg_params(logger, &new_device_cfg, curr_device_cfg.capabilities_min_tx_power, curr_device_cfg.capabilities_max_tx_power, paws_cfg, antenna_info->gain, curr_device_cfg.x_000295_external_pa_gain)))
	{
		// force the FAP to be disabled
		new_device_cfg.admin_state = 0;
	}
	
	// write the config if necessary
	if (!(process_device_cfg_params(logger, &new_device_cfg, &curr_device_cfg, admin_state_changed, cfg_changed, reboot_needed, spectrum_override->present)))
		return false;

	return true;
}


//#######################################################################################
void paws_reboot_master_device(void)
{
	system_reboot();
}


//#######################################################################################
bool paws_config_master_device_override(void* logger, paws_setting_override_t* spectrum_override, paws_antenna_info_t* antenna_info)
{
	device_cfg_params_t curr_device_cfg;
	device_cfg_params_t new_device_cfg;
	memset(&curr_device_cfg, 0, sizeof(device_cfg_params_t));
	memset(&new_device_cfg, 0, sizeof(device_cfg_params_t));

	bool admin_state_changed, cfg_changed, reboot_needed;
	UNUSED_PARAM(antenna_info);

	if (spectrum_override->present)
	{
		if (!(sql_get_master_cfg(&curr_device_cfg, false)))
			return false;

		// create new config
		new_device_cfg.admin_state = true;
		new_device_cfg.bandwidth = spectrum_override->bandwidth_rb;
		new_device_cfg.earfcndl = spectrum_override->earfcndl;
		new_device_cfg.earfcnul = spectrum_override->earfcnul;
		new_device_cfg.ref_sig_power = spectrum_override->ref_sig_pwr;
		new_device_cfg.pmax = spectrum_override->pmax;
		if (!(limit_and_validate_cfg(logger, &new_device_cfg, curr_device_cfg.capabilities_min_tx_power, curr_device_cfg.capabilities_max_tx_power, antenna_info->gain, (MAX_RADIATED_POWER_DBM * 10), curr_device_cfg.x_000295_external_pa_gain)))
		{
			// force the FAP to be disabled
			new_device_cfg.admin_state = 0;
		}

		// write the config if necessary
		if (!(process_device_cfg_params(logger, &new_device_cfg, &curr_device_cfg, &admin_state_changed, &cfg_changed, &reboot_needed, false)))
			return false;

		if (reboot_needed)
		{
			paws_reboot_master_device();
		}

		return true;
	}

	return false;
}