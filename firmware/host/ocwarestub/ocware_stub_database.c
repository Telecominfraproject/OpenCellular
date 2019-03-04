/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include <ocware_stub_main_module.h>
#include <ocmw_schema.h>

static OCWareStubsizeflag s_typeFlag;
static OCWareStubDatabase s_ocwareGlobalData[MAX_NUMBER_PARAM];
static OCWareStubPostData s_postGlobalArray[MAX_POST_DEVICE];
static OCWareStubAlertData alertGlobalData[MAX_NUMBER_PARAM];

static int16_t s_defInt16Val = DEFAULT_INT16;
static int64_t s_defInt64Val = DEFAULT_INT64;
static int8_t s_defInt8Val = DEFAULT_INT8;
static int32_t s_defInt32Val = DEFAULT_INT32;
static int8_t s_defEnumVal = DEFAULT_ENUM;
static int8_t s_debugSubsystem = 0;

int8_t debugGetCommand = STUB_FAILED;
int8_t debugSetCommand = STUB_FAILED;
int8_t PostResult = STUB_FAILED;
int8_t PostEnable = STUB_FAILED;
int16_t allAlertCount = 0;
extern const Component sys_schema[];
/******************************************************************************
 * Function Name    : ocware_stub_set_gpioinfo
 * Description      : Modify the GPIO information in the DB for debug subsystem
 *
 * @param gStructIndex - Index of the database (by value)
 * @param payload  - payload field of the message from MW (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_set_gpioinfo(int16_t gStructIndex,
                                         char *payload)
{
    OCWareDebugGPIOinfo **GPIOInfo;
    ocware_stub_ret ret = STUB_FAILED;
    int8_t pos = 0;
    int8_t index = 0;

    if (!(gStructIndex <= MAX_NUMBER_PARAM && payload)) {
        return ret;
    }

    GPIOInfo = (OCWareDebugGPIOinfo**)&s_ocwareGlobalData[gStructIndex].data;
    for(index = 0; index< MAX_GPIO_COMP_NBR; index++) {
        if (GPIOInfo[index]->pin_nbr == payload[pos]) {
            ret = STUB_SUCCESS;
            GPIOInfo[index]->value = payload[pos + 1];
            break;
        }
    }

    return ret;
}

/******************************************************************************
 * Function Name    : ocware_stub_set_i2cinfo
 * Description      : Modify the I2C information in the DB for debug subsystem
 *
 * @param gStructIndex - Index of the database (by value)
 * @param payload  - payload field of the message from MW (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_set_i2cinfo(int16_t gStructIndex,
                                        char *payload)
{
    OCWareDebugI2Cinfo **I2CInfo;
    ocware_stub_ret ret = STUB_FAILED;
    int8_t pos = 0;
    int8_t index = 0;
    if (!(gStructIndex <= MAX_NUMBER_PARAM && payload)) {
        return ret;
    }
    I2CInfo = (OCWareDebugI2Cinfo **)&s_ocwareGlobalData[gStructIndex].data;
    for(index = 0; index < MAX_I2C_COMP_NBR; index++) {
        if (I2CInfo[index]->slaveAddress == payload[pos]) {
            ret = STUB_SUCCESS;
            break;
        }
    }

    if (ret !=STUB_SUCCESS)
        return ret;

    I2CInfo[index]->slaveAddress =  payload[pos];
    I2CInfo[index]->numOfBytes = payload[pos + 1];
    I2CInfo[index]->regAddress = payload[pos + 2];
    if (I2CInfo[index]->numOfBytes == 1) {
        I2CInfo[index]->regValue = payload[pos + 3];
    } else {
        I2CInfo[index]->regValue = ((uint8_t)payload[pos + 3] << 0x08)
                                                        +  payload[pos + 4];
        payload[pos + 4] =
            (uint8_t)(I2CInfo[index]->regValue & MASK_LSB);
        payload[pos + 3] =
            (uint8_t)((I2CInfo[index]->regValue & MASK_MSB) >> SHIFT_NIBBLE);
    }
    return ret;
}
/******************************************************************************
 * Function Name    : ocware_stub_set_mdioinfo
 * Description      : Modify the MDIO information in the DB for debug subsystem
 *
 * @param gStructIndex - Index of the database (by value)
 * @param payload  - payload field of the message from MW (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_set_mdioinfo(int16_t gStructIndex,
                                        char *payload)
{
    OCWareDebugMDIOinfo **MDIOInfo;
    ocware_stub_ret ret = STUB_FAILED;
    int8_t pos = 0;
    int8_t index = 0;
    if (!(gStructIndex <= MAX_NUMBER_PARAM && payload)) {
        return ret;
    }
    MDIOInfo = (OCWareDebugMDIOinfo **)&s_ocwareGlobalData[gStructIndex].data;
    for(index = 0; index< MAX_MDIO_COMP_NBR; index++) {
        if (MDIOInfo[index]->regAddress == payload[pos]) {
            ret = STUB_SUCCESS;
            break;
        }
    }
    if (ret !=STUB_SUCCESS)
        return ret;

    MDIOInfo[index]->regAddress = payload[pos];
    MDIOInfo[index]->regValue = ((uint8_t)payload[pos + 3] << 0x08)
                                                    +  payload[pos + 2];
    payload[pos + 2] =
            (uint8_t)(MDIOInfo[index]->regValue & MASK_LSB);
    payload[pos + 3] =
            (uint8_t)((MDIOInfo[index]->regValue & MASK_MSB) >> SHIFT_NIBBLE);
    return ret;
}
/******************************************************************************
 * Function Name    :  ocware_stub_get_post_database
 * Description      :  extract device number and status from the post database
 *                     depending on the subsytem id
 *
 * @param msgFrameData  - output pointer to the OCMPheader field of the message
 *                     from MW (by reference)
 * @param   payload - output pointer to the payload field of the message from MW
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_get_post_database(OCMPMessage *msgFrameData,
                                              char *payload)
{
    int16_t index = 0;
    uint8_t pos = 0;

    for(index = 0; index < MAX_POST_DEVICE; index++) {
        if ((s_postGlobalArray[index].SubsystemId
                == payload[0]) &&
            (s_postGlobalArray[index].DeviceNumber > 0)) {

            payload[pos] = s_postGlobalArray[index].SubsystemId;
            payload[pos + 1] =
                s_postGlobalArray[index].DeviceNumber;
            payload[pos + 2] =
                s_postGlobalArray[index].Status;
            pos = pos + 3;
        }
    }
    msgFrameData->parameters = pos/3;
    return STUB_SUCCESS;
}

/******************************************************************************
 * Function Name    :  ocware_stub_frame_alert_msgframe
 * Description      :  extract alert data from based on subsystem
 *
 * @param msgFrameData  - output pointer to the OCMPheader field of the message
 *                     from MW (by reference)
 * @param   payload - output pointer to the payload field of the message from MW
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_frame_alert_msgframe(char *buffer)
{
    uint32_t ret = 0;
    uint32_t subSystemNbr = 0;
    uint32_t index = 0;
    uint32_t alertIndex = 0;
    uint32_t subsystemIndex = 0;
    uint8_t printIndex = 0;
    uint8_t tempAlertCount = allAlertCount;

    OCMPMessageFrame *msgFrame = (OCMPMessageFrame *)buffer;
    OCMPMessage *msgFrameData = (OCMPMessage*)&msgFrame->message;
    subSystemNbr = msgFrameData->subsystem;
    if (subSystemNbr != 0) {
        for (subsystemIndex = 0; subsystemIndex < allAlertCount;
            subsystemIndex++) {
            if (alertGlobalData[subsystemIndex].subsystemId == subSystemNbr)
                break;
        }
        alertIndex = subsystemIndex;
        while (alertGlobalData[alertIndex].subsystemId == subSystemNbr)
            alertIndex++;
        allAlertCount = alertIndex;
    }
    for (index = subsystemIndex; index < allAlertCount; index++) {
        /*create the msgframe*/
        ret = ocware_stub_parse_alert_get_message(buffer, index);
        printf(" \n Sending Data :\n");
        for (printIndex = 0; printIndex < OC_EC_MSG_SIZE; printIndex++) {
            printf("0x%x  ", buffer[printIndex] & 0xff);
        }
        ret = ocware_stub_send_msgframe_middleware(&buffer,
                             sizeof(OCMPMessageFrame));
        if (ret != STUB_SUCCESS) {
            printf("ocware_stub_send_msgframe_middleware failed: error value :"
                            "%d\n", ret);
            return STUB_FAILED;
        }
    }
    allAlertCount = tempAlertCount;
    return STUB_SUCCESS;
}
/******************************************************************************
 * Function Name    :  ocware_stub_get_alert_database
 * Description      :  extract alert data from lookup table
 *
 * @param msgFrameData  - output pointer to the OCMPheader field of the message
 *                     from MW (by reference)
 * @param   payload - output pointer to the payload field of the message from MW
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_get_alert_database(OCMPMessage *msgFrameData,
                                              char *payload, int8_t index)
{
    uint8_t pos = 0;
    uint8_t defHour = 10;
    uint8_t defMinut = 22;
    uint8_t defSecond = 15;
    uint8_t defDay = 2;
    uint8_t defMonth = 7;
    uint8_t defYear = 18;
    if (index == (allAlertCount - 1)) {
        msgFrameData->action = OCMP_AXN_TYPE_REPLY;
    } else {
        msgFrameData->action = OCMP_AXN_TYPE_ACTIVE;
    }
    payload[pos + 1] = 2;
    strcpy(&payload[pos + 1], (char*)&defHour);
    strcpy(&payload[pos + 2], (char*)&defMinut);
    strcpy(&payload[pos + 3], (char*)&defSecond);
    strcpy(&payload[pos + 4], (char*)&defDay);
    strcpy(&payload[pos + 5], (char*)&defMonth);
    strcpy(&payload[pos + 6], (char*)&defYear);
    strncpy(&payload[pos + 7], (char*)alertGlobalData[index].data,
                  alertGlobalData[index].paramSize);
    strncpy(&payload[pos + 9], (char*)alertGlobalData[index].data,
                  alertGlobalData[index].paramSize);
    msgFrameData->subsystem = alertGlobalData[index].subsystemId;
    msgFrameData->componentID = alertGlobalData[index].componentId;
    msgFrameData->parameters = alertGlobalData[index].paramId;

    return STUB_SUCCESS;
}

/******************************************************************************
 * Function Name    : ocware_stub_get_gpioinfo
 * Description      : retrieve the GPIO information in the DB for debug
 *                    subsystem
 *
 * @param gStructIndex  index of the database (by value)
 * @param payload   output pointer to the payload filed of the
 *                  message from MW (by pointer)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_get_gpioinfo(int16_t gStructIndex,
                                         char *payload)
{
    OCWareDebugGPIOinfo **GPIOInfo;
    ocware_stub_ret ret = STUB_FAILED;
    uint8_t pos = 0;
    uint8_t index = 0;

    if (!(gStructIndex <= MAX_NUMBER_PARAM && payload)) {
        return ret;
    }
    GPIOInfo = (OCWareDebugGPIOinfo**)&s_ocwareGlobalData[gStructIndex].data;
    for(index = 0; index< MAX_GPIO_COMP_NBR; index++) {
        if (GPIOInfo[index]->pin_nbr == payload[pos]) {
            ret = STUB_SUCCESS;
            break;
        }
    }

    if(ret == STUB_SUCCESS) {
        payload[pos] = GPIOInfo[index]->pin_nbr;
        payload[pos + 1] = GPIOInfo[index]->value;
    }
    return ret;
}
/******************************************************************************
 * Function Name    : ocware_stub_get_i2cinfo
 * Description      : retrieve the I2C information in the DB for debug
 *                    subsystem
 *
 * @param gStructIndex  index of the database (by value)
 * @param payload   output pointer to the payload filed of the
 *                  message from MW (by pointer)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_get_i2cinfo(int16_t gStructIndex,
                                        char *payload)
{
    OCWareDebugI2Cinfo **I2CInfo;
    ocware_stub_ret ret = STUB_FAILED;
    uint8_t pos = 0;
    uint8_t index = 0;

    if (!(gStructIndex <= MAX_NUMBER_PARAM && payload)) {
        return ret;
    }

    I2CInfo = (OCWareDebugI2Cinfo**)&s_ocwareGlobalData[gStructIndex].data;
    for(index = 0; index< MAX_I2C_COMP_NBR; index++) {
        if (I2CInfo[index]->slaveAddress == payload[pos]) {
            ret = STUB_SUCCESS;
            break;
        }
    }

    if(ret == STUB_SUCCESS) {
        payload[pos] = I2CInfo[index]->slaveAddress;
        payload[pos + 1] = I2CInfo[index]->numOfBytes;
        payload[pos + 2] = I2CInfo[index]->regAddress;
        if (I2CInfo[index]->numOfBytes == 1) {
            payload[pos + 3] = (uint8_t)I2CInfo[index]->regValue;
        } else {
            payload[pos + 4]
                = (uint8_t)(I2CInfo[index]->regValue & MASK_LSB);
            payload[pos + 3] = (uint8_t)
                    ((I2CInfo[index]->regValue & MASK_MSB) >> SHIFT_NIBBLE);
        }
    }
    return ret;
}
/******************************************************************************
 * Function Name    : ocware_stub_get_mdioinfo
 * Description      : retrieve the MDIO information in the DB for debug
 *                    subsystem
 *
 * @param gStructIndex  index of the database (by value)
 * @param payload   output pointer to the payload filed of the
 *                  message from MW (by pointer)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_get_mdioinfo(int16_t gStructIndex,
                                        char *payload)
{
    OCWareDebugMDIOinfo **MDIOInfo;
    ocware_stub_ret ret = STUB_FAILED;
    uint8_t pos = 0;
    uint8_t index = 0;

    if (!(gStructIndex <= MAX_NUMBER_PARAM && payload)) {
        return ret;
    }

    MDIOInfo = (OCWareDebugMDIOinfo**)&s_ocwareGlobalData[gStructIndex].data;
    for(index = 0; index < MAX_MDIO_COMP_NBR; index++) {
        if (MDIOInfo[index]->regAddress == payload[pos]) {
            ret = STUB_SUCCESS;
            break;
        }
    }

    if(ret == STUB_SUCCESS) {
        payload[pos] = MDIOInfo[index]->regAddress;
        payload[pos + 2] = (uint8_t)
                    (MDIOInfo[index]->regValue & MASK_LSB);
        payload[pos + 3] = (uint8_t)
                    ((MDIOInfo[index]->regValue & MASK_MSB) >> SHIFT_NIBBLE);
    }
    return ret;
}
/******************************************************************************
 * Function Name    :  ocware_stub_debug_subsytem
 * Description      :  Process the command message for debug subsytem
 *
 * @param   gStructIndex - index to the DB (by value)
 * @param   payload  - output pointer to the payload field of the message
 *                     from MW (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_debug_subsytem(int16_t gStructIndex,
                                           char *payload,
                                           int8_t flag)
{
    if (!(gStructIndex <= MAX_NUMBER_PARAM && payload)) {
        return STUB_FAILED;
    }
    switch(s_ocwareGlobalData[gStructIndex].componentId) {
        case 2:
            if ( flag == OCMP_AXN_TYPE_GET) {
                ocware_stub_get_i2cinfo(gStructIndex, payload);
            } else {
                ocware_stub_set_i2cinfo(gStructIndex, payload);
            }
            break;
        case 8:
            if ( flag == OCMP_AXN_TYPE_GET) {
                ocware_stub_get_mdioinfo(gStructIndex, payload);
            } else {
                ocware_stub_set_mdioinfo(gStructIndex, payload);
            }
            break;
        default:
            if ( flag == OCMP_AXN_TYPE_GET) {
                ocware_stub_get_gpioinfo(gStructIndex, payload);
            } else {
                ocware_stub_set_gpioinfo(gStructIndex, payload);
            }
    }
    return STUB_SUCCESS;
}
/******************************************************************************
 * Function Name    : ocware_stub_get_database
 * Description      : Function to retrieve data from the DB
 *
 * @param   msgFrameData - Pointer to the message frame field of the message
 *                         from MW (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_get_database(OCMPMessage *msgFrameData)
{
    char *payload = NULL;
    ocware_stub_ret ret = STUB_FAILED;
    int8_t subsystem = 0;
    int8_t component = 0;
    int8_t  msgtype = 0;
    int8_t parampos = 0;
    int16_t paramId = 0;
    int16_t gStructIndex = 0;

    if(msgFrameData == NULL) {
        return ret;
    }
    payload =  (char*)&msgFrameData->info;
    subsystem = msgFrameData->subsystem;
    component = msgFrameData->componentID;
    msgtype = msgFrameData->msgtype;
    paramId = msgFrameData->parameters;

    for(gStructIndex = 0; gStructIndex < MAX_NUMBER_PARAM; gStructIndex++) {
        if((subsystem == s_ocwareGlobalData[gStructIndex].subsystemId) &&
           (component == s_ocwareGlobalData[gStructIndex].componentId) &&
           (msgtype == s_ocwareGlobalData[gStructIndex].msgtype) &&
           (paramId == s_ocwareGlobalData[gStructIndex].paramId))
        {
            ret = STUB_SUCCESS;
            parampos = s_ocwareGlobalData[gStructIndex].paramPos;
            break;
        }
    }

    if(ret == STUB_SUCCESS) {
        if(subsystem == s_debugSubsystem) {
            ret = ocware_stub_debug_subsytem(gStructIndex, payload,
                                                    OCMP_AXN_TYPE_GET);
        } else {
            strncpy(&payload[parampos],
                    (char*)s_ocwareGlobalData[gStructIndex].data,
                    s_ocwareGlobalData[gStructIndex].paramSize);
        }
    }
    return ret;
}

/******************************************************************************
 * Function Name    : ocware_stub_set_database
 * Description      : Function to modify data in the DB
 *
 * @param   msgFrameData - Pointer to the message frame field of the message
 *                         from MW (by reference)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_set_database(OCMPMessage *msgFrameData)
{
    char *payload = NULL;
    ocware_stub_ret ret = STUB_FAILED;
    int8_t subsystem = 0;
    int8_t component = 0;
    int8_t msgtype = 0;
    int8_t parampos = 0;
    int16_t paramId = 0;
    int16_t gStructIndex = 0;

    if(msgFrameData == NULL) {
        return ret;
    }
    payload =  (char*)&msgFrameData->info;
    subsystem = msgFrameData->subsystem;
    component = msgFrameData->componentID;
    msgtype = msgFrameData->msgtype;
    paramId = msgFrameData->parameters;

    for(gStructIndex = 0; gStructIndex < MAX_NUMBER_PARAM; gStructIndex++) {
        if( (subsystem == s_ocwareGlobalData[gStructIndex].subsystemId) &&
            (component == s_ocwareGlobalData[gStructIndex].componentId) &&
            (msgtype == s_ocwareGlobalData[gStructIndex].msgtype) &&
            (paramId == s_ocwareGlobalData[gStructIndex].paramId))
        {
            ret = STUB_SUCCESS;
            parampos = s_ocwareGlobalData[gStructIndex].paramPos;
            break;
        }
    }

    if(ret == STUB_SUCCESS) {
        if(subsystem == s_debugSubsystem) {
            ret = ocware_stub_debug_subsytem(gStructIndex, payload,
                                                    OCMP_AXN_TYPE_SET);
        } else {
            strncpy((char*)s_ocwareGlobalData[gStructIndex].data,
                    &payload[parampos],
                    s_ocwareGlobalData[gStructIndex].paramSize);
        }
    }
    return ret;
}

/******************************************************************************
 * Function Name    : ocware_stub_fill_init_value
 * Description      : Function to fill default value in the DB
 *
 * @param   param - pointer to the parameter field in a particular subsytem and
 *                  component (by value)
 *          gStructIndex - index to the DB (by value)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_fill_init_value(const Parameter *param,
                                            uint16_t gStructIndex)
{
    const char *paramtype;
    int16_t def_lasterror = 0x101;
    int16_t def_netOp = 0x001;

    if(param == NULL) {
        return STUB_FAILED;
    }

    paramtype = DATA_TYPE_MAP[param->type];

    if (!strcmp("lasterror",param->name)) {
        strcpy(s_ocwareGlobalData[gStructIndex].data, (char*)&def_lasterror);
        return STUB_SUCCESS;
    } else if (!strcmp("network_operatorinfo",param->name)) {
        strcpy(s_ocwareGlobalData[gStructIndex].data, (char*)&def_netOp);
        return STUB_SUCCESS;
    }
    if (!strcmp("uint16",paramtype)) {
        strcpy(s_ocwareGlobalData[gStructIndex].data, (char *)&s_defInt16Val);
    } else if (!strcmp("int16",paramtype)) {
        strcpy(s_ocwareGlobalData[gStructIndex].data, (char*)&s_defInt16Val);
    } else if (!strcmp("uint8",paramtype)) {
        strcpy(s_ocwareGlobalData[gStructIndex].data, (char*)&s_defInt8Val);
    } else if (!strcmp("int8",paramtype)) {
        strcpy(s_ocwareGlobalData[gStructIndex].data, (char*)&s_defInt8Val);
    } else if (!strcmp("uint32",paramtype)) {
        strcpy(s_ocwareGlobalData[gStructIndex].data, (char*)&s_defInt32Val);
    } else if (!strcmp("uint64",paramtype)) {
        strcpy(s_ocwareGlobalData[gStructIndex].data, (char *)&s_defInt64Val);
    } else if (!strcmp("string",paramtype)) {
        strcpy(s_ocwareGlobalData[gStructIndex].data, DEFAULT_STRING);
    } else if (!strcmp("enum",paramtype)) {
        strcpy(s_ocwareGlobalData[gStructIndex].data, (char*)&s_defEnumVal);
    } else {
        strcpy(s_ocwareGlobalData[gStructIndex].data, (char*)&s_defInt8Val);
    }
    return STUB_SUCCESS;
}
/******************************************************************************
 * Function Name    : ocware_stub_get_paramSize
 * Description      : Return the size of the parameter
 *
 * @param   paramtype - pointer to a particular parameter in the schema
 *                      (by reference)
 *          msgtype - msgtype field of the message from MW (by value)
 *          param_name - pointer to the name of a parameter in the schema
 *                      (by reference)
 * @return Size of the parameter
 *
 ******************************************************************************/
