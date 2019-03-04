#include <ocmw_schema.h>
#include <ctype.h>
#include <logger.h>

int64_t recvdParamVal;
extern int32_t responseCount;
int32_t eepromFlag;
static int32_t s_typeFlag;
char dataOutBufFromEc[PARAM_STR_BUFF_SIZE] = {0};
static char *s_paramEnumValue[OCMW_VALUE_TYPE_MFG] = {0};
ocmwSchemaSendBuf *ecSendBufBkp;
ocwarePostResultData ocwarePostArray[TEMP_STR_BUFF_SIZE] = {{0}};
uint8_t  ocwarePostArrayIndex;
extern alertlist alerts;
static uint8_t s_alertIndex = 0;
extern int8_t alertFlag;
/******************************************************************************
 * Function Name    : ocmw_parse_eepromdata_from_ec
 * Description      : parse the eeprom data coming from EC to AP
 * Input(s)         : ecInputData
 * Output(s)        :
 ******************************************************************************/
int8_t ocmw_parse_eepromdata_from_ec (ocmwSendRecvBuf ecInputData)
{
    memset(dataOutBufFromEc, 0, sizeof(dataOutBufFromEc));
    if (ecInputData.msgType == OCMP_MSG_TYPE_STATUS) {
        memcpy(dataOutBufFromEc, &ecInputData.pbuf[ecSendBufBkp->paramPos],
               sizeof(ecInputData.pbuf));
    }else if (ecInputData.msgType == OCMP_MSG_TYPE_CONFIG) {
        memcpy(dataOutBufFromEc, ecInputData.pbuf, EEPROM_CONFIG_SIZE);
    }

    return SUCCESS;
}

/******************************************************************************
 * Function Name    : ocmw_parse_testingmodule_struct_from_ec
 * Description      : parse the testmodule struct data coming from EC to AP
 * Input(s)         : ecInputData
 * Output(s)        :
 ******************************************************************************/
int32_t ocmw_parse_testingmodule_struct_from_ec(ocmwSendRecvBuf ecInputData)
{
    uint32_t paramVal2 = 0;
    eOperatorStat paramVal = 0;
    size_t size = 0;

    memset(dataOutBufFromEc, 0, sizeof(dataOutBufFromEc));
    size = sizeof(dataOutBufFromEc);

    paramVal = *((uint8_t *)
                &(ecInputData.pbuf[TWO_G_SIM_NET_OPTR_STATUS_OFFSET]));
    paramVal2 = *((uint16_t *) ecInputData.pbuf);
    switch (paramVal) {
        case TWOG_SIM_STAT_UNKNOWN:
            snprintf(dataOutBufFromEc, size,
                     "Operator code : %u Operator status : %u(STAT_UNKNOWN)",
                     paramVal2, paramVal);
            break;

        case TWOG_SIM_STAT_AVAILABLE:
            snprintf(dataOutBufFromEc, size,
                     "Operator code : %u Operator status : %u(STAT_AVAILABLE)",
                     paramVal2, paramVal);
            break;

        case TWOG_SIM_STAT_CURRENT:
            snprintf(dataOutBufFromEc, size,
                    "Operator code : %u Operator status : %u(STAT_CURRENT)",
                    paramVal2, paramVal);
            break;

        case TWOG_SIM_STAT_FORBIDDEN:
            snprintf(dataOutBufFromEc, size,
                    "Operator code : %u Operator status : %u(STAT_FORBIDDEN)",
                    paramVal2, paramVal);
            break;

        default:
            break;
    }

    return SUCCESS;
}

/******************************************************************************
 * Function Name    : ocmw_parse_obc_struct_from_ec
 * Description      : parse the obc data coming from EC to AP
 * Input(s)         : ecInputData
 * Output(s)        :
 ******************************************************************************/
int32_t ocmw_parse_obc_struct_from_ec(ocmwSendRecvBuf ecInputData)
{
    uint32_t paramVal2 = 0;
    Source paramVal = 0;
    size_t size = 0;
    memset(dataOutBufFromEc, 0, sizeof(dataOutBufFromEc));
    size = sizeof(dataOutBufFromEc);

    paramVal = (ecInputData.pbuf[ecSendBufBkp->paramPos]);
    paramVal2 = *((uint16_t *)
                &(ecInputData.pbuf[IRIDIUM_LASTERR_ERROR_CODE_OFFSET]));
    switch (paramVal) {
        case ERR_RC_INTERNAL:
            snprintf(dataOutBufFromEc, size,
                    "Error Source : %u(ERR_RC_INTERNAL) Error Code : %u",
                    paramVal, paramVal2);
            break;

        case ERR_SRC_CMS:
            snprintf(dataOutBufFromEc, size,
                    "Error Source : %u(ERR_SRC_CMS) Error Code : %u",
                    paramVal, paramVal2);
            break;

        case ERR_SRC_CME:
            snprintf(dataOutBufFromEc, size,
                    "Error Source : %u(ERR_SRC_CME) Error Code : %u",
                    paramVal, paramVal2);
            break;

        default:
            break;
    }

    return SUCCESS;
}

/******************************************************************************
 * Function Name    : ocmw_get_paramSize
 * Description      : get the parameter size as per schema
 * Input(s)         : paramtype, msgtype, param_name
 * Output(s)        :
 ******************************************************************************/
int8_t ocmw_get_paramSize(const char* paramtype, int8_t msgtype,
                                const char* param_name)
{
    int8_t paramSize = 0;

    if (paramtype == NULL) {
        printf("Invalid paramtype\n");
        return FAILED;
    }

    if (!strcmp("uint16",paramtype)) {
        paramSize = sizeof(uint16_t);
        s_typeFlag = OCMW_VALUE_TYPE_UINT16;
    } else if (!strcmp("int16",paramtype)) {
        paramSize = sizeof(int16_t);
        s_typeFlag = OCMW_VALUE_TYPE_INT16;
    } else if (!strcmp("uint8",paramtype)) {
        paramSize = sizeof(uint8_t);
        s_typeFlag = OCMW_VALUE_TYPE_UINT8;
    } else if (!strcmp("int8",paramtype)) {
        paramSize = sizeof(int8_t);
        s_typeFlag = OCMW_VALUE_TYPE_INT8;
    } else if (!strcmp("uint32",paramtype)) {
        paramSize = sizeof(uint32_t);
        s_typeFlag = OCMW_VALUE_TYPE_UINT32;
    } else if (!strcmp("uint64",paramtype)) {
        paramSize = sizeof(uint64_t);
    } else if (!strcmp("enum",paramtype)) {
            s_typeFlag = OCMW_VALUE_TYPE_STRUCT;
            paramSize = OCMW_VALUE_TYPE_ENUM;
    } else {
        paramSize = sizeof(int8_t);
    }

    return paramSize;
}

