/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include <ocware_stub_main_module.h>

extern int16_t allAlertCount;
/******************************************************************************
 * Function Name    : ocware_stub_frame_newmsgframe
 * Description      : Frame the response packet from stub function
 *
 * @param buf - pointer to complete message from MW (by reference)
 * @param option  -  success or failure (by value)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_frame_newmsgframe(char *buffer, int32_t option,
                                              uint8_t *alertFlag)
{
    int32_t ret = 0, index = 0;
    OCMPMessageFrame *msgFrame = (OCMPMessageFrame *)buffer;
    OCMPMessage *msgFrameData = (OCMPMessage *)&msgFrame->message;

    /* Frame the header packet for sending data to ec */
    msgFrame->header.ocmpFrameLen = 0;
    msgFrame->header.ocmpInterface = OCMP_COMM_IFACE_ETHERNET;
    msgFrame->header.ocmpSeqNumber = 0;
    msgFrame->header.ocmpSof = OCMP_MSG_SOF;
    msgFrame->header.ocmpTimestamp = 0;

    switch (msgFrameData->msgtype) {
        case OCMP_MSG_TYPE_COMMAND:
            ret = ocware_stub_parse_command_message(buffer, alertFlag);
            break;

        case OCMP_MSG_TYPE_POST:
            ret = ocware_stub_parse_post_get_message(buffer);
            break;
        default:
            ret = ocware_stub_get_set_params(msgFrameData);
    }

    if (ret == STUB_SUCCESS && (msgFrameData->msgtype != OCMP_MSG_TYPE_ALERT)) {
        /* Setting the action type as REPLY */
        msgFrame->message.action = OCMP_AXN_TYPE_REPLY;
        printf(" \n Sending Data :\n");
        for (index = 0; index < OC_EC_MSG_SIZE; index++) {
            printf("0x%x  ", buffer[index] & 0xff);
        }
    }
    return ret;
}
/******************************************************************************
 * Function Name    : ocware_stub_validate_msgframe_header
 * Description      : validate the msgframe header
 *
 * @param buf - pointer to complete message from MW (by reference)
 * @param msgFrameData  - pointer to ocmpgeader field of the message from
 *                         MW (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_validate_msgframe_header(char *buf,
                                                     OCMPMessage *msgFrameData)
{
    OCMPMessageFrame *MsgFrame = (OCMPMessageFrame *)buf;
    OCMPHeader *header = (OCMPHeader *)&MsgFrame->header;

    if (header->ocmpFrameLen == 0) {
        if (header->ocmpSeqNumber == 0) {
            if (header->ocmpSof == OCMP_MSG_SOF) {
                if (header->ocmpTimestamp == 0) {
                    memcpy(msgFrameData, &MsgFrame->message,
                           sizeof(OCMPMessage));
                    return STUB_SUCCESS;
                }
            }
        }
    }
    return STUB_FAILED;
}
/******************************************************************************
 * Function Name    : host_ocstubmain_func
 * Description      : Handle all the functionality for stub
 *
 * @param argc - number of argunents passed (by value)
 * @param argv  - arguments passed when invoking stub (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
int32_t host_ocstubmain_func(int32_t argc, char *argv[])
{
    int32_t ret = 0;
    char *buffer = NULL;
    int32_t loopCount = 0;
    char *rootpath = NULL;
    uint8_t alertFlag = 0;
    OCMPMessageFrame *msgFrame = NULL;
    ret = ocware_stub_init_ethernet_comm();
    if (ret != STUB_SUCCESS) {
        printf("\n ERROR: Init Failed - %d", __LINE__);
        ocware_stub_deinit_ethernet_comm();
        return STUB_FAILED;
    }

    msgFrame = (OCMPMessageFrame *)malloc(sizeof(OCMPMessageFrame));
    if (msgFrame == NULL) {
        printf("\n ERROR: malloc");
        ocware_stub_deinit_ethernet_comm();
        return STUB_FAILED;
    }

    buffer = (char *)malloc(OC_EC_MSG_SIZE);
    if (buffer == NULL) {
        printf("\n ERROR: malloc");
        ocware_stub_deinit_ethernet_comm();
        return STUB_FAILED;
    }

    rootpath = (char *)malloc(100);
    if (rootpath == NULL) {
        printf("\n ERROR: malloc");
        ocware_stub_deinit_ethernet_comm();
        return STUB_FAILED;
    }

    ocware_stub_init_database();

    printf("\n\n OCWARE STUB APllication Started...\n");

    while (1) {
        memset(buffer, 0, sizeof(OCMPMessageFrame));
        ret = ocware_stub_recv_msgfrom_middleware(&buffer,
                                                  sizeof(OCMPMessageFrame));
        if (ret != STUB_SUCCESS) {
            printf("ocware_stub_recv_msgfrom_middleware failed: error value :"
                   " %d\n",
                   ret);
            return STUB_FAILED;
        }

        //#ifndef OCWARE_STUB_DEBUG
        printf(" \n Received Data :\n");
        for (loopCount = 0; loopCount < OC_EC_MSG_SIZE; loopCount++) {
            printf("0x%x  ", buffer[loopCount] & 0xff);
        }
        printf("\n");
        //#endif
        ret = ocware_stub_validate_msgframe_header(buffer, &msgFrame->message);
        if (ret != STUB_SUCCESS) {
            printf("ocware_stub_validate_msgframe_header failed: error value :"
                   "%d\n",
                   ret);
            return STUB_FAILED;
        }
        ret = ocware_stub_frame_newmsgframe(buffer, ret, &alertFlag);
        if (ret != STUB_SUCCESS) {
            printf("ocware_stub_frame_newmsgframe failed: error value :"
                   "%d\n",
                   ret);
            return STUB_FAILED;
        }
        if (alertFlag > 0) {
            ret = ocware_stub_frame_alert_msgframe(buffer);
        } else {
            ret = ocware_stub_send_msgframe_middleware(
                &buffer, sizeof(OCMPMessageFrame));
            if (ret != STUB_SUCCESS) {
                printf(
                    "ocware_stub_send_msgframe_middleware failed: error value :"
                    "%d\n",
                    ret);
                return STUB_FAILED;
            }
        }
    }
    free(msgFrame);
    free(buffer);
    return STUB_SUCCESS;
}
/******************************************************************************
 * Function Name    : main
 * Description      : start the stub process
 *
 * @param argc - number of argunents passed (by value)
 * @param argv  - arguments passed when invoking stub (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
#ifndef OCWARE_UNITY_TEST
int32_t main(int32_t argc, char *argv[])
{
    int32_t ret = 0;
    ret = host_ocstubmain_func(argc, argv);
    return ret;
}
#else
int32_t host_ocwaremain(int32_t argc, char *argv[])
{
    int32_t ret = 0;
    ret = host_ocstubmain_func(argc, argv);
    return ret;
}
#endif
