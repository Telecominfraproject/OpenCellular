/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "common/inc/global/Framework.h"
#include <driverlib/sysctl.h>
#include "helpers/memory.h"
#include "inc/common/bigbrother.h"
#include "inc/common/global_header.h"
#include "inc/common/post.h"
#include "inc/devices/eeprom.h"
#include "inc/subsystem/gpp/gpp.h" /* For resetting AP */
#include "inc/subsystem/sys/sys.h"
#include "inc/utils/ocmp_util.h"
#include "src/filesystem/fs_wrapper.h"
#include <driverlib/flash.h>
#include <driverlib/sysctl.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/System.h>
#include <xdc/std.h>

#define OCFS_TASK_PRIORITY 1
#define OCFS_TASK_STACK_SIZE 4096

Task_Struct ocFSTask;
Char ocFSTaskStack[OCFS_TASK_STACK_SIZE];

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
    // Permission granted from the System.
    // Sending the activate POST message to POST subsystem.
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
                    PostResult[iter].devSno; // Device serial Number
                postResultMsg->message.ocmp_data[(3 * index) + 2] =
                    PostResult[iter].status; // Status ok
                index++;
            }
        }
        LOGGER_DEBUG(
            "BIGBROTHER:INFO::POST message sent for subsystem 0x%x.\n");
        /*Size of payload*/
        postResultMsg->header.ocmpFrameLen = index * 3;
        /*Updating Subsystem*/
        // postResultMsg->message.subsystem =
        // (OCMPSubsystem)PostResult[iter].subsystem;
        /* Number of devices found under subsystem*/
        postResultMsg->message.parameters = index;
        index = 0;
    } else {
        LOGGER(
            "BIGBROTHER:ERROR:: Failed to allocate memory for POST results.\n");
    }
    memcpy(((OCMPMessageFrame *)getpostResult), postResultMsg, 64);
    SysCtlDelay(100);
    if(postResultMsg) {
        /* Free memory which was allocated for post frame.*/
        free(postResultMsg);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : sys_post_init
 **
 **    DESCRIPTION     : Create the task for file system
 **
 **    ARGUMENTS       : SPI driver configuration, return value
 **
 **    RETURN TYPE     : bool
 **
 *****************************************************************************/
bool sys_post_init(void *driver, void *returnValue)
{
    Semaphore_construct(&semFSstruct, 0, NULL);
    semFilesysMsg = Semaphore_handle(&semFSstruct);
    if (!semFilesysMsg) {
        LOGGER_DEBUG("FS:ERROR:: Failed in Creating Semaphore");
        return false;
    }
    Semaphore_construct(&semFSreadStruct, 0, NULL);
    semFSreadMsg = Semaphore_handle(&semFSreadStruct);
    if (!semFSreadMsg) {
        LOGGER_DEBUG("FS:ERROR:: Failed in Creating Semaphore");
        return false;
    }

    Semaphore_construct(&semFSwriteStruct, 0, NULL);
    semFSwriteMsg = Semaphore_handle(&semFSwriteStruct);
    if (!semFSwriteMsg) {
        LOGGER_DEBUG("FS:ERROR:: Failed in Creating Semaphore");
        return false;
    }

    /* Create Message Queue for RX Messages */
    fsTxMsgQueue = Util_constructQueue(&fsTxMsg);
    if (!fsTxMsgQueue) {
        LOGGER_ERROR("FS:ERROR:: Failed in Constructing Message Queue for");
        return false;
    }
    fsRxMsgQueue = Util_constructQueue(&fsRxMsg);
    if (!fsRxMsgQueue) {
        LOGGER_ERROR("FS:ERROR:: Failed in Constructing Message Queue for");
        return false;
    }

    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stackSize = OCFS_TASK_STACK_SIZE;
    taskParams.stack = &ocFSTaskStack;
    taskParams.instance->name = "FileSys_t";
    taskParams.priority = OCFS_TASK_PRIORITY;
    taskParams.arg0 = (UArg)driver;
    taskParams.arg1 = (UArg)returnValue;
    Util_create_task(&taskParams, &fs_wrapper_fileSystem_init, true);
    //Task_construct(&ocFSTask, fs_wrapper_fileSystem_init, &taskParams, NULL);
    LOGGER_DEBUG("FS:INFO:: Creating filesystem task function.\n");
    return true;
}
