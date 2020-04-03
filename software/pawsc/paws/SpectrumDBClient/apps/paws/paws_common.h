/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_COMMON_H_
#define PAWS_COMMON_H_


#include "paws_sm.h"
#include "utils/utils.h"

// common macros used by "users" of paws_sm

#define POPULATE_PAWS_SM_FUNC(sm,fnc)	sm->paws_sm_hdr.funcs->fnc = fnc;
#define POPULATE_PUBLIC_FUNC(sm,fnc)	sm->public_func_store.fnc = fnc;

#define PAWS_SM_FUNC(sm,fnc)			(sm)->paws_sm->local_funcs.fnc				// This is used to directly call a paws_sm function even though it might have overridden by this class
#define LOCAL_FUNC(sm,fnc)				((paws_sm_t*)sm)->paws_sm_hdr.funcs->fnc	// This is used to call a paws_sm function which "might have" been overridden by this class
#define PUBLIC_FUNC(sm,fnc)				(sm)->public_func_store.fnc 
#define COMBINER_PUBLIC_FUNC(sm,fnc)    PUBLIC_FUNC((paws_combiner_sm_t*)sm, fnc)
#define MASTER_PUBLIC_FUNC(sm,fnc)      PUBLIC_FUNC((paws_master_sm_t*)sm, fnc)
#define GOP_PUBLIC_FUNC(sm,fnc)			PUBLIC_FUNC((paws_gop_slave_sm_t*)sm, fnc)
#define SOP_PUBLIC_FUNC(sm,fnc)			PUBLIC_FUNC((paws_sop_slave_sm_t*)sm, fnc)

#define GOP_FUNC(sm, fnc)				(sm)->gop_slave_hdr.funcs->fnc
#define SOP_FUNC(sm, fnc)				(sm)->sop_slave_hdr.funcs->fnc

#define PAWS_SM_DATA(sm)				((paws_sm_t*)sm)->paws_sm_hdr.data
#define PAWS_SM_STL_DATA(sm)			((paws_sm_t*)sm)->paws_sm_hdr.stl_data
#define PRIVATE_DATA(sm)				(sm)->private_data_store



#define CREATOR_SM(sm_)					((paws_sm_t*)sm_)->paws_sm_hdr.creator


#define PAWS_LOG_TYPE					"PAWS"
#define CLOUD_LOGGER_TVWSDB_MSG			"TVWSDB-MSG"
#define CLOUD_LOGGER_DEVICE_CFG			"DEVICE-CFG"


#endif // #define PAWS_COMMON_H_