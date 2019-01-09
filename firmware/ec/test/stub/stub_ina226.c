/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include <stdint.h>
#include "include/test_ina226.h"

uint16_t ina226_stub_set_currentLimit(uint16_t currentLimt)
{
    uint16_t expCurrentLimt = 0;
    expCurrentLimt = (2048 * (currentLimt / (0.1)) / (0x6400));
    return expCurrentLimt;
}

uint16_t ina226_stub_get_currentLimit(uint16_t currentLimt)
{
    uint16_t expCurrentLimt = 0;
    expCurrentLimt = ((currentLimt * (0.1) * (0x6400)) / 2048);
    return expCurrentLimt;
}

uint16_t ina226_stub_get_busVlotage_status(uint16_t busVoltage)
{
    uint16_t expBusVoltage = 0;
    expBusVoltage = busVoltage * 1.25;
    return expBusVoltage;
}

uint16_t ina226_stub_get_shuntVlotage_status(uint16_t shuntVoltage)
{
    uint16_t expshuntVoltage = 0;
    expshuntVoltage = shuntVoltage * 2.5;
    return expshuntVoltage;
}

uint16_t ina226_stub_get_current_status(uint16_t current)
{
    uint16_t expCurrent = 0;
    expCurrent = current * 0.1;
    return expCurrent;
}

uint16_t ina226_stub_get_power_status(uint16_t power)
{
    uint16_t expPower = 0;
    expPower = power * 2.5;
    return expPower;
}