/******************************************************************************
 * Function Name    : ocmw_frame_subsystem_from_schema
 * Description      : frame subsyetem as per schema
 * Input(s)         : schema, subSystemInfo
 * Output(s)        :
 ******************************************************************************/
int32_t ocmw_frame_subsystem_from_schema(const Component *subSystem,
        subSystemInfo *systemInfo)
{
    int32_t index = 0;
    memset(&systemInfo->totalNum,0,sizeof(0));
    while (subSystem && subSystem->name) {
        systemInfo->totalNum += 1;
        systemInfo->Info[index].number = index;
        strncpy(systemInfo->Info[index].name, subSystem->name,
                                        strlen(subSystem->name));
        index++;
        subSystem += 1;
    }
    return SUCCESS;
}

/******************************************************************************
 * Function Name    : ocmw_frame_postTable_from_schema
 * Description      : frame post as per schema
 * Input(s)         : schema, postInfo
 * Output(s)        :
 ******************************************************************************/
int32_t ocmw_frame_postTable_from_schema(const Component *subSystem)
{
    int32_t index = 0;
    int32_t count = 0;
    const Component *component = NULL;
    const Component *subComponent = NULL;
    uint8_t subSystemNum = 0;
    char tempComp[PARAM_STR_BUFF_SIZE] = {0};
    char tempCompdev[PARAM_STR_BUFF_SIZE] = {0};
    ocwarePostArrayIndex = 0;
    while (subSystem && subSystem->name) {
        count = 1;
        component = subSystem->components;
        while (component && component->name) {
            subComponent = component->components;
            if ((subComponent == NULL) &&
                        ((!(component->postDisabled)))) {
                memset(tempComp, 0, strlen(tempComp));
                strncpy(tempComp,component->name,strlen(component->name));
                tempComp[0] = toupper(tempComp[0]);
                strncpy((ocwarePostArray[index].subsysName),
                                subSystem->name, strlen(subSystem->name));
                strncpy((ocwarePostArray[index].deviceName),
                                tempComp, strlen(tempComp));
                ocwarePostArray[index].devsn = count++;
                ocwarePostArray[index].subsystem =  subSystemNum;
                ocwarePostArrayIndex++;
                index++;
            }
            while (subComponent && subComponent->name) {
                if (!(subComponent->postDisabled)) {
                    strncpy(ocwarePostArray[index].subsysName,
                                    subSystem->name, strlen(subSystem->name));
                    memset(ocwarePostArray[index].deviceName, 0,
                                    sizeof(ocwarePostArray[index].deviceName));
                    memset(tempComp, 0, strlen(tempComp));
                    memset(tempCompdev, 0, strlen(tempCompdev));
                    strncpy(tempComp, component->name, strlen(component->name));
                    strncpy(tempCompdev, subComponent->name,
                                        strlen(subComponent->name));
                    tempComp[0] = toupper(tempComp[0]);
                    tempCompdev[0] = toupper(tempCompdev[0]);

                    if (!strcmp(component->name,"comp_all")) {
                        strcpy((ocwarePostArray[index].deviceName),
                                    tempCompdev);
                    } else {
                        if ((snprintf(ocwarePostArray[index].deviceName,
                                    OCMW_POST_DEVICE_SIZE, "%s %s",
                                    tempComp, tempCompdev)) < 0) {
                            return FAILED;
                        }
                    }
                    ocwarePostArray[index].devsn = count++;
                    ocwarePostArray[index].subsystem =  subSystemNum;
                    ocwarePostArrayIndex++;
                    index++;
                }
                subComponent += 1;
            }
            component += 1;
        }
        printf("\n");
        subSystem += 1;
        subSystemNum++;
    }
    return SUCCESS;
}

/******************************************************************************
 * Function Name    : ocmw_get_noOfElements
 * Description      : get the number of parameter for components
 * Input(s)         : param_list
 * Output(s)        : noOfElement, size
 ******************************************************************************/
void ocmw_get_noOfElements(const Parameter *param_list, int32_t *noOfElement,
        int32_t *size)
{
    int8_t elementCount = 0;
    int8_t pSize = 0;

    if (param_list == NULL) {
        *noOfElement = 0;
        *size =  0;
        return;
    }
    while (param_list->name) {
        if ((!strcmp("uint16", DATA_TYPE_MAP[param_list->type])) ||
            (!strcmp("int16", DATA_TYPE_MAP[param_list->type]))) {
            pSize = pSize + sizeof(uint16_t);
        } else {
            pSize = pSize + sizeof(int8_t);
        }
        elementCount++;
        param_list++;
    }
    *noOfElement = elementCount;
    *size = pSize;
}
/******************************************************************************
 * Function Name    : ocmw_get_alert_value
 * Description      : get the alert value from message
 * Input(s)         : paramtype, ecReceivedMsg
 * Output(s)        : recvdAlertVal
 ******************************************************************************/
void ocmw_get_alert_value(const Parameter *param, const char* paramtype,
            char *alertVal, char *alertVal1, OCMPMessageFrame *ecReceivedMsg)
{
    int8_t paramPos = 7;
    int32_t recvdAlertVal = 0;
    int32_t recvdAlertVal1 = 0;
    char *alertEnumValue[2] = {0};
    char regStr[BUF_SIZE] = {0};
    char regStr1[BUF_SIZE] = {0};
    int8_t enumCount = 0;
    int8_t strCount = 0;
    int8_t index = 0;

    if ((param == NULL) || (paramtype == NULL)) {
        logdebug("Invalid paramtype\n");
        return ;
    }

    if (strcmp("uint16",paramtype) == 0) {
        recvdAlertVal = *((uint16_t *)
                            &(ecReceivedMsg->message.info[paramPos]));
        recvdAlertVal1 = *((uint16_t *)
                            &(ecReceivedMsg->message.info[paramPos + 2]));
    } else if (strcmp("int16",paramtype) == 0) {
        recvdAlertVal = *((int16_t *)
                            &(ecReceivedMsg->message.info[paramPos]));
        recvdAlertVal1 = *((int16_t *)
                            &(ecReceivedMsg->message.info[paramPos + 2]));
    } else if (strcmp("uint8",paramtype) == 0) {
        recvdAlertVal = *((uint8_t *)
                            &(ecReceivedMsg->message.info[paramPos]));
        recvdAlertVal1 = *((uint8_t *)
                            &(ecReceivedMsg->message.info[paramPos + 2]));
    } else if (strcmp("string",paramtype) == 0) {
        memcpy(regStr, &ecReceivedMsg->message.info[paramPos], param->size);
        memcpy(regStr1, &ecReceivedMsg->message.info[paramPos + 2],
            param->size);
        strCount++;
    }

    if (enumCount) {
        sprintf(alertVal, "%u(%s)", recvdAlertVal, regStr);
        sprintf(alertVal1, "%u(%s)", recvdAlertVal1, regStr1);
    } else if (strCount) {
        sprintf(alertVal, "%s", regStr);
        sprintf(alertVal1, "%s", regStr1);
    } else {
        sprintf(alertVal, "%u", recvdAlertVal);
        sprintf(alertVal1, "%u", recvdAlertVal1);
    }
    for (index = 0; index < enumCount; index++) {
        ocmw_free_global_pointer((void**)&alertEnumValue[index]);
    }
    return;
}

