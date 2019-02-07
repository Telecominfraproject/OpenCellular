/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/global/Framework.h"
#include "common/inc/ocmp_wrappers/ocmp_eeprom_cat24c04.h"
#include "inc/common/global_header.h"
#include "inc/devices/eeprom.h"

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>

#include <string.h>

static uint16_t s_alertPointer = 0;
Semaphore_Handle sem;

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
        sem = Semaphore_create(1, NULL, NULL);
        return POST_DEV_CFG_DONE;
    }
    return POST_DEV_CFG_FAIL;
}

void OCMP_alertLog(OCMPMessageFrame *pMsg, void *driver)
{
    Semaphore_pend(sem, BIOS_WAIT_FOREVER);

    Eeprom_Cfg *eeprom = (Eeprom_Cfg *)driver;
    if (s_alertPointer < 64) {
        s_alertPointer++;
    } else {
        return;
    }

    eeprom_enable_write(eeprom);
    for(int i = 0; i <64; i++) {
        eeprom_write(eeprom, (s_alertPointer*64)+i, (uint8_t *)pMsg+i, 1);
    }
    Semaphore_post(sem);

    /* For debugging Purposes */
#if 0
    uint8_t byte[64];
    for(int i = 0; i <64; i++) {
        eeprom_read(eeprom, (s_alertPointer*64)+i, byte+i, 1);
    }
    for(int i = 0; i <64; i++) {
    LOGGER_DEBUG("0x%x  ",byte[i]);
    }
#endif
    return;
}
#if 0
/*
 * The SKU method of turning on powere lines is currently not needed , we will keep it in codebase
 * if similar implementation is needed in future
 */
/*    Logic for the SKU ID stored in the eeprom is as below
 *
 * |  Address  |   Type     |     Value              |
 * --------------------------------------------------
 * |      0    |   LTE/SDR  |    'L'- LTE            |
 * |           |            |    'S' – SDR           |
 *
 * |      1    |     GBC    |    '0' - No GBC        |
 * |                        |    '1' - GBC is present|
 *
 * |      2    |     Band   |     'B'                |
 *
 * |      3    |    value   |   '3' - band 3         |
 * |           |            |   '5' - Band 5         |
 * |           |            |   '28' - band 28       |
 *
 * |      4-7  | Reserved    for future use          |
 *
 */
static ePostCode _probe_sku_id(void *driver, POSTData *postData)
{
    ePostCode returnValue = POST_DEV_NOSTATUS;
    Eeprom_Cfg *eeprom_cfg = (Eeprom_Cfg *)driver;
    Eeprom_Skucfg sku_id;
    /*
     * As of now we are using only band id and GBC presence
     */
    eeprom_read(eeprom_cfg, OC_SKU_BAND_ADDRESS, &sku_id.band_nbr, 1);
    eeprom_read(eeprom_cfg, OC_SKU_GBC_ADDRESS, &sku_id.gbc_presence, 1);

    switch (sku_id.band_nbr) {
        case BAND3:
        case BAND5:
        case BAND28:
            returnValue = eeprom_handle_sku_id(eeprom_cfg, &sku_id);
            break;
        default:
            /* Error Condition */
            returnValue = POST_DEV_CRITICAL_FAULT;
    }
    return returnValue;
}
#endif

const Driver_fxnTable CAT24C04_psu_sid_fxnTable = {
    /* Message handlers */
   .cb_init = _init_eeprom,
   //.cb_probe = _probe_sku_id
};

const Driver_fxnTable CAT24C04_psu_inv_fxnTable= {
    .cb_init = _init_eeprom,
};