int8_t ocware_stub_get_paramSize(const char* paramtype,
                                 int8_t msgtype,
                                 const char* param_name)
{
    int8_t paramSize = 0;

    if (paramtype == NULL) {
        printf("Invalid paramtype\n");
        return STUB_FAILED;
    }

    if (!strcmp("uint16",paramtype)) {
        paramSize = sizeof(uint16_t);
    } else if (!strcmp("int16",paramtype)) {
        paramSize = sizeof(int16_t);
    } else if (!strcmp("uint8",paramtype)) {
        paramSize = sizeof(uint8_t);
    } else if (!strcmp("int8",paramtype)) {
        paramSize = sizeof(int8_t);
    } else if (!strcmp("uint32",paramtype)) {
        paramSize = sizeof(uint32_t);
    } else if (!strcmp("uint64",paramtype)) {
        paramSize = sizeof(uint64_t);
    } else if (!strcmp("string",paramtype)) {
       if (s_typeFlag == OCSTUB_VALUE_TYPE_MFG) {
       paramSize = SIZE_OF_TYPE_MFG;
       } else if (s_typeFlag == OCSTUB_VALUE_TYPE_MODEL) {
           paramSize = SIZE_OF_TYPE_MODEL;
       } else if (s_typeFlag == OCSTUB_VALUE_TYPE_GETMODEL) {
           paramSize = SIZE_OF_TYPE_GETMODEL;
       } else if (s_typeFlag == OCSTUB_VALUE_TYPE_OCSERIAL_INFO) {
           paramSize = SIZE_OF_TYPE_OCSERIAL_INFO;
       } else if (s_typeFlag == OCSTUB_VALUE_TYPE_GBCBOARD_INFO) {
           paramSize = SIZE_OF_TYPE_GBCBOARD_INFO;
       } else {
           if (msgtype == OCMP_MSG_TYPE_CONFIG) {
               paramSize = EEPROM_CONFIG_MAX_SIZE;
           } else if (msgtype == OCMP_MSG_TYPE_STATUS) {
               paramSize = EEPROM_STATUS_MAX_SIZE;
           }
       }
    } else if (!strcmp("enum",paramtype)) {
       if (s_typeFlag == OCSTUB_VALUE_TYPE_REGISTRATION) {
            paramSize = SIZE_OF_TYPE_REGISTRATION; /* TO DO */
       } else if (s_typeFlag == OCSTUB_VALUE_TYPE_NWOP_STRUCT) {
            paramSize = SIZE_OF_NWOP_STRUCT;
       } else if (s_typeFlag == OCSTUB_VALUE_TYPE_LAST_ERROR) {
            paramSize = SIZE_OF_LAST_ERROR;
       } else {
            paramSize = OCMW_VALUE_TYPE_ENUM;
       }
    } else {
        paramSize = sizeof(int8_t);
    }

    return paramSize;
}
/******************************************************************************
 * Function Name    : ocware_stub_fill_init_debug_value
 * Description      : Fill default values for debug subsytem
 *
 * @param   gStructIndex - index to the DB (by value)
 *          flag - OCSTUB_VALUE_TYPE_GPIO_DEBUG -To modify GPIO parameters
 *                 OCSTUB_VALUE_TYPE_I2C_DEBUG - To modify I2C parameters
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_fill_init_debug_value(int16_t gStructIndex,
                                                  OCWareStubsizeflag flag)
{
    OCWareDebugI2Cinfo I2CInfo[MAX_I2C_COMP_NBR];
    OCWareDebugGPIOinfo GPIOInfo[MAX_GPIO_COMP_NBR];
    OCWareDebugMDIOinfo MDIOInfo[MAX_MDIO_COMP_NBR];
    uint8_t index = 0;

    if(gStructIndex >= MAX_NUMBER_PARAM) {
        return STUB_FAILED;
    }
    if ( flag == OCSTUB_VALUE_TYPE_GPIO_DEBUG) {
        for(index = 0; index < MAX_GPIO_COMP_NBR; index++) {
            GPIOInfo[index].pin_nbr = GPIO_PIN_NBR + index;
            GPIOInfo[index].value = GPIO_VALUE;
        }
        memcpy(s_ocwareGlobalData[gStructIndex].data,
            (char *)GPIOInfo, sizeof(GPIOInfo)*MAX_GPIO_COMP_NBR);

    } else if ( flag == OCSTUB_VALUE_TYPE_MDIO_DEBUG){
        for(index = 0; index < MAX_MDIO_COMP_NBR; index++) {
            MDIOInfo[index].regAddress = I2C_REG_ADDRESS + index;
            MDIOInfo[index].regValue = I2C_REG_VALUE;
        }
        memcpy(s_ocwareGlobalData[gStructIndex].data,
            (char *)MDIOInfo, sizeof(MDIOInfo)*MAX_MDIO_COMP_NBR);
    }else {
        for(index = 0; index < MAX_I2C_COMP_NBR; index++) {
            I2CInfo[index].slaveAddress = I2C_SLAVE_ADDRESS + index;
            I2CInfo[index].numOfBytes = I2C_NUM_BYTES;
            I2CInfo[index].regAddress = I2C_REG_ADDRESS;
            I2CInfo[index].regValue = I2C_REG_VALUE;
        }
        memcpy(s_ocwareGlobalData[gStructIndex].data,
            (char *)I2CInfo, sizeof(I2CInfo)*MAX_I2C_COMP_NBR);
    }

    return STUB_SUCCESS;
}

/******************************************************************************
 * Function Name    : ocware_create_command_debug_database
 * Description      : Create entries into the DB for debug subsystem
 *
 * @param : subsystemNbr - subsytem number in the schema (by value)
 *          gStructIndex - output pointer to the index in the DB (by reference)
 *          subSystem - pointer to the debug subsytem in the schema
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_create_command_debug_database(int8_t subsystemNbr,
                                                     int16_t *gStructIndex,
                                                     const Component *subSystem)
{
    ocware_stub_ret ret = STUB_FAILED;
    const Component *component = NULL;
    const Component *subComponent = NULL;
    const Driver *driver = NULL;
    const Command *command = NULL;
    int8_t componentNbr = 1;
    int8_t count = 0;
    int8_t commandCount = 0;
    s_debugSubsystem = subsystemNbr;

    if((gStructIndex == NULL) || (subSystem == NULL)) {
        return STUB_FAILED;
    }
    for(component = subSystem->components; component && component->name;
                                                            component++) {
        /* component loop */
        subComponent = component->components;
        count = 1;
        while (subComponent && subComponent->name) {
            driver = subComponent->driver;
            if(driver !=NULL) {
                /* No Need to read every command as
                 * current implementation is to have one entry
                 * for one subsystem
                 */
                s_ocwareGlobalData[*gStructIndex].subsystemId = subsystemNbr;
                s_ocwareGlobalData[*gStructIndex].componentId = componentNbr;
                s_ocwareGlobalData[*gStructIndex].msgtype =
                                                    OCMP_MSG_TYPE_COMMAND;
                s_ocwareGlobalData[*gStructIndex].paramPos = 0;

                if (strncmp(component->name,"I2C",
                    strlen("I2C")) == 0) {
                    s_ocwareGlobalData[*gStructIndex].paramId = count;
                    s_ocwareGlobalData[*gStructIndex].paramSize
                            = sizeof(OCWareDebugI2Cinfo) * MAX_I2C_COMP_NBR;
                    s_ocwareGlobalData[*gStructIndex].data
                            = malloc(s_ocwareGlobalData[*gStructIndex].paramSize);
                    if (s_ocwareGlobalData[*gStructIndex].data == NULL) {
                        printf("Malloc failed\n");
                        return STUB_FAILED;
                    } else {
                        memset(s_ocwareGlobalData[*gStructIndex].data, 0,
                                s_ocwareGlobalData[*gStructIndex].paramSize);
                    }
                    ret = ocware_stub_fill_init_debug_value(*gStructIndex,
                                OCSTUB_VALUE_TYPE_I2C_DEBUG);
                } else if (strncmp(component->name,"ethernet",
                    strlen("ethernet")) == 0) {
                    s_ocwareGlobalData[*gStructIndex].paramId = count;
                    s_ocwareGlobalData[*gStructIndex].paramSize
                            = sizeof(OCWareDebugMDIOinfo) * MAX_MDIO_COMP_NBR;
                    s_ocwareGlobalData[*gStructIndex].data
                            = malloc(s_ocwareGlobalData[*gStructIndex].paramSize);
                    if (s_ocwareGlobalData[*gStructIndex].data == NULL) {
                        printf("Malloc failed\n");
                        return STUB_FAILED;
                    } else {
                        memset(s_ocwareGlobalData[*gStructIndex].data, 0,
                                s_ocwareGlobalData[*gStructIndex].paramSize);
                    }
                    ret = ocware_stub_fill_init_debug_value(*gStructIndex,
                                OCSTUB_VALUE_TYPE_MDIO_DEBUG);

                } else {
                    /* Currently its all GPIO */
                    s_ocwareGlobalData[*gStructIndex].paramId = count;
                    s_ocwareGlobalData[*gStructIndex].paramSize
                            = sizeof(OCWareDebugGPIOinfo) * MAX_GPIO_COMP_NBR;
                    s_ocwareGlobalData[*gStructIndex].data
                            = malloc(s_ocwareGlobalData[*gStructIndex].paramSize);
                    if (s_ocwareGlobalData[*gStructIndex].data == NULL) {
                        printf("Malloc failed\n");
                        return STUB_FAILED;
                    } else {
                        memset(s_ocwareGlobalData[*gStructIndex].data, 0,
                                s_ocwareGlobalData[*gStructIndex].paramSize);
                    }
                    ret = ocware_stub_fill_init_debug_value(*gStructIndex,
                               OCSTUB_VALUE_TYPE_GPIO_DEBUG);
                }
                (*gStructIndex)++;
                if((debugGetCommand == STUB_FAILED) ||
                        (debugSetCommand == STUB_FAILED)) {
                    command = driver->commands;
                    commandCount = 0;
                    while(command && command->name) {
                        if ((strncmp(command->name, "get", strlen("get"))) ==
                                0) {
                            debugGetCommand = commandCount;
                        } else if ((strncmp(command->name, "set",strlen("set")))
                                == 0) {
                            debugSetCommand = commandCount;
                        }
                        command += 1;
                        commandCount++;
                    }
                }
            }
            subComponent += 1;
            count ++;
        }
        componentNbr++;
    }
    return ret;
}