/******************************************************************************
 * Function Name    : ocmw_extract_dateTime_from_msgFrame
 * Description      : extract date time for alerts
 * Input(s)         : ecReceivedMsg
 * Output(s)        :
 ******************************************************************************/
void ocmw_extract_dateTime_from_msgFrame(OCMPMessageFrame *ecReceivedMsg)
{
    uint8_t hour = 0;
    uint8_t minuts = 0;
    uint8_t second = 0;
    uint8_t day = 0;
    uint8_t month = 0;
    uint8_t year = 0;
    int8_t paramPos = 1;

    hour = *((uint8_t *)
            &(ecReceivedMsg->message.info[paramPos]));
    minuts = *((uint8_t *)
            &(ecReceivedMsg->message.info[paramPos + 1]));
    second = *((uint8_t *)
            &(ecReceivedMsg->message.info[paramPos + 2]));
    day = *((uint8_t *)
            &(ecReceivedMsg->message.info[paramPos + 3]));
    month = *((uint8_t *)
            &(ecReceivedMsg->message.info[paramPos + 4]));
    year = *((uint8_t *)
            &(ecReceivedMsg->message.info[paramPos + 5]));
    sprintf(alerts.list[s_alertIndex].datetime, "%d:%d:%d %d/%d/20%d",
               hour, minuts, second, day, month, year);
    return;
}

/******************************************************************************
 * Function Name    : ocmw_handle_alert_msg
 * Description      : parse schema for alert message
 * Input(s)         : compBase, ecReceivedMsg
 * Output(s)        :
 ******************************************************************************/
void ocmw_handle_alert_msg(const Component *compBase,
                        OCMPMessageFrame *ecReceivedMsg, int8_t *alertRecord)
{
    const Component *component = NULL;
    const Component *subComponent = NULL;
    const Component *subSystem = compBase;
    const Driver *devDriver = NULL;
    const Parameter *param = NULL;
    char response[RES_STR_BUFF_SIZE] = {0};
    int8_t count = 0;
    int8_t countParamId = 0;
    static int8_t alertCount = 0;
    int8_t ret = 0;
    int32_t alertElements = 0;
    int32_t alertElementsTemp = 0;
    int32_t size = 0;
    sMsgParam *sMsgFrameParam = (sMsgParam *) malloc(sizeof(sMsgParam));

    if ((subSystem == NULL) || (sMsgFrameParam == NULL)) {
        logdebug("Invalid Memory\n");
        return;
    }
    if ((alertFlag == 0) && (alerts.nalerts == 0)) {
        alerts.list = (struct allAlertEvent *) calloc(ALERT_MAX_BUFF_SIZE,
                                                sizeof(struct allAlertEvent));
    }
    memset(sMsgFrameParam, 0, sizeof(sMsgParam));
    sMsgFrameParam->component = 1;
    while (subSystem && subSystem->name) {
        if (sMsgFrameParam->subsystem == ecReceivedMsg->message.subsystem) {
            component = subSystem->components;
            while (component && component->name) {
                if (ecReceivedMsg->message.componentID ==
                                        sMsgFrameParam->component) {
                    devDriver = component->driver;
                    if (devDriver != NULL) {
                        param = devDriver->alerts;
                        while (param && param->name) {
                            if(ecReceivedMsg->message.parameters ==
                                    pow(2, countParamId)) {
                                sprintf(alerts.list[s_alertIndex].string,
                                        "%s.%s.alerts.%s",
                                        subSystem->name, component->name,
                                        param->name);
                                ocmw_get_alert_value(param,
                                        DATA_TYPE_MAP[param->type],
                                        alerts.list[s_alertIndex].value,
                                        alerts.list[s_alertIndex].actualValue,
                                        ecReceivedMsg);
				                alertCount++;
                                break;
                            }
                            countParamId = countParamId + 1;
                            param += 1;
                        }
                    }
                    subComponent = component->components;
                    while (subComponent && subComponent->name) {
                        countParamId = 0;
                        devDriver = subComponent->driver;
                        if (devDriver != NULL) {
                            param = devDriver->alerts;
                            ocmw_get_noOfElements(param,&alertElements, &size);
                            while (param && param->name) {
                                if(ecReceivedMsg->message.parameters ==
                                    pow(2, countParamId + alertElementsTemp)){
                                    sprintf(alerts.list[s_alertIndex].string,
                                        "%s.%s.alerts.%s.%s", subSystem->name,
                                        component->name, subComponent->name,
                                        param->name);
                                    ocmw_get_alert_value(param,
                                        DATA_TYPE_MAP[param->type],
                                        alerts.list[s_alertIndex].value,
                                        alerts.list[s_alertIndex].actualValue,
                                        ecReceivedMsg);
                                    count++;
				                    alertCount++;
                                    break;
                                }
                                countParamId = countParamId + 1;
                                param += 1;
                            }
                        }
                        if (count > 0)
                            break;
                        alertElementsTemp = alertElementsTemp + alertElements;
                        subComponent += 1;
                    }
                    break;
                }
                sMsgFrameParam->component += 1;
                component += 1;
            }
            break;
        }
        sMsgFrameParam->subsystem += 1;
        subSystem += 1;
    }
    if (ecReceivedMsg->message.action == OCMP_AXN_TYPE_ACTIVE) {
        strcpy(alerts.list[s_alertIndex].action,"ACTIVE");
    } else if (ecReceivedMsg->message.action == OCMP_AXN_TYPE_CLEAR) {
        strcpy(alerts.list[s_alertIndex].action,"CLEAR");
    }
    ocmw_extract_dateTime_from_msgFrame(ecReceivedMsg);
    logdebug("Alert : %25s%10s%5s\n", alerts.list[s_alertIndex].string,
        alerts.list[s_alertIndex].action, alerts.list[s_alertIndex].value);
    alerts.nalerts = s_alertIndex;
    if ((alertFlag > 0) &&
        (ecReceivedMsg->message.action == OCMP_AXN_TYPE_REPLY)) {
        *alertRecord = 0;
	    if (alertCount > 0) {
            alerts.nalerts = s_alertIndex - 1;
	    } else {
		    alerts.nalerts = 0;
	    }
        sem_post(&semCliReturn);
        s_alertIndex = 0;
	    alertFlag = 0;
    } else if (alertFlag == 0) {
        *alertRecord = 0;
        ret = ocmw_handle_show_alerts(response);
        if (ret == FAILED)
            logdebug("Alert send error\n");
        s_alertIndex = 0;
    }
    free(sMsgFrameParam);
    s_alertIndex++;
    return;
}
/******************************************************************************
 * Function Name    : ocmw_parse_command_msgframe
 * Description      : parse the command
 * Input(s)         : Schema, msgFrame, actiontype, ecSendBuf
 * Output(s)        :
 ******************************************************************************/
