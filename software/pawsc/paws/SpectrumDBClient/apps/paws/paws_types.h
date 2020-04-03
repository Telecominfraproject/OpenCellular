/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_TYPES_
#define PAWS_TYPES_

#include <time.h>

#include "utils/types.h"
#include "paws_dal_devices.h"
#include "paws_dal_settings.h"
#include "paws_dal_state.h"
#include "paws_dal_database.h"


int ul_dl_spec_cfg_freq_compare(ul_dl_spec_cfg_t* p1, ul_dl_spec_cfg_t* p2);
int ul_dl_spec_cfg_pwr_compare(ul_dl_spec_cfg_t* p1, ul_dl_spec_cfg_t* p2);

extern int ul_dl_spec_cfg_compare(ul_dl_spec_cfg_t* p1, ul_dl_spec_cfg_t* p2);

#endif // PAWS_TYPES_

