/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

/* OC includes */
#include <logger.h>
#include <ocmw_core.h>
#include <ocmw_eth_comm.h>
#include <ocmw_uart_comm.h>
#include <ocwdg_daemon.h>
#include <postframe.h>
#include <occli_common.h>
#include <ocmw_occli_comm.h>

static int32_t loopCountPost = 0;
extern debugI2CData I2CInfo;
extern debugMDIOData MDIOInfo;
extern uint8_t mcuMsgBuf[OCMP_MSG_SIZE];
extern ocmwSchemaSendBuf *ecSendBufBkp;
int32_t responseCount = 0;
static int8_t s_paramInfoBackup[MAX_PARM_COUNT];
static int32_t s_semTimeOut;
static int32_t s_totalSubsystem = 0;
extern const Component sys_schema[];
subSystemInfo systemInfo;

extern ocwarePostResultData ocwarePostArray[TEMP_STR_BUFF_SIZE];
extern uint8_t ocwarePostArrayIndex;
ocwarePostReplyCode ocmwReplyCode[] = {
    /* Message Type, reply code, Description */
    { OCMP_MSG_TYPE_POST, POST_DEV_NOSTATUS, "POST DEV NOSTATUS" },
    { OCMP_MSG_TYPE_POST, POST_DEV_MISSING, "DEV MISSING" },
    { OCMP_MSG_TYPE_POST, POST_DEV_ID_MISMATCH, "DEV ID MISMATCH" },
    { OCMP_MSG_TYPE_POST, POST_DEV_FOUND, "DEV FOUND" },
    { OCMP_MSG_TYPE_POST, POST_DEV_CFG_DONE, "CFG DONE" },
    { OCMP_MSG_TYPE_POST, POST_DEV_NO_CFG_REQ, "NO CFG REQUIRED" },
    { OCMP_MSG_TYPE_POST, POST_DEV_CFG_FAIL, "CFG FAILED" },
    { OCMP_MSG_TYPE_POST, POST_DEV_FAULTY, "FAULT" },
    { OCMP_MSG_TYPE_POST, POST_DEV_CRITICAL_FAULT, "CRITICAL FAULT" },
    { OCMP_MSG_TYPE_POST, POST_DEV_NO_DRIVER_EXIST, "NO DRIVER EXIST" }
};
/******************************************************************************
 * Function Name    : ocmw_free_global_pointer
 * Description      : This Function used to free the memory allocated for
 *                    global variable
 * Input(s)         : ptr
 * Output(s)        :
 ******************************************************************************/
void ocmw_free_global_pointer(void **ptr)
{
    if (*ptr != NULL) {
        free(*ptr);
        *ptr = NULL;
    }
    return;
}
/******************************************************************************
 * Function Name    : ocmw_retrieve_post_results_count
 * Description      : This Function used to count the post structure size
 * Input(s)         : postResult
 * Output(s)        :
 ******************************************************************************/
char ocmw_retrieve_post_results_count(ocwarePostResults *postResult)
{
    postResult->count = sizeof(ocwarePostArray) / sizeof(ocwarePostArray[0]);
    return SUCCESS;
}

/******************************************************************************
 * Function Name    : ocmw_retrieve_post_results
 * Description      : This Function used to get the post results
 * Input(s)         : postResult
 * Output(s)        :
 ******************************************************************************/
char ocmw_retrieve_post_results(ocwarePostResults *postResult)
{
    memcpy(postResult->results, ocwarePostArray,
           postResult->count * sizeof(ocwarePostResultData));
    memset(ocwarePostArray, 0, sizeof(ocwarePostResultData));
    return SUCCESS;
}

/******************************************************************************
 * Function Name    : ocmw_retrieve_reply_code_desc
 * Description      : This Function used to get the reply code based the post
                      result data
 * Input(s)         : replyCode
 * Output(s)        :
 ******************************************************************************/
