/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#include "common/inc/ocmp_wrappers/ocmp_mac.h"

#include "inc/common/global_header.h"

#include <stdio.h>
#include <string.h>

typedef enum { OC_SYS_CONF_MAC_ADDRESS = 0 } eOCConfigParamId;

/*****************************************************************************
 **    FUNCTION NAME   : _get_mac_address
 **
 **    DESCRIPTION     : Get EC MAC address.
 **
 **    ARGUMENTS       : Pointer MAC address.
 **
 **    RETURN TYPE     : true on Success and false on Failure
 **
 *****************************************************************************/
static ReturnStatus _get_mac_address(uint8_t *macAddress)
{
    uint32_t ulUser0 = 0, ulUser1 = 0;

    /* Get the MAC address */
    if ((FlashUserGet(&ulUser0, &ulUser1)) != 0) {
        return RETURN_NOTOK;
    }
    uint8_t i = 0;
    uint8_t temp[6] = { '\0' };

    if ((ulUser0 != 0xffffffff) && (ulUser1 != 0xffffffff)) {
        /*
         *  Convert the 24/24 split MAC address from NV ram into a 32/16 split
         *  MAC address needed to program the hardware registers, then program
         *  the MAC address into the Ethernet Controller registers.
         */
        temp[0] = ((ulUser0 >> 0) & 0xff);
        temp[1] = ((ulUser0 >> 8) & 0xff);
        temp[2] = ((ulUser0 >> 16) & 0xff);
        temp[3] = ((ulUser1 >> 0) & 0xff);
        temp[4] = ((ulUser1 >> 8) & 0xff);
        temp[5] = ((ulUser1 >> 16) & 0xff);

        for (i = 0; i < 6; i++) {
            sprintf((char *)&macAddress[i * 2], "%X", ((temp[i] & 0xf0) >> 4));
            sprintf((char *)&macAddress[(i * 2) + 1], "%X", temp[i] & 0xf);
        }
    } else {
        strncpy((char *)macAddress, "FFFFFFFFFFFF", 12);
    }

    return RETURN_OK;
}

static uint32_t str_to_val(const char *str)
{
    uint32_t value = 0;
    uint8_t temp;
    char *ptr;
    value = strtol(str, &ptr, 16);
    temp = (value & 0XFF0000) >> 16;
    value = (value & 0X00FF00) | ((value & 0X0000FF) << 16);
    value = value | temp;
    return value;
}

/*****************************************************************************
 **    FUNCTION NAME   : _set_mac_address
 **
 **    DESCRIPTION     : Set EC MAC address.
 **
 **    ARGUMENTS       : Pointer to MAC address
 **
 **    RETURN TYPE     : true on Success and false on Failure
 **
 *****************************************************************************/
ReturnStatus _set_mac_address(const uint8_t *macAddress)
{
    uint32_t ulUser0, ulUser1;
    if (macAddress != NULL) {
        char temp[6];
        strncpy(temp, (const char *)macAddress, 6);
        ulUser0 = str_to_val(temp);
        strncpy(temp, (const char *)(macAddress + 6), 6);
        ulUser1 = str_to_val(temp);
        /* Set the MAC address */
        if ((FlashUserSet(ulUser0, ulUser1)) != 0) {
            return RETURN_NOTOK;
        } else {
            if (FlashUserSave() != 0) {
                return RETURN_NOTOK;
            }
        }
        /*SysCtlDelay(2000000);
        SysCtlReset();*/
    }

    return RETURN_OK;
}

/*****************************************************************************
 **    FUNCTION NAME   : mac_get_config_parameters_data
 **
 **    DESCRIPTION     : Get OC Config Message.
 **
 **    ARGUMENTS       : pointer to Driver config, Parameter info
 **                      and pointer to MAC Address.
 **
 **    RETURN TYPE     : true on Success and false on Failure
 **
 *****************************************************************************/
static bool _mac_get_config_parameters_data(void **driver, unsigned int param,
                                            void *pOCConfigData)
{
    const eOCConfigParamId paramIndex = (eOCConfigParamId)param;
    ReturnStatus status = RETURN_OK;
    switch (paramIndex) {
        case OC_SYS_CONF_MAC_ADDRESS: {
            memset(pOCConfigData, '\0', OC_MAC_ADDRESS_SIZE + 1);
            status = _get_mac_address(pOCConfigData);
            LOGGER_DEBUG("SYS:INFO:: OC Connect1 MAC Address: %s.\n",
                         pOCConfigData);
            break;
        }
        default: {
            status = RETURN_NOTOK;
        }
    }
    return (status == RETURN_OK);
}

/*****************************************************************************
 **    FUNCTION NAME   : mac_set_config_parameters_data
 **
 **    DESCRIPTION     : Set OC Config Message.
 **
 **    ARGUMENTS       : pointer to Driver config, Parameter info
 **                      and pointer to MAC Address.
 **
 **    RETURN TYPE     : true on Success and false on Failure
 **
 *****************************************************************************/
static bool _mac_set_config_parameters_data(void **driver, unsigned int param,
                                            const void *pOCConfigData)
{
    const eOCConfigParamId paramIndex = (eOCConfigParamId)param;
    ReturnStatus status = RETURN_OK;
    switch (paramIndex) {
        case OC_SYS_CONF_MAC_ADDRESS: {
            LOGGER_DEBUG("SYS:INFO:: Set OC Connect1 MAC Address to: %s.\n",
                         pOCConfigData);
            _set_mac_address(pOCConfigData);
            break;
        }
        default: {
            status = RETURN_NOTOK;
        }
    }
    return (status == RETURN_OK);
}

static ePostCode _probe_mac(void *driver, const void *config,
                            const void *alert_token)
{
    uint8_t macAddress[14];
    uint32_t ulUser0 = 0, ulUser1 = 0;

    /* Get the MAC address */
    if ((FlashUserGet(&ulUser0, &ulUser1)) != 0) {
        return POST_DEV_MISSING;
    }
    uint8_t i = 0;
    uint8_t temp[6] = { '\0' };

    if ((ulUser0 != 0xffffffff) && (ulUser1 != 0xffffffff)) {
        /*
             *  Convert the 24/24 split MAC address from NV ram into a 32/16 split
             *  MAC address needed to program the hardware registers, then program
             *  the MAC address into the Ethernet Controller registers.
             */
        temp[0] = ((ulUser0 >> 0) & 0xff);
        temp[1] = ((ulUser0 >> 8) & 0xff);
        temp[2] = ((ulUser0 >> 16) & 0xff);
        temp[3] = ((ulUser1 >> 0) & 0xff);
        temp[4] = ((ulUser1 >> 8) & 0xff);
        temp[5] = ((ulUser1 >> 16) & 0xff);

        for (i = 0; i < 6; i++) {
            sprintf((char *)&macAddress[i * 2], "%X", ((temp[i] & 0xf0) >> 4));
            sprintf((char *)&macAddress[(i * 2) + 1], "%X", temp[i] & 0xf);
        }
    } else {
        strncpy((char *)macAddress, "FFFFFFFFFFFF", 12);
        return POST_DEV_MISSING;
    }
    return POST_DEV_FOUND;
}

/* Dummy Initialize for MAC */
static ePostCode _init_mac(void *driver, const void *config,
                           const void *alert_token)
{
    return POST_DEV_NO_CFG_REQ;
}

const Driver_fxnTable MAC_fxnTable = {
    .cb_probe = _probe_mac,
    .cb_init = _init_mac,
    .cb_get_config = _mac_get_config_parameters_data,
    .cb_set_config = _mac_set_config_parameters_data,
};
