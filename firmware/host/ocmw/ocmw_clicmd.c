/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

/* stdlib includes */
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <logger.h>
#include <inttypes.h>
/* OC includes */
#include <ocmw_occli_comm.h>
#include <ocmw_core.h>
#include <ocmw_msgproc.h>
#include <ocmp_frame.h>
#include <ctype.h>

#define INVALID_SYNTAX "Error : Invalid syntax"
#define INSUFFICIENT_PARAM "Error : Insufficient parameters"

extern int8_t obcTestingmoduleData[MAX_PARM_COUNT];
extern int8_t dataOutBufFromEc[MAX_PARM_COUNT];
extern int8_t eepromStatusFlag;
extern int64_t recvdParamVal;
extern int8_t eepromStatusFlag;
extern int8_t eepromStatusFlag;
extern int32_t eepromFlag;
extern uint8_t ocwarePostArrayIndex;
debugI2CData I2CInfo;
debugMDIOData MDIOInfo;
int8_t alertFlag = 0;
alertlist alerts;

struct matchString {
    const char *key;
    ocmw_token_t token;
} ocmwTokenTable[] = {
    { "set", SET_STR },
    { "get", GET_STR },
    { "reset", RESET_STR },
    { "enable", ENABLE_STR },
    { "disable", DISABLE_STR },
    { "active", ACTIVE_STR },
    { "echo", ECHO_STR },
    { "disconnect_nw", DISCONNECT_STR },
    { "connect_nw", CONNECT_STR },
    { "send", SEND_STR },
    { "dial", DIAL_STR },
    { "answer", ANSWER_STR },
    { "hangup", HANGUP_STR },
    { "en_loopBk", ELOOPBK_STR },
    { "dis_loopBk", DLOOPBK_STR },
    { "en_pktGen", EPKTGEN_STR },
    { "dis_pktGen", DPKTGEN_STR },
    { "getAlertLogs", ALERTLOG_STR },
    { NULL, MAX_STR },
};

struct matchSetGet {
    const char *key;
    ocmw_setGet token;
} ocmwSetGetTable[] = {
    { "hci.led.fw", HCI_STR },
    { "debug", DEBUG_STR },
    { "system.comp_all.post.results", RESULT_STR },
    { "system.comp_all.post.enable", ENABLE_SET_STR },
    { NULL, GETSETMAX },
};
/**************************************************************************
 * Function Name    : ocmw_tokenize_class
 * Description      : This Function used to extract the class from the param
 *                    string
 * Input(s)         : str, option
 * Output(s)        : param
 ***************************************************************************/
static int32_t ocmw_tokenize_class(char *str, char *param, int32_t option)
{
    char *token;
    int32_t count = 0;

    token = strtok(str, " .");
    if (token == NULL)
        return -1;

    while (token) {
        if (count == 2) {
            strncpy(param, token, PARAMSTR_NUMBER_LEN);
            break;
        }
        token = strtok(NULL, " .");
        count++;

        if (option == 1) {
            strncpy(param, token, PARAMSTR_NUMBER_LEN);
            break;
        }
    }
    return SUCCESS;
}

/**************************************************************************
 * Function Name    : ocmw_tokenize
 * Description      : This Function used to extract the CLI string
 * Input(s)         : cmdstr
 * Output(s)        : strTokenCount, strTokenArray
 ***************************************************************************/
static int32_t ocmw_tokenize(const char *cmdstr, int32_t *strTokenCount,
                             char ***strTokenArray)
{
    char *str = NULL;
    char *saveptr = NULL;
    char *paramStr = NULL;
    char *token = NULL;
    char **localStrTokenArray = NULL;
    char **temp = NULL;
    const char *delim = " ";
    int32_t localStrTokenCount = 0;
    int32_t index = 0;

    /* Split the actiontype from the cmdstr */
    paramStr = strrchr(cmdstr, '.');
    *paramStr = ' ';

    for (index = 1, str = (char *)cmdstr;; index++, str = NULL) {
        token = strtok_r(str, delim, &saveptr);
        if (token == NULL) {
            break;
        }

        localStrTokenCount++;
        temp = localStrTokenArray;
        localStrTokenArray =
            realloc(localStrTokenArray, sizeof(char *) * localStrTokenCount);
        if (localStrTokenArray == NULL) {
            logerr("realloc failed");
            /* Free the original block of memory before realloc */
            if (temp != NULL) {
                free(temp);
            }
            return FAILED;
        }
        localStrTokenArray[localStrTokenCount - 1] = token;
    }

    *strTokenCount = localStrTokenCount;
    *strTokenArray = localStrTokenArray;
    return SUCCESS;
}

/**************************************************************************
 * Function Name    : ocmw_check_numeric_number
 * Description      : This Function used to validate the mobile number
 * Input(s)         : numstring
 * Output(s)        :
 ***************************************************************************/
int32_t ocmw_check_numeric_number(char *numstring)
{
    int32_t index = 0;
    int32_t len = strlen(numstring);

    while (index < len) {
        if ((*(numstring + index) < '0') || (*(numstring + index) > '9')) {
            return FAILED;
        }
        index++;
    }
    return SUCCESS;
}
/**************************************************************************
 * Function Name    : ocmw_frame_alert_response
 * Description      : This Function used to frame alert response
 * Input(s)         : alerts
 * Output(s)        : response
 ***************************************************************************/
int32_t ocmw_frame_alert_response(char *response)
{
    int32_t index = 0;
    char alertstr[ALERT_MAX_BUFF_SIZE] = { 0 };

    if (response == NULL) {
        logerr("%s(): NULL pointer error", __func__);
        return FAILED;
    }
#if 0
    if (alerts.nalerts == index) {
	if ((snprintf(response, RES_STR_BUFF_SIZE, "%s",
                "No Alert Recorded")) < 0) {
        return FAILED;
    }
        return SUCCESS;

    }
#endif
    strncpy(response,
            "-------------------------------------------------------"
            "------------------------------------------------\n",
            ALERT_STR_BUFF_SIZE);
    if (snprintf(alertstr, ALERT_STR_BUFF_SIZE, "%-52s%-19s%-8s%-7s%-5s\n",
                 "ALERT DESCRIPTION", "TIME", "STATUS", "LIMIT",
                 "ACTUAL_VALUE") < 0) {
        return FAILED;
    }
    strncat(response, alertstr, ALERT_STR_BUFF_SIZE);
    if ((snprintf(alertstr, ALERT_STR_BUFF_SIZE,
                  "-----------------------------------------------------------"
                  "--------------------------------------------\n")) < 0) {
        return FAILED;
    }
    strncat(response, alertstr, ALERT_STR_BUFF_SIZE);
    for (index = 0; index < alerts.nalerts + 1; index++) {
        if (snprintf(alertstr, ALERT_STR_BUFF_SIZE, "%-52s%-19s%-8s%-7s%-5s\n",
                     alerts.list[index].string, alerts.list[index].datetime,
                     alerts.list[index].action, alerts.list[index].value,
                     alerts.list[index].actualValue) < 0) {
            return FAILED;
        }
        strncat(response, alertstr, ALERT_STR_BUFF_SIZE);
    }
    if ((snprintf(alertstr, ALERT_STR_BUFF_SIZE,
                  "------------------------------------------------------------"
                  "-------------------------------------------\n")) < 0) {
        return FAILED;
    }
    strncat(response, alertstr, ALERT_STR_BUFF_SIZE);

    return SUCCESS;
}
/**************************************************************************
 * Function Name    : ocmw_handle_show_alerts
 * Description      : This Function used to handle the alerts commands
 * Input(s)         : alerts
 * Output(s)        : response
 ***************************************************************************/
