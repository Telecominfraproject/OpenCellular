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

const Driver_fxnTable CAT24C04_psu_sid_fxnTable = {
    /* Message handlers */
   .cb_init = _init_eeprom,
};

const Driver_fxnTable CAT24C04_psu_inv_fxnTable= {
    .cb_init = _init_eeprom,
};
