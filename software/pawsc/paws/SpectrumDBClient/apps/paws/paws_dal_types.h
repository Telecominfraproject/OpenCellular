/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_DAL_TYPES_
#define PAWS_DAL_TYPES_

#include "paws_dal_devices.h"
#include "paws_dal_settings.h"
#include "paws_dal_state.h"
#include "paws_dal_database.h"
#include "paws_dal_control_plane.h"


extern spectrum_schedule_t* spectrum_sched_new(void);
extern spectrum_schedule_t* spectrum_sched_vcopy(spectrum_schedule_t* sched);
extern void spectrum_sched_free(spectrum_schedule_t** sched);

extern spectrum_spec_t* spectrum_spec_new(void);
extern spectrum_spec_t* spectrum_spec_vcopy(spectrum_spec_t* spec);
extern void spectrum_spec_free(spectrum_spec_t** spec);

extern avail_spectrum_t* avail_spectrum_new(void);
extern avail_spectrum_t* avail_spectrum_vcopy(avail_spectrum_t* spec); 
extern void avail_spectrum_free(avail_spectrum_t** avail_spec);

extern spec_cfg_t* spec_cfg_new(void);
extern spec_cfg_t* spec_cfg_vcopy(spec_cfg_t* spec);
extern void spec_cfg_free(spec_cfg_t** cfg);

extern ul_dl_spec_cfg_t* ul_dl_spec_cfg_new(void);
extern ul_dl_spec_cfg_t* ul_dl_spec_cfg_vcopy(ul_dl_spec_cfg_t* cfg);
extern void ul_dl_spec_cfg_free(ul_dl_spec_cfg_t** cfg);

extern sm_state_info_t* sm_state_info_new(void);
extern int sm_state_info_free(void* info_);
extern void sm_state_info_free_and_null(sm_state_info_t** info);

extern paws_sm_state_info_t* paws_sm_state_info_new(void);
extern void paws_sm_state_info_free(paws_sm_state_info_t** info);

#endif // PAWS_DAL_TYPES_


