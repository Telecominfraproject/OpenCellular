/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_SM_SLAVE_H_
#define PAWS_SM_SLAVE_H_

#include "paws_slave_sm_types.h"

extern paws_gop_slave_sm_t* paws_gop_slave_sm_create(void* creator, char* sm_name, paws_device_info_t* master_info, paws_gps_location_t* gps, float default_max_location_change, uint32_t default_max_polling_secs,
	paws_slave_info_t *slave_info, float min_dbm_100k, uint16_t db_retry_secs, logger_cfg_t* msg_log_cfg, uint32_t max_polling_quick_secs, sm_state_info_t* sm_state_info);
extern void paws_gop_slave_sm_free(paws_gop_slave_sm_t** paws_gop_slave_sm);

extern paws_sop_slave_sm_t* paws_sop_slave_sm_create(void* creator, char* sm_name, paws_device_info_t* master_info, paws_gps_location_t* gps, float default_max_location_change, uint32_t default_max_polling_secs,
	paws_device_info_t *device_info, float min_dbm_100k, uint16_t db_retry_secs, logger_cfg_t* msg_log_cfg, uint32_t max_polling_quick_secs, sm_state_info_t* sm_state_info);
extern void paws_sop_slave_sm_free(paws_sop_slave_sm_t** paws_sop_slave_sm);

#endif // #define PAWS_SM_SLAVE_H_
