/*
 * customAction_header.h
 *
 *  Created on: Aug 3, 2018
 *      Author: vthakur
 */

#ifndef customAction_header
#define customAction_header

#define MAX_CUSTOM_ACTION 2
typedef struct {
	uint32_t customAction;
}__attribute__((packed, aligned(1))) CustomAction;

#endif