int32_t ocmw_parse_command_msgframe(const Component *compBase,
                            strMsgFrame *msgFrame, uint8_t actiontype,
                            ocmwSchemaSendBuf *ecSendBuf)
{
    const Component *component = NULL;
    const Component *subSystem = compBase;
    const Component *subcomponent = NULL;
    const Command *comm = NULL;
    const Driver *driver = NULL;
    sMsgParam *sMsgFrameParam = (sMsgParam *) malloc(sizeof(sMsgParam));
    int16_t paramId = 0;
    int8_t subCount = 0;
    int8_t actionType = 0;
    int8_t count = 0;
    int8_t componentCount = 0;
    int8_t paramSize = 0;
    int8_t paramPos = 0;
    recvdParamVal = 0;
    eepromFlag = 0;
    s_typeFlag = 0;

    if (subSystem == NULL) {
        printf("Invalid Memory\n");
        return -1;
    }
    if (sMsgFrameParam == NULL) {
        return -1;
    }
    memset(sMsgFrameParam, 0, sizeof(sMsgParam));
    if (ecSendBufBkp == NULL) {
        ecSendBufBkp =
            (ocmwSchemaSendBuf *) malloc(sizeof(ocmwSchemaSendBuf));
    }
    if (ecSendBufBkp == NULL) {
        return -1;
    }
    memset(ecSendBufBkp, 0, sizeof(ocmwSchemaSendBuf));
    /* Component id is starting from 1 */
    sMsgFrameParam->component = 1;
    while (subSystem && subSystem->name) {
        if (strncmp(subSystem->name, msgFrame->subsystem,
                strlen(msgFrame->subsystem)) == 0) {
            ecSendBuf->subsystem = (sMsgFrameParam->subsystem);
            component = subSystem->components;
            while (component && component->name) {
                if (strcmp(component->name, msgFrame->component) == 0) {
                    ecSendBuf->componentId = sMsgFrameParam->component;
                    comm = component->commands;
                    /*
                     * Here we check if command is present in
                     * subsystem->component->command
                     */
                    while (comm && comm->name) {
                        if (strncmp(comm->name, msgFrame->parameter,
                                    strlen(msgFrame->parameter)) == 0) {
                            paramId = 0;
                            actionType = count;
                            subCount ++;
                            break;
                        }
                        count++;
                        comm += 1;
                    }
                    if (subCount != 0)
                        break;
                    driver = component->driver;
                    /*
                     * Here we check if command is present in
                     * subsystem->component->driver->command
                     */
                    componentCount = 0;
                    if(driver != NULL) {
                        comm = driver->commands;
                        while (comm && comm->name) {
                            if (strncmp(comm->name, msgFrame->parameter,
                                        strlen(msgFrame->parameter)) == 0) {
                                paramId = 0;
                                actionType = componentCount;
                                subCount ++;
                                break;
                            }
                            componentCount++;
                            comm += 1;
                        }
                    }
                    if (subCount != 0)
                        break;
                    /*
                     * Here we check if command is present in
                     * subsystem->component->subcomponent->driver->command
                     */
                    count = 1;
                    subcomponent = component->components;
                    while (subcomponent && subcomponent->name) {
                        if (strcmp(subcomponent->name, msgFrame->subcomponent)
                                == 0) {
                            driver = subcomponent->driver;
                            if(driver != NULL) {
                                comm = driver->commands;
                                componentCount = 0;
                                while (comm && comm->name) {
                                    if (strncmp(comm->name, msgFrame->parameter,
                                        strlen(msgFrame->parameter)) == 0) {
                                        /*
                                        * Here subcomponentId = paramId
                                        */
                                        paramId = count;
                                        actionType = componentCount;
                                        subCount ++;
                                        /*break as we found the command */
                                        break;
                                    }
                                    comm += 1;
                                    componentCount++;
                                }
                                break;
                            }
                        }
                        count++;
                        sMsgFrameParam->parameter += 1;
                        subcomponent += 1;
                    }
                }
                sMsgFrameParam->component += 1;
                component += 1;
            }
            /*
             * Break here because we are searching in only one subsystem
             */
            break;
        }
        sMsgFrameParam->subsystem += 1;
        subSystem += 1;
    }
    ecSendBuf->msgType = OCMP_MSG_TYPE_COMMAND;
    ecSendBuf->actionType = actionType;
    ecSendBuf->paramPos = paramPos;
    ecSendBuf->paramId = paramId;
    ecSendBuf->paramSize = paramSize;
    if(msgFrame->parameter) {
        memset(ecSendBuf->commandType, 0,
                strlen(ecSendBuf->commandType));
        strncpy(ecSendBuf->commandType, msgFrame->parameter,
                                strlen(msgFrame->parameter));
    }
    memcpy(ecSendBufBkp,ecSendBuf,sizeof(ocmwSchemaSendBuf));

    free(sMsgFrameParam);

    if (subCount ==  0) {
        return FAILED;
    }

    return SUCCESS;
}

/******************************************************************************
 * Function Name    : ocmw_parse_post_msgframe
 * Description      : parse the post
 * Input(s)         : Schema, msgFrame, actiontype, ecSendBuf
 * Output(s)        :
 ******************************************************************************/
