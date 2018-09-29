/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef SYSTEM_STATES_H_
#define SYSTEM_STATES_H_

/*****************************************************************************
 *                           ENUM DEFINITIONS
 *****************************************************************************/
typedef enum {
    SS_STATE_PWRON = 1,
    SS_STATE_INIT,
    SS_STATE_CFG,
    SS_STATE_RDY,
    SS_STATE_ACTIVE,
    SS_STATE_FAULTY,
    SS_STATE_RESET,
    SS_STATE_SHUTDOWN
} eSubSystemStates;

typedef enum {
    OC_STATE_PWRON = 1,
    OC_STATE_POST,
    OC_STATE_RDY,
    OC_STATE_CONFIGURED,
    OC_STATE_RF_ACTIVE,
    OC_STATE_FAULTY,
    OC_STATE_DEGRADED,
    OC_STATE_LOST_SYNC,
    OC_STATE_UPGRADING,
    OC_STATE_RF_FAILURE,
    OC_STATE_ETH_FAILURE
} eOCSystemState;

#endif /* SYSTEM_STATES_H_ */
