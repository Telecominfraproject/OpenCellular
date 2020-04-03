/*
Copyright(c) Microsoft Corporation.All rights reserved.
Licensed under the MIT License.
*/

#include <stdint.h>
#include <stdbool.h>

#ifndef LTE_DB_INFO_H_
#define LTE_DB_INFO_H_

#include "utils/types.h"

extern bool lte_db_set_location(char* db);

extern char* lte_db_get_location(void);

extern bool lte_db_get_enb_info(int *port, device_name_t device_name, device_id_t *device_id);

extern bool lte_db_get_RxTxStatus(int *tx_status);


#endif // LTE_DB_INFO_H_