int32_t ocmw_handle_show_alerts(char *response)
{
    int32_t ret = 0;

    if (response == NULL) {
        logerr("%s(): NULL pointer error", __func__);
        return FAILED;
    }
    strncpy(response, "", RES_STR_BUFF_SIZE);
    ret = ocmw_frame_alert_response(response);
    if (ret < 0) {
        free(alerts.list);
        return ret;
    }
    ret = ocmw_send_alert_to_occli(response, RES_STR_BUFF_SIZE);
    free(alerts.list);
    alerts.nalerts = 0;
    return ret;
}

/**************************************************************************
 * Function Name    : ocmw_handle_show_all_alerts
 * Description      : This Function used to handle the syncronous alerts
 * Input(s)         : strTokenArray
 * Output(s)        : response
 ***************************************************************************/
static int32_t ocmw_handle_show_all_alerts(char *strTokenArray[],
                                           char *response)
{
    char paramStr[PARAM_STR_BUFF_SIZE];
    void *paramVal;
    char tempParamStr[PARAM_STR_MAX_BUFF_SIZE] = { 0 };
    int32_t ret = 0;

    if (strTokenArray == NULL || response == NULL) {
        logerr("%s(): NULL pointer error", __func__);
        return FAILED;
    }
    alerts.nalerts = 0;
    alerts.list = (struct allAlertEvent *)calloc(ALERT_MAX_BUFF_SIZE,
                                                 sizeof(struct allAlertEvent));
    alertFlag++;
    strcpy(tempParamStr, strTokenArray[0]);
    strncpy(response, "", RES_STR_BUFF_SIZE);

    if ((snprintf(paramStr, PARAM_STR_BUFF_SIZE, "%s.%s", strTokenArray[1],
                  strTokenArray[0])) < 0) {
        return FAILED;
    }

    ret = ocmw_msgproc_send_msg(&strTokenArray[0], 0, OCMP_MSG_TYPE_COMMAND,
                                (int8_t *)paramStr, &paramVal);

    if ((snprintf(response, RES_STR_BUFF_SIZE, "%s : %s", tempParamStr,
                  (ret != 0) ? "Failed" : "Success")) < 0) {
        return FAILED;
    }

    if (ret < 0) {
        logerr("Error in reading alerts");
    }
    ret = ocmw_frame_alert_response(response);
    alertFlag = 0;
    alerts.nalerts = 0;
    free(alerts.list);
    return ret;
}

/**************************************************************************
 * Function Name    : ocmw_handle_set_config
 * Description      : This Function used to handle the set config commands
 * Input(s)         : strTokenArray
 * Output(s)        : response
 ***************************************************************************/
static int32_t ocmw_handle_set_config(char *strTokenArray[], char *response)
{
    char *paramStr;
    void *paramvalue;
    int32_t ret = 0;
    char tempParamStr[PARAM_STR_MAX_BUFF_SIZE] = { 0 };

    if (strTokenArray == NULL || response == NULL) {
        logerr("%s(): NULL pointer error", __func__);
        return FAILED;
    }
    paramStr = strTokenArray[0];
    paramvalue = strTokenArray[2];
    strcpy(tempParamStr, strTokenArray[0]);

    ret = ocmw_msgproc_send_msg(&strTokenArray[0], OCMP_AXN_TYPE_SET,
                                OCMP_MSG_TYPE_CONFIG, (int8_t *)paramStr,
                                paramvalue);

    if ((snprintf(response, RES_STR_BUFF_SIZE, "%s.%s = %s : %s",
                  strTokenArray[0], strTokenArray[1], strTokenArray[2],
                  (ret != 0) ? "Failed" : "Success")) < 0) {
        return FAILED;
    }

    return ret;
}

/**************************************************************************
 * Function Name    : ocmw_handle_show_config
 * Description      : This Function used to handle the show config commands
 * Input(s)         : strTokenArray
 * Output(s)        : response
 ***************************************************************************/
static int32_t ocmw_handle_show_config(char *strTokenArray[], char *response)
{
    char *paramStr;
    char tempParamStr[PARAM_STR_MAX_BUFF_SIZE] = { 0 };
    int32_t paramVal = 0;
    int32_t ret = 0;

    if (strTokenArray == NULL || response == NULL) {
        logerr("%s(): NULL pointer error", __func__);
        return FAILED;
    }
    paramStr = strTokenArray[0];
    strcpy(tempParamStr, strTokenArray[0]);

    ret = ocmw_msgproc_send_msg(&strTokenArray[0], OCMP_AXN_TYPE_GET,
                                OCMP_MSG_TYPE_CONFIG, (int8_t *)paramStr,
                                &paramVal);

    if (ret != 0) {
        if ((snprintf(response, RES_STR_BUFF_SIZE, "%s.%s : Failed",
                      tempParamStr, strTokenArray[1])) < 0) {
            return FAILED;
        }
    } else {
        if (eepromFlag > 0) {
            if ((snprintf(response, RES_STR_BUFF_SIZE, "%s.%s = %s",
                          tempParamStr, strTokenArray[1], dataOutBufFromEc)) <
                0) {
                return FAILED;
            }
        } else {
            if ((snprintf(response, RES_STR_BUFF_SIZE, "%s.%s = %" PRId64,
                          (int8_t *)tempParamStr, strTokenArray[1],
                          recvdParamVal)) < 0) {
                return FAILED;
            }
        }
    }
    return ret;
}

