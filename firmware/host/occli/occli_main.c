/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

/* stdlib includes */
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <logger.h>
#include <util.h>

/* sudo apt-get install libedit-dev : Install editline  */
#include <editline/readline.h>

/* OC includes */
#include <occli_common.h>

#define HISTORY ".occli_history" /* Saved under ${HOME} direcotory */

#define SWAP(s, e)  do {                                \
                    int32_t t; t = s; s = e; e = t;     \
                    } while (0)

/* MAX_CLIENTRY represents the maximum entries which autofill can take */
#define     OCCLI_MAX_CLISTRING             1000
#define     OCCLI_MAX_SUBSTRING             75
#define     OCCLI_STRING_MAX_LEN            100
#define     OCCLI_CALC_SCHEMA_SIZE          1
#define     OCCLI_PARSE_SCHEMA              2
#define     MAX_ETHERNET_PORT               4
#define     OPENCELLULAR                    "opencellular# "

extern char *rl_line_buffer;
static char *s_allParams[OCCLI_MAX_CLISTRING];
static char *s_subSetParams[OCCLI_MAX_SUBSTRING];
static char *s_strCli = NULL;
extern const Component sys_schema[];

/**************************************************************************
 * Function Name    : occli_trim_extra_spaces
 * Description      : This Function used to remove extra space inbetween
 *                    words and trailing spaces from the string
 * Input(s)         : string
 * Output(s)        : string
 ***************************************************************************/
int8_t occli_trim_extra_spaces(char *string)
{
    int8_t length = 0;
    char *end = NULL;

    if (string == NULL) {
        logerr("Invalid memory location \n");
        return FAILED;
    }

    /* Logic to remove trailing spaces */
    length = strlen(string);
    end = string + length - 1;

    while(end >= string && isblank(*end)) {
        end--;
    }

    *(end + 1) = '\0';
    return SUCCESS;
}
/**************************************************************************
 * Function Name    : occli_copy_text
 * Description      : This Function Return a copy of the string entered
                      by the user in cli
 * Input(s)         :
 * Output(s)        : s_strCli
 ***************************************************************************/
static int8_t occli_copy_text ()
{
    int8_t ret = FAILED;
    int16_t length;

    if (isblank(rl_line_buffer[0])) {
        printf("\nBlank Space is not allowed as the first character\n");
        printf("Please press ctrl+L to clear screen\n");
        return ret;
    }

    length = rl_point;
    s_strCli = (char*)realloc(s_strCli, length + 1);

    if (s_strCli == NULL) {
        printf("\n ERROR: realloc");
    } else {
        ret = SUCCESS;
        strncpy (s_strCli, rl_line_buffer, length);
        s_strCli[length] = '\0';
    }

    if (ret == SUCCESS) {
       ret = occli_trim_extra_spaces(s_strCli);
    }

    return ret;
}
/**************************************************************************
 * Function Name    : occli_init_subSetParams
 * Description      : This Function is used to allocate memory for
 *                    subSetParams
 * Input(s)         : text
 * Output(s)        :
 ***************************************************************************/
static int8_t occli_init_subSetParams(const char *text)
{
    char *paramStr = NULL;
    int32_t len = 0;
    int16_t listIdx = 0;
    int16_t subsetIdx = 0;

    if (text == NULL) {
        logerr("Invalid memory location \n");
        return FAILED;
    }

    if (s_subSetParams != NULL) {
        for (listIdx = 0; (s_subSetParams[listIdx] != NULL &&
                    listIdx < OCCLI_MAX_SUBSTRING) ; listIdx++) {
            free(s_subSetParams[listIdx]);
            s_subSetParams[listIdx] = NULL;
        }
    }

    len = strlen(text);
    listIdx = 0;
    while ((paramStr = s_allParams[listIdx])) {
        if (strncmp(text, paramStr, len) == 0) {
            subsetIdx++;
            if(subsetIdx >= OCCLI_MAX_SUBSTRING) {
                break;
            }
        }
        listIdx++;
    }

    for (listIdx= 0; listIdx < subsetIdx; listIdx++) {
        s_subSetParams[listIdx] = (char *)calloc(1, OCCLI_STRING_MAX_LEN);
        if ((s_subSetParams[listIdx]) == NULL) {
            logerr("calloc error");
            return -ENOMEM;
        }
    }

    return SUCCESS;
}
/**************************************************************************
 * Function Name    : occli_all_param_generator
 * Description      : This Function used to generate the all param
 *                    list
 * Input(s)         : text, state
 * Output(s)        :
 ***************************************************************************/
