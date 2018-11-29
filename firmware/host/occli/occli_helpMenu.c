#include <stdint.h>
#include <stdlib.h>
#include <logger.h>
#include <occli_common.h>

/**************************************************************************
 * Function Name    : occli_print_opencelluar
 * Description      : This Function print Opencelluar at prompt
 * Input(s)         :
 * Output(s)        :
 ***************************************************************************/
void occli_print_opencelluar()
{
    printf("\nHELP Usage:\n\t\"subsystem --help\""
           " OR \"subsystem.component --help\"");
    printf("\nEXIT:\n\t\"quit\"");

    printf("\nDisplay:\n\t\"Press Double TAB\"");
    printf("\nClear Screen:\n\t\"Press CTRL + L \"\n\n");
    return;
}

/**************************************************************************
 * Function Name    : occli_fill_data_from_param
 * Description      : This Function fill array data based on msgType
 * Input(s)         : param, helpMsgType, index
 * Output(s)        : index, helpMenuArray
 ***************************************************************************/
int8_t occli_fill_data_from_param(const Parameter *param,
                                  helpMenu *helpMenuArray[], int32_t *index,
                                  char *helpMsgType)
{
    int32_t helpIndex = *index;

    if ((helpMenuArray == NULL)) {
        logerr("Invalid Memory \n");
        return FAILED;
    }

    if (param && param->name) {
        strncpy(helpMenuArray[helpIndex]->msgType, helpMsgType,
                strlen(helpMsgType));
        if (strcmp(helpMsgType, "config") == 0) {
            strncpy(helpMenuArray[helpIndex]->actionType, "get/set",
                    strlen("get/set"));
        } else {
            strncpy(helpMenuArray[helpIndex]->actionType, "get", strlen("get"));
        }
        do {
            strncpy(helpMenuArray[helpIndex]->parameter, param->name,
                    strlen(param->name));
            param += 1;
            helpIndex++;
        } while (param && param->name);
        *index = helpIndex;
    }
    return SUCCESS;
}

/**************************************************************************
 * Function Name    : occli_fill_data_from_command
 * Description      : This Function fill array data for command
 * Input(s)         : command, index
 * Output(s)        : index, helpMenuArray
 ***************************************************************************/
int8_t occli_fill_data_from_command(const Command *command,
                                    helpMenu *helpMenuArray[], int32_t *index,
                                    char *subSysComp)
{
    int32_t helpIndex = *index;

    if ((helpMenuArray == NULL)) {
        logerr("Invalid Memory \n");
        return FAILED;
    }

    if (command && command->name) {
        strncpy(helpMenuArray[helpIndex]->msgType, "command",
                strlen("command"));
        while (command && command->name) {
            strncpy(helpMenuArray[helpIndex]->actionType, command->name,
                    strlen(command->name));

            if (strncmp(subSysComp, "debug", strlen("debug")) == 0) {
                if (strstr(subSysComp, "I2C")) {
                    if (strcmp(command->name, "get") == 0) {
                        strcpy(helpMenuArray[helpIndex++]->arguments,
                               "Slave Address(Decimal)");
                        strcpy(helpMenuArray[helpIndex++]->arguments,
                               "Number of Bytes(1/2)");
                        strcpy(helpMenuArray[helpIndex++]->arguments,
                               "Register Address(Decimal)");
                    } else {
                        strcpy(helpMenuArray[helpIndex++]->arguments,
                               "Slave Address(Decimal)");
                        strcpy(helpMenuArray[helpIndex++]->arguments,
                               "Number of Bytes(1/2)");
                        strcpy(helpMenuArray[helpIndex++]->arguments,
                               "Register Address(Decimal)");
                        strcpy(helpMenuArray[helpIndex]->arguments,
                               "Register Data(Decimal)");
                    }
                } else {
                    if (strcmp(command->name, "get") == 0) {
                        strcpy(helpMenuArray[helpIndex++]->arguments,
                               "Pin Number");
                    } else {
                        strcpy(helpMenuArray[helpIndex++]->arguments,
                               "Pin Number");
                        strcpy(helpMenuArray[helpIndex]->arguments,
                               "Value(0/1)");
                    }
                }
            } else if ((strncmp(subSysComp, "hci", strlen("hci")) == 0) &&
                       (strcmp(command->name, "set") == 0)) {
                strcpy(helpMenuArray[helpIndex++]->arguments, "Off/Red/Green");
            }
            if (strcmp(command->name, "send") == 0) {
                strcpy(helpMenuArray[helpIndex++]->arguments, "MSISDN Number");
                strcpy(helpMenuArray[helpIndex]->arguments, "Message");
            } else if (strcmp(command->name, "dial") == 0) {
                strcpy(helpMenuArray[helpIndex]->arguments, "MSISDN Number");
            }
            command += 1;
            helpIndex++;
        }
        *index = helpIndex;
    }

    return SUCCESS;
}