/******************************************************************************
 * Function Name    : ocware_stub_fill_alert_value
 * Description      : Function to fill default alert value in the DB
 *
 * @param   param - pointer to the parameter field in a particular subsytem and
 *                  component (by value)
 *          gStructIndex - index to the DB (by value)
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 *****************************************************************************/
ocware_stub_ret ocware_stub_fill_alert_value(const Parameter *param,
                                            uint16_t gStructIndex)
{
    const char *paramtype;
    if(param == NULL) {
        return STUB_FAILED;
    }

    paramtype = DATA_TYPE_MAP[param->type];

    if (!strcmp("uint16",paramtype)) {
        strcpy(alertGlobalData[gStructIndex].data, (char *)&s_defInt16Val);
    } else if (!strcmp("int16",paramtype)) {
        strcpy(alertGlobalData[gStructIndex].data, (char*)&s_defInt16Val);
    } else if (!strcmp("uint8",paramtype)) {
        strcpy(alertGlobalData[gStructIndex].data, (char*)&s_defInt8Val);
    } else if (!strcmp("int8",paramtype)) {
        strcpy(alertGlobalData[gStructIndex].data, (char*)&s_defInt8Val);
    } else if (!strcmp("string",paramtype)) {
        strcpy(alertGlobalData[gStructIndex].data, DEFAULT_STRING);
    } else if (!strcmp("enum",paramtype)) {
        strcpy(alertGlobalData[gStructIndex].data, (char*)&s_defEnumVal);
    } else {
        strcpy(alertGlobalData[gStructIndex].data, (char*)&s_defInt8Val);
    }
    return STUB_SUCCESS;
}
/******************************************************************************
 * Function Name    : ocware_stub_create_alert_database
 * Description      : Parse the schema and add alert entries in the DB
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_create_alert_database()
{
    ocware_stub_ret ret = STUB_FAILED;
    int8_t subsystemNbr = 0;
    int8_t componentNbr = 0;
    const Component *component = NULL;
    const Component *subComponent = NULL;
    const Component  *subSystem = NULL;
    const Driver *devDriver = NULL;
    const Parameter *param = NULL;
    int8_t count = 0;
    int8_t alertCount = 0;
    int16_t gStructIndex = 0;
    int16_t paramPos = 0;

    for (subSystem = sys_schema; subSystem && subSystem->name; subSystem++) {
        /* Subsystem loop */
        componentNbr = 1;
        for(component = subSystem->components; component && component->name;
                                                                component++) {
            /* component loop */
            devDriver = component->driver;
            if (devDriver != NULL) {
                /* This is for componenets w/o any sub component */
                count = 0;
                paramPos = 0;
                param = devDriver->alerts;
                /* Alert related parameters */
                while (param && param->name) { /*Parameter loop */
                    alertGlobalData[gStructIndex].subsystemId = subsystemNbr;
                    alertGlobalData[gStructIndex].componentId = componentNbr;
                    alertGlobalData[gStructIndex].paramId
                                       = pow(2, count);
                    alertGlobalData[gStructIndex].paramSize =
                                        ocware_stub_get_paramSize (
                                        DATA_TYPE_MAP[param->type],
                                        OCMP_MSG_TYPE_CONFIG, param->name);
                    alertGlobalData[gStructIndex].msgtype =
                        OCMP_MSG_TYPE_CONFIG;
                    alertGlobalData[gStructIndex].data = malloc(sizeof(char)*
                                    alertGlobalData[gStructIndex].paramSize);
                    if (alertGlobalData[gStructIndex].data == NULL) {
                        printf("Malloc failed\n");
                        return STUB_FAILED;
                    } else {
                        memset(alertGlobalData[gStructIndex].data, 0,
                                alertGlobalData[gStructIndex].paramSize);
                    }
                    ocware_stub_fill_alert_value(param, gStructIndex);
                    paramPos += alertGlobalData[gStructIndex].paramSize;
                    gStructIndex++;
                    param += 1;
                    count = count + 1;
                    allAlertCount++;
                }
            }
            alertCount = 0;
            subComponent = component->components;
            while (subComponent && subComponent->name) {
                devDriver = subComponent->driver;
                if(devDriver == NULL) {
                    subComponent += 1;
                    continue;
                }
                param = devDriver->alerts;
                /* Config related parameters */
                while (param && param->name) { /*Parameter loop */
                    alertGlobalData[gStructIndex].subsystemId = subsystemNbr;
                    alertGlobalData[gStructIndex].componentId = componentNbr;
                    alertGlobalData[gStructIndex].paramId
                                = pow(2, alertCount);
                    alertGlobalData[gStructIndex].paramSize
                                = ocware_stub_get_paramSize (
                                               DATA_TYPE_MAP[param->type],
                                               OCMP_MSG_TYPE_CONFIG, param->name);
                    alertGlobalData[gStructIndex].msgtype = OCMP_MSG_TYPE_CONFIG;
                    alertGlobalData[gStructIndex].data
                                = malloc(alertGlobalData[gStructIndex].paramSize);
                    if (alertGlobalData[gStructIndex].data == NULL) {
                        printf("Malloc failed\n");
                        return STUB_FAILED;
                    } else {
                        memset(alertGlobalData[gStructIndex].data, 0,
                                alertGlobalData[gStructIndex].paramSize);
                    }
                    ocware_stub_fill_alert_value(param, gStructIndex);
                    gStructIndex++;
                    param += 1;
                    alertCount += 1;
                    allAlertCount++;
                }
                subComponent += 1;
            }
            componentNbr++;
        }
        subsystemNbr++;
    }
    return ret;
}