static char* occli_all_param_generator(const char* text, int32_t state)
{
    char *paramstr = NULL;
    char *token = NULL;
    char subStr[OCCLI_STRING_MAX_LEN] = {0};
    char tempStr[OCCLI_STRING_MAX_LEN] = {0};
    static int32_t s_listidx = 0;
    static int32_t s_subSetIdx = 0;
    static int32_t s_len = 0;
    int8_t index = 0 ;
    bool isEntryFound = false;

    if (text == NULL) {
        logerr("Invalid memory location \n");
        rl_attempted_completion_over = 1;
        return NULL;
    }

    if (state == 0) {
        s_listidx = 0;
        s_subSetIdx = 0;
        s_len = strlen(text);
        if (occli_init_subSetParams(text) != SUCCESS) {
            rl_attempted_completion_over = 1;
            return NULL;
        }
    }

    while ((paramstr = s_allParams[s_listidx]) != NULL) {
        isEntryFound = false;
        s_listidx++;
        if (strncmp(s_strCli, paramstr, s_len) == 0) {
            strcpy(subStr, paramstr);
            token = strtok(subStr + s_len, ".");

            memset(tempStr, 0, OCCLI_STRING_MAX_LEN);
            rl_completion_append_character = '\0';
            if (token == NULL) {
                /*
                 * case where the user has entered the complete string,
                 * autofill is not needed in this case , append a space
                 */
                sprintf(tempStr, "%s ", text);
            } else {
                /* Autofill will be needed in all cases here */
                if (strncmp(text, "", 1) == 0) {
                    /* case where the user hasn't entered any string */
                    sprintf(tempStr, "%s", token);
                } else if(strncmp(paramstr + s_len, "." , 1) == 0) {
                    /*
                     * case where the user has entered complete string
                     * for either subsystem/component (ie) "system"
                     */
                    sprintf(tempStr, "%s.%s", text,token);
                } else {
                    /*
                     * case where the user has entered subset of the string
                     * for example "sys"
                     */
                    sprintf(tempStr, "%s%s", text,token);
                }
            }
            /*
             * The for loop below is to ensure that duplicate entries arent
             * displayed in the cli
             */
            for (index = 0; index < s_subSetIdx; index++) {
                if (strcmp(s_subSetParams[index], tempStr) == 0) {
                    isEntryFound = true;
                    break;
                }
            }
            if(isEntryFound != true) {
                strcpy(s_subSetParams[s_subSetIdx], tempStr);
                s_subSetIdx++;
                return strdup(s_subSetParams[s_subSetIdx - 1]);
            }
        }
    }
    return NULL;
}
/**************************************************************************
 * Function Name    : occli_custom_completion
 * Description      : This Function used to handle the TAB operation
 * Input(s)         : text, start, end
 * Output(s)        :
 ***************************************************************************/
static char** occli_custom_completion(const char* text, int32_t start,
        int32_t end)
{
    char** matches = NULL;
    int8_t ret = FAILED;

    if (text == NULL) {
        logerr("Invalid memory location \n");
        rl_attempted_completion_over = 1;
        return NULL;
    }

    ret = occli_copy_text();
    if (ret != SUCCESS) {
        rl_attempted_completion_over = 1;
        return NULL;
    }

    if (start == 0) {
        matches = rl_completion_matches(text, occli_all_param_generator);
    } else {
        rl_attempted_completion_over = 1;
    }

    return matches;
}

/**************************************************************************
 * Function Name    : occli_strjoin
 * Description      : This Function used to join the string
 * Input(s)         : srcstr, delimstr
 * Output(s)        : deststrPtr
 ***************************************************************************/