int32_t ocmw_parse_post_msgframe(const Component *compBase,
                            strMsgFrame *msgFrame, uint8_t actiontype,
                            ocmwSchemaSendBuf *ecSendBuf)
{
    const Component *component = NULL;
    const Component *subSystem = compBase;
    const Post *param = NULL;
    const Driver *devDriver = NULL;
    int16_t paramId = 0;
    int8_t count = 0;
    int8_t paramSize = 0;
    int8_t paramPos = 0;
    sMsgParam *sMsgFrameParam = (sMsgParam *) malloc(sizeof(sMsgParam));
    if (sMsgFrameParam == NULL) {
        return FAILED;
    }
    if (ecSendBufBkp == NULL) {
        ecSendBufBkp =
            (ocmwSchemaSendBuf *) malloc(sizeof(ocmwSchemaSendBuf));
    }
    if (ecSendBufBkp == NULL) {
        return FAILED;
    }
    recvdParamVal = 0;
    eepromFlag = 0;
    s_typeFlag = 0;
    if (subSystem == NULL) {
        printf("Invalid Memory\n");
        return FAILED;
    }
    memset(sMsgFrameParam, 0, sizeof(sMsgParam));
    sMsgFrameParam->component = 1;
    memset(ecSendBufBkp, 0, sizeof(ocmwSchemaSendBuf));
    while (subSystem && subSystem->name) {
        if (strncmp(subSystem->name, msgFrame->subsystem,
                strlen(msgFrame->subsystem)) == 0) {
            ecSendBuf->subsystem = (sMsgFrameParam->subsystem);
            component = subSystem->components;
            while (component && component->name) {
                if (strcmp(component->name, msgFrame->component) == 0) {
                    ecSendBuf->componentId = (sMsgFrameParam->component);
                    devDriver = component->driver;
                    ecSendBuf->msgType = OCMP_MSG_TYPE_POST;
                    ecSendBuf->actionType = actiontype;
                    param = devDriver->post;
                    while (param && param->name) {
                        if (strncmp(param->name, msgFrame->subcomponent,
                                strlen(msgFrame->subcomponent)) == 0) {
                            paramId = pow(2,count);
                            break;
                        }
                        count++;
                        param += 1;
                    }
                    break;
                }
                sMsgFrameParam->component += 1;
                component += 1;
            }
            break;
        }
        sMsgFrameParam->subsystem += 1;
        subSystem += 1;
    }
    ecSendBuf->paramPos = paramPos;
    ecSendBuf->paramId = paramId;
    ecSendBuf->paramSize = paramSize;
    memcpy(ecSendBufBkp, ecSendBuf, sizeof(ocmwSchemaSendBuf));
    free(sMsgFrameParam);
    return SUCCESS;
}

/******************************************************************************
 * Function Name    : ocmw_parse_driver_from_component
 * Description      : parse driver as per schema
 * Input(s)         : msgFrame, ecSendBuf, devDriver, actiontype
 * Output(s)        :
 ******************************************************************************/
int32_t ocmw_parse_driver_from_component(strMsgFrame *msgFrame,
            ocmwSchemaSendBuf *ecSendBuf, const Driver *devDriver,
                                                uint8_t actiontype)
{
    const Parameter *param = NULL;
    Enum_Map *value_enum = NULL;
    int8_t count = 0;
    int8_t index = 0;
    int8_t regValue = 0;
    int32_t temp = 0;
    if (strncmp(msgFrame->msgtype, "config", strlen("config")) == 0) {
        param = devDriver->config;
        ecSendBuf->msgType = OCMP_MSG_TYPE_CONFIG;
        ecSendBuf->actionType = actiontype;
        while (param && param->name) {
            if (strncmp(param->name, msgFrame->parameter,
                    strlen(msgFrame->parameter)) == 0) {
                ecSendBuf->paramId = pow(2,count);
                ecSendBuf->paramPos = temp + ecSendBuf->paramPos;
                ecSendBuf->paramSize = ocmw_get_paramSize (
                                        DATA_TYPE_MAP[param->type],
                                        OCMP_MSG_TYPE_CONFIG, param->name);
                break;
            }
            count = count + 1;
            if ((!strcmp("uint16", DATA_TYPE_MAP[param->type])) ||
                    (!strcmp("int16", DATA_TYPE_MAP[param->type]))) {
                ecSendBuf->paramPos = ecSendBuf->paramPos + sizeof(uint16_t);
            } else if ((!strcmp("uint8", DATA_TYPE_MAP[param->type])) ||
                    (!strcmp("int8", DATA_TYPE_MAP[param->type]))) {
                ecSendBuf->paramPos = ecSendBuf->paramPos + sizeof(uint8_t);
            }
            param += 1;
        }
    } else if (strncmp(msgFrame->msgtype, "status", strlen("status")) == 0) {
        param = devDriver->status;
        ecSendBuf->msgType = OCMP_MSG_TYPE_STATUS;
        ecSendBuf->actionType = actiontype;
        while (param && param->name) {
            if (strncmp(param->name, msgFrame->parameter,
                    strlen(msgFrame->parameter)) == 0) {
                ecSendBuf->paramId = pow(2,count);
                if ((strncmp(param->name,"registration",
                        strlen(msgFrame->parameter)) == 0) ||
                        (strncmp(param->name,"gps_lock",
                        strlen(msgFrame->parameter)) == 0)) {
                    value_enum = param->values;
                    for (index =index; index < OCMW_VALUE_TYPE_MFG; index++) {
                        s_paramEnumValue[index] = (char *)malloc(BUF_SIZE);
                    }
                    while (value_enum && value_enum->name) {
                        regValue = value_enum->value;
                        strncpy((s_paramEnumValue[regValue]), value_enum->name,
                            ENUM_BUF_SIZE);
                        value_enum += 1;
                    }
                    s_typeFlag = OCMW_VALUE_TYPE_STRUCT;
                } else if(strncmp(param->name, "network_operatorinfo",
                    strlen(msgFrame->parameter)) == 0) {
                    s_typeFlag = OCMW_VALUE_TYPE_NWOP_STRUCT;
                }
                if ((strncmp(msgFrame->subsystem, "obc",
                        strlen(msgFrame->subsystem)) == 0) ||
                        (strncmp(msgFrame->subsystem, "testmodule",
                        strlen(msgFrame->subsystem)) == 0)) {
                    ecSendBuf->paramPos = 0;
                } else {
                    ecSendBuf->paramPos = temp + ecSendBuf->paramPos;
                }
                if ((param->size) &&
                        (strcmp(DATA_TYPE_MAP[param->type],"enum") != 0)) {
                    ecSendBuf->paramSize = param->size;
                } else {
                    ecSendBuf->paramSize = ocmw_get_paramSize(
                                            DATA_TYPE_MAP[param->type],
                                            OCMP_MSG_TYPE_STATUS, param->name);
                }
                break;
            }
            count = count + 1;
            if ((!strcmp("uint16", DATA_TYPE_MAP[param->type])) ||
                    (!strcmp("int16", DATA_TYPE_MAP[param->type]))) {
                ecSendBuf->paramPos = ecSendBuf->paramPos + sizeof(uint16_t);
            } else if ((!strcmp("uint8", DATA_TYPE_MAP[param->type])) ||
                    (!strcmp("int8", DATA_TYPE_MAP[param->type]))) {
                ecSendBuf->paramPos = ecSendBuf->paramPos + sizeof(uint8_t);
            } else if (!strcmp("string", DATA_TYPE_MAP[param->type])) {
                ecSendBuf->paramPos = ecSendBuf->paramPos +
                                                EEPROM_STATUS_MAX_SIZE;
            }
            param += 1;
        }
    } else if (strncmp(msgFrame->msgtype, "alerts", strlen("alerts")) == 0) {
        param = devDriver->alerts;
    }
    return SUCCESS;
}

/******************************************************************************
 * Function Name    : ocmw_parse_driver_from_subcomponent
 * Description      : parse driver as per schema
 * Input(s)         : msgFrame, ecSendBuf, subComponent, actiontype
 * Output(s)        :
 ******************************************************************************/