/**************************************************************************
 * Function Name    : occli_fill_data_from_post
 * Description      : This Function fill array data for command
 * Input(s)         : post, index
 * Output(s)        : index, helpMenuArray
 ***************************************************************************/
int8_t occli_fill_data_from_post(const Post *post, helpMenu *helpMenuArray[],
                                 int32_t *index, char *subSysComp)
{
    int32_t helpIndex = *index;

    if ((helpMenuArray == NULL)) {
        logerr("Invalid Memory \n");
        return FAILED;
    }

    if (post && post->name) {
        strncpy(helpMenuArray[helpIndex]->msgType, "post", strlen("post"));
        while (post && post->name) {
            strncpy(helpMenuArray[helpIndex]->actionType, post->name,
                    strlen(post->name));
            post += 1;
            helpIndex++;
        }
        *index = helpIndex;
    }

    return SUCCESS;
}

/**************************************************************************
 * Function Name    : occli_fill_data_from_driver
 * Description      : This Function will fill array data from driver
 * Input(s)         : devDriver
 * Output(s)        : helpMenuArray, index
 ***************************************************************************/
int8_t occli_fill_data_from_driver(const Driver *devDriver,
                                   helpMenu *helpMenuArray[], int32_t *index,
                                   char *subSysComp)
{
    const Parameter *param = NULL;
    const Command *command = NULL;
    const Post *post = NULL;
    int8_t ret = FAILED;

    if ((helpMenuArray == NULL) || (devDriver == NULL)) {
        logerr("Invalid Memory \n");
        return FAILED;
    }
    param = devDriver->config;
    ret = occli_fill_data_from_param(param, helpMenuArray, index, "config");

    param = devDriver->status;
    ret = occli_fill_data_from_param(param, helpMenuArray, index, "status");

    command = devDriver->commands;
    ret =
        occli_fill_data_from_command(command, helpMenuArray, index, subSysComp);
    post = devDriver->post;
    ret = occli_fill_data_from_post(post, helpMenuArray, index, subSysComp);
    return ret;
}

/**************************************************************************
 * Function Name    : occli_printHelpMenu_on_console
 * Description      : This Function will print CLI help Menu
 * Input(s)         : helpMenuArray, index, subSystem
 * Output(s)        : NA
 ***************************************************************************/