/**************************************************************************
 * Function Name    : ocmw_handle_show_status
 * Description      : This Function used to handle the status commands
 * Input(s)         : strTokenArray
 * Output(s)        : response
 ***************************************************************************/
static int32_t ocmw_handle_show_status(char *strTokenArray[], char *response)
{
    char *paramStr;
    int32_t ret = 0;
    int32_t value = 0;
    char tempParamStr[PARAM_STR_MAX_BUFF_SIZE] = { 0 };

    if (strTokenArray == NULL || response == NULL) {
        logerr("%s(): NULL pointer error", __func__);
        return FAILED;
    }
    paramStr = strTokenArray[0];
    strcpy(tempParamStr, strTokenArray[0]);

    ret = ocmw_msgproc_send_msg(&strTokenArray[0], OCMP_AXN_TYPE_GET,
                                OCMP_MSG_TYPE_STATUS, (int8_t *)paramStr,
                                (void *)&value);

    if (ret != 0) {
        if ((snprintf(response, RES_STR_BUFF_SIZE, "%s.%s : Failed",
                      tempParamStr, strTokenArray[1])) < 0) {
            return FAILED;
        }
    } else if (eepromFlag > 0) {
        if ((snprintf(response, RES_STR_BUFF_SIZE, "%s.%s = %s", tempParamStr,
                      strTokenArray[1], dataOutBufFromEc)) < 0) {
            return FAILED;
        }
    } else {
        if ((snprintf(response, RES_STR_BUFF_SIZE, "%s.%s = %" PRId64,
                      tempParamStr, strTokenArray[1], recvdParamVal)) < 0) {
            return FAILED;
        }
    }

    return ret;
}

/**************************************************************************
 * Function Name    : ocmw_handle_debug_command_function
 * Description      : This Function used to handle the debug commands
 * Input(s)         : strTokenArray, action
 * Output(s)        : response
 ***************************************************************************/
static int32_t ocmw_handle_debug_command_function(char *strTokenArray[],
                                                  char *response)
{
    char paramStr[PARAM_STR_BUFF_SIZE] = { 0 };
    char displayStr[PARAM_STR_BUFF_SIZE] = { 0 };
    void *paramvalue = NULL;
    int32_t value = 0;
    int32_t ret = 0;

    debugGPIOData GPIOInfo;
    if (strTokenArray == NULL || response == NULL) {
        logerr("%s(): NULL pointer error", __func__);
        return FAILED;
    }
    if ((strncmp("debug", strTokenArray[0], strlen("debug")) == 0) &&
        (strncmp(strTokenArray[1], "set", strlen("set")) == 0)) {
        /* Registers debug option */
        if ((strncmp("debug.I2C", strTokenArray[0], strlen("debug.I2C")) ==
             0) &&
            (strncmp(strTokenArray[1], "set", strlen("set")) == 0)) {
            I2CInfo.slaveAddress = atoi(strTokenArray[2]);
            I2CInfo.numOfBytes = atoi(strTokenArray[3]);
            I2CInfo.regAddress = atoi(strTokenArray[4]);
            I2CInfo.regValue = atoi(strTokenArray[5]);
            paramvalue = (void *)&I2CInfo;
            sprintf(displayStr,
                    "%s (slave address :%s noOfBytes :%s"
                    " Register Address :%s) %s= %s",
                    strTokenArray[0], strTokenArray[2], strTokenArray[3],
                    strTokenArray[4], strTokenArray[1], strTokenArray[5]);
        } else if ((strncmp("debug.ethernet", strTokenArray[0],
                            strlen("debug.ethernet")) == 0)) {
            MDIOInfo.regAddress = atoi(strTokenArray[2]);
            MDIOInfo.regValue = atoi(strTokenArray[3]);
            paramvalue = (void *)&MDIOInfo;
            sprintf(displayStr, "%s (Register Address :%s) %s = %s",
                    strTokenArray[0], strTokenArray[2], strTokenArray[1],
                    strTokenArray[3]);
        } else {
            GPIOInfo.pin = atoi(strTokenArray[2]);
            GPIOInfo.value = atoi(strTokenArray[3]);
            paramvalue = (void *)&GPIOInfo;
            sprintf(displayStr, "%s (Pin No :%s) %s = %s", strTokenArray[0],
                    strTokenArray[2], strTokenArray[1], strTokenArray[3]);
        }
    } else if ((strncmp("debug", strTokenArray[0], strlen("debug")) == 0) &&
               (strncmp(strTokenArray[1], "get", strlen("get")) == 0)) {
        /* Registers debug option */
        if ((strncmp("debug.I2C", strTokenArray[0], strlen("debug.I2C")) ==
             0) &&
            (strncmp(strTokenArray[1], "get", strlen("get")) == 0)) {
            I2CInfo.slaveAddress = atoi(strTokenArray[2]);
            I2CInfo.numOfBytes = atoi(strTokenArray[3]);
            I2CInfo.regAddress = atoi(strTokenArray[4]);
            paramvalue = (void *)&I2CInfo;
            sprintf(displayStr,
                    "%s (slave address :%s noOfBytes :%s"
                    " Register Address :%s) %s",
                    strTokenArray[0], strTokenArray[2], strTokenArray[3],
                    strTokenArray[4], strTokenArray[1]);
        } else if ((strncmp("debug.ethernet", strTokenArray[0],
                            strlen("debug.ethernet")) == 0)) {
            MDIOInfo.regAddress = atoi(strTokenArray[2]);
            paramvalue = (void *)&MDIOInfo;
            sprintf(displayStr, "%s (Register Address :%s) %s",
                    strTokenArray[0], strTokenArray[2], strTokenArray[1]);
        } else {
            GPIOInfo.pin = atoi(strTokenArray[2]);
            paramvalue = (void *)&GPIOInfo;
            sprintf(displayStr, "%s (Pin No :%s) %s", strTokenArray[0],
                    strTokenArray[2], strTokenArray[1]);
        }
    } else {
        paramvalue = (void *)&value;
    }
    if ((snprintf(paramStr, PARAM_STR_BUFF_SIZE, "%s.%s", strTokenArray[1],
                  strTokenArray[0])) < 0) {
        return FAILED;
    }
    ret = ocmw_msgproc_send_msg(&strTokenArray[0], 0, OCMP_MSG_TYPE_COMMAND,
                                (int8_t *)paramStr, paramvalue);

    if (ret != SUCCESS) {
        snprintf(response, RES_STR_BUFF_SIZE, "%s : Failed", displayStr);
    } else {
        if ((snprintf(response, RES_STR_BUFF_SIZE, "%s %s", strTokenArray[0],
                      dataOutBufFromEc)) < 0) {
            return FAILED;
        }
    }
    return ret;
}