static int8_t occli_strjoin(char **deststrPtr, const char *srcstr,
        const char *delimstr)
{
    char *tmp;
    const int32_t alloclen = OCCLI_STRING_SIZE;
    size_t destSize, deststrLen, srcstrLen, delimstrLen;

    if (deststrPtr == NULL || srcstr == NULL || delimstr == NULL) {
        logerr("NULL pointer error");
        return -EFAULT;
    }

    if (*deststrPtr == NULL) {
        *deststrPtr = calloc(alloclen, sizeof(char));
        if (*deststrPtr == NULL) {
            logerr("calloc error");
            return -ENOMEM;
        }
        deststrLen = strlen(*deststrPtr);
    } else {
        delimstrLen = strlen(delimstr);
        srcstrLen = strlen(srcstr);
        deststrLen = strlen(*deststrPtr);
        destSize = ((deststrLen / alloclen) + 1) * alloclen;

        if ((srcstrLen + delimstrLen + 1) > (destSize - deststrLen)) {
            /* allocate more memory to concatenate the srcstr */
            tmp = *deststrPtr;
            *deststrPtr = realloc(*deststrPtr, destSize + alloclen);
            if (*deststrPtr == NULL) {
                logerr("realloc error");
                free(tmp);
                return -ENOMEM;
            }
        }
    }

    /* strcat the delimiting string to deststr, only when the deststr is not
     an empty string */
    if (deststrLen > 0) {
        strcat(*deststrPtr, delimstr);
    }
    /* strcat the new string */
    strcat(*deststrPtr, srcstr);

    return SUCCESS;
}

/**************************************************************************
 * Function Name    : occli_alertthread_messenger_to_ocmw
 * Description      : This Function is used to handle the alert message
 * Input(s)         : pThreadData
 * Output(s)        :
 ***************************************************************************/
void * occli_alertthread_messenger_to_ocmw(void *pThreadData)
{
    char response[RES_STR_BUFF_SIZE] = {0};
    int32_t ret = 0;

    /* Receive the CLI command execution response string from OCMW over UDP
     * socket */

    while (1) {
        memset(response, 0, sizeof(response));
        ret = occli_recv_alertmsg_from_ocmw(response, sizeof(response));
        if (ret < 0) {
            printf("occli_recv_alertmsg_from_ocmw failed: error value : %d\n", ret);
        } else {
            printf("%s\n", response);
        }
    }
}
/**************************************************************************
 * Function Name    : occli_parse_cliString
 * Description      : This Function is used to check the validity of the
 *                      cli string
 * Input(s)         : cliString
 * Output(s)        : SUCCESS/FAILURE
 ***************************************************************************/
int8_t occli_parse_cliString(char *cliString)
{
    int8_t ret = FAILED;
    int16_t index = 0;
    char tempStr[OCCLI_STRING_MAX_LEN] = {0};
    char *token = NULL;

    if (cliString == NULL) {
        return ret;
    }
    strcpy(tempStr, cliString);
    token = strtok(tempStr, " ");

    for(index = 0; ((index < OCCLI_MAX_CLISTRING) &&
                    (s_allParams[index] != NULL)); index++) {

        if (strcmp(token, s_allParams[index]) == 0) {
            ret = SUCCESS;
            break;
        }
    }

    return ret;
}
/**************************************************************************
 * Function Name    : occli_frame_commands
 * Description      : This Function is used to frame the cli commands
 * Input(s)         : root, occliData->option
 * Output(s)        : s_allParams, occliData->totalStr, occliData->sizeNum
 ***************************************************************************/
int8_t occli_frame_commands(const Component *root,
                             OCCLI_ARRAY_PARAM *occliData)
{
    const Component *subSystem = root;
    const Component *component = subSystem->components;
    const Component *subComponent = NULL;
    const Command *command = NULL;
    const Driver *driver = NULL;


    if ((root == NULL) ||
            (occliData == NULL)) {
        logerr("Invalid Memory \n");
        return FAILED;
    }

    /* subsytem->component->command */
    subSystem = root;
    while (subSystem && subSystem->name) {
        component = subSystem->components;
        while (component && component->name) {
            command = component->commands;
            while (command &&
                    command->name) {
                if (occliData->option == OCCLI_CALC_SCHEMA_SIZE) {
                    occliData->sizeNum += 1;
                } else {
                    snprintf(s_allParams[occliData->totalStr++],
                            OCCLI_SNPRINTF_MAX_LEN, "%s.%s.%s",
                            subSystem->name, component->name, command->name);
                }
                command += 1;
            }
            component += 1;
        }
        subSystem += 1;
    }

    /* subsytem->component->driver->command */
    subSystem = root;
    while (subSystem && subSystem->name) {
        component = subSystem->components;
        while (component && component->name) {
            driver = component->driver;
            if(driver != NULL) {
                command = driver->commands;
                while (command &&
                        command->name) {
                    if (occliData->option == OCCLI_CALC_SCHEMA_SIZE) {
                        occliData->sizeNum += 1;
                    } else {
                        snprintf(s_allParams[occliData->totalStr++],
                                    OCCLI_SNPRINTF_MAX_LEN, "%s.%s.%s",
                                    subSystem->name, component->name,
                                    command->name);
                    }
                    command += 1;
                }
            }
            component += 1;
        }
        subSystem += 1;
    }

    /* subsytem->component->subComponent->driver->command */
    subSystem = root;
    while (subSystem && subSystem->name) {
        component = subSystem->components;
        while (component && component->name) {
            subComponent = component->components;
            while (subComponent && subComponent->name) {
                driver = subComponent->driver;
                if(driver != NULL) {
                    command = driver->commands;
                    while (command &&
                            command->name) {
                        if (occliData->option == OCCLI_CALC_SCHEMA_SIZE) {
                            occliData->sizeNum += 1;
                        } else {
                            snprintf(s_allParams[occliData->totalStr++],
                                        OCCLI_STRING_MAX_LEN, "%s.%s.%s.%s",
                                        subSystem->name, component->name,
                                        subComponent->name, command->name);
                        }
                        command += 1;
                    }
                }
                subComponent += 1;
            }
            component += 1;
        }
        subSystem += 1;
    }
    return SUCCESS;
}
/**************************************************************************
 * Function Name    : occli_frame_string_from_schemaDriver
 * Description      : This Function is used to frame the cli string
 * Input(s)         : param, strFrame, msgTypeStr
 * Output(s)        : ocmwclistr, occliData
 ***************************************************************************/
