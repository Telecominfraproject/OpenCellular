/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef INC_DEVICES_OCMP_EEPROM_H_
#define INC_DEVICES_OCMP_EEPROM_H_

#include "common/inc/global/Framework.h"

SCHEMA_IMPORT const Driver_fxnTable CAT24C04_psu_sid_fxnTable;
SCHEMA_IMPORT const Driver_fxnTable CAT24C04_psu_inv_fxnTable;

static const Driver CAT24C04_psu_sid = {
    .name = "EEPROM",
    .status = (Parameter[]){
        { .name = "ocserialinfo", .type = TYPE_STR, .size = 21 },
        { .name = "gbcboardinfo", .type = TYPE_STR, .size = 21 },
    },
    .fxnTable = &CAT24C04_psu_sid_fxnTable,
};

static const Driver CAT24C04_psu_inv = {
    .name = "Inventory",
    .fxnTable = &CAT24C04_psu_inv_fxnTable,
};

#endif /* INC_DEVICES_OCMP_EEPROM_H_ */
