/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
//*****************************************************************************
//                                HEADER FILES
/* Board Header files */
#include "common/inc/global/Framework.h"
#include "inc/subsystem/power/power.h"
#include "inc/common/post.h"

#define SUBSYTEM_CHECK (PostResult[iter].subsystem <= getpostResultMsg->message.ocmp_data[0])
extern POSTData PostResult[POST_RECORDS];

/*****************************************************************************
 **    FUNCTION NAME   : psuCore_pre_init
 **
 **    DESCRIPTION     : Get POST results from EEPROM.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : bool
 **
 *****************************************************************************/
bool psuCore_pre_init(void *driver, void *returnValue)
{
    //Configuring GPIOS
    PWRSRC_Dev *gpioCfg = (PWRSRC_Dev *)driver;

    OcGpio_configure(&gpioCfg->cfg.pin_dc_present, OCGPIO_CFG_INPUT);
    OcGpio_configure(&gpioCfg->cfg.pin_poe_prsnt_n, OCGPIO_CFG_INPUT);
    OcGpio_configure(&gpioCfg->cfg.pin_int_bat_prsnt, OCGPIO_CFG_INPUT);
    OcGpio_configure(&gpioCfg->cfg.pin_disable_dc_input, OCGPIO_CFG_OUTPUT);
    OcGpio_configure(&gpioCfg->cfg.pin_dc_input_fault, OCGPIO_CFG_INPUT);
    OcGpio_configure(&gpioCfg->cfg.pin_oc_input_present, OCGPIO_CFG_INPUT);
    OcGpio_configure(&gpioCfg->cfg.pin_power_off, OCGPIO_CFG_OUTPUT);
    OcGpio_write(&gpioCfg->cfg.pin_disable_dc_input, false);
    OcGpio_write(&gpioCfg->cfg.pin_power_off, false);

    return true;
}

/*****************************************************************************
 **    FUNCTION NAME   : PWR_post_get_results
 **
 **    DESCRIPTION     : Get POST results from EEPROM.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : bool
 **
 *****************************************************************************/
bool PWR_post_get_results(void **getpostResult)
{
    ReturnStatus status = RETURN_OK;
    /* Return the POST results*/
    uint8_t iter = 0x00;
    uint8_t index = 0x00;
    OCMPMessageFrame *getpostResultMsg = (OCMPMessageFrame *)getpostResult;
    /* Get the subsystem info for which message is required */
    OCMPMessageFrame *postResultMsg = create_ocmp_msg_frame(
            getpostResultMsg->message.subsystem, OCMP_MSG_TYPE_POST,
            0x05,0x00,0x00,40);
    if (postResultMsg) {
        /* Getting data assigned*/
        postResultMsg->header.ocmpSof = getpostResultMsg->header.ocmpSof;
        postResultMsg->header.ocmpInterface = getpostResultMsg->header
                                                .ocmpInterface;
        postResultMsg->header.ocmpSeqNumber = getpostResultMsg->header
                                                .ocmpSeqNumber;
        for (iter = 0; SUBSYTEM_CHECK; iter++) {
            if (PostResult[iter].subsystem
                    == getpostResultMsg->message.ocmp_data[0]) {
                postResultMsg->message.ocmp_data[(3 * index) + 0] =
                                        PostResult[iter].subsystem;
                postResultMsg->message.ocmp_data[(3 * index) + 1] =
                        PostResult[iter].devSno; //Device serial Number
                postResultMsg->message.ocmp_data[(3 * index) + 2] =
                        PostResult[iter].status; //Status ok
                index++;
            }
        }
        LOGGER_DEBUG("POWER:INFO::POST message sent for subsystem 0x%x.\n");
        /*Size of payload*/
        postResultMsg->header.ocmpFrameLen = index * 3;
        /*Updating Subsystem*/
        //postResultMsg->message.subsystem = (OCMPSubsystem)PostResult[iter].subsystem;
        /* Number of devices found under subsystem*/
        postResultMsg->message.parameters = index;
        index = 0;
    } else {
        LOGGER("POWER:ERROR:: Failed to allocate memory for POST results.\n");
    }
    memcpy(((OCMPMessageFrame*)getpostResult), postResultMsg, 64);
    return status;
}