char ocmw_retrieve_reply_code_desc(ocwarePostReplyCode *replyCode)
{
    int32_t postSize = 0;
    int32_t sysIndex = 0;

    postSize = sizeof(ocmwReplyCode) / sizeof(ocmwReplyCode[0]);

    for (sysIndex = 0; sysIndex < postSize; sysIndex++) {
        if ((ocmwReplyCode[sysIndex].msgtype == replyCode->msgtype) &&
            (ocmwReplyCode[sysIndex].replycode == replyCode->replycode)) {
            memset(replyCode->desc, 0, OCMW_POST_DESC_SIZE);
            strncpy(replyCode->desc, ocmwReplyCode[sysIndex].desc,
                    strlen(ocmwReplyCode[sysIndex].desc));
        }
    }

    return SUCCESS;
}

/******************************************************************************
 * Function Name    : ocmw_update_post_status
 * Description      : This Function used to update the post results
 * Input(s)         : subsystem, devsn, status
 * Output(s)        :
 ******************************************************************************/
char ocmw_update_post_status(uint8_t subsystem, uint8_t devsn, uint8_t status)
{
    int32_t sysIndex = 0;
    for (sysIndex = 0; sysIndex < ocwarePostArrayIndex; sysIndex++) {
        if ((ocwarePostArray[sysIndex].subsystem == subsystem) &&
            (ocwarePostArray[sysIndex].devsn == devsn)) {
            ocwarePostArray[sysIndex].status = status;
        }
    }

    return SUCCESS;
}

/******************************************************************************
 * Function Name    : ocmw_init
 * Description      : This Function used to initialize the mware
 * Input(s)         :
 * Output(s)        :
 ******************************************************************************/
int32_t ocmw_init()
{
#ifdef INTERFACE_ETHERNET
    pthread_t ethMsgPaserThreadId;
#else
    pthread_t uartMsgPaserThreadId;
#endif
    int32_t ret = 0;
    ret = sem_init(&semecMsgParser, 0, 0);
    if (ret != 0) {
        logerr("sem_init(): semecMsgParser failed.");
        return ret;
    }

    ret = ocwdg_init();
    if (ret != 0) {
        logerr("ocwdg_init() failed.");
        return ret;
    }

    ret = sem_init(&semCliReturn, 0, 0);
    if (ret != 0) {
        logerr("sem_init(): semCliReturn failed.");
        return ret;
    }

    ret = sem_init(&semCommandPost, 0, 0);
    if (ret != 0) {
        logerr("sem_init(): semCommandPost failed.");
        return ret;
    } else {
#ifdef INTERFACE_ETHERNET
        /* Create the msg parser thread to parse
         * the msg coming from ethernet ec to ap
         */
        ret = pthread_create(&ethMsgPaserThreadId, NULL,
                             ocmw_thread_ethmsgparser, NULL);
        if (ret != 0) {
            return ret;
        }
#else
        /* Create the msg parser thread to parse
         * the msg coming from uart ec to ap
         */
        ret = pthread_create(&uartMsgPaserThreadId, NULL,
                             ocmw_thread_uartmsgparser, NULL);
        if (ret != 0) {
            logerr("pthread_create() failed.");
        }
#endif /* INTERFACE_ETHERNET */
    }
    return ret;
}

/**************************************************************************
 * Function Name    : ocmw_tokenize_class_str
 * Description      : This Function used to extract the Subsystem,componentID,
 *Messagetype, parameter and Subcomponent from the param string Input(s) : str
 * Output(s)        : msgFrame
 ***************************************************************************/
