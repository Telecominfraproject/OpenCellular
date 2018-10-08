/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/global/OC_CONNECT1.h"
#include "common/inc/ocmp_wrappers/ocmp_se98a.h"
#include "inc/subsystem/hci/hci.h"

SCHEMA_IMPORT OcGpio_Port ec_io;
SCHEMA_IMPORT OcGpio_Port sync_io;
/*****************************************************************************
 *                               SYSTEM CONFIG
 *****************************************************************************/
//LED Temperature sensor
SE98A_Dev led_hci_ts = {
    .cfg =
            {
                    .dev = { .bus = OC_CONNECT1_I2C8,
                             .slave_addr = HCI_LED_TEMP_SENSOR_ADDR },
                    .pin_evt = NULL,
            },
    .obj = {},
};

//LED IO Expander
HciLedCfg led_hci_ioexp = {
    .sx1509_dev[HCI_LED_DRIVER_LEFT] =
            {
                    .bus = OC_CONNECT1_I2C8,
                    .slave_addr = LED_SX1509_LEFT_ADDRESS,
            },
    .sx1509_dev[HCI_LED_DRIVER_RIGHT] =
            {
                    .bus = OC_CONNECT1_I2C8,
                    .slave_addr = LED_SX1509_RIGHT_ADDRESS,
            },
    /* EC_GPIO */
    .pin_ec_gpio = { &ec_io, OC_EC_HCI_LED_RESET },
};

//HCI factory Config
const SE98A_Config fact_led_se98a_cfg = {
    .lowlimit = -20,
    .highlimit = 75,
    .critlimit = 80,
};