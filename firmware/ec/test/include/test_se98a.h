/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _TEST_SE98A_H
#define _TEST_SE98A_H

#include "common/inc/global/Framework.h"
#include "common/inc/ocmp_wrappers/ocmp_se98a.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"
#include "inc/common/byteorder.h"
#include "inc/devices/se98a.h"
#include "src/helpers/math.h"
#include <stdbool.h>
#include <string.h>
#include <ti/sysbios/knl/Task.h>
#include "unity.h"

/* ======================== Constants & variables =========================== */
#define AP_COMPONENET 2
#define GPP_SUBSYSTEM 6
#define GPP_TEMP_SENS_DEVICE_ID 0
#define POST_DATA_NULL 0x0000
#define SE98A_CFG_EOCTL 8
#define SE98A_DEFAULT_ACTION 4
#define SE98A_DEFAULT_INIT_VALUE 0
#define SE98A_DEFAULT_TEMP 23
#define SE98A_DEVICE_ID 0xA102
#define SE98A_EVT_DEFAULT 1 << 3
#define SE98A_INVALID_DEVICE_ID 0xFACE
#define SE98A_INVALID_MFG_ID 0xABCD
#define SE98A_MFG_ID 0x1131

typedef struct Test_AlertData {
    bool triggered;
    SE98A_Event evt;
    int8_t temp;
    void *ctx;
} s_alert_data;

typedef enum SE98ARegs {
    SE98A_REG_CAPABILITY = 0,
    SE98A_REG_CONFIG,
    SE98A_REG_HIGH_LIMIT,
    SE98A_REG_LOW_LIMIT,
    SE98A_REG_CRITICAL_LIMIT,
    SE98A_REG_MEASURED_TEMP,
    SE98A_REG_MFG_ID,
    SE98A_REG_DEVICE_ID,
} SE98ARegs;

int16_t ocmp_se98a_set_temp_limit(int8_t tempLimitValue);
int16_t ocmp_se98a_get_temp_value(int16_t statusVal);
uint8_t ocmp_se98a_dev_id(uint16_t devId);
#endif
