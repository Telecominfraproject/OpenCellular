/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "utils.h"

//#######################################################################################
void free_and_null(void** data) 
{
	if ((data) && (*data)) 
	{
		free(*data);
		*data = NULL;
	}
}



//#######################################################################################
// find position of hexstring substr inside hexstring src.  Returns start address.
uint8_t* hexstrstr(uint8_t* src, uint32_t src_len, uint8_t* substr, uint32_t substr_len)
{
	if ((!src) || (!substr))
		return NULL;

	if (src_len < substr_len)
		return NULL;

	uint16_t i = 0;
	for (i = 0; i < (src_len - substr_len) + 1; i++)
	{
		if ((memcmp(&src[i], substr, substr_len)) == 0)
			return &src[i];
	}
	return NULL;
}