int32_t ocmw_parse_driver_from_subcomponent (strMsgFrame *msgFrame,
            ocmwSchemaSendBuf *ecSendBuf, const Component *subComponent,
                                                uint8_t actiontype)
{
    const Driver *devDriver = NULL;
    const Parameter *param = NULL;
    int32_t noOfElementTemp = 0;
    int32_t noOfElement = 0;
    int32_t temp = 0;
    int32_t size = 0;
    int8_t count = 0;
    while (subComponent && subComponent->name) {
        devDriver = subComponent->driver;
        temp = temp + size;
        if (strncmp(msgFrame->msgtype, "config", strlen("config")) == 0) {
            param = devDriver->config;
            ocmw_get_noOfElements(param,&noOfElement, &size);
        } else if (strncmp(msgFrame->msgtype, "status",
            strlen("status")) == 0) {
            param = devDriver->status;
            ocmw_get_noOfElements(param,&noOfElement, &size);
        }
        if (strncmp(subComponent->name, msgFrame->subcomponent,
                strlen(msgFrame->subcomponent)) == 0) {
            devDriver = subComponent->driver;
            if (strncmp(msgFrame->msgtype, "config", strlen("config")) == 0) {
                param = devDriver->config;
                ecSendBuf->msgType = OCMP_MSG_TYPE_CONFIG;
                ecSendBuf->actionType = actiontype;
                while (param && param->name) {
                    if (strncmp(param->name, msgFrame->parameter,
                        strlen(msgFrame->parameter)) == 0) {
                        ecSendBuf->paramId = pow(2,
                                                count + noOfElementTemp);
                        ecSendBuf->paramPos = temp + ecSendBuf->paramPos;
                        if (param->size) {
                            ecSendBuf->paramSize = param->size;
                        } else {
                            ecSendBuf->paramSize = ocmw_get_paramSize(
                                            DATA_TYPE_MAP[param->type],
                                            OCMP_MSG_TYPE_CONFIG, param->name);
                        }
                        break;
                    }
                    count = count + 1;
                    if ((!strcmp("uint16", DATA_TYPE_MAP[param->type])) ||
                            (!strcmp("int16", DATA_TYPE_MAP[param->type]))) {
                        ecSendBuf->paramPos =
                                ecSendBuf->paramPos + sizeof(uint16_t);
                    } else if ((!strcmp("uint8", DATA_TYPE_MAP[param->type]))
                        || (!strcmp("int8", DATA_TYPE_MAP[param->type]))) {
                        ecSendBuf->paramPos =
                            ecSendBuf->paramPos + sizeof(uint8_t);
                    }
                    param += 1;
                }
            } else if (strncmp(msgFrame->msgtype, "status",
                    strlen("status")) == 0) {
                param = devDriver->status;
                ecSendBuf->msgType = OCMP_MSG_TYPE_STATUS;
                ecSendBuf->actionType = actiontype;
                while (param && param->name) {
                    if (strncmp(param->name, msgFrame->parameter,
                        strlen(msgFrame->parameter)) == 0) {
                        ecSendBuf->paramId =
                            pow(2, count + noOfElementTemp);
                        ecSendBuf->paramPos = temp + ecSendBuf->paramPos;
                        if (param->size) {
                            ecSendBuf->paramSize = param->size;
                        } else {
                            ecSendBuf->paramSize = ocmw_get_paramSize (
                                    DATA_TYPE_MAP[param->type],
                                    OCMP_MSG_TYPE_STATUS, param->name);
                        }
                        break;
                    }
                    count = count + 1;
                    if ((!strcmp("uint16", DATA_TYPE_MAP[param->type])) ||
                            (!strcmp("int16", DATA_TYPE_MAP[param->type]))) {
                        ecSendBuf->paramPos =
                            ecSendBuf->paramPos + sizeof(uint16_t);
                    } else if ((!strcmp("uint8", DATA_TYPE_MAP[param->type]))
                            || (!strcmp("int8", DATA_TYPE_MAP[param->type]))) {
                        ecSendBuf->paramPos =
                                    ecSendBuf->paramPos + sizeof(uint8_t);
                    } else if (!strcmp("string", DATA_TYPE_MAP[param->type])) {
                        ecSendBuf->paramPos = ecSendBuf->paramPos +
                                                    EEPROM_STATUS_MAX_SIZE;
                    }
                    param += 1;
                }
            } else if (strncmp(msgFrame->msgtype, "alerts",
                    strlen("alerts")) == 0) {
                param = devDriver->alerts;
            }
            break;
        }
        noOfElementTemp = noOfElementTemp + noOfElement;
        subComponent += 1;
    }
    return SUCCESS;
}

/******************************************************************************
 * Function Name    : ocmw_parse_msgframe
 * Description      : parse msgframe as per schema
 * Input(s)         : compBase, msgFrame, actiontype, ecSendBuf
 * Output(s)        :
 ******************************************************************************/
int32_t ocmw_parse_msgframe(const Component *compBase, strMsgFrame *msgFrame,
            uint8_t actiontype,ocmwSchemaSendBuf *ecSendBuf)
{
    const Component *component = NULL;
    const Component *subComponent = NULL;
    const Component *subSystem = compBase;
    const Driver *devDriver = NULL;
    sMsgParam *sMsgFrameParam = (sMsgParam *) malloc(sizeof(sMsgParam));
    recvdParamVal = 0;
    eepromFlag = 0;
    s_typeFlag = 0;

    if (sMsgFrameParam == NULL) {
        return FAILED;
    }
    memset(ecSendBuf, 0, sizeof(ocmwSchemaSendBuf));
    memset(sMsgFrameParam, 0, sizeof(sMsgParam));

    if (ecSendBufBkp == NULL) {
        ecSendBufBkp =
            (ocmwSchemaSendBuf *) malloc(sizeof(ocmwSchemaSendBuf));
    }
    if (ecSendBufBkp == NULL) {
        return -1;
    }
    memset(ecSendBufBkp, 0, sizeof(ocmwSchemaSendBuf));
    if (subSystem == NULL) {
        printf("Invalid Memory\n");
        return FAILED;
    }

    /* Component id is starting from 1*/
    sMsgFrameParam->component = 1;
    while (subSystem && subSystem->name) {
        if (strncmp(subSystem->name, msgFrame->subsystem,
                strlen(msgFrame->subsystem)) == 0) {
            ecSendBuf->subsystem = (sMsgFrameParam->subsystem);
            component = subSystem->components;
            while (component && component->name) {
                if (strcmp(component->name, msgFrame->component) == 0) {
                    ecSendBuf->componentId = sMsgFrameParam->component;
                    devDriver = component->driver;
                    if (devDriver != NULL) {
                        ocmw_parse_driver_from_component(msgFrame,
                                                ecSendBuf,devDriver,actiontype);
                    }
                    subComponent = component->components;
                    if (subComponent != NULL){
                        ocmw_parse_driver_from_subcomponent(msgFrame,
                                                ecSendBuf,subComponent,actiontype);
                    }
                    break;
                }
                sMsgFrameParam->component += 1;
                component += 1;
            }
            break;
        }
        printf("\n");
        sMsgFrameParam->subsystem += 1;
        subSystem += 1;
    }
    memcpy(ecSendBufBkp, ecSendBuf, sizeof(ocmwSchemaSendBuf));
    free(sMsgFrameParam);

    return SUCCESS;
}
/******************************************************************************
 * Function Name    : ocmw_parse_message_fram_from_ec
 * Description      : parse the msgframe data coming from EC to AP
 * Input(s)         : ecReceivedMsg
 * Output(s)        :
 ******************************************************************************/
