/*
 *  controlHeader.h 
 *
 *  Created on: Aug 3, 2018
 *      Author: vthakur
 */
#ifndef controlHeader
#define controlHeader

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    OCP_CTRL_NONE = 0,
    OCP_CTRL_DISABLE_ALL_PORT = 1,
    OCP_CTRL_DISABLE_PORT = 2,
    OCP_CTRL_DISABLE_PORT_WITH_TIMER = 3,
    OCP_CTRL_ENABLE_PORT = 4,
    OCP_CTRL_ENABLE_PORT_WITH_TIMER = 5,
    OCP_CTRL_EMABLE_ALL_PORT = 6,
    OCP_CTRL_ENABLE_LOG = 7,
    OCP_CTRL_REBOOT = 8
} OCPCtrlFuncMap;

typedef struct {
    uint8_t action;
    uint8_t port;
    uint32_t timer;

}__attribute__((packed, aligned(1))) OCPControlRequest;

typedef struct {
    uint8_t result;
} __attribute__((packed, aligned(1))) OCPControlResponse;

#endif
