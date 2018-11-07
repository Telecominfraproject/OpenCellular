/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef I2CBUS_H_
#define I2CBUS_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "inc/common/global_header.h"

#include <ti/drivers/I2C.h>

/*****************************************************************************
 *                              STRUCT DEFINITIONS
 *****************************************************************************/
typedef struct I2C_Dev {
    unsigned int bus;
    uint8_t slave_addr;
} I2C_Dev;

/*****************************************************************************
 *                             FUNCTION DECLARATIONS
 *****************************************************************************/
I2C_Handle i2c_open_bus(unsigned int index);

/* Wrapper to ease migration */
#define i2c_get_handle i2c_open_bus

void i2c_close_bus(I2C_Handle *i2cHandle);
ReturnStatus i2c_reg_write(I2C_Handle i2cHandle, uint8_t deviceAddress,
                           uint8_t regAddress, uint16_t value,
                           uint8_t numofBytes);
ReturnStatus i2c_reg_read(I2C_Handle i2cHandle, uint8_t deviceAddress,
                          uint8_t regAddress, uint16_t *value,
                          uint8_t numofBytes);

#endif /* I2CBUS_H_ */
