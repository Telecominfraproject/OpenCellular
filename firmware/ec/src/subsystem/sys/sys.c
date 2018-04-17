/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "inc/subsystem/sys/sys.h"

#include "inc/devices/eeprom.h"
#include "inc/subsystem/gpp/gpp.h" /* For resetting AP */
#include "src/registry/Framework.h"

#include <driverlib/flash.h>
#include <driverlib/sysctl.h>

#include <stdio.h>
#include <string.h>

#define OC_MAC_ADDRESS_SIZE     13

typedef enum {
    OC_SYS_CONF_MAC_ADDRESS = 0
} eOCConfigParamId;

typedef enum {
    OC_STAT_SYS_SERIAL_ID = 0,
    OC_STAT_SYS_GBC_BOARD_ID,
    OC_STAT_SYS_STATE
} eOCStatusParamId;

OCSubsystem ssSystem = {
    .taskStackSize = 1024,
    .taskPriority = 4,
};

/* Driver definitions for the various devices within SYS */
static ePostCode _init_eeprom(void *driver, const void *config,
                              const void *alert_token);
static bool _sid_get_status_parameters_data(void *drvier,
                                            unsigned int param,
                                            void *data);
static bool _mac_get_config_parameters_data(void *driver,
                                            unsigned int param,
                                            void *pOCConfigData);
static bool _mac_set_config_parameters_data(void *driver,
                                            unsigned int param,
                                            const void *pOCConfigData);

const Driver Driver_EepromSID = {
    .name = "EEPROM",
    .status = (Parameter[]){
        { .name = "gbcboardinfo", .type = TYPE_STR, .size = 21 },
        { .name = "ocserialinfo", .type = TYPE_STR, .size = 21 },
    },
    .cb_init = _init_eeprom,
    .cb_get_status = _sid_get_status_parameters_data,
};

const Driver Driver_EepromInv = {
    .name = "EEPROM",
    .cb_init = _init_eeprom
};

const Driver Driver_MAC = {
    .name = "MAC",
    .config = (Parameter[]){
        { .name = "address", .type = TYPE_STR,
          .size = OC_MAC_ADDRESS_SIZE + 1 }
    },
    .cb_get_config = _mac_get_config_parameters_data,
    .cb_set_config = _mac_set_config_parameters_data,
};

/* Intitialize an EEPROM device and test writing to the very end of memory */
static ePostCode _init_eeprom(void *driver, const void *config,
                              const void *alert_token)
{
    Eeprom_Cfg *eeprom = driver;
    uint8_t write = 0x01;
    uint8_t read = 0x00;

    eeprom_init(eeprom);
    eeprom_enable_write(eeprom);
    eeprom_write(eeprom, OC_TEST_ADDRESS, &write, 1);
    NOP_DELAY(); /* TODO: the eeprom driver should handle this */
    eeprom_disable_write(eeprom);
    eeprom_read(eeprom, OC_TEST_ADDRESS, &read, 1);

    if (write == read) {
        return POST_DEV_CFG_DONE;
    }
    return POST_DEV_CFG_FAIL;
}