static int32_t ocmw_tokenize_class_str(const int8_t *str, strMsgFrame *msgFrame,
                                       uint8_t msgtype)
{
    char *token;
    int32_t count = 0;
    char *tempstr = (char *)malloc(PARAM_STR_MAX_BUFF_SIZE);

    if (str == NULL)
        return FAILED;

    memset(tempstr, 0, PARAM_STR_MAX_BUFF_SIZE);
    memcpy(tempstr, str, PARAM_STR_MAX_BUFF_SIZE);
    memset(msgFrame, 0, sizeof(strMsgFrame));

    token = strtok(tempstr, " .");

    if (token == NULL)
        return FAILED;

    if (msgtype == OCMP_MSG_TYPE_COMMAND) {
        if ((strcmp(token, "set") != 0) && ((strcmp(token, "get") != 0))) {
            strcpy(msgFrame->parameter, token);
            while (token) {
                if (count == 1) {
                    strcpy(msgFrame->subsystem, token);
                } else if (count == 2) {
                    strcpy(msgFrame->component, token);
                }
                token = strtok(NULL, " .");
                count++;
                if (token == NULL)
                    break;
            }
            if (count == 2) {
                strcpy(msgFrame->component, "comp_all");
            }
        } else {
            while (token) {
                if (count == 1) {
                    strcpy(msgFrame->subsystem, token);
                } else if (count == 2) {
                    strcpy(msgFrame->component, token);
                } else if (count == 3) {
                    strcpy(msgFrame->subcomponent, token);
                } else if (count == 0) {
                    strcpy(msgFrame->parameter, token);
                }
                token = strtok(NULL, " .");
                count++;
                if (token == NULL)
                    break;
            }
            if (count == 2) {
                strcpy(msgFrame->component, "comp_all");
            }
        }
    } else {
        strcpy(msgFrame->subsystem, token);
        while (token) {
            if (count == 1) {
                strcpy(msgFrame->component, token);
            } else if (count == 2) {
                strcpy(msgFrame->msgtype, token);
            } else if (count == 3) {
                strcpy(msgFrame->subcomponent, token);
            } else if (count == 4) {
                strcpy(msgFrame->parameter, token);
            }
            token = strtok(NULL, " .");
            count++;
        }
        if (count < 5) {
            strcpy(msgFrame->parameter, msgFrame->subcomponent);
        }

        if (strncmp(msgFrame->component, "post", strlen("post")) == 0) {
            strcpy(msgFrame->component, "post");
            strcpy(msgFrame->msgtype, "post");
            strcpy(msgFrame->parameter, msgFrame->subcomponent);
        }
    }

    free(tempstr);

    return SUCCESS;
}
/******************************************************************************
 * Function Name    : ocmw_fill_payload_data_for_commands
 * Description      : Packetize the msg payload for commands
 * Input(s)         :strTokenArray, msgFrame, ecMsgFrame, paramVal
 * Output(s)        :
 ******************************************************************************/
void ocmw_fill_payload_data_for_commands(char *strTokenArray[],
                                         strMsgFrame *msgFrame,
                                         OCMPMessageFrame *ecMsgFrame,
                                         void *paramVal)
{
    if (msgFrame == NULL) {
        return;
    }
    // Handling sending data for test module
    if (strncmp("testmodule", msgFrame->subsystem,
                strlen(msgFrame->subsystem)) == 0) {
        if ((strncmp(strTokenArray[1], "send", strlen("send")) == 0) ||
            (strncmp(strTokenArray[1], "dial", strlen("dial")) == 0)) {
            memcpy(&ecMsgFrame->message.info[0], paramVal, MAX_PARM_COUNT);
        } else {
            memset(ecMsgFrame->message.info, 0, MAX_PARM_COUNT);
        }
    }
    // Handling ethernet packet genrator command
    if (strncmp("ethernet", msgFrame->subsystem, strlen(msgFrame->subsystem)) ==
        0) {
        if (strstr(strTokenArray[1], "loopBk")) {
            memcpy(&ecMsgFrame->message.info[0], paramVal, sizeof(uint8_t));
        } else if ((strncmp(strTokenArray[1], "en_pktGen",
                            strlen("en_pktGen")) == 0)) {
            memcpy(&ecMsgFrame->message.info[0], (uint16_t *)paramVal,
                   sizeof(uint16_t));
        }
    }
}

/******************************************************************************
 * Function Name    : ocmw_fill_debug_i2c_payload
 * Description      : Packetize the msg payload for debug i2c
 * Input(s)         : argv, ecMsgFrame
 * Output(s)        :
 ******************************************************************************/