int8_t occli_frame_string_from_schemaDriver(const Parameter *param,
                        OCCLI_ARRAY_PARAM *occliData,
                        strMsgFrame *strFrame)
{
    if ((occliData == NULL) || (param == NULL) || (strFrame == NULL)) {
        logerr("Invalid Memory \n");
        return FAILED;
    }

    if (occliData->option == OCCLI_CALC_SCHEMA_SIZE) {
        occliData->sizeNum += 1;
    } else {
        if (strlen(strFrame->subcomponent)) {
            snprintf(s_allParams[occliData->totalStr++],
                OCCLI_SNPRINTF_MAX_LEN, "%s.%s.%s.%s.%s.%s",
                strFrame->subsystem, strFrame->component,
                strFrame->msgtype, strFrame->subcomponent,
                param->name, strFrame->parameter);
        } else {
            snprintf(s_allParams[occliData->totalStr++],
                OCCLI_SNPRINTF_MAX_LEN, "%s.%s.%s.%s.%s",
                strFrame->subsystem, strFrame->component,
                strFrame->msgtype, param->name, strFrame->parameter);
        }
    }
    return SUCCESS;
}
// TO use for POST
/**************************************************************************
 * Function Name    : occli_frame_string_from_postDriver
 * Description      : This Function is used to frame the cli string
 * Input(s)         : param, strFrame, msgTypeStr
 * Output(s)        : ocmwclistr, occliData
 ***************************************************************************/
int8_t occli_frame_string_from_postDriver(const Post *param,
                        OCCLI_ARRAY_PARAM *occliData,
                        strMsgFrame *strFrame)
{
    if ((occliData == NULL) || (param == NULL) || (strFrame == NULL)) {
        logerr("Invalid Memory \n");
        return FAILED;
    }

    if (occliData->option == OCCLI_CALC_SCHEMA_SIZE) {
        occliData->sizeNum += 1;
    } else {
        if (strlen(strFrame->subcomponent)) {
            snprintf(s_allParams[occliData->totalStr++],
                OCCLI_SNPRINTF_MAX_LEN, "%s.%s.%s.%s.%s.%s",
                strFrame->subsystem, strFrame->component,
                strFrame->msgtype, strFrame->subcomponent,
                param->name, strFrame->parameter);
        } else {
            snprintf(s_allParams[occliData->totalStr++],
                OCCLI_SNPRINTF_MAX_LEN, "%s.%s.%s.%s.%s",
                strFrame->subsystem, strFrame->component,
                strFrame->msgtype, param->name, strFrame->parameter);
        }
    }
    return SUCCESS;
}
/**************************************************************************
 * Function Name    : occli_frame_string_from_schema
 * Description      : This Function is used to frame the cli string
 * Input(s)         : devDriver, strFrame
 * Output(s)        : ocmwclistr, occliData
 ***************************************************************************/
