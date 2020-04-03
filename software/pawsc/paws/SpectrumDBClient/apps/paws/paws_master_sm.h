/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_SM_MASTER_H_
#define PAWS_SM_MASTER_H_

#include "paws_master_sm_types.h"

extern paws_master_sm_t* paws_master_sm_create(void* creator, const char* sm_name, paws_device_info_t* master_info, paws_gps_location_t* gps, float min_dbm_100k, uint16_t db_retry_secs, 
	logger_cfg_t* msg_log_cfg, uint32_t max_polling_quick_secs, sm_state_info_t* sm_state_info);
extern void paws_master_sm_free(paws_master_sm_t** paws_master_sm);

#endif // #define PAWS_SM_MASTER_H_
