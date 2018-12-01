/**
 * Copyright (c) 2018-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef G510_H_
#define G510_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "common/inc/global/post_frame.h"
#include "inc/subsystem/testModule/testModule.h"

/*****************************************************************************
 *                              STRUCT DEFINITIONS
 *****************************************************************************/


/*****************************************************************************
 *                             FUNCTION DECLARATIONS
 *****************************************************************************/
ePostCode g510_task_init(void *driver, const void *config,
                         const void *alert_token);
bool g510_get_imei(TestMod_2G_Status_Data *p2gStatusData);
bool g510_get_imsi(TestMod_2G_Status_Data *p2gStatusData);
bool g510_get_mfg(TestMod_2G_Status_Data *p2gStatusData);
bool g510_get_model(TestMod_2G_Status_Data *p2gStatusData);
bool g510_get_rssi(TestMod_2G_Status_Data *p2gStatusData);
bool g510_get_ber(TestMod_2G_Status_Data *p2gStatusData);
bool g510_get_regStatus(TestMod_2G_Status_Data *p2gStatusData);
bool g510_get_cellId(TestMod_2G_Status_Data *p2gStatusData);
#endif /* G510_H_ */