void ocmw_fill_debug_i2c_payload(char *strTokenArray[],
                                 OCMPMessageFrame *ecMsgFrame)
{
    uint8_t pos = 0;
    ecMsgFrame->message.info[pos] = I2CInfo.slaveAddress;
    ecMsgFrame->message.info[pos + 1] = I2CInfo.writeCount;
    if (I2CInfo.writeCount == 1) {
        ecMsgFrame->message.info[pos + 2] = (uint8_t)I2CInfo.regAddress;
    } else if (I2CInfo.writeCount == 2) {
        ecMsgFrame->message.info[pos + 3] =
            (uint8_t)(I2CInfo.regAddress & 0xff);
        ecMsgFrame->message.info[pos + 2] =
            (uint8_t)((I2CInfo.regAddress & 0xff00) >> 8);
    } else {
        ecMsgFrame->message.info[pos + 5] =
            (uint8_t)(I2CInfo.regAddress & 0x000000ff);
        ecMsgFrame->message.info[pos + 4] =
            (uint8_t)((I2CInfo.regAddress & 0xff00) >> 8);
        ecMsgFrame->message.info[pos + 3] =
            (uint8_t)((I2CInfo.regAddress & 0x00ff0000) >> 16);
        ecMsgFrame->message.info[pos + 2] =
            (uint8_t)((I2CInfo.regAddress & 0xff000000) >> 24);
    }
    ecMsgFrame->message.info[pos + 6] = I2CInfo.numOfBytes;
    if (strncmp(strTokenArray[1], "set", strlen("set")) == 0) {
        if (I2CInfo.numOfBytes == 1) {
            printf("\nSwatee ");
            ecMsgFrame->message.info[pos + 7] = (uint8_t)I2CInfo.regValue;
        } else {
            ecMsgFrame->message.info[pos + 7] =
                (uint8_t)(I2CInfo.regValue & 0xff);
            ecMsgFrame->message.info[pos + 8] =
                (uint8_t)((I2CInfo.regValue & 0xff00) >> 8);
        }
    }
}
/******************************************************************************
 * Function Name    : ocmw_msg_packetize_and_send
 * Description      : Packetize the msg frame before sending data to ec
 * Input(s)         :actionType, msgType, paramStr, interface, paramVal
 * Output(s)        :
 ******************************************************************************/