/**************************************************************************
 * Function Name    : ocmw_handle_testmod_command_function
 * Description      : This Function used to handle the test modules commands
 * Input(s)         : strTokenArray, action
 * Output(s)        : response
 ***************************************************************************/
static int32_t ocmw_handle_testmod_command_function(char *strTokenArray[],
                                                    char *response)
{
    char paramStr[PARAM_STR_BUFF_SIZE] = { 0 };
    char msgstr[PARAM_STR_BUFF_SIZE] = { 0 };
    void *paramvalue = NULL;
    int32_t value = 0;
    int32_t len = 0;
    int32_t ret = 0;
    char tempParamStr[PARAM_STR_MAX_BUFF_SIZE] = { 0 };

    if (strTokenArray == NULL || response == NULL) {
        logerr("%s(): NULL pointer error", __func__);
        return FAILED;
    }

    if ((strncmp(strTokenArray[1], "send", strlen("send"))) == 0) {
        if (strlen(strTokenArray[2]) > OCMW_MAX_IMEI_SIZE) {
            if ((snprintf(response, RES_STR_BUFF_SIZE,
                          "%s.%s "
                          "(number = %s, msg = %s) : Error : Number "
                          "Invalid",
                          strTokenArray[0], strTokenArray[1], strTokenArray[2],
                          strTokenArray[3])) < 0) {
                return FAILED;
            }
            return FAILED;
        }
        if (ocmw_check_numeric_number(strTokenArray[2]) != SUCCESS) {
            if ((snprintf(response, RES_STR_BUFF_SIZE,
                          "%s.%s "
                          "(number = %s, msg = %s) : Error : Number "
                          "Invalid",
                          strTokenArray[0], strTokenArray[1], strTokenArray[2],
                          strTokenArray[3])) < 0) {
                return FAILED;
            }
            return FAILED;
        }
        len = strlen(strTokenArray[3]);
        len = (len < OCMW_MAX_MSG_SIZE) ? len : OCMW_MAX_MSG_SIZE - 1;
        if ((snprintf(msgstr, PARAM_STR_BUFF_SIZE, "%s", strTokenArray[2])) <
            0) {
            return FAILED;
        }
        if ((snprintf(&msgstr[TESTMOD_MAX_LEN], PARAM_STR_BUFF_SIZE, "%s",
                      strTokenArray[3])) < 0) {
            return FAILED;
        }
        paramvalue = (void *)msgstr;
    } else if (strncmp(strTokenArray[1], "dial", strlen("dial")) == 0) {
        if (strlen(strTokenArray[2]) > OCMW_MAX_IMEI_SIZE) {
            if ((snprintf(response, RES_STR_BUFF_SIZE,
                          "%s.%s "
                          "(number = %s) : Error : Number "
                          "Invalid",
                          strTokenArray[0], strTokenArray[1],
                          strTokenArray[2])) < 0) {
                return FAILED;
            }
            return FAILED;
        }
        if (ocmw_check_numeric_number(strTokenArray[2]) != SUCCESS) {
            if ((snprintf(response, RES_STR_BUFF_SIZE,
                          "%s.%s "
                          "(number = %s) : Error : Number "
                          "Invalid",
                          strTokenArray[0], strTokenArray[1],
                          strTokenArray[2])) < 0) {
                return FAILED;
            }
            return FAILED;
        }
        paramvalue = (void *)(strTokenArray[2]);
    } else {
        paramvalue = (void *)&value;
    }
    strcpy(tempParamStr, strTokenArray[0]);
    if ((snprintf(paramStr, PARAM_STR_BUFF_SIZE, "%s.%s", strTokenArray[1],
                  strTokenArray[0])) < 0) {
        return FAILED;
    }
    ret = ocmw_msgproc_send_msg(&strTokenArray[0], 0, OCMP_MSG_TYPE_COMMAND,
                                (int8_t *)paramStr, paramvalue);
    if (strncmp(strTokenArray[1], "get", strlen("get")) == 0) {
        if ((snprintf(response, RES_STR_BUFF_SIZE, "%s.%s = %s", tempParamStr,
                      strTokenArray[1], dataOutBufFromEc)) < 0) {
            return FAILED;
        }
    } else if (strncmp(strTokenArray[1], "send", strlen("send")) == 0) {
        if ((snprintf(response, RES_STR_BUFF_SIZE,
                      "%s.%s (number = %s msg = %s)"
                      " : %s",
                      strTokenArray[0], strTokenArray[1], strTokenArray[2],
                      strTokenArray[3],
                      (ret != SUCCESS) ? "Failed" : "Success")) < 0) {
            return FAILED;
        }
    } else if (strncmp(strTokenArray[1], "dial", strlen("dial")) == 0) {
        if ((snprintf(response, RES_STR_BUFF_SIZE,
                      "%s.%s (number = %s) :"
                      " %s",
                      strTokenArray[0], strTokenArray[1], strTokenArray[2],
                      (ret != SUCCESS) ? "Failed" : "Success")) < 0) {
            return FAILED;
        }
    } else {
        if ((snprintf(response, RES_STR_BUFF_SIZE, "%s.%s : %s",
                      strTokenArray[0], strTokenArray[1],
                      (ret != SUCCESS) ? "Failed" : "Success")) < 0) {
            return FAILED;
        }
    }
    return ret;
}
/**************************************************************************

 * Function Name    : ocmw_handle_ethernet_command_function
 * Description      : This Function used to handle the loopBack and packent
 *                    genrator commands
 * Input(s)         : strTokenArray, action
 * Output(s)        : response
 ***************************************************************************/
