/*
 *  configHeader.h 
 *
 *  Created on: Aug 3, 2018
 *      Author: vthakur
 */
#ifndef config_header
#define config_header

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERIAL_ID_LENGTH 32

typedef enum {
    OCP_CONFIG_NONE = 0,
    OCP_CONFIG_ID = 1,
    OCP_CONFIG_RTC = 2,
} OCPConfigFuncMap;

typedef struct {
    bool extendedReq;
}__attribute__((packed, aligned(1))) OCPConfigRequest;

typedef struct {
    char serialId[SERIAL_ID_LENGTH];
} __attribute__((packed, aligned(1))) OCPConfigResponse;

#endif
