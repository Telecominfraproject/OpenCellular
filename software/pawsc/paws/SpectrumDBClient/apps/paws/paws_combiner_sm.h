/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_SM_COMBINER_H_
#define PAWS_SM_COMBINER_H_

#include "paws_combiner_sm_types.h"

extern paws_combiner_sm_t* paws_combiner_sm_create(const char* sm_name, uint32_t control_plane_status_port, uint32_t control_plane_status_periodicity);

extern void paws_combiner_sm_free(paws_combiner_sm_t** paws_combiner_sm);

#endif // #define PAWS_SM_COMBINER_H_


