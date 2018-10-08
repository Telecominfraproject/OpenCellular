/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

/*****************************************************************************
 *                                HEADER FILES
 *****************************************************************************/
#include "inc/utils/ocmp_util.h"

/*****************************************************************************
 **    FUNCTION NAME   : OCMP_mallocFrame
 **
 **    DESCRIPTION     : API to allocate an OCMP frame of a given data length
 **
 **    ARGUMENTS       : size of the payload of frame
 **
 **    RETURN TYPE     : OCMPMessageFrame
 **
 *****************************************************************************/
OCMPMessageFrame *OCMP_mallocFrame(uint16_t len)
{
    OCMPMessageFrame *pMsg;
    // Allocate memory for NPI Frame
    pMsg = (OCMPMessageFrame *)malloc(sizeof(OCMPMessageFrame) + len);
    if (pMsg != NULL) {
        // Assign Data Length of Frame
        pMsg->header.ocmpFrameLen = len;
        // Assign pData to first byte of payload
        // Pointer arithmetic of + 1 is equal to sizeof(OCMPMessageFrame) bytes
        // then cast to unsigned char * for pData
        //pMsg->message.ocmp_data = (unsigned char *)(pMsg + 1);
        //pMsg->message.ocmp_data = (unsigned char *)(pMsg + 2);
    }
    return pMsg;
}

/*****************************************************************************
 **    FUNCTION NAME   : create_ocmp_msg_frame
 **
 **    DESCRIPTION     : Create a OCMP message.
 **
 **    ARGUMENTS       : OCMPSubsystem subSystem,
 **                      OCMPMsgType msgtype,
 **                      OCMPActionType actionType,
 **                      ComponentId,
 **                      ParemeterID,
 **                      Payload size
 **
 **    RETURN TYPE     : OCMPMessageFrame
 **
 *****************************************************************************/
OCMPMessageFrame *
create_ocmp_msg_frame(OCMPSubsystem subSystem, OCMPMsgType msgtype,
                      OCMPActionType actionType, uint8_t componentId,
                      uint16_t parameters, uint8_t payloadSize)
{
    OCMPMessageFrame *ocmp_msg =
            (OCMPMessageFrame *)OCMP_mallocFrame(payloadSize);
    if (ocmp_msg) {
        *ocmp_msg = (OCMPMessageFrame){
            .header =
                    {
                            .ocmpSof = OCMP_MSG_SOF,
                            .ocmpInterface = OCMP_COMM_IFACE_USB,
                            .ocmpFrameLen = payloadSize,
                            //.ocmp_seqNumber = 0x00;
                            //.ocmp_timestamp = 0x00; //Get RTC TimeStamp
                    },
            .message =
                    {
                            .subsystem = subSystem,
                            .componentID = componentId,
                            .parameters = parameters,
                            .msgtype = msgtype,
                            .action = actionType,
                    }
        };
        memset(&(ocmp_msg->message.ocmp_data[0]), 0x00, payloadSize);
    }
    return ocmp_msg;
}

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
                                             uint16_t parameters)
{
    OCMPMessageFrame *ocmpAlertMsg = (OCMPMessageFrame *)OCMP_mallocFrame(1);
    if (ocmpAlertMsg != NULL) {
        memset(ocmpAlertMsg, 0x00, (sizeof(OCMPMessageFrame)));
        memcpy(ocmpAlertMsg, ocmpEventMsg, (sizeof(OCMPMessageFrame)) + 1);
        ocmpAlertMsg->message.msgtype = OCMP_MSG_TYPE_ALERT;
        ocmpAlertMsg->message.componentID = componentId;
        ocmpAlertMsg->message.parameters = parameters;
    }
    return ocmpAlertMsg;
}