int32_t ocmw_msg_packetize_and_send(char *strTokenArray[], uint8_t action,
                                    uint8_t msgType, const int8_t *paramStr,
                                    uint8_t interface, void *paramVal)
{
    int32_t ret = 0;
    int32_t paramValue = 0;
    int32_t dataSize = 0;
    int32_t pos = 0;
    int32_t paramValLen = 0;
    int32_t paramValFlag = 0;
    const int8_t *tempStr = paramStr;
    strMsgFrame msgFramestruct;
    OCMPMessageFrame ecMsgFrame;
    OCMPHeader ecMsgHeader;
    OCMPMessage ecCoreMsg;

    static int32_t loopCountSend = 0;
    ocmwSchemaSendBuf ecSendBuf;

    paramValLen = strlen((char *)paramVal);
    if (!((msgType == OCMP_MSG_TYPE_COMMAND) &&
          ((strncmp(strTokenArray[1], "get", strlen("get")) == 0) ||
           (strncmp(strTokenArray[1], "set", strlen("get")) == 0)))) {
        if (paramValLen == 1 || paramValLen <= 5) {
            logdebug("Paramvalue is of integer type : %d\n", atoi(paramVal));
        } else {
            paramValFlag = 1;
            logdebug("Paramvalue is of string type : %s \n", (char *)paramVal);
        }
    } else {
        paramValFlag = 1;
        logdebug("Paramvalue is of string type : %s \n", (char *)paramVal);
    }

    if (paramStr == NULL) {
        logdebug(" Paramstr is NULL \n");
        return ret = FAILED;
    } else {
        ocmw_tokenize_class_str(tempStr, &msgFramestruct, msgType);
        if (msgType == OCMP_MSG_TYPE_COMMAND) {
            ret =
                ocmw_parse_command_msgframe(sys_schema, &msgFramestruct, action,
                                            &ecSendBuf, &strTokenArray[0]);
        } else if (msgType == OCMP_MSG_TYPE_POST) {
            strcpy((char *)s_paramInfoBackup, msgFramestruct.parameter);
            ret = ocmw_parse_post_msgframe(sys_schema, &msgFramestruct, action,
                                           &ecSendBuf);
        } else {
            ret = ocmw_parse_msgframe(sys_schema, &msgFramestruct, action,
                                      &ecSendBuf);
        }
        if (ret < 0) {
            return ret;
        }
    }

    /* Frame the header packet for sending data to ec */
    ecMsgHeader.ocmpFrameLen = 0;
    ecMsgHeader.ocmpInterface = interface;
    ecMsgHeader.ocmpSeqNumber = 0;
    ecMsgHeader.ocmpSof = OCMP_MSG_SOF;
    ecMsgHeader.ocmpTimestamp = 0;

    /* Frame the Core packet for sending data to ec */
    ecCoreMsg.action = ecSendBuf.actionType;
    ecCoreMsg.msgtype = ecSendBuf.msgType;
    ecCoreMsg.componentID = ecSendBuf.componentId;
    ecCoreMsg.parameters = ecSendBuf.paramId;
    ecCoreMsg.subsystem = ecSendBuf.subsystem;

    /* Construct the final packet */
    ecMsgFrame.header = ecMsgHeader;
    ecMsgFrame.message = ecCoreMsg;

    /* Populate the Core packet payload */
    ecMsgFrame.message.info = (int8_t *)malloc(sizeof(int8_t) * MAX_PARM_COUNT);
    if (ecMsgFrame.message.info == NULL) {
        logdebug("\n Memory allocation failed \n");
        return ret = FAILED;
    }
    memset(ecMsgFrame.message.info, 0, MAX_PARM_COUNT);
    if ((msgType == OCMP_MSG_TYPE_POST) &&
        (strncmp(strTokenArray[1], "set", strlen("set")) == 0)) {
        logdebug("OCMP_MSG_TYPE_POST:ENABLE:%s()\n", __func__);
    }
    if (strstr(strTokenArray[0], "debug")) {
        if (strstr(strTokenArray[0], "I2C")) {
            if ((strncmp(strTokenArray[1], "get", strlen("get")) == 0)) {
                dataSize = sizeof(debugI2CData) - 2;
            } else {
                dataSize = sizeof(debugI2CData);
            }
        } else if (strstr(strTokenArray[0], "ethernet")) {
            if ((strncmp(strTokenArray[1], "get", strlen("get")) == 0)) {
                dataSize = sizeof(uint16_t);
            } else {
                dataSize = sizeof(debugMDIOData);
            }
        } else {
            if ((strncmp(strTokenArray[1], "get", strlen("get")) == 0)) {
                dataSize = sizeof(int8_t);
            } else {
                dataSize = sizeof(debugGPIOData);
            }
        }
        paramValLen = dataSize;
    } else {
        dataSize = ecSendBuf.paramSize;
    }
    if ((strncmp(strTokenArray[1], "set", strlen("set")) == 0) &&
        (msgType != OCMP_MSG_TYPE_POST)) {
        pos = ecSendBuf.paramPos;
        if (paramValFlag == 0) {
            paramValue = atoi(paramVal);
            memcpy(&ecMsgFrame.message.info[pos], &paramValue, dataSize);
        } else {
            if (strstr(strTokenArray[0], "I2C")) {
                ocmw_fill_debug_i2c_payload(&strTokenArray[0], &ecMsgFrame);
            } else if (strstr(strTokenArray[0], "debug.ethernet")) {
                ecMsgFrame.message.info[pos] =
                    (uint8_t)(MDIOInfo.regAddress & 0xff);
                ecMsgFrame.message.info[pos + 1] =
                    (uint8_t)((MDIOInfo.regAddress & 0xff00) >> 8);
                ecMsgFrame.message.info[pos + 2] =
                    (uint8_t)(MDIOInfo.regValue & 0xff);
                ecMsgFrame.message.info[pos + 3] =
                    (uint8_t)((MDIOInfo.regValue & 0xff00) >> 8);

            } else {
                memcpy(&ecMsgFrame.message.info[pos], (char *)paramVal,
                       paramValLen);
            }
        }
    }
    if ((strncmp(strTokenArray[1], "get", strlen("get")) == 0) &&
        (strstr(strTokenArray[0], "debug"))) {
        if (strstr(strTokenArray[0], "I2C")) {
            ocmw_fill_debug_i2c_payload(&strTokenArray[0], &ecMsgFrame);
        } else {
            pos = ecSendBuf.paramPos;
            memcpy(&ecMsgFrame.message.info[pos], (char *)paramVal,
                   paramValLen);
        }
    }
    // Fill payload data for commands
    ocmw_fill_payload_data_for_commands(&strTokenArray[0], &msgFramestruct,
                                        &ecMsgFrame, paramVal);

    if (!((msgType == OCMP_MSG_TYPE_POST) &&
          (strncmp(strTokenArray[1], "get", strlen("get")) == 0))) {
        ocmw_send_msg(ecMsgFrame, interface);
    }

    /* Wait on the POST msgtype semaphore */
    if ((msgType == OCMP_MSG_TYPE_POST) &&
        ((strncmp(strTokenArray[1], "get", strlen("get")) == 0))) {
        ocmw_frame_subsystem_from_schema(sys_schema, &systemInfo);
        s_totalSubsystem = systemInfo.totalNum;
        ocmw_frame_postTable_from_schema(sys_schema);
        loopCountSend = systemInfo.Info[0].number;
        loopCountPost = systemInfo.Info[0].number;

        while (loopCountSend < s_totalSubsystem) {
            memset(&(ecMsgFrame.message.info[0]), loopCountSend, 1);
            /* Send the message to ec */
            ocmw_send_msg(ecMsgFrame, interface);
            loopCountSend++;

            /* Waiting on the lock to be released by receiving  thread */
            ret = ocmw_sem_wait(&semCommandPost, OCMW_TIMED_WAIT_INDEX);
            if (ret != 0) {
                perror("sem_wait");
            }

            /*
             * Logic to check if firmware has sent post data currently we
             * expect only actiontype mismatch other errors can be taken care
             * as required
             */
            if (loopCountSend != loopCountPost) {
                logerr("POST: Error : Actiontype mismatch in message from FW");
                free(ecMsgFrame.message.info);
                return FAILED;
            }
        }
        loopCountPost = systemInfo.Info[0].number;
        loopCountSend = systemInfo.Info[0].number;
    }

    free(ecMsgFrame.message.info);

    /* Semaphore wait function call */
    ret = ocmw_sem_wait(&semCliReturn, OCMW_TIMED_WAIT_INDEX);
    if (ret == FAILED) {
        logdebug("Message is not received from EC: releasing the lock \n");
        return ret;
    }

    if (responseCount == 0) {
        ret = FAILED;
    } else {
        ret = 0;
        responseCount = 0;
    }
    logdebug(" \n Semaphore released : %s() \n", __func__);

    return ret;
}

