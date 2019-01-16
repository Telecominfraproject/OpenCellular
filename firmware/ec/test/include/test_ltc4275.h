/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _TEST_LTC4275_H
#define _TEST_LTC4275_H

#include "common/inc/global/Framework.h"
#include "common/inc/ocmp_wrappers/ocmp_ltc4275.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"
#include "inc/devices/ltc4275.h"
#include <string.h>
#include <ti/sysbios/knl/Task.h>
#include "unity.h"

/* ======================== Constants & variables =========================== */
#define LTC4275_DEFAULT_EVT 8
#define LTC4275_INVALID_PARAM_ID 2
#define LTC4275_PD_PWRGD_ALERT 0x60
#define LTC4275_POST_DATA 0xFF
#define LTC4275_PWR_PD_NT2P 0x40
#define POST_DATA_NULL 0
#endif
