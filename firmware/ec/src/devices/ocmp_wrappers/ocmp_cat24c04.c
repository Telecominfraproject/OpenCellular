/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/ocmp_wrappers/ocmp_eeprom_cat24c04.h"

#include "common/inc/global/Framework.h"
#include "inc/common/global_header.h"
#include "inc/devices/eeprom.h"

#include <string.h>

static bool _sid_get_status_parameters_data(void *driver, unsigned int param,
                                            void *data);
/*****************************************************************************
 **    FUNCTION NAME   : _sid_get_status_parameters_data
 **
 **    DESCRIPTION     : Reading Serial Id's from the GBC EEPROM.
 **
 **    ARGUMENTS       : Pointer to OCMPMessageFrame structure
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
static bool _sid_get_status_parameters_data(void *driver, unsigned int param,
                                            void *data)
{
    const eOCStatusParamId paramIndex = (eOCStatusParamId)param;
    const Eeprom_Cfg *cfg = (Eeprom_Cfg *)driver;
    ReturnStatus status = RETURN_OK;
    switch (paramIndex) {
        case OC_STAT_SYS_SERIAL_ID: {
            memset(data, '\0', OC_CONNECT1_SERIAL_SIZE + 1);
            status = eeprom_read_oc_info(data);
            LOGGER_DEBUG("SYS:INFO:::: OC Connect1 serial id %s.\n", data);
            break;
        }
        case OC_STAT_SYS_GBC_BOARD_ID: {
            memset(data, '\0', OC_GBC_BOARD_INFO_SIZE + 1);
            status = eeprom_read_board_info(cfg, data);
            LOGGER_DEBUG("SYS:INFO:::: OC Connect1 GBC board is  %s.\n", data);
            break;
        }
        default: {
            status = RETURN_NOTOK;
        }
    }
    return (status == RETURN_OK);
}

/*****************************************************************************
 **    FUNCTION NAME   : Sdr_InventoryGetStatus
 **
 **    DESCRIPTION     : Reading Serial Id's from the SDR EEPROM.
 **
 **    ARGUMENTS       : Pointer to OCMPMessageFrame structure
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
bool Sdr_InventoryGetStatus(void *driver, unsigned int param_id,
                            void *return_buf)
{
    const Eeprom_Cfg *cfg = (Eeprom_Cfg *)driver;
    switch (param_id) {
        case 0: /* TODO: gross magic number */
            memset(return_buf, '\0', OC_SDR_BOARD_INFO_SIZE + 1);
            if (eeprom_read_board_info(cfg, return_buf) == RETURN_OK) {
                return true;
            }
            LOGGER_DEBUG("SDR:INFO:: Board id: %s\n", return_buf);
            break;
        default:
            LOGGER_ERROR("SDR:ERROR::Unknown param %u\n", param_id);
            break;
    }
    return false;
}
/*****************************************************************************
 **    FUNCTION NAME   : RFFE_InventoryGetStatus
 **
 **    DESCRIPTION     : Reading Serial Id's from the RFFE EEPROM.
 **
 **    ARGUMENTS       : Pointer to OCMPMessageFrame structure
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
bool RFFE_InventoryGetStatus(void *driver, unsigned int param_id,
                             void *return_buf)
{
    const Eeprom_Cfg *cfg = (Eeprom_Cfg *)driver;
    switch (param_id) {
        case 0: /* TODO: gross magic number */
            memset(return_buf, '\0', OC_RFFE_BOARD_INFO_SIZE + 1);
            if (eeprom_read_board_info(cfg, return_buf) == RETURN_OK) {
                return true;
            }
            LOGGER_DEBUG("RFFE:INFO:: Board id: %s\n", return_buf);
            break;
        default:
            LOGGER_ERROR("RFFE:ERROR::Unknown param %u\n", param_id);
            break;
    }
    return false;
}

static ePostCode _init_eeprom(void *driver, const void **config,
                              const void *alert_token)
{
    Eeprom_Cfg *eeprom = (Eeprom_Cfg *)driver;
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

const Driver_fxnTable CAT24C04_gbc_sid_fxnTable = {
    /* Message handlers */
    .cb_init = _init_eeprom,
    .cb_get_status = _sid_get_status_parameters_data,
};

const Driver_fxnTable CAT24C04_gbc_inv_fxnTable = {
    .cb_init = _init_eeprom,
};

const Driver_fxnTable CAT24C04_sdr_inv_fxnTable = {
    /* Message handlers */
    .cb_init = _init_eeprom,
    .cb_get_status = Sdr_InventoryGetStatus,
};

const Driver_fxnTable CAT24C04_fe_inv_fxnTable = {
    /* Message handlers */
    .cb_init = _init_eeprom,
    .cb_get_status = RFFE_InventoryGetStatus,
};