/******************************************************************************
 * Function Name    : ocmw_thread_uartmsgparser
 * Description      : Thread to parse the data coming from EC to AP
 *                    through uart communication
 * Input(s)         : pthreadData
 * Output(s)        :
 ******************************************************************************/
void *ocmw_thread_uartmsgparser(void *pthreadData)
{
    logdebug("Uart task created \n");
    while (1) {
        /* Waiting on the  semecMsgParser to be released by uart */
        sem_wait(&semecMsgParser);
        ocmw_ec_msgparser();
    }
}

/******************************************************************************
 * Function Name    : ocmw_thread_ethmsgparser
 * Description      : Thread to parse the data coming from EC to AP
 *                    through ethernet communication
 * Input(s)         : pthreadData
 * Output(s)        :
 ******************************************************************************/
void *ocmw_thread_ethmsgparser(void *pthreadData)
{
    int8_t ethRecvBuf[OCMP_MSG_SIZE] = { 0 };

    logdebug("Ethernet task created \n");
    while (1) {
        memset(ethRecvBuf, 0, sizeof(ethRecvBuf));
        ocmw_recv_eth_msgfrom_ec(ethRecvBuf, sizeof(ethRecvBuf), OCMW_EC_DEV);
        ocmw_ec_msgparser();
    }
}