/******************************************************************************
 * Function Name    : ocware_stub_create_post_database
 * Description      : Parse the schema and add post entries
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 *****************************************************************************/
ocware_stub_ret ocware_stub_create_post_database()
{
    int8_t subsystemNbr = 0;
    const Component *component = NULL;
    const Component *subComponent = NULL;
    const Component *subSystem = NULL;
    int8_t count = 1;
    int16_t PostIndex = 0;

    for (subSystem = sys_schema; subSystem && subSystem->name; subSystem++) {
        /* Subsystem loop */
        count = 1;
        for(component = subSystem->components; component && component->name;
                                                            component++) {
            /* component loop */
                subComponent = component->components;
                if ((subComponent == NULL) &&
                        (component->postDisabled != POST_DISABLED)) {
                    s_postGlobalArray[PostIndex].SubsystemId = subsystemNbr;
                    s_postGlobalArray[PostIndex].DeviceNumber = count;
                    if(count < 10) {
                        s_postGlobalArray[PostIndex].Status = count - 1;
                    } else {
                        s_postGlobalArray[PostIndex].Status = 9;
                    }
                    PostIndex++;
                    count++;
                }
                /* If subcomponents exist */
                while (subComponent && subComponent->name) {
                    if (subComponent->postDisabled != POST_DISABLED) {
                        s_postGlobalArray[PostIndex].SubsystemId = subsystemNbr;
                        s_postGlobalArray[PostIndex].DeviceNumber = count;
                        if(count < 10) {
                            s_postGlobalArray[PostIndex].Status = count - 1;
                        } else {
                            s_postGlobalArray[PostIndex].Status = 9;
                        }
                        PostIndex++;
                        count++;
                    }
                    subComponent += 1;
                } /* sub comp loop */
        } /* component loop */
        subsystemNbr++;
    }/*subsystem loop */
    return STUB_SUCCESS;
}

