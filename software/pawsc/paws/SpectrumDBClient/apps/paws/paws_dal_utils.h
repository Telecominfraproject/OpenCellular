/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_DAL_UTILS_H_
#define PAWS_DAL_UTILS_H_

#include "json-parser/json.h"

#include "paws_dal_types.h"


extern char* get_init_paws_dal_encode_str(void);

extern char* paws_sm_state_info_2_jsonstr(paws_sm_state_info_t* paws_sm_state_info);
extern char* paws_db_info_2_jsonstr(paws_db_info_t* db_info);
extern bool json_2_paws_db_item(json_value* jval, paws_db_item_t* db_item);

extern paws_sm_state_info_t* json2_paws_sm_state_info(json_value* jval);

#endif