void occli_printHelpMenu_on_console(helpMenu *helpMenuArray[], int32_t index,
                                    char *subSystem)
{
    int32_t printIndex = 0;

    if ((strcmp(subSystem, "testmodule") == 0) ||
        (strcmp(subSystem, "hci") == 0)) {
        /*Printing for testmodule and hci subSystem */
        printf("\nExample :\n");
        if (strcmp(subSystem, "testmodule") == 0) {
            printf("testmodule.2gsim send 9789799425 hi\n");
            printf("testmodule.2gsim dial 9789799425\n");
            printf("testmodule.2gsim connect_nw\n\n");
        } else {
            printf("hci.led set 1\n\n");
        }

        printf("----------------------------------------------------------"
               "-------------------------------------------\n");
        printf("%-12s%-12s%-15s%-10s%-15s%-20s%-15s\n", "Subsystem",
               "Component", "SubComponent", "MsgType", "ActionType",
               "Parameter", "Arguments");
        printf("----------------------------------------------------------"
               "-------------------------------------------\n");
        for (printIndex = 0; printIndex < index; printIndex++) {
            printf("%-12s%-12s%-15s%-10s%-15s%-20s%-15s\n",
                   helpMenuArray[printIndex]->subSystem,
                   helpMenuArray[printIndex]->component,
                   helpMenuArray[printIndex]->subComponent,
                   helpMenuArray[printIndex]->msgType,
                   helpMenuArray[printIndex]->actionType,
                   helpMenuArray[printIndex]->parameter,
                   helpMenuArray[printIndex]->arguments);
        }
        printf("----------------------------------------------------------"
               "-------------------------------------------\n");
    } else if (strcmp(subSystem, "debug") == 0) {
        /*Printing for debug subSystem */
        printf("\nGet Example :\n");
        printf("debug.I2C.bus0 get 104 2 58\n");
        printf("debug.gbc.ioexpanderx70 get 1\n\n");
        printf("Set Example :\n");
        printf("debug.I2C.bus0 set 104 2 58 1\n");
        printf("debug.gbc.ioexpanderx70 set 1 0\n");
        printf("\n-------------------------------------------------------"
               "----------------------------------------"
               "---------\n");
        printf("%-15s%-15s%-15s%-15s%-15s%-20s\n", "Subsystem", "Component",
               "SubComponent", "MsgType", "ActionType", "Arguments");
        printf("-------------------------------------------------------"
               "----------------------------------------"
               "---------\n");
        for (printIndex = 0; printIndex < index; printIndex++) {
            printf("%-15s%-15s%-15s%-15s%-15s%-20s\n",
                   helpMenuArray[printIndex]->subSystem,
                   helpMenuArray[printIndex]->component,
                   helpMenuArray[printIndex]->subComponent,
                   helpMenuArray[printIndex]->msgType,
                   helpMenuArray[printIndex]->actionType,
                   helpMenuArray[printIndex]->arguments);
        }
        printf("-------------------------------------------------------"
               "----------------------------------------"
               "---------\n");
    } else {
        /*Printing for all othere subSystem */
        /*  Dispalay of parameter default value and unit in help menu
         *  will be take care with common schema factory config
         */
        printf("\n------------------------------------------------------------"
               "----------------------------------\n");
        printf("%-12s%-17s%-18s%-12s%-12s%-23s\n", "Subsystem", "Component",
               "SubComponent", "MsgType", "ActionType", "Parameter");
        printf("\n------------------------------------------------------------"
               "----------------------------------\n");
        for (printIndex = 0; printIndex < index; printIndex++) {
            printf("%-12s%-17s%-18s%-12s%-12s%-23s\n",
                   helpMenuArray[printIndex]->subSystem,
                   helpMenuArray[printIndex]->component,
                   helpMenuArray[printIndex]->subComponent,
                   helpMenuArray[printIndex]->msgType,
                   helpMenuArray[printIndex]->actionType,
                   helpMenuArray[printIndex]->parameter);
        }
        printf("\n------------------------------------------------------------"
               "----------------------------------\n");
    }
}

/**************************************************************************
 * Function Name    : occli_free_helpMenupointer
 * Description      : This is an inline function to free memory
 * Input(s)         : helpMenuArray
 * Output(s)        : helpMenuArray
 ***************************************************************************/
static void occli_free_helpMenupointer(helpMenu *helpMenuArray[])
{
    int32_t mallocIndex = 0;

    for (mallocIndex = 0; mallocIndex < OCCLI_HELP_MAX_SIZE; mallocIndex++) {
        if (helpMenuArray[mallocIndex])
            free(helpMenuArray[mallocIndex]);
    }

    return;
}

/**************************************************************************
 * Function Name    : occli_printHelpMenu
 * Description      : This Function will print CLI help Menu
 * Input(s)         : root, cmd
 * Output(s)        : NA
 ***************************************************************************/
