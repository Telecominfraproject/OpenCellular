/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_se98a.h"
#include <stdint.h>

int16_t ocmp_se98a_set_temp_limit(int8_t tempLimitValue)
{
    uint16_t expTempLimit = 0;
    expTempLimit = ((int16_t)tempLimitValue & 0x00FF) << 4;
    if (tempLimitValue < 0) {
        expTempLimit |= 0x1000;
    }
    return expTempLimit;
}

int16_t ocmp_se98a_get_temp_value(int16_t statusVal)
{
    int8_t retTempStatus = 0;
    int16_t expTempStatus = 0;
    int16_t tempTempStatus = statusVal;

    expTempStatus = (tempTempStatus & 0x0FFC);
    if (tempTempStatus & 0x1000) {
        expTempStatus |= 0xF000;
    }

    expTempStatus = round(expTempStatus / 16.0f);
    retTempStatus = CONSTRAIN(expTempStatus, INT8_MIN, INT8_MAX);
    return retTempStatus;
}

uint8_t ocmp_se98a_dev_id(uint16_t devId)
{
    uint8_t expDevId = 0;
    expDevId = ((uint8_t)((devId) >> 8));
    return expDevId;
}