int8_t occli_frame_string_from_schema(const Driver *devDriver,
                        OCCLI_ARRAY_PARAM *occliData,
                        strMsgFrame *strFrame)
{
    const Post *postParam = NULL;
    const Parameter *param = NULL;
    int8_t ret = SUCCESS;

    if ((devDriver == NULL) || (occliData == NULL) || (strFrame == NULL)) {
        logerr("Invalid Memory \n");
        return FAILED;
    }

    postParam = devDriver->post;
    while (postParam && postParam->name) {
        strcpy(strFrame->msgtype, "post");
        if (strncmp(postParam->name, "enable", strlen(postParam->name)) == 0) {
            strcpy(strFrame->parameter, "set");
        } else {
            strcpy(strFrame->parameter, "get");
        }
        ret = occli_frame_string_from_postDriver(postParam, occliData, strFrame);
        postParam++;
    }

    param = devDriver->config;
    while (param && param->name) {
        strcpy(strFrame->msgtype, "config");
        strcpy(strFrame->parameter, "set");
        ret = occli_frame_string_from_schemaDriver(param, occliData, strFrame);

        strcpy(strFrame->parameter, "get");
        ret = occli_frame_string_from_schemaDriver(param, occliData, strFrame);
        param++;
    }

    param = devDriver->status;
    while (param && param->name) {
        strcpy(strFrame->msgtype, "status");
        strcpy(strFrame->parameter, "get");
        ret = occli_frame_string_from_schemaDriver(param, occliData, strFrame);
        param++;
    }

#ifdef OCCLI_ALERT_STRING
    param = devDriver->alerts;
    strcpy(strFrame->msgtype, "alert");
    ret = occli_frame_string_from_schemaDriver(param, occliData, strFrame);
#endif

    return ret;
}
/**************************************************************************
 * Function Name    : occli_frame_string
 * Description      : This Function is used to frame the cli string
 * Input(s)         : root
 * Output(s)        : ocmwclistr, occliData
 ***************************************************************************/
int8_t occli_frame_string(const Component *root, OCCLI_ARRAY_PARAM *occliData)
{
    const Component *component = NULL, *subComponent = NULL, *subSystem = root;
    const Driver *devDriver = NULL;
    int8_t ret = 0;

    strMsgFrame *strFrame = (strMsgFrame *) malloc(sizeof(strMsgFrame));

    if ((strFrame == NULL) || (root == NULL) ||
         (occliData == NULL)) {
        logerr("Invalid Memory \n");
        return FAILED;
    }

    /* Config/Status/Alerts Table array entry Creation */
    while (subSystem && subSystem->name) {
        memset(strFrame, '\0', sizeof(strMsgFrame));
        strcpy(strFrame->subsystem, subSystem->name);
        component = subSystem->components;
        while (component && component->name) {
            memset(strFrame->subcomponent, '\0', OCCLI_CHAR_ARRAY_SIZE);
            strcpy(strFrame->component, component->name);
            devDriver = component->driver;
            if (devDriver != NULL) {
                ret = occli_frame_string_from_schema(devDriver,
                                    occliData, strFrame);
                if (ret == FAILED) {
                    return ret;
                }
            }
            subComponent = component->components;
            while (subComponent && subComponent->name) {
                strcpy(strFrame->subcomponent, subComponent->name);
                devDriver = subComponent->driver;
                if (devDriver != NULL) {
                    ret = occli_frame_string_from_schema(devDriver,
                                    occliData, strFrame);
                    if (ret == FAILED) {
                        return ret;
                    }
                }
                subComponent += 1;
            }
            component += 1;
        }
        subSystem += 1;
    }
    free(strFrame);
    return SUCCESS;
}

/**************************************************************************
 * Function Name    : main
 * Description      :
 * Input(s)         : argc, argv
 * Output(s)        :
 ***************************************************************************/
