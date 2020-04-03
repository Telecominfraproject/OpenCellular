/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_DAL_CONTROL_PLANE_H_
#define PAWS_DAL_CONTROL_PLANE_H_

#include <stdint.h>
#include <stdbool.h>

#include "utils/types.h"

typedef struct {
	ip_addr_t	status_ipaddr;
	uint32_t	status_port;
	uint32_t	status_periodicity;
} control_plane_cfg_t;

extern void* paws_dal_control_plane_create(control_plane_cfg_t* cfg);

extern void paws_dal_control_plane_free(void** control_plane);

extern bool paws_dal_control_plane_send_status(void* control_plane, bool status);


#endif // PAWS_DAL_CONTROL_PLANE_H_
