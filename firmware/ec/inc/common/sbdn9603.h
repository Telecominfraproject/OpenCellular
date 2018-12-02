/**
 * Copyright (c) 2018-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef SBDN9603_H_
#define SBDN9603_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "inc/subsystem/obc/obc.h"

/*****************************************************************************
 *                             FUNCTION DECLARATIONS
 *****************************************************************************/
bool sbd9603_get_imei(OBC_Iridium_Status_Data *pIridiumStatusData);
bool sbd9603_get_lastError(OBC_Iridium_Status_Data *pIridiumStatusData);
bool sbd9603_get_mfg(OBC_Iridium_Status_Data *pIridiumStatusData);
bool sbd9603_get_model(OBC_Iridium_Status_Data *pIridiumStatusData);
bool sbd9603_get_queueLength(OBC_Iridium_Status_Data *pIridiumStatusData);
bool sbd9603_get_regStatus(OBC_Iridium_Status_Data *pIridiumStatusData);
bool sbd9603_get_signalqual(OBC_Iridium_Status_Data *pIridiumStatusData);
ReturnStatus sbd_init(const Iridium_Cfg *iridium);

#endif /* SBDN9603_H_ */