/******************************************************************************
 * Function Name    : ocmw_ec_msgparser
 * Description      : parse the data coming from EC to AP
 * Input(s)         :
 * Output(s)        :
 ******************************************************************************/
void ocmw_ec_msgparser(void)
{
    uint8_t msgType = 0;
    uint8_t actionType = 0;
    uint8_t subsystemPost = 0;
    uint8_t devsn = 0;
    uint8_t status = 0;
    uint8_t indexCount = 0;
    uint16_t paramInfo = 0;
    int32_t ret = 0;
    int32_t sendPktNonpayloadSize = 0;
    sMsgParam dmsgFrameParam;
    OCMPMessageFrame ecReceivedMsg;

    sendPktNonpayloadSize =
        (sizeof(OCMPMessage) - sizeof(void *) + sizeof(OCMPHeader));

    ecReceivedMsg.message.info = (void *)malloc(sizeof(char) * MAX_PARM_COUNT);
    if (ecReceivedMsg.message.info == NULL) {
        logerr("Memory allocation failed for "
               "ecReceivedMsg.message.info \n");

        return;
    }

    /* parse the data packet */
    memcpy((void *)&ecReceivedMsg, (void *)mcuMsgBuf, sendPktNonpayloadSize);
    memcpy(ecReceivedMsg.message.info, &mcuMsgBuf[sendPktNonpayloadSize],
           MAX_PARM_COUNT);

    msgType = ecReceivedMsg.message.msgtype;
    actionType = ecReceivedMsg.message.action;
    paramInfo = ecReceivedMsg.message.parameters;

    /*
     * TODO:Temporary fix for handling alerts
     */
    if (msgType == OCMP_MSG_TYPE_ALERT) {
        free(ecReceivedMsg.message.info);
        return;
    }

    printf("Received from ec :\n");
    for (indexCount = 0; indexCount < OCMP_MSG_SIZE; indexCount++) {
        printf("0x%x  ", mcuMsgBuf[indexCount]);
    }
    printf("\n");

    /* In case of timeout, return from the thread
     * without processing the data to avoid sync issue.
     */
    if (s_semTimeOut) {
        s_semTimeOut = 0;
        ocmw_free_global_pointer((void **)&ecSendBufBkp);
        return;
    }

    if ((msgType == OCMP_MSG_TYPE_POST) &&
        strcmp((char *)s_paramInfoBackup, "set") == 0) {
        if (actionType == OCMP_AXN_TYPE_REPLY) {
            responseCount++;
        }
        /* Release the lock on which cli is waiting */
        ocmw_free_global_pointer((void **)&ecSendBufBkp);
        sem_post(&semCliReturn);
        return;
    }

    /* Release the lock on the POST msgtype semaphore */
    if (msgType == OCMP_MSG_TYPE_POST) {
        if (actionType != OCMP_AXN_TYPE_REPLY) {
            ocmw_free_global_pointer((void **)&ecSendBufBkp);
            ret = sem_post(&semCommandPost);
            if (ret != 0) {
                perror("sem_wait");
            }
            return;
        }

        for (; loopCountPost < s_totalSubsystem;) {
            /* Waiting on the lock to be released by receiving  thread */
            for (indexCount = 0; indexCount < paramInfo; indexCount++) {
                subsystemPost = ecReceivedMsg.message
                                    .info[indexCount * POST_MAIN_PAYLOAD_SIZE +
                                          POST_MAIN_PAYLOAD_SUBSYSTEM_OFFSET];
                devsn = ecReceivedMsg.message
                            .info[indexCount * POST_MAIN_PAYLOAD_SIZE +
                                  POST_MAIN_PAYLOAD_DEVSN_OFFSET];
                status = ecReceivedMsg.message
                             .info[indexCount * POST_MAIN_PAYLOAD_SIZE +
                                   POST_MAIN_PAYLOAD_STATUS_OFFSET];
                ret = ocmw_update_post_status(subsystemPost, devsn, status);
            }

            loopCountPost++;

            ret = sem_post(&semCommandPost);
            if (ret != 0) {
                perror("sem_wait");
            }
            if (loopCountPost < s_totalSubsystem) {
                /* Do Nothing */
            } else {
                /* Release the lock on which watchdog thread is waiting */
                responseCount++;
                ocmw_free_global_pointer((void **)&ecSendBufBkp);
                sem_post(&semCliReturn);
            }
            return;
        }
    }

    memset(&dmsgFrameParam, 0, sizeof(sMsgParam));
    ocmw_deserialization_msgframe(sys_schema, &dmsgFrameParam, &ecReceivedMsg);
    /* Released  the lock so that cli is released from the lock */
    if (ecReceivedMsg.message.info) {
        free(ecReceivedMsg.message.info);
    }
    sem_post(&semCliReturn);
}

