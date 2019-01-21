/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_debugI2c.h"

S_I2C_Cfg I2C_INVALID_DEV = {
    .bus = DEBUG_I2C_INVALID_BUS,
};

S_OCI2C s_oci2c = { .slaveAddress = DEBUG_I2C_SLAVE_ADDR,
                    .reg_address = DEBUG_I2C_REG_ADDR,
                    .reg_value = DEBUG_I2C_REG_VALUE,
                    .number_of_bytes = DEBUG_I2C_NO_OF_BYTES };

S_OCI2C s_oci2c_invalid = { .slaveAddress = DEBUG_I2C_INVALID_SLAVE_ADDR,
                            .reg_address = DEBUG_I2C_REG_ADDR,
                            .reg_value = DEBUG_I2C_REG_VALUE,
                            .number_of_bytes = DEBUG_I2C_NO_OF_BYTES };

uint8_t DEBUG_I2C_regs[] = {
    [DEBUG_I2C_INTERRUPT_STATUS] = 0x00,
    [DEBUG_I2C_INTERRUPT_MASK] = 0x00,
    [DEBUG_I2C_END] = 0x00,
};
