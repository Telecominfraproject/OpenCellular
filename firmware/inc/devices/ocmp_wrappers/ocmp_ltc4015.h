/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef _OCMP_LTC4015_H
#define _OCMP_LTC4015_H

typedef struct LTC4015_Config {
    int16_t batteryVoltageLow;
    int16_t batteryVoltageHigh;
    int16_t batteryCurrentLow;
    int16_t inputVoltageLow;
    int16_t inputCurrentHigh;
    uint16_t inputCurrentLimit;
    uint16_t icharge;
    uint16_t vcharge;
} LTC4015_Config;

extern const Driver LTC4015;

#endif /* _OCMP_LTC4015_H */
