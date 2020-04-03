/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_MASTER_SM_TYPES_H_
#define PAWS_MASTER_SM_TYPES_H_

#include "paws_sm_types.h"

typedef struct {
	int(*run_tick)(void* sm_);
} paws_master_public_funcs_t;

typedef struct {
	int			dummy;			// unused
} paws_master_private_data_t;

typedef struct {
	paws_sm_header_t			paws_sm_hdr;			// THIS MUST BE FIRST IN ANY SM WHICH HAS A PAWS_SM.    
	paws_sm_funcs_t				paws_sm_func_store;
	paws_master_private_data_t	private_data_store;
	paws_master_public_funcs_t	public_func_store;

	paws_sm_t*					paws_sm;
} paws_master_sm_t;



#endif // #define PAWS_MASTER_SM_TYPES_H_