static int8_t ocmw_handle_ethernet_command_function(char *strTokenArray[],
                                                    char *response)
{
    char paramStr[PARAM_STR_BUFF_SIZE] = { 0 };
    void *paramvalue = NULL;

    int32_t value = 0;
    int32_t ret = 0;
    char tempParamStr[PARAM_STR_MAX_BUFF_SIZE] = { 0 };

    if (strTokenArray == NULL || response == NULL) {
        logerr("%s(): NULL pointer error", __func__);
        return FAILED;
    }
    value = atoi(strTokenArray[2]);

    if (((value > 2) || (value < 0)) && (strstr(strTokenArray[1], "loopBk"))) {
        if ((snprintf(response, RES_STR_BUFF_SIZE,
                      "%s.%s "
                      "(number = %s) : Error : Number Invalid",
                      strTokenArray[0], strTokenArray[1], strTokenArray[2])) <
            0) {
            return FAILED;
        }
        return FAILED;
    }

    paramvalue = (void *)&value;
    strcpy(tempParamStr, strTokenArray[0]);
    if ((snprintf(paramStr, PARAM_STR_BUFF_SIZE, "%s.%s", strTokenArray[1],
                  strTokenArray[0])) < 0) {
        return FAILED;
    }
    ret = ocmw_msgproc_send_msg(&strTokenArray[0], 0, OCMP_MSG_TYPE_COMMAND,
                                (int8_t *)paramStr, paramvalue);
    if (strncmp(strTokenArray[1], "dis_pktGen", strlen("dis_pktGen")) == 0) {
        if ((snprintf(response, RES_STR_BUFF_SIZE, "%s.%s : %s",
                      strTokenArray[0], strTokenArray[1],
                      (ret != 0) ? "Failed" : "Success")) < 0) {
            return FAILED;
        }
    } else {
        /* processing enable_pktGen */
        if ((snprintf(response, RES_STR_BUFF_SIZE, "%s.%s %s : %s",
                      strTokenArray[0], strTokenArray[1], strTokenArray[2],
                      (ret != 0) ? "Failed" : "Success")) < 0) {
            return FAILED;
        }
    }
    return ret;
}
/**************************************************************************
 * Function Name    : ocmw_handle_hci_led_set_command_function
 * Description      : This Function used to handle the test modules commands
 * Input(s)         : strTokenArray, action
 * Output(s)        : response
 ***************************************************************************/
static int32_t ocmw_handle_hci_led_set_command_function(char *strTokenArray[],
                                                        char *response)
{
    char paramStr[PARAM_STR_BUFF_SIZE] = { 0 };
    void *paramvalue = NULL;
    int32_t value = 0;
    int32_t ret = 0;
    char tempParamStr[PARAM_STR_MAX_BUFF_SIZE] = { 0 };

    if (strTokenArray == NULL || response == NULL) {
        logerr("%s(): NULL pointer error", __func__);
        return FAILED;
    }
    value = atoi(strTokenArray[2]);
    paramvalue = (void *)&value;
    strcpy(tempParamStr, strTokenArray[0]);
    if ((snprintf(paramStr, PARAM_STR_BUFF_SIZE, "%s.%s", strTokenArray[1],
                  strTokenArray[0])) < 0) {
        return FAILED;
    }
    ret = ocmw_msgproc_send_msg(&strTokenArray[0], 0, OCMP_MSG_TYPE_COMMAND,
                                (int8_t *)paramStr, paramvalue);
    if (strncmp(strTokenArray[1], "get", strlen("get")) == 0) {
        if ((snprintf(response, RES_STR_BUFF_SIZE, "%s.%s = %s", tempParamStr,
                      strTokenArray[1], dataOutBufFromEc)) < 0) {
            return FAILED;
        }
    } else {
        if ((snprintf(response, RES_STR_BUFF_SIZE, "%s.%s %s : %s",
                      strTokenArray[0], strTokenArray[1], strTokenArray[2],
                      (ret != 0) ? "Failed" : "Success")) < 0) {
            return FAILED;
        }
    }
    return ret;
}

/**************************************************************************
 * Function Name    : ocmw_handle_command_function
 * Description      : This Function used to handle the command messages
 * Input(s)         : strTokenArray, action
 * Output(s)        : response
 ***************************************************************************/
static int32_t ocmw_handle_command_function(char *strTokenArray[],
                                            char *response)
{
    char paramStr[PARAM_STR_BUFF_SIZE];
    int32_t ret = 0;
    int32_t paramVal = 0;
    char tempParamStr[PARAM_STR_MAX_BUFF_SIZE] = { 0 };

    if (strTokenArray == NULL || response == NULL) {
        logerr("%s(): NULL pointer error", __func__);
        return FAILED;
    }
    strcpy(tempParamStr, strTokenArray[0]);

    if ((snprintf(paramStr, PARAM_STR_BUFF_SIZE, "%s.%s", strTokenArray[1],
                  strTokenArray[0])) < 0) {
        return FAILED;
    }

    ret = ocmw_msgproc_send_msg(&strTokenArray[0], 0, OCMP_MSG_TYPE_COMMAND,
                                (int8_t *)paramStr, &paramVal);
    if ((snprintf(response, RES_STR_BUFF_SIZE, "%s.%s : %s", strTokenArray[0],
                  strTokenArray[1], (ret != 0) ? "Failed" : "Success")) < 0) {
        return FAILED;
    }

    return ret;
}

/**************************************************************************
 * Function Name    : ocmw_handle_post_command
 * Description      : This Function used to handle the post commands
 * Input(s)         : strTokenArray
 * Output(s)        : response
 ***************************************************************************/