int32_t ocmw_parse_message_fram_from_ec(OCMPMessageFrame *ecReceivedMsg)
{
    int32_t ret = 0;
    int32_t index = 0;
    int32_t paramVal = 0;
    char regStr[BUF_SIZE] = {0};
    size_t size = 0;

    ocmwSendRecvBuf ecInputData;
    switch(ecSendBufBkp->paramSize) {
        case (sizeof(uint16_t)):
            if (s_typeFlag == OCMW_VALUE_TYPE_UINT16) {
                recvdParamVal =
                    *((uint16_t *)
                    &(ecReceivedMsg->message.info[ecSendBufBkp->paramPos]));
            } else if (s_typeFlag == OCMW_VALUE_TYPE_INT16) {
                recvdParamVal =
                    *((int16_t *)
                    &(ecReceivedMsg->message.info[ecSendBufBkp->paramPos]));
            }
            break;
        case (sizeof(uint8_t)) :
            if (s_typeFlag == OCMW_VALUE_TYPE_UINT8) {
                recvdParamVal =
                    *((uint8_t *)
                    &(ecReceivedMsg->message.info[ecSendBufBkp->paramPos]));
            } else if (s_typeFlag == OCMW_VALUE_TYPE_INT8) {
                recvdParamVal =
                    *((int8_t *)
                    &(ecReceivedMsg->message.info[ecSendBufBkp->paramPos]));
            } else if (s_typeFlag == OCMW_VALUE_TYPE_ENUM) {
                recvdParamVal =
                    *((int8_t *)
                    &(ecReceivedMsg->message.info[ecSendBufBkp->paramPos]));
            }
            break;
        case (sizeof(uint32_t)):
            if (s_typeFlag == OCMW_VALUE_TYPE_UINT32) {
                recvdParamVal =
                    *((uint32_t *)
                    &(ecReceivedMsg->message.info[ecSendBufBkp->paramPos]));
            } else {
                memset(dataOutBufFromEc, 0 ,
                                    sizeof(dataOutBufFromEc));
                memcpy(dataOutBufFromEc, ecReceivedMsg->message.info,
                                    OCMW_VALUE_TYPE_MODEL);
                eepromFlag++;
            }
            break;
        case (sizeof(uint64_t)):
            recvdParamVal =
                *((uint64_t *)
                &(ecReceivedMsg->message.info[ecSendBufBkp->paramPos]));
            break;
        case EEPROM_CONFIG_MAX_SIZE:
        case EEPROM_CONFIG_SIZE:
        case EEPROM_SDR_STATUS_SIZE:
        case EEPROM_STATUS_MAX_SIZE:
            memset(&ecInputData, 0, sizeof(ocmwSendRecvBuf));
            ecInputData.numOfele = 0;
            ecInputData.paramInfo = ecReceivedMsg->message.parameters;
            ecInputData.componentId = ecReceivedMsg->message.componentID;
            ecInputData.msgType = ecReceivedMsg->message.msgtype;
            ecInputData.actionType = ecReceivedMsg->message.action;
            ecInputData.subsystem = ecReceivedMsg->message.subsystem;
            memcpy(ecInputData.pbuf, ecReceivedMsg->message.info,
                                                    MAX_PARM_COUNT);
            ret = ocmw_parse_eepromdata_from_ec(ecInputData);
            eepromFlag++;
            break;
        case OCMW_VALUE_TYPE_MFG:
            memset(dataOutBufFromEc, 0, sizeof(dataOutBufFromEc));
            memcpy(dataOutBufFromEc,ecReceivedMsg->message.info,
                                    OCMW_VALUE_TYPE_MFG);
            eepromFlag++;
            break;
        case OCMW_VALUE_TYPE_GETMODEL:
            memset(dataOutBufFromEc, 0, sizeof(dataOutBufFromEc));
            memcpy(dataOutBufFromEc, ecReceivedMsg->message.info,
                                            OCMW_VALUE_TYPE_GETMODEL);
            eepromFlag++;
            break;
        case OCMW_VALUE_TYPE_ENUM:
        case OCMW_VALUE_TYPE_COMPLEX:
            memset(&ecInputData, 0, sizeof(ocmwSendRecvBuf));
            ecInputData.numOfele = 0;
            ecInputData.paramInfo = ecReceivedMsg->message.parameters;
            ecInputData.componentId = ecReceivedMsg->message.componentID;
            ecInputData.msgType = ecReceivedMsg->message.msgtype;
            ecInputData.actionType = ecReceivedMsg->message.action;
            ecInputData.subsystem = ecReceivedMsg->message.subsystem;
            memcpy(ecInputData.pbuf, ecReceivedMsg->message.info,
                                                    MAX_PARM_COUNT);
            if (s_typeFlag == OCMW_VALUE_TYPE_STRUCT) {
                paramVal = *((uint8_t *) ecInputData.pbuf);
                memset(dataOutBufFromEc, 0, sizeof(dataOutBufFromEc));
                size = sizeof(dataOutBufFromEc);
                if (s_paramEnumValue[paramVal]) {
                    strcpy(regStr,s_paramEnumValue[paramVal]);
                    snprintf(dataOutBufFromEc, size,
                                 "%u(%s) ", paramVal,regStr);
                    for (index = 0; index < OCMW_VALUE_TYPE_MFG; index++) {
                        ocmw_free_global_pointer(
                            (void**)&s_paramEnumValue[index]);
                    }
                }
            }else if (s_typeFlag == OCMW_VALUE_TYPE_NWOP_STRUCT) {
                ret = ocmw_parse_testingmodule_struct_from_ec(ecInputData);
            } else {
                ret = ocmw_parse_obc_struct_from_ec(ecInputData);
            }
            eepromFlag++;
            break;
        default:
            break;
    }
    if (ret < 0) {
        responseCount = 0;
    } else {
        responseCount++;
    }
    return SUCCESS;
}

/******************************************************************************
 * Function Name    : ocmw_deserialise_debug_gpio_value
 * Description      : parse the msgframe data coming from EC to AP
 * Input(s)         : ecReceivedMsg
 * Output(s)        : dataOutBufFromEc
 ******************************************************************************/
