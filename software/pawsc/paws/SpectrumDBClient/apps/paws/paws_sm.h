/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_SM_H_
#define PAWS_SM_H_

#include "paws_sm_types.h"



// funntions
extern paws_sm_t* paws_sm_create(void* creator, paws_sm_funcs_t* child_funcs, const char* sm_name);
extern void paws_sm_free(paws_sm_t** paws_sm);

#endif // #define PAWS_SM_H_