int8_t occli_printHelpMenu(const Component *root, char *cmd)
{
    int32_t index = 0;
    int32_t mallocIndex = 0;
    const Component *subSystem = root;
    const Component *component = NULL;
    const Component *subComponent = NULL;
    const Driver *devDriver = NULL;
    const Command *command = NULL;
    char *token = NULL;
    char *cliStr = NULL;
    char *cmdBkp = NULL;
    char componentStr[OCCLI_CHAR_ARRAY_SIZE] = { 0 };
    char subSys[OCCLI_CHAR_ARRAY_SIZE] = { 0 };
    char subSysComp[OCCLI_CHAR_ARRAY_SIZE] = { 0 };
    int8_t ret = FAILED;
    int8_t count = 1;
    int8_t subCount = 0;
    int8_t compCount = 0;
    helpMenu *helpMenuArray[OCCLI_HELP_MAX_SIZE];

    if ((subSystem == NULL) || (cmd == NULL)) {
        logerr("Invalid Memory \n");
        return FAILED;
    }

    cmdBkp = (char *)malloc(sizeof(cmd));
    if (cmdBkp == NULL) {
        logerr("Invalid Memory \n");
        return FAILED;
    }
    strcpy(cmdBkp, cmd);

    /* Tokenizing string for subsystem and component */
    cliStr = strtok(cmd, " ");
    if ((cliStr == NULL) || (strstr(cliStr, "help"))) {
        printf("%s : Error : Incorrect request\n", cmdBkp);
        printf("Usage : subsystem --help or subsystem.component --help\n");
        free(cmdBkp);
        return FAILED;
    } else {
        token = strtok(cliStr, " .");
        strcpy(subSys, token);
        while (token) {
            token = strtok(NULL, " .");
            if (token == NULL)
                break;
            count++;
            if (count > 2) {
                printf("%s : Error : Incorrect request\n", cmdBkp);
                printf("Usage : subsystem --help"
                       " OR subsystem.component --help\n");
                free(cmdBkp);
                return FAILED;
            }
            strcpy(componentStr, token);
        }
    }
    for (mallocIndex = 0; mallocIndex < OCCLI_HELP_MAX_SIZE; mallocIndex++) {
        helpMenuArray[mallocIndex] = (helpMenu *)malloc(sizeof(helpMenu));
        if (helpMenuArray[mallocIndex] == NULL) {
            logerr("Invalid Memory \n");
            free(cmdBkp);
            return FAILED;
        } else {
            memset(helpMenuArray[mallocIndex], '\0', sizeof(helpMenu));
        }
    }
    /* Parsing schema */
    while (subSystem && subSystem->name) {
        if (strcmp(subSys, subSystem->name) == 0) {
            subCount++;
            strncpy((helpMenuArray[index++]->subSystem), subSystem->name,
                    strlen(subSystem->name));
            component = subSystem->components;
            while (component && component->name) {
                if ((count == 2)) {
                    if (strcmp(component->name, componentStr)) {
                        component += 1;
                        continue;
                    } else {
                        compCount++;
                    }
                }
                strncpy((helpMenuArray[index]->component), component->name,
                        strlen(component->name));
                sprintf(subSysComp, "%s.%s", subSystem->name, component->name);
                command = component->commands;
                ret = occli_fill_data_from_command(command, helpMenuArray,
                                                   &index, subSysComp);
                devDriver = component->driver;
                if (devDriver != NULL) {
                    ret = occli_fill_data_from_driver(devDriver, helpMenuArray,
                                                      &index, subSysComp);
                    if (ret == FAILED) {
                        logerr("\noccli_fill_data_from_driver Error");
                        occli_free_helpMenupointer(helpMenuArray);
                        return ret;
                    }
                }
                index++;
                subComponent = component->components;
                while (subComponent && subComponent->name) {
                    strncpy((helpMenuArray[index]->subComponent),
                            subComponent->name, strlen(subComponent->name));
                    command = subComponent->commands;
                    ret = occli_fill_data_from_command(command, helpMenuArray,
                                                       &index, subSysComp);
                    devDriver = subComponent->driver;
                    if (devDriver != NULL) {
                        ret = occli_fill_data_from_driver(
                            devDriver, helpMenuArray, &index, subSysComp);
                        if (ret == FAILED) {
                            logerr("\noccli_fill_data_from_driver Error");
                            occli_free_helpMenupointer(helpMenuArray);
                            return ret;
                        }
                    }
                    index++;
                    subComponent += 1;
                }
                component += 1;
            }
            if ((count == 2) && (compCount == 0)) {
                printf("%s : Error : Invalid Component\n", cmdBkp);
                occli_free_helpMenupointer(helpMenuArray);
                free(cmdBkp);
                return FAILED;
            }
        }
        subSystem += 1;
    }
    if (subCount == 0) {
        printf("%s : Error : Invalid Subsystem\n", cmdBkp);
        occli_free_helpMenupointer(helpMenuArray);
        free(cmdBkp);
        return FAILED;
    }
    occli_printHelpMenu_on_console(helpMenuArray, index, subSys);
    occli_free_helpMenupointer(helpMenuArray);
    free(cmdBkp);
    return SUCCESS;
}
