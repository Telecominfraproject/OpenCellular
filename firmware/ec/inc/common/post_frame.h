/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef POST_FRAME_H_
#define POST_FRAME_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include <stdint.h>

/*****************************************************************************
 *                          STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
typedef enum {
    POST_DEV_FOUND = 0,
    POST_DEV_MISSING,
    POST_DEV_ID_MISMATCH,
    POST_DEV_CFG_DONE,
    POST_DEV_CFG_FAIL,
    POST_DEV_FAULTY,
    POST_DEV_CRITICAL_FAULT
} ePostCode;

typedef struct __attribute__((packed, aligned(1))) {
    uint8_t subsystem;
    uint8_t devSno;
    uint8_t i2cBus;
    uint8_t devAddr;
    uint16_t devId;
    uint16_t manId;
    uint8_t status;
} POSTData;

#endif /* POST_FRAME_H_ */