static int32_t ocmw_handle_post_command(char *strTokenArray[], char *response)
{
    char paramStr[PARAM_STR_BUFF_SIZE], tmp[TEMP_STR_BUFF_SIZE];
    char subSys[PARAM_STR_BUFF_SIZE];
    int32_t ret = 0;
    int32_t index = 0;
    int32_t paramVal = 0;
    int32_t count = 0;
    ocwarePostResults postResult;
    ocwarePostReplyCode reply;

    if (strTokenArray == NULL || response == NULL) {
        logerr("%s(): NULL pointer error", __func__);
        return FAILED;
    }

    strcpy(subSys, postResult.results[0].subsysName);
    if (strcmp("set", strTokenArray[1]) == 0) {
        if ((snprintf(paramStr, PARAM_STR_BUFF_SIZE, "%s.%s", strTokenArray[0],
                      strTokenArray[1])) < 0) {
            return FAILED;
        }
        ret = ocmw_msgproc_send_msg(&strTokenArray[0], OCMP_AXN_TYPE_SET,
                                    OCMP_MSG_TYPE_POST, (int8_t *)paramStr,
                                    &paramVal);
        if ((snprintf(response, RES_STR_BUFF_SIZE, "%s : %s", paramStr,
                      (ret != 0) ? "Failed" : "Success")) < 0) {
            return FAILED;
        }
    } else if (strcmp("get", strTokenArray[1]) == 0) {
        if ((snprintf(paramStr, PARAM_STR_BUFF_SIZE, "%s.%s", strTokenArray[0],
                      strTokenArray[1])) < 0) {
            return FAILED;
        }
        ret = ocmw_msgproc_send_msg(&strTokenArray[0], OCMP_AXN_TYPE_GET,
                                    OCMP_MSG_TYPE_POST, (int8_t *)paramStr,
                                    &paramVal);
        if (ret != 0) {
            if ((snprintf(response, RES_STR_BUFF_SIZE, "%s : Failed",
                          paramStr)) < 0) {
                return FAILED;
            }
        }
        postResult.count = ocwarePostArrayIndex;

        ret = ocmw_retrieve_post_results(&postResult);
        if (ret < 0) {
            logerr("%s error", paramStr);
            return FAILED;
        }

        strncpy(
            response,
            "-------------------------------------------------------------------------------\n",
            RES_STR_BUFF_SIZE);
        if ((snprintf(tmp, TEMP_STR_BUFF_SIZE, "%-16s%-40s%-20s\n", "Subsystem",
                      "Device Name", "POST Status")) < 0) {
            return FAILED;
        }

        strncat(response, tmp, TEMP_STR_BUFF_SIZE);
        if ((snprintf(
                tmp, TEMP_STR_BUFF_SIZE,
                "-------------------------------------------------------------------------------\n")) <
            0) {
            return FAILED;
        }
        strncat(response, tmp, TEMP_STR_BUFF_SIZE);
        for (index = 0; index < postResult.count; index++) {
            reply.msgtype = OCMP_MSG_TYPE_POST;
            reply.replycode = postResult.results[index].status;

            ret = ocmw_retrieve_reply_code_desc(&reply);
            if (ret < 0) {
                strncpy(reply.desc, "", PARAM_STR_BUFF_SIZE);
            }
            if (strcmp(subSys, postResult.results[index].subsysName) != 0) {
                strcpy(subSys, postResult.results[index].subsysName);
                count = 0;
            }

            if (count > 0) {
                strcpy(postResult.results[index].subsysName, " ");
                if ((snprintf(tmp, TEMP_STR_BUFF_SIZE, "%-16s%-40s%-20s\n",
                              postResult.results[index].subsysName,
                              postResult.results[index].deviceName,
                              reply.desc)) < 0) {
                    return FAILED;
                }
                strncat(response, tmp, TEMP_STR_BUFF_SIZE);
            } else {
                strcpy(subSys, postResult.results[index].subsysName);
                postResult.results[index].subsysName[0] =
                    toupper(postResult.results[index].subsysName[0]);
                if ((snprintf(tmp, TEMP_STR_BUFF_SIZE, "%-16s\n",
                              postResult.results[index].subsysName)) < 0) {
                    return FAILED;
                }
                strncat(response, tmp, TEMP_STR_BUFF_SIZE);
                strcpy(postResult.results[index].subsysName, " ");
                if ((snprintf(tmp, TEMP_STR_BUFF_SIZE, "%-16s%-40s%-20s\n",
                              postResult.results[index].subsysName,
                              postResult.results[index].deviceName,
                              reply.desc)) < 0) {
                    return FAILED;
                }
                strncat(response, tmp, TEMP_STR_BUFF_SIZE);
                count++;
            }
        }

        if ((snprintf(
                tmp, TEMP_STR_BUFF_SIZE,
                "-------------------------------------------------------------------------------\n")) <
            0) {
            return FAILED;
        }

        strncat(response, tmp, TEMP_STR_BUFF_SIZE);

    } else {
        if ((snprintf(response, RES_STR_BUFF_SIZE,
                      "%s.%s : Error : Unknown parameter ", strTokenArray[0],
                      strTokenArray[1])) < 0) {
            return FAILED;
        }
        return FAILED;
    }
    return ret;
}
/**************************************************************************
 * Function Name    : ocmw_free_pointer
 * Description      : This is an inline function to free memory
 * Input(s)         : strTokenArray
 * Output(s)        : strTokenArray
 ***************************************************************************/
static void ocmw_free_pointer(char **strTokenArray)
{
    if (strTokenArray) {
        free(strTokenArray);
    }
    return;
}
/**************************************************************************
 * Function Name    : ocmw_match_set_get_string
 * Description      : This Function used to handle the cli set get
 * Input(s)         : str
 * Output(s)        :
 ***************************************************************************/
static ocmw_token_t ocmw_match_set_get_string(char *str)
{
    struct matchSetGet *index = ocmwSetGetTable;
    for (; index->key != NULL &&
           strncmp(index->key, str, strlen(index->key)) != 0;
         ++index)
        ;
    return index->token;
}
/**************************************************************************
 * Function Name    : ocmw_match_string
 * Description      : This Function used to handle the cli commands
 * Input(s)         : str
 * Output(s)        :
 ***************************************************************************/
static ocmw_token_t ocmw_match_string(char *str)
{
    struct matchString *index = ocmwTokenTable;
    for (; index->key != NULL && strcmp(index->key, str) != 0; ++index)
        ;
    return index->token;
}
/**************************************************************************
 * Function Name    : ocmw_frame_errorString
 * Description      : This Function used to handle the cli commands error
 * Input(s)         : cmdStr, errorString
 * Output(s)        : response
 ***************************************************************************/
int32_t ocmw_frame_errorString(const char *cmdStr, char *errorString,
                               char *response)
{
    if (errorString == NULL || response == NULL) {
        logerr("%s(): NULL pointer error", __func__);
        return FAILED;
    }
    if (snprintf(response, RES_STR_BUFF_SIZE, "%s : %s", cmdStr, errorString)) {
        logerr("%s(): Sprintf error", __func__);
        return FAILED;
    }
    strcat(response, "\nPlease refer help menu:\n\"<subsystem>"
                     " --help or <subsytem>.<component> --help\"");
    return SUCCESS;
}
/**************************************************************************
 * Function Name    : ocmw_clicmd_handler
 * Description      : This Function used to handle the cli commands
 * Input(s)         : cmdstr
 * Output(s)        : response
 ***************************************************************************/