void ocmw_deserialise_debug_gpio_value(OCMPMessageFrame *ecReceivedMsg)
{
    int8_t pin = 0;
    int8_t value = 0;
    size_t size = 0;

    memset(dataOutBufFromEc, 0,  sizeof(dataOutBufFromEc));
    size = sizeof(dataOutBufFromEc);
    pin = *((uint8_t *)&(ecReceivedMsg->message.info[ecSendBufBkp->paramPos]));
    value = *((uint8_t *)&(ecReceivedMsg->message.info[ecSendBufBkp->paramPos +
                                                        1]));
    snprintf(dataOutBufFromEc, size, "(Pin No : %d) %s = %d", pin,
                ecSendBufBkp->commandType, value);
    return;
}
/******************************************************************************
 * Function Name    : ocmw_deserialise_debug_mdio_value
 * Description      : parse the msgframe data coming from EC to AP
 * Input(s)         : ecReceivedMsg
 * Output(s)        : dataOutBufFromEc
 ******************************************************************************/
void ocmw_deserialise_debug_mdio_value(OCMPMessageFrame *ecReceivedMsg)
{
    uint16_t registerAddress = 0;
    uint16_t registerData = 0;
    size_t size = 0;
    registerAddress = *((uint16_t *)
                 &(ecReceivedMsg->message.info[ecSendBufBkp->paramPos]));
    registerData = *((uint16_t *)
                 &(ecReceivedMsg->message.info[ecSendBufBkp->paramPos + 2]));
    memset(dataOutBufFromEc, 0, sizeof(dataOutBufFromEc));
    size = sizeof(dataOutBufFromEc);
    snprintf(dataOutBufFromEc, size, "(Register Address : %d) %s = %d",
                registerAddress, ecSendBufBkp->commandType, registerData);
    return;
}
/******************************************************************************
 * Function Name    : ocmw_deserialise_debug_i2c_value
 * Description      : parse the msgframe data coming from EC to AP
 * Input(s)         : ecReceivedMsg
 * Output(s)        : dataOutBufFromEc
 ******************************************************************************/
void ocmw_deserialise_debug_i2c_value(OCMPMessageFrame *ecReceivedMsg)
{
    uint8_t slaveAddress = 0;
    uint8_t noOfBytes = 0;
    uint8_t registerAddress = 0;
    uint16_t registerData = 0;
    size_t size = 0;

    slaveAddress = *((uint8_t *)
                 &(ecReceivedMsg->message.info[ecSendBufBkp->paramPos]));
    noOfBytes = *((uint8_t *)
                 &(ecReceivedMsg->message.info[ecSendBufBkp->paramPos + 1]));
    registerAddress = *((uint8_t *)
                 &(ecReceivedMsg->message.info[ecSendBufBkp->paramPos + 2]));

    if (noOfBytes == OCMW_VALUE_TYPE_UINT8) {
        registerData = *((uint8_t *)
            &(ecReceivedMsg->message.info[ecSendBufBkp->paramPos + 3]));
    } else {
        registerData = ((uint16_t )((((uint8_t)
             (ecReceivedMsg->message.info[ecSendBufBkp->paramPos
             + 3]) & 0xff) << 8) |
             ((uint8_t)
             ((ecReceivedMsg->message.info[ecSendBufBkp->paramPos
             + 4])) & 0xff)));
    }

    memset(dataOutBufFromEc, 0, sizeof(dataOutBufFromEc));
    size = sizeof(dataOutBufFromEc);
    snprintf(dataOutBufFromEc, size, "(slave address :%d noOfBytes :%d Register"
        " Address :%d) %s = %d", slaveAddress, noOfBytes,
        registerAddress, ecSendBufBkp->commandType, registerData);

    if (strcmp(ecSendBufBkp->commandType, "set") == 0) {
        strcat(dataOutBufFromEc, " : Success");
    }
    return;
}
/******************************************************************************
 * Function Name    : ocmw_deserialization_msgframe
 * Description      : parse the msgframe data coming from EC to AP
 * Input(s)         : subSystem, ecReceivedMsg
 * Output(s)        : dMsgFrameParam
 ******************************************************************************/
void ocmw_deserialization_msgframe(const Component *subSystem,
                sMsgParam *dMsgFrameParam, OCMPMessageFrame *ecReceivedMsg)
{
    int32_t ret = 0;
    int8_t index = 0;

    if ((subSystem == NULL) || (ecSendBufBkp == NULL)) {
        return;
    }

    if (ecReceivedMsg->message.parameters != 0) {
        if ((ecReceivedMsg->message.subsystem ==
                            ecSendBufBkp->subsystem) &&
            (ecReceivedMsg->message.componentID ==
                          ecSendBufBkp->componentId) &&
            (ecReceivedMsg->message.msgtype ==
                              ecSendBufBkp->msgType) &&
            (ecReceivedMsg->message.action ==
                              OCMP_AXN_TYPE_REPLY) &&
            (ecReceivedMsg->message.parameters ==
                             ecSendBufBkp->paramId)) {

            ret = ocmw_parse_message_fram_from_ec(ecReceivedMsg);
            if (ret < 0) {
                printf("\nocmw_parse_message_fram_from_ec error");
            }
        }
    }
    if (ecReceivedMsg->message.msgtype == OCMP_MSG_TYPE_COMMAND) {
        if ((ecReceivedMsg->message.subsystem ==
                            ecSendBufBkp->subsystem) &&
            (ecReceivedMsg->message.componentID ==
                          ecSendBufBkp->componentId) &&
            (ecReceivedMsg->message.msgtype ==
                              ecSendBufBkp->msgType) &&
            (ecReceivedMsg->message.action ==
                              OCMP_AXN_TYPE_REPLY) &&
            (ecReceivedMsg->message.parameters ==
                             ecSendBufBkp->paramId)) {
            if ((ecReceivedMsg->message.subsystem == DEBUG_SUBSYSTEM_NBR)) {
                if (ecReceivedMsg->message.componentID < DEBUG_I2C) {
                    ocmw_deserialise_debug_i2c_value(ecReceivedMsg);
                } else if (ecReceivedMsg->message.componentID ==
                                                DEBUG_MDIO) {
                    ocmw_deserialise_debug_mdio_value(ecReceivedMsg);
                } else {
                    ocmw_deserialise_debug_gpio_value(ecReceivedMsg);
                }
            }
            responseCount++;
        }
    }
    /* Free backup info if stored */
    for (index = 0; index < OCMW_VALUE_TYPE_MFG; index++) {
        ocmw_free_global_pointer((void**)&s_paramEnumValue[index]);
    }
    if (ecSendBufBkp) {
        ocmw_free_global_pointer((void**)&ecSendBufBkp);
    }
}