int32_t main(int32_t argc, char *argv[])
{
    char *line = NULL;
    char *clistr = NULL;
    char response[RES_STR_BUFF_SIZE] = {0};
    char historyFile[HIT_FILE_BUFF_SIZE];
    char *cmdstr = NULL;
    const char* prompt = "opencellular# ";
    int32_t index = 0;
    int32_t ret = 0;
    pthread_t alertThreadId;
    sMsgParam msgFrameParam;
    OCCLI_ARRAY_PARAM occliArray;

    /* Initialize logging */
    initlog("occli");

    memset(&msgFrameParam, 0, sizeof(sMsgParam));
    memset (&occliArray, 0, sizeof(OCCLI_ARRAY_PARAM));

    occliArray.option = OCCLI_CALC_SCHEMA_SIZE;

    ret = occli_frame_string(sys_schema, &occliArray);
    if (ret != 0) {
        logerr("init_ocmw_comm() failed.");
        return FAILED;
    }

    ret = occli_frame_commands(sys_schema, &occliArray);
    if (ret != 0) {
        logerr("init_ocmw_comm() failed.");
        return FAILED;
    }

    for (index= 0; index< occliArray.sizeNum; index++) {
        if(!(s_allParams[index] = (char *)malloc(OCCLI_STRING_MAX_LEN))) {
            return FAILED;
        }

    }

    occliArray.option = OCCLI_PARSE_SCHEMA;
    ret = occli_frame_string(sys_schema, &occliArray);
    if (ret != 0) {
        logerr("init_ocmw_comm() failed.");
        return FAILED;
    }

    ret = occli_frame_commands(sys_schema, &occliArray);
    if (ret != 0) {
        logerr("init_ocmw_comm() failed.");
        return FAILED;
    }

    /* Initialize Middleware IPC communication */
    ret = occli_init_comm();
    if (ret != 0) {
        logerr("init_ocmw_comm() failed.");
        return FAILED;
    }

    ret = pthread_create(&alertThreadId, NULL,
            occli_alertthread_messenger_to_ocmw, NULL);
    if (ret != 0) {
        return ret;
    }
    /* Execute the OC command (argv[1:]) and exit */
    if (strcmp("occmd", basename(argv[0])) == 0) {
        for (index= 1; index < argc; index++) {
            if (occli_strjoin(&cmdstr, argv[index], " ") != 0) {
                logerr("occli_strjoin error");
                break;
            }
        }

        ret = occli_parse_cliString(cmdstr);
        if (ret != SUCCESS) {
            printf("%s : Error : Invalid String\n", cmdstr);
            free(cmdstr);
            cmdstr = NULL;
        }

        if (cmdstr != NULL) {
            occli_send_cmd_to_ocmw(cmdstr, strlen(cmdstr));
            memset(response, 0, sizeof(response));
            occli_recv_cmd_resp_from_ocmw(response, sizeof(response));

            printf("%s\n", response);
            free(cmdstr);
        } else {
            printf("internal error\n");
        }
    }
    /* Entering interactive CLI */
    else if (strcmp("occli", basename(argv[0])) == 0) {

        /* Initialize readline */
        using_history();
        if ((snprintf(historyFile, HIT_FILE_BUFF_SIZE, "%s/%s", getenv("HOME"),
                HISTORY)) < 0) {
            return FAILED;
        }

        read_history(historyFile);
        rl_attempted_completion_function = occli_custom_completion;

        /*Printing hints at the start of opencelluar */
        occli_print_opencelluar();

        while (1) {
            line = readline(prompt);
            if (line == NULL) {
                break;
            }
            clistr = line;

            /* trim initial white spaces */
            while (isblank(*clistr)) {
                ++clistr;
            }
            occli_trim_extra_spaces(clistr);
            if (strcmp(clistr, "") == 0) {
                continue;
            }

            /* Quit the CLI, when command 'quit' is received */
            if (strcmp(clistr, "quit") == 0) {
                free(line);
                break;
            }
            /* Print the help manu */
            if((strstr(clistr, "help"))) {
                if ((strstr(clistr, "--help"))) {
                    ret = occli_printHelpMenu(sys_schema, clistr);
                    if (ret == FAILED) {
                        logerr("occli_printHelpMenu failed.\n");
                    }
                    continue;
                } else {
                    printf("%s : Error : Incorrect request\n", clistr);
                    printf ("Usage : subsystem --help"
                                " OR subsystem.component --help\n");
                    continue;
                }
            }

            ret = occli_parse_cliString(clistr);
            if (ret != SUCCESS) {
                printf("%s : Error : Invalid String\n", clistr);
                continue;
            }

            add_history(clistr);

            occli_send_cmd_to_ocmw(clistr, strlen(clistr));

            memset(response, 0, sizeof(response));
            occli_recv_cmd_resp_from_ocmw(response, sizeof(response));

            /* Print the command execution results in the CLI */
            printf("%s\n", response);

            free(line);
        }

        write_history(historyFile);
    }

    for (index= 0; ((s_allParams[index] != NULL) &&
                       (index < OCCLI_MAX_CLISTRING)); index++) {
        free(s_allParams[index]);
    }

    for (index= 0; ((s_subSetParams[index] != NULL) &&
                       (index < OCCLI_MAX_SUBSTRING)); index++) {
        free(s_subSetParams[index]);
    }
    occli_deinit_comm();
    deinitlog();

    return SUCCESS;
}
