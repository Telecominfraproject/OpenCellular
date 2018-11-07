/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef INC_UTILS_OCMP_UTIL_H_
#define INC_UTILS_OCMP_UTIL_H_

#include "common/inc/global/ocmp_frame.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/*****************************************************************************
 **    FUNCTION NAME   : OCMP_mallocFrame
 **
 **    DESCRIPTION     : Allocates memory for OCMP packets.
 **
 **    ARGUMENTS       : length
 **
 **    RETURN TYPE     : OCMPMessageFrame
 **
 *****************************************************************************/

OCMPMessageFrame *OCMP_mallocFrame(uint16_t len);

/*****************************************************************************
 **    FUNCTION NAME   : create_ocmp_msg_frame
 **
 **    DESCRIPTION     : Create a OCMP message.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : OCMPMessageFrame
 **
 *****************************************************************************/
OCMPMessageFrame *
create_ocmp_msg_frame(OCMPSubsystem subSystem, OCMPMsgType msgtype,
                      OCMPActionType actionType, uint8_t componentId,
                      uint16_t parameters, uint8_t payloadSize);

/*****************************************************************************
 **    FUNCTION NAME   : create_ocmp_alert_from_Evt
 **
 **    DESCRIPTION     : Create the OCMP Alert frame from the Event message.
 **
 **    ARGUMENTS       : OCMPMessageFrame to be used to create Alert,
 **                      ComponentId,
 **                      ParemeterID
 **
 **    RETURN TYPE     : OCMPMessageFrame
 **
 *****************************************************************************/
OCMPMessageFrame *create_ocmp_alert_from_Evt(OCMPMessageFrame *ocmpEventMsg,
                                             uint8_t componentId,
                                             uint16_t parameters);

#endif /* INC_UTILS_OCMP_UTIL_H_ */
