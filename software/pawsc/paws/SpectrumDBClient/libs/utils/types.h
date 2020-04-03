/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef TYPES_
#define TYPES_

#include <stdint.h>
#include <stdbool.h>

typedef union
{
	int8_t		s8;
	int16_t		s16;
	int32_t		s32;
	uint8_t		u8;
	uint16_t	u16;
	uint32_t	u32;
	void*		p;
} anytype_u;

typedef enum {
	LTE_UL = 0,
	LTE_DL
} lte_direction_e;

#define MAX_ADDR_STR	    	(200)
typedef char host_addr_t[MAX_ADDR_STR];

#define MAX_IP_ADDR_STR	    	(41)
typedef char ip_addr_t[MAX_IP_ADDR_STR];

#define MAX_FILENAME_LEN		(100)
typedef char filename_t[MAX_FILENAME_LEN];

#define MAX_DEVICE_NAME_LEN		(30)
typedef char device_name_t[MAX_DEVICE_NAME_LEN];

typedef uint16_t device_id_t;


#endif // TYPES_


