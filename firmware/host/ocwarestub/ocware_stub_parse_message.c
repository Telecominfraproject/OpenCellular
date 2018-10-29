/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include <occli_common.h>
#include <ocmw_occli_comm.h>
#include <ocware_stub_main_module.h>

/******************************************************************************
 * Function Name    :  ocware_stub_parse_debug_actiontype
 * Description      :  Convert debug actiontype into the SET/GET
 *
 * @param msgFrameData  - output pointer to the OCMPheader field of the message
 *                     from MW (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 *******************************************************************************/
ocware_stub_ret ocware_stub_parse_debug_actiontype(OCMPMessage *msgFrameData)
{
    ocware_stub_ret ret = STUB_SUCCESS;

    if ((debugSetCommand != STUB_FAILED) ||
         (debugGetCommand != STUB_FAILED)) {

        if ((msgFrameData->action) == debugGetCommand) {
            msgFrameData->action = OCMP_AXN_TYPE_GET;
        } else if ((msgFrameData->action) == debugSetCommand) {
            msgFrameData->action = OCMP_AXN_TYPE_SET;
        } else {
            ret = STUB_FAILED;
        }
    } else {
        ret = STUB_FAILED;
    }
    return ret;
}

/******************************************************************************
 * Function Name    :  ocware_stub_handle_post_enable
 * Description      :  Process the post enable message
 *
 * @param msgFrameData  - output pointer to the OCMPheader field of the message
 *                     from MW (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_handle_post_enable(OCMPMessage *msgFrameData)
{
    return STUB_SUCCESS;
}

/******************************************************************************
 * Function Name    : ocware_stub_get_set_params
 * Description      : Function to check if GET/SET operation is to be performed
 *
 * @param   msgFrameData - Pointer to the message frame field of the message
 *                         from MW (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_get_set_params(OCMPMessage *msgFrameData)
{
    ocware_stub_ret ret = STUB_FAILED;

    if (msgFrameData == NULL) {
        return ret;
    }

    switch (msgFrameData->action) {
        case OCMP_AXN_TYPE_GET:
            ret = ocware_stub_get_database(msgFrameData);
            break;

        case OCMP_AXN_TYPE_SET:
            ret = ocware_stub_set_database(msgFrameData);
            break;

        default:
            ret = STUB_FAILED;
    }
    return ret;

}
/******************************************************************************
 * Function Name    : ocware_stub_parse_post_get_message
 * Description      : Parse post messages from MW
 *
 * @param   buffer - output pointer to the message from MW
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_parse_post_get_message(char *buffer)
{
    ocware_stub_ret ret = STUB_FAILED;
    OCMPMessageFrame *msgFrame = NULL;
    OCMPMessage *msgFrameData = NULL;
    uint16_t paramId;
    char *payload = NULL;

    if(buffer == NULL) {
        return ret;
    }
    msgFrame = (OCMPMessageFrame *)buffer;
    msgFrameData = (OCMPMessage*)&msgFrame->message;
    payload = (char *)&msgFrameData->info;
    msgFrameData->action = OCMP_AXN_TYPE_REPLY;
    paramId = msgFrameData->parameters;

    if(paramId == PostResult) {
        ret = ocware_stub_get_post_database(msgFrameData, payload);
    } else if (paramId == PostEnable) {
        ret = ocware_stub_handle_post_enable(msgFrameData);
    }
    return ret;
}