/******************************************************************************
 * Function Name    : ocmw_send_msg
 * Description      : Send the message from ap to ec via uart/usb/ethernet.
 * Input(s)         : ecMsgFrame, interface
 * Output(s)        :
 ******************************************************************************/
int32_t ocmw_send_msg(OCMPMessageFrame ecMsgFrame, uint8_t interface)
{
    int32_t ret = 0;
    int32_t sentDev = OCMW_EC_DEV;
#ifdef INTERFACE_STUB_EC
    int8_t ethRecvBuf[OCMP_MSG_SIZE];
#endif
    switch (interface) {
        case OCMP_COMM_IFACE_UART:
        case OCMP_COMM_IFACE_USB:
            /* Send the packetize data to ec  through uart*/
            ret =
                ocmw_send_uart_msg_to_ec((uint8_t *)&ecMsgFrame, OCMP_MSG_SIZE);
            break;

        case OCMP_COMM_IFACE_ETHERNET:
            sentDev = OCMW_EC_DEV;
#ifdef INTERFACE_STUB_EC
            sentDev = OCMW_EC_STUB_DEV;
#else
            sentDev = OCMW_EC_DEV;
#endif
            /* Send the packetize data to ec  through ethernet*/
            ret = ocmw_send_eth_msgto_ec((int8_t *)&ecMsgFrame, OCMP_MSG_SIZE,
                                         sentDev);
#ifdef INTERFACE_STUB_EC
            memset(ethRecvBuf, 0, sizeof(ethRecvBuf));
            ocmw_recv_eth_msgfrom_ec(ethRecvBuf, sizeof(ethRecvBuf),
                                     OCMW_EC_STUB_DEV);
            ocmw_ec_msgparser();
#endif
            break;

        case OCMP_COMM_IFACE_SBD:
            break;
        default:
            break;
    }

    if (ret != 0) {
        logerr("ocmw_send_msg() failed \n");
    }
    return ret;
}

/******************************************************************************
 * Function Name    : ocmw_sem_wait
 * Description      : semaphore wait function to wait either for infinite time
 *                    or for timeout period.
 * Input(s)         : semId, semWaitType
 * Output(s)        :
 ******************************************************************************/
int32_t ocmw_sem_wait(sem_t *semId, int32_t semWaitType)
{
    int32_t ret = 0;
    struct timespec timeSpecObj;

    if (semId == NULL) {
        logerr("Invalid semId \n");
        ret = FAILED;
        return ret;
    }

    if (semWaitType == OCMW_BUSY_WAIT_INDEX) {
        /* Waiting on the lock to be released by receiving  thread */
        ret = ocmw_sem_wait_nointr(semId);
        if (ret != 0) {
            perror("sem_wait");
        }
    } else if (semWaitType == OCMW_TIMED_WAIT_INDEX) {
        ret = clock_gettime(CLOCK_REALTIME, &timeSpecObj);
        if (ret != 0) {
            perror("clock_gettime");
        }
        timeSpecObj.tv_sec += OCMW_SEM_WAIT_TIME;
        timeSpecObj.tv_nsec += 0;

        ret = ocmw_sem_timedwait_nointr(semId, &timeSpecObj);
        if (ret != 0) {
            if (errno == ETIMEDOUT) {
                logdebug("sem_timedwait() timed out\n");
            } else {
                perror("sem_timedwait");
            }
            logdebug("Message is not received from EC: releasing the lock \n");
            s_semTimeOut = 1;
        }
    } else {
        printf("Invalid param \n");
    }
    return ret;
}