/******************************************************************************
 * Function Name    : ocware_stub_init_post
 * Description      : Calculate the param id for post messages
 *
 * @param   devDriver   - pointer to the post driver
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 *****************************************************************************/
ocware_stub_ret ocware_stub_init_post(const Driver *devDriver)
{
    ocware_stub_ret ret = STUB_FAILED;
    const Post *post = NULL;
    uint16_t count = 0;

    if(devDriver == NULL)
        return ret;

    post = devDriver->post;
    while (post && post->name) {
        if (strncmp(post->name, "results",
                strlen(post->name)) == 0) {
            PostResult = pow(2, count);
        } else if (strncmp(post->name, "enable",
                strlen(post->name)) == 0) {
            PostEnable = pow(2, count);
        } else {
            printf("Post init failed\n");
            return ret;
        }
        count++;
        post++;
    }

    if ((PostResult != STUB_FAILED) &&
        (PostEnable != STUB_FAILED)) {
        ret = STUB_SUCCESS;
    } else {
            printf("Post init failed\n");
    }
    return ret;
}

/******************************************************************************
 * Function Name    : ocware_stub_init_database
 * Description      : Parse the schema and add entries in the DB
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_init_database()
{

    ocware_stub_ret ret = STUB_FAILED;
    int8_t subsystemNbr = 0;
    int8_t componentNbr = 0;
    const Component *component = NULL;
    const Component *subComponent = NULL;
    const Component *subSystem = NULL;
    const Driver *devDriver = NULL;
    const Parameter *param = NULL;
    int8_t count = 0;
    int8_t configCount = 0;
    int8_t statusCount = 0;
    int16_t gStructIndex = 0;
    int16_t paramPos = 0;
    int16_t configParamPos = 0;
    int16_t statusParamPos = 0;

    for (subSystem = sys_schema; subSystem && subSystem->name; subSystem++) {
        /* Subsystem loop */
        componentNbr = 1;
        if (strncmp(subSystem->name, "debug",
                strlen(subSystem->name)) == 0) {
            ocware_create_command_debug_database(subsystemNbr,
                         &gStructIndex, subSystem);
        }
        for(component = subSystem->components; component && component->name;
                                                        component++) {
            /* component loop */
            devDriver = component->driver;
            if (devDriver != NULL) {
                if (devDriver->post != NULL) {
                /* POST is currently designed to be existing in one driver */
                    ret = ocware_stub_init_post(devDriver);

                    if(ret != STUB_SUCCESS)
                        return ret;
                }
                /* This is for componenets w/o any sub component */
                count = 0;
                paramPos = 0;
                param = devDriver->config;
                /* Config related parameters */
                while (param && param->name) { /*Parameter loop */
                    s_ocwareGlobalData[gStructIndex].subsystemId = subsystemNbr;
                    s_ocwareGlobalData[gStructIndex].componentId = componentNbr;
                    s_ocwareGlobalData[gStructIndex].paramId
                                       = pow(2,count);
                    s_ocwareGlobalData[gStructIndex].paramSize =
                                        ocware_stub_get_paramSize (
                                        DATA_TYPE_MAP[param->type],
                                        OCMP_MSG_TYPE_CONFIG, param->name);
                    s_ocwareGlobalData[gStructIndex].msgtype =
                                                        OCMP_MSG_TYPE_CONFIG;
                    s_ocwareGlobalData[gStructIndex].paramPos = paramPos;
                    s_ocwareGlobalData[gStructIndex].data = malloc(sizeof(char)*
                                    s_ocwareGlobalData[gStructIndex].paramSize);
                    if (s_ocwareGlobalData[gStructIndex].data == NULL) {
                        printf("Malloc failed\n");
                        return STUB_FAILED;
                    } else {
                        memset(s_ocwareGlobalData[gStructIndex].data, 0,
                                s_ocwareGlobalData[gStructIndex].paramSize);
                    }
                    ocware_stub_fill_init_value(param, gStructIndex);
                    paramPos += s_ocwareGlobalData[gStructIndex].paramSize;
                    gStructIndex++;
                    param += 1;
                    count = count + 1;
                }
                count = 0;
                paramPos = 0;
                param = devDriver->status;
                /* Status related parameters */
                while (param && param->name) { /* Parameter loop */
                    if ((strncmp(subSystem->name, "obc",
                            strlen(subSystem->name)) == 0) ||
                        (strncmp(subSystem->name, "testmodule",
                            strlen(subSystem->name)) == 0) ||
                        (strncmp(subSystem->name, "system",
                            strlen(subSystem->name)) == 0)) {
                        if (strncmp(param->name, "mfg",
                            strlen(param->name))== 0) {
                            s_typeFlag = OCSTUB_VALUE_TYPE_MFG;
                        } else if (strncmp(param->name,
                            "model", strlen(param->name)) == 0) {
                            if ((strncmp(subSystem->name, "testmodule",
                                strlen(subSystem->name))) == 0) {
                                s_typeFlag = OCSTUB_VALUE_TYPE_GETMODEL;
                            } else {
                                s_typeFlag = OCSTUB_VALUE_TYPE_MODEL;
                            }
                        } else if (strncmp(param->name,
                            "registration", strlen(param->name)) == 0) {
                            s_typeFlag = OCSTUB_VALUE_TYPE_REGISTRATION;
                        } else if(strncmp(param->name, "network_operatorinfo",
                                    strlen(param->name)) == 0) {
                            s_typeFlag =
                                OCSTUB_VALUE_TYPE_NWOP_STRUCT;
                        } else if(strncmp(param->name, "lasterror",
                                    strlen(param->name)) == 0) {
                            s_typeFlag =
                                OCSTUB_VALUE_TYPE_LAST_ERROR;
                        } else if(strncmp(param->name, "ocserialinfo",
                                    strlen(param->name)) == 0) {
                            s_typeFlag =
                                OCSTUB_VALUE_TYPE_OCSERIAL_INFO;
                        } else if(strncmp(param->name, "gbcboardinfo",
                                    strlen(param->name)) == 0) {
                            s_typeFlag =
                                OCSTUB_VALUE_TYPE_GBCBOARD_INFO;
                        } else {
                        }
                    }
                    s_ocwareGlobalData[gStructIndex].subsystemId = subsystemNbr;
                    s_ocwareGlobalData[gStructIndex].componentId = componentNbr;
                    s_ocwareGlobalData[gStructIndex].paramId = pow(2,count);
                    s_ocwareGlobalData[gStructIndex].paramSize =
                                ocware_stub_get_paramSize (
                                            DATA_TYPE_MAP[param->type],
                                            OCMP_MSG_TYPE_STATUS, param->name);
                    s_ocwareGlobalData[gStructIndex].paramPos = paramPos;
                    s_ocwareGlobalData[gStructIndex].msgtype =
                                                        OCMP_MSG_TYPE_STATUS;
                    paramPos += s_ocwareGlobalData[gStructIndex].paramSize;
                    s_ocwareGlobalData[gStructIndex].data = malloc(sizeof(char)*
                                s_ocwareGlobalData[gStructIndex].paramSize);
                    if (s_ocwareGlobalData[gStructIndex].data == NULL) {
                        printf("Malloc failed\n");
                        return STUB_FAILED;
                    } else {
                        memset(s_ocwareGlobalData[gStructIndex].data, 0,
                                s_ocwareGlobalData[gStructIndex].paramSize);
                    }
                    ocware_stub_fill_init_value(param, gStructIndex);
                    if ((strncmp(subSystem->name, "obc",
                        strlen(subSystem->name)) == 0) ||
                        (strncmp(subSystem->name, "testmodule",
                        strlen(subSystem->name)) == 0)) {
                       s_ocwareGlobalData[gStructIndex].paramPos = 0;
                    }
                    gStructIndex++;
                    param += 1;
                    count = count + 1;
                }
            }
            /* If subcomponents exist */
            configCount = 0;
            configParamPos = 0;
            statusCount = 0;
            statusParamPos = 0;
            subComponent = component->components;

            while (subComponent && subComponent->name) {
                devDriver = subComponent->driver;
                if(devDriver == NULL) {
                    subComponent += 1;
                    continue;
                }
                param = devDriver->config;
                /* Config related parameters */
                while (param && param->name) { /*Parameter loop */
                    s_ocwareGlobalData[gStructIndex].subsystemId = subsystemNbr;
                    s_ocwareGlobalData[gStructIndex].componentId = componentNbr;
                    s_ocwareGlobalData[gStructIndex].paramId
                                = pow(2,configCount);
                    s_ocwareGlobalData[gStructIndex].paramSize
                                = ocware_stub_get_paramSize (
                                               DATA_TYPE_MAP[param->type],
                                               OCMP_MSG_TYPE_CONFIG, param->name);
                    s_ocwareGlobalData[gStructIndex].msgtype =
                                                    OCMP_MSG_TYPE_CONFIG;
                    s_ocwareGlobalData[gStructIndex].paramPos = configParamPos;
                    configParamPos += s_ocwareGlobalData[gStructIndex].paramSize;
                    s_ocwareGlobalData[gStructIndex].data
                                = malloc(s_ocwareGlobalData[gStructIndex].paramSize);
                    if (s_ocwareGlobalData[gStructIndex].data == NULL) {
                        printf("Malloc failed\n");
                        return STUB_FAILED;
                    } else {
                        memset(s_ocwareGlobalData[gStructIndex].data, 0,
                                s_ocwareGlobalData[gStructIndex].paramSize);
                    }
                    ocware_stub_fill_init_value(param, gStructIndex);
                    gStructIndex++;
                    param += 1;
                    configCount += 1;
                }
                param = devDriver->status;
                /* Status related parameters */
                while (param && param->name) { /* Parameter loop */
                     s_ocwareGlobalData[gStructIndex].subsystemId = subsystemNbr;
                     s_ocwareGlobalData[gStructIndex].componentId = componentNbr;
                     s_ocwareGlobalData[gStructIndex].paramId
                                    = pow(2,statusCount);
                     s_ocwareGlobalData[gStructIndex].paramSize
                                    = ocware_stub_get_paramSize (
                                            DATA_TYPE_MAP[param->type],
                                            OCMP_MSG_TYPE_STATUS, param->name);
                     s_ocwareGlobalData[gStructIndex].paramPos = statusParamPos;
                     s_ocwareGlobalData[gStructIndex].msgtype =
                                                            OCMP_MSG_TYPE_STATUS;
                     statusParamPos += s_ocwareGlobalData[gStructIndex].paramSize;
                     s_ocwareGlobalData[gStructIndex].data
                                 = malloc(s_ocwareGlobalData[gStructIndex].paramSize);
                     if (s_ocwareGlobalData[gStructIndex].data == NULL) {
                         printf("Malloc failed\n");
                         return STUB_FAILED;
                     } else {
                         memset(s_ocwareGlobalData[gStructIndex].data, 0,
                                s_ocwareGlobalData[gStructIndex].paramSize);
                     }
                     ocware_stub_fill_init_value(param, gStructIndex);
                     gStructIndex++;
                     param += 1;
                     statusCount += 1;
                }
                subComponent += 1;
            }
        componentNbr++;
        }
    subsystemNbr++;
    }
    ret = ocware_stub_create_post_database();
    ret = ocware_stub_create_alert_database();
    return ret;
}

