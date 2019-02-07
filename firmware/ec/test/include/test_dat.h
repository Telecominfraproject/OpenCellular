/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _TEST_DAT_H
#define _TEST_DAT_H

#include "common/inc/global/Framework.h"
#include "common/inc/global/OC_CONNECT1.h"
#include "common/inc/ocmp_wrappers/ocmp_dat-xxr5a-pp.h"
#include "drivers/GpioPCA9557.h"
#include "drivers/PinGroup.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"
#include "helpers/array.h"
#include "helpers/attribute.h"
#include "inc/devices/dat-xxr5a-pp.h"
#include "inc/subsystem/rffe/rffe_ctrl.h"
#include "include/test_PCA9557.h"
#include <string.h>
#include <ti/sysbios/knl/Task.h>
#include "unity.h"

#define DAT_AATN_VALUE_0 0
#define DAT_AATN_VALUE_1 1
#define DAT_AATN_VALUE_2 2
#define DAT_AATN_VALUE_4 4
#define DAT_AATN_VALUE_8 8
#define DAT_AATN_VALUE_16 16
#define DAT_AATN_VALUE_32 32
#define DAT_AATN_VALUE_41 41
#define DAT_AATN_VALUE_51 51
#define DAT_AATN_VALUE_63 63
#define DAT_AATN_VALUE_64 64
#define DAT_ATTN0_OUTPUT_VALUE_GET 0x00
#define DAT_ATTN1_OUTPUT_VALUE_GET 0x04
#define DAT_ATTN1_OUTPUT_VALUE_SET 0x7E
#define DAT_ATTN2_OUTPUT_VALUE_GET 0x08
#define DAT_ATTN4_OUTPUT_VALUE_GET 0x10
#define DAT_ATTN8_OUTPUT_VALUE_GET 0x20
#define DAT_ATTN16_OUTPUT_VALUE_GET 0x40
#define DAT_ATTN32_OUTPUT_VALUE_GET 0x02
#define DAT_ATTN41_OUTPUT_VALUE_GET 0x26
#define DAT_ATTN51_OUTPUT_VALUE_GET 0x4E
#define DAT_ATTN63_OUTPUT_VALUE_GET 0x7E
#define DAT_ATTN64_OUTPUT_VALUE_GET 0x7E
#define DAT_DEFAULT_VALUE 0xFF
#define DAT_INIT_DIR_CONFIG_VALUE 0x00
#define DAT_INIT_POLARITY_VALUE 0x00
#define DAT_INIT_OUTPUT_VALUE 0x7E
#define DAT_INVALID_PARAM 40
#define RFFE_CHANNEL1_INVALID_SLAVE_ADDR 0x51

typedef enum DatConfig {
    DAT_CONFIG_ATTENUATION = 0,
} DatConfig;

#endif
