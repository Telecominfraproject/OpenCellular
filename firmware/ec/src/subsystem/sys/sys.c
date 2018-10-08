/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "inc/subsystem/sys/sys.h"

#include "common/inc/global/Framework.h"
#include "inc/common/post.h"
#include "inc/devices/eeprom.h"
#include "inc/subsystem/gpp/gpp.h" /* For resetting AP */

#include <driverlib/flash.h>
#include <driverlib/sysctl.h>

#include <stdio.h>
#include <string.h>

#define OC_MAC_ADDRESS_SIZE 13

extern POSTData PostResult[POST_RECORDS];

typedef enum { OC_SYS_CONF_MAC_ADDRESS = 0 } eOCConfigParamId;

/* Resets the AP and then the EC */
bool SYS_cmdReset(void *driver, void *params)
{
    /* TODO: we don't give any indication that the message was received, perhaps
     * a timed shutdown would be more appropriate */

    const Gpp_gpioCfg *cfg = (Gpp_gpioCfg *)driver;

    /* Reset AP */
    OcGpio_write(&cfg->pin_ec_reset_to_proc, false);
    Task_sleep(100);
    OcGpio_write(&cfg->pin_ec_reset_to_proc, true);
    Task_sleep(100);

    /* EC Software Reset */
    SysCtlReset();

    /* We'll never reach here, but keeps the compiler happy */
    return false;
}

/* Simply returns true to let us know the system is alive */
bool SYS_cmdEcho(void *driver, void *params)
{
    return true;
}

/*
/*****************************************************************************
 **    FUNCTION NAME   : SYS_post_enable
 **
 **    DESCRIPTION     : Start the executing for POST
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : bool
 **
 *****************************************************************************/
bool SYS_post_enable(void **postActivate)
{
    ReturnStatus status = RETURN_NOTOK;
    LOGGER("SYS:INFO:: Starting POST test for OpenCellular.\n");
    //Permission granted from the System.
    //Sending the activate POST message to POST subsystem.
    OCMPMessageFrame *postExeMsg = create_ocmp_msg_frame(
            OC_SS_SYS, OCMP_MSG_TYPE_POST, OCMP_AXN_TYPE_ACTIVE, 0x00, 0x00, 1);
    if (postExeMsg != NULL) {
        status = RETURN_OK;
        *postActivate = (OCMPMessageFrame *)postExeMsg;
    }
    return (status == RETURN_OK);
}

/*****************************************************************************
 **    FUNCTION NAME   : SYS_post_get_results
 **
 **    DESCRIPTION     : Get POST results from EEPROM.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : bool
 **
 *****************************************************************************/
bool SYS_post_get_results(void **getpostResult)
{
    ReturnStatus status = RETURN_OK;
    /* Return the POST results*/
    uint8_t iter = 0x00;
    uint8_t index = 0x00;
    OCMPMessageFrame *getpostResultMsg = (OCMPMessageFrame *)getpostResult;
    /* Get the subsystem info for which message is required */
    OCMPMessageFrame *postResultMsg = create_ocmp_msg_frame(
            getpostResultMsg->message.subsystem, OCMP_MSG_TYPE_POST,
            OCMP_AXN_TYPE_REPLY, 0x00, 0x00, 40);
    if (postResultMsg) {
        /* Getting data assigned*/
        postResultMsg->header.ocmpSof = getpostResultMsg->header.ocmpSof;
        postResultMsg->header.ocmpInterface =
                getpostResultMsg->header.ocmpInterface;
        postResultMsg->header.ocmpSeqNumber =
                getpostResultMsg->header.ocmpSeqNumber;
        for (iter = 0; iter < POST_RECORDS; iter++) {
            if (PostResult[iter].subsystem ==
                getpostResultMsg->message.ocmp_data[0]) {
                postResultMsg->message.ocmp_data[(3 * index) + 0] =
                        PostResult[iter].subsystem;
                postResultMsg->message.ocmp_data[(3 * index) + 1] =
                        PostResult[iter].devSno; //Device serial Number
                postResultMsg->message.ocmp_data[(3 * index) + 2] =
                        PostResult[iter].status; //Status ok
                index++;
            }
        }
        LOGGER_DEBUG(
                "BIGBROTHER:INFO::POST message sent for subsystem 0x%x.\n");
        /*Size of payload*/
        postResultMsg->header.ocmpFrameLen = index * 3;
        /*Updating Subsystem*/
        //postResultMsg->message.subsystem = (OCMPSubsystem)PostResult[iter].subsystem;
        /* Number of devices found under subsystem*/
        postResultMsg->message.parameters = index;
        index = 0;
    } else {
        LOGGER("BIGBROTHER:ERROR:: Failed to allocate memory for POST results.\n");
    }
    memcpy(((OCMPMessageFrame *)getpostResult), postResultMsg, 64);
    return status;
}