int32_t ocmw_clicmd_handler(const char *cmdStr, char *response)
{
    char **strTokenArray = NULL;
    char class[PARAMSTR_NUMBER_LEN] = { 0 };
    char paramStr[PARAM_STR_BUFF_SIZE] = { 0 };
    int32_t strTokenCount = 0;
    int32_t ret = 0;

    if (cmdStr == NULL || response == NULL) {
        logerr("%s(): NULL pointer error", __func__);
        return FAILED;
    }

    if (strcmp(cmdStr, "") == 0)
        return FAILED;

    ret = ocmw_tokenize(cmdStr, &strTokenCount, &strTokenArray);
    if (ret < 0) {
        logerr("Error: command string ocmw_tokenize failed");
        if ((snprintf(response, RES_STR_BUFF_SIZE,
                      "%s : Internal error occured in parsing CLI parameters",
                      cmdStr)) < 0) {
            return FAILED;
        }
        ocmw_free_pointer(strTokenArray);
        return FAILED;
    } else {
        if (strTokenCount > 1) {
            strncpy(paramStr, strTokenArray[0], strlen(strTokenArray[0]));
            /* Process the command */
            switch (ocmw_match_string(strTokenArray[1])) {
                case SET_STR:
                    switch (ocmw_match_set_get_string(strTokenArray[0])) {
                        case HCI_STR:
                            if (strTokenCount == 3) {
                                ret = ocmw_handle_hci_led_set_command_function(
                                    strTokenArray, response);
                                ocmw_free_pointer(strTokenArray);
                                return (ret != 0) ? FAILED : SUCCESS;
                            } else {
                                ret =
                                    (strTokenCount < 3 ?
                                         ocmw_frame_errorString(
                                             cmdStr, INSUFFICIENT_PARAM,
                                             response) :
                                         ocmw_frame_errorString(
                                             cmdStr, INVALID_SYNTAX, response));
                                ocmw_free_pointer(strTokenArray);
                                return FAILED;
                            }
                            break;
                        case DEBUG_STR:
                            if (strncmp("debug.I2C", strTokenArray[0],
                                        strlen("debug.I2C")) == 0) {
                                if (strTokenCount == 6) {
                                    ret = ocmw_handle_debug_command_function(
                                        strTokenArray, response);
                                    ocmw_free_pointer(strTokenArray);
                                    return (ret != 0) ? FAILED : SUCCESS;
                                } else {
                                    ret = (strTokenCount < 6 ?
                                               ocmw_frame_errorString(
                                                   cmdStr, INSUFFICIENT_PARAM,
                                                   response) :
                                               ocmw_frame_errorString(
                                                   cmdStr, INVALID_SYNTAX,
                                                   response));
                                    ocmw_free_pointer(strTokenArray);
                                    return FAILED;
                                }
                            } else {
                                if (strTokenCount == 4) {
                                    ret = ocmw_handle_debug_command_function(
                                        strTokenArray, response);
                                    ocmw_free_pointer(strTokenArray);
                                    return (ret != 0) ? FAILED : SUCCESS;
                                } else {
                                    ret = (strTokenCount < 4 ?
                                               ocmw_frame_errorString(
                                                   cmdStr, INSUFFICIENT_PARAM,
                                                   response) :
                                               ocmw_frame_errorString(
                                                   cmdStr, INVALID_SYNTAX,
                                                   response));
                                    ocmw_free_pointer(strTokenArray);
                                    return FAILED;
                                }
                            }
                            break;
                        case ENABLE_SET_STR:
                            if (strTokenCount == 2) {
                                ret = ocmw_handle_post_command(strTokenArray,
                                                               response);
                                return (ret != 0) ? FAILED : SUCCESS;
                            } else {
                                ret =
                                    (strTokenCount < 2 ?
                                         ocmw_frame_errorString(
                                             cmdStr, INSUFFICIENT_PARAM,
                                             response) :
                                         ocmw_frame_errorString(
                                             cmdStr, INVALID_SYNTAX, response));
                                ocmw_free_pointer(strTokenArray);
                                return FAILED;
                            }
                            break;
                        default:
                            if (strTokenCount == 3) {
                                ocmw_tokenize_class(paramStr, class, 2);
                                if (strcmp("config", class) == 0) {
                                    ret = ocmw_handle_set_config(
                                        &strTokenArray[0], response);
                                    ocmw_free_pointer(strTokenArray);
                                    return (ret != 0) ? FAILED : SUCCESS;
                                } else {
                                    if ((snprintf(response, RES_STR_BUFF_SIZE,
                                                  "[Error]: "
                                                  "Incorrect %s request '%s'",
                                                  strTokenArray[0],
                                                  strTokenArray[1])) < 0) {
                                        return FAILED;
                                    }
                                    ocmw_free_pointer(strTokenArray);
                                    return FAILED;
                                }
                            } else {
                                ret =
                                    (strTokenCount < 3 ?
                                         ocmw_frame_errorString(
                                             cmdStr, INSUFFICIENT_PARAM,
                                             response) :
                                         ocmw_frame_errorString(
                                             cmdStr, INVALID_SYNTAX, response));
                                ocmw_free_pointer(strTokenArray);
                                return FAILED;
                            }
                            break;
                    }
                    break;
                case GET_STR:
                    switch (ocmw_match_set_get_string(strTokenArray[0])) {
                        case RESULT_STR:
                            if (strTokenCount == 2) {
                                ret = ocmw_handle_post_command(strTokenArray,
                                                               response);
                                ocmw_free_pointer(strTokenArray);
                                return (ret != 0) ? FAILED : SUCCESS;
                            } else {
                                ret =
                                    (strTokenCount < 2 ?
                                         ocmw_frame_errorString(
                                             cmdStr, INSUFFICIENT_PARAM,
                                             response) :
                                         ocmw_frame_errorString(
                                             cmdStr, INVALID_SYNTAX, response));
                                ocmw_free_pointer(strTokenArray);
                                return FAILED;
                            }
                            break;
                        case DEBUG_STR:
                            if (strncmp("debug.I2C", strTokenArray[0],
                                        strlen("debug.I2C")) == 0) {
                                if (strTokenCount == 5) {
                                    ret = ocmw_handle_debug_command_function(
                                        strTokenArray, response);
                                    ocmw_free_pointer(strTokenArray);
                                    return (ret != 0) ? FAILED : SUCCESS;
                                } else {
                                    ret = (strTokenCount < 5 ?
                                               ocmw_frame_errorString(
                                                   cmdStr, INSUFFICIENT_PARAM,
                                                   response) :
                                               ocmw_frame_errorString(
                                                   cmdStr, INVALID_SYNTAX,
                                                   response));
                                    ocmw_free_pointer(strTokenArray);
                                    return FAILED;
                                }
                            } else {
                                if (strTokenCount == 3) {
                                    ret = ocmw_handle_debug_command_function(
                                        strTokenArray, response);
                                    ocmw_free_pointer(strTokenArray);
                                    return (ret != 0) ? FAILED : SUCCESS;
                                } else {
                                    ret = (strTokenCount < 3 ?
                                               ocmw_frame_errorString(
                                                   cmdStr, INSUFFICIENT_PARAM,
                                                   response) :
                                               ocmw_frame_errorString(
                                                   cmdStr, INVALID_SYNTAX,
                                                   response));
                                    ocmw_free_pointer(strTokenArray);
                                    return FAILED;
                                }
                            }
                            break;
                        default:
                            if (strTokenCount == 2) {
                                ocmw_tokenize_class(paramStr, class, 2);
                                if (strcmp("config", class) == 0) {
                                    ret = ocmw_handle_show_config(
                                        &strTokenArray[0], response);
                                    ocmw_free_pointer(strTokenArray);
                                    return (ret != 0) ? FAILED : SUCCESS;
                                } else if (strcmp("status", class) == 0) {
                                    ret = ocmw_handle_show_status(
                                        &strTokenArray[0], response);
                                    ocmw_free_pointer(strTokenArray);
                                    return (ret != 0) ? FAILED : SUCCESS;
                                } else {
                                    if ((snprintf(response, RES_STR_BUFF_SIZE,
                                                  "[Error]: "
                                                  "Incorrect %s request '%s'",
                                                  strTokenArray[0],
                                                  strTokenArray[1])) < 0) {
                                        return FAILED;
                                    }
                                    ocmw_free_pointer(strTokenArray);
                                    return FAILED;
                                }
                            } else {
                                ret =
                                    (strTokenCount < 2 ?
                                         ocmw_frame_errorString(
                                             cmdStr, INSUFFICIENT_PARAM,
                                             response) :
                                         ocmw_frame_errorString(
                                             cmdStr, INVALID_SYNTAX, response));
                                ocmw_free_pointer(strTokenArray);
                                return FAILED;
                            }
                            break;
                    }
                case RESET_STR:
                case ENABLE_STR:
                case DISABLE_STR:
                case ACTIVE_STR:
                case ECHO_STR:
                    if (strTokenCount == 2) {
                        ret = ocmw_handle_command_function(strTokenArray,
                                                           response);
                        ocmw_free_pointer(strTokenArray);
                        return (ret != 0) ? FAILED : SUCCESS;
                    } else {
                        ret = (strTokenCount < 2 ?
                                   ocmw_frame_errorString(
                                       cmdStr, INSUFFICIENT_PARAM, response) :
                                   ocmw_frame_errorString(
                                       cmdStr, INVALID_SYNTAX, response));
                        ocmw_free_pointer(strTokenArray);
                        return FAILED;
                    }
                    break;
                case DISCONNECT_STR:
                case CONNECT_STR:
                case ANSWER_STR:
                case HANGUP_STR:
                    if (strTokenCount == 2) {
                        ret = ocmw_handle_testmod_command_function(
                            strTokenArray, response);
                        ocmw_free_pointer(strTokenArray);
                        return (ret != 0) ? FAILED : SUCCESS;
                    } else {
                        ret = (strTokenCount < 2 ?
                                   ocmw_frame_errorString(
                                       cmdStr, INSUFFICIENT_PARAM, response) :
                                   ocmw_frame_errorString(
                                       cmdStr, INVALID_SYNTAX, response));
                        ocmw_free_pointer(strTokenArray);
                        return FAILED;
                    }
                    break;
                case SEND_STR:
                    if (strTokenCount == 4) {
                        ret = ocmw_handle_testmod_command_function(
                            strTokenArray, response);
                        ocmw_free_pointer(strTokenArray);
                        return (ret != 0) ? FAILED : SUCCESS;
                    } else {
                        ret = (strTokenCount < 4 ?
                                   ocmw_frame_errorString(
                                       cmdStr, INSUFFICIENT_PARAM, response) :
                                   ocmw_frame_errorString(
                                       cmdStr, INVALID_SYNTAX, response));
                        ocmw_free_pointer(strTokenArray);
                        return FAILED;
                    }
                    break;
                case DIAL_STR:
                    if (strTokenCount == 3) {
                        ret = ocmw_handle_testmod_command_function(
                            strTokenArray, response);
                        ocmw_free_pointer(strTokenArray);
                        return (ret != 0) ? FAILED : SUCCESS;
                    } else {
                        ret = (strTokenCount < 3 ?
                                   ocmw_frame_errorString(
                                       cmdStr, INSUFFICIENT_PARAM, response) :
                                   ocmw_frame_errorString(
                                       cmdStr, INVALID_SYNTAX, response));
                        ocmw_free_pointer(strTokenArray);
                        return FAILED;
                    }
                    break;
                case ELOOPBK_STR:
                case DLOOPBK_STR:
                case EPKTGEN_STR:
                    if (strTokenCount == 3) {
                        ret = ocmw_handle_ethernet_command_function(
                            strTokenArray, response);
                        ocmw_free_pointer(strTokenArray);
                        return (ret != 0) ? FAILED : SUCCESS;
                    } else {
                        ret = (strTokenCount < 3 ?
                                   ocmw_frame_errorString(
                                       cmdStr, INSUFFICIENT_PARAM, response) :
                                   ocmw_frame_errorString(
                                       cmdStr, INVALID_SYNTAX, response));
                        ocmw_free_pointer(strTokenArray);
                        return FAILED;
                    }
                    break;
                case DPKTGEN_STR:
                    if (strTokenCount == 2) {
                        ret = ocmw_handle_ethernet_command_function(
                            strTokenArray, response);
                        ocmw_free_pointer(strTokenArray);
                        return (ret != 0) ? FAILED : SUCCESS;
                    } else {
                        ret = (strTokenCount < 2 ?
                                   ocmw_frame_errorString(
                                       cmdStr, INSUFFICIENT_PARAM, response) :
                                   ocmw_frame_errorString(
                                       cmdStr, INVALID_SYNTAX, response));
                        ocmw_free_pointer(strTokenArray);
                        return FAILED;
                    }
                    break;
                case ALERTLOG_STR:
                    if (strTokenCount == 2) {
                        ret = ocmw_handle_show_all_alerts(&strTokenArray[0],
                                                          response);
                        ocmw_free_pointer(strTokenArray);
                        return (ret != 0) ? FAILED : SUCCESS;
                    } else {
                        ret = (strTokenCount < 2 ?
                                   ocmw_frame_errorString(
                                       cmdStr, INSUFFICIENT_PARAM, response) :
                                   ocmw_frame_errorString(
                                       cmdStr, INVALID_SYNTAX, response));
                        ocmw_free_pointer(strTokenArray);
                        return FAILED;
                    }
                    break;
                default:
                    break;
            }
        } else {
            if ((snprintf(
                    response, RES_STR_BUFF_SIZE,
                    "%s : Error : Invalid parameters \n Please refer help menu"
                    ":\n\"<subsystem> --help or "
                    "<subsytem>.<component> --help\"",
                    cmdStr)) < 0) {
                return FAILED;
            }
            ocmw_free_pointer(strTokenArray);
            return FAILED;
        }
    }
    ocmw_free_pointer(strTokenArray);
    return ret;
}
