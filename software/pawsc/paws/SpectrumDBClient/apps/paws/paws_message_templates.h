/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_MESSAGE_TEMPLATES_H_
#define PAWS_MESSAGE_TEMPLATES_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "json-parser/json_utils.h"



//#######################################################################################
json_value* get_init_req_template(void* sm_);
json_value* get_available_spectrum_req_template(void* sm_);
json_value* get_slave_gop_available_spectrum_req_template(void* sm_);
json_value* get_slave_sop_available_spectrum_req_template(void* sm_);
json_value* get_spectrum_use_notify_req_template(void* sm_);
json_value* get_gop_slave_spectrum_use_notify_req_template(void* sm_);
json_value* get_sop_slave_spectrum_use_notify_req_template(void* sm_);


#endif // PAWS_MESSAGE_TEMPLATES_H_