/******************************************************************************
 * Function Name    : ocware_stub_parse_command_message
 * Description      : Parse command messages from MW
 *
 * @param   buffer - output pointer to the message from MW
 *
 * @return STUB_SUCCESS - for success
 *         STUB_FAILED  - for failure
 ******************************************************************************/
ocware_stub_ret ocware_stub_parse_command_message(char *buffer,
                                                        uint8_t *alertFlag)
{
    OCMPMessageFrame *msgFrame = (OCMPMessageFrame *)buffer;
    OCMPMessage *msgFrameData = (OCMPMessage*)&msgFrame->message;
    ocware_stub_ret ret = STUB_FAILED;

    if (buffer == NULL) {
        return ret;
    }

    /*
     * The current logic is to return success for all commands other than
     * the debug subsystem. For debug subsytem the commands are treated
     * similar to the get/set logic used in config msg type
     */
    ret = STUB_SUCCESS;
    if(s_debugSubsystem == msgFrameData->subsystem) {
        ocware_stub_parse_debug_actiontype(msgFrameData);
        ret = ocware_stub_get_set_params(msgFrameData);
    }
    *alertFlag = ocware_stub_parse_command_from_schema(msgFrameData);
    msgFrameData->action = OCMP_AXN_TYPE_REPLY;
    return ret;
}