/*****************************************************************************
 **    FUNCTION NAME   : bb_get_mac_address
 **
 **    DESCRIPTION     : Get EC MAC address.
 **
 **    ARGUMENTS       : Pointer to OCMPMessageFrame structure
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
static ReturnStatus bb_get_mac_address(uint8_t *macAddress)
{
    uint32_t ulUser0 = 0, ulUser1 = 0;

    /* Get the MAC address */
    if((FlashUserGet(&ulUser0, &ulUser1)) != 0) {
        return RETURN_NOTOK;
    }
    uint8_t i = 0;
    uint8_t temp[6] = {'\0'};

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

        for( i = 0; i < 6; i++ )
        {
            sprintf((char *)&macAddress[i*2], "%X", ((temp[i]&0xf0) >> 4));
            sprintf((char *)&macAddress[(i*2)+1], "%X", temp[i]&0xf);
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
 **    FUNCTION NAME   : bb_set_mac_address
 **
 **    DESCRIPTION     : Set EC MAC address.
 **
 **    ARGUMENTS       : Pointer to OCMPMessageFrame structure
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
ReturnStatus bb_set_mac_address(const uint8_t *macAddress)
{
    uint32_t ulUser0, ulUser1;
    if(macAddress != NULL) {
        char temp[6];
        strncpy(temp, (const char *)macAddress, 6);
        ulUser0 = str_to_val(temp);
        strncpy(temp, (const char *)(macAddress + 6), 6);
        ulUser1 = str_to_val(temp);
        /* Set the MAC address */
        if((FlashUserSet(ulUser0, ulUser1)) != 0) {
            return RETURN_NOTOK;
        } else {
            if(FlashUserSave() != 0) {
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
 **    ARGUMENTS       : Pointer to OCMPMessageFrame structure
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
static bool _mac_get_config_parameters_data(void *driver,
                                            unsigned int param,
                                            void *pOCConfigData)
{
    const eOCConfigParamId paramIndex = (eOCConfigParamId)param;
    ReturnStatus status = RETURN_OK;
    switch (paramIndex) {
        case OC_SYS_CONF_MAC_ADDRESS:
        {
            memset(pOCConfigData, '\0', OC_MAC_ADDRESS_SIZE + 1);
            status = bb_get_mac_address(pOCConfigData);
            LOGGER_DEBUG("SYS:INFO:: OC Connect1 MAC Address: %s.\n",
                         pOCConfigData);
            break;
        }
        default:
        {
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
 **    ARGUMENTS       : Pointer to OCMPMessageFrame structure
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
static bool _mac_set_config_parameters_data(void *driver,
                                            unsigned int param,
                                            const void *pOCConfigData)
{
    const eOCConfigParamId paramIndex = (eOCConfigParamId)param;
    ReturnStatus status = RETURN_OK;
    switch (paramIndex) {
        case OC_SYS_CONF_MAC_ADDRESS:
        {
            LOGGER_DEBUG("SYS:INFO:: Set OC Connect1 MAC Address to: %s.\n",
                         pOCConfigData);
            bb_set_mac_address(pOCConfigData);
            break;
        }
        default:
        {
            status = RETURN_NOTOK;
        }
    }
    return (status == RETURN_OK);
}

/*****************************************************************************
 **    FUNCTION NAME   : bb_process_get_status_parameters_data
 **
 **    DESCRIPTION     : Get OC Status Message.
 **
 **    ARGUMENTS       : Pointer to OCMPMessageFrame structure
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
static bool _sid_get_status_parameters_data(void *drvier,
                                            unsigned int param,
                                            void *data)
{
    const eOCStatusParamId paramIndex = (eOCStatusParamId)param;
    ReturnStatus status = RETURN_OK;
    switch (paramIndex) {
        case OC_STAT_SYS_SERIAL_ID:
        {
            memset(data, '\0', OC_CONNECT1_SERIAL_SIZE + 1);
            status = eeprom_read_oc_info(data);
            LOGGER_DEBUG("SYS:INFO:::: OC Connect1 serial id %s.\n",
                         data);
            break;
        }
        case OC_STAT_SYS_GBC_BOARD_ID:
        {
            memset(data, '\0', OC_GBC_BOARD_INFO_SIZE + 1);
            status = eeprom_read_board_info(OC_SS_SYS, data);
            LOGGER_DEBUG("SYS:INFO:::: OC Connect1 GBC board is  %s.\n",
                         data);
            break;
        }
        default:
        {
            status = RETURN_NOTOK;
        }
    }
    return (status == RETURN_OK);
}

/* Resets the AP and then the EC */
bool SYS_cmdReset(void *driver, void *params)
{
    /* TODO: we don't give any indication that the message was received, perhaps
     * a timed shutdown would be more appropriate */

    const Gpp_Cfg *cfg = driver;

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
