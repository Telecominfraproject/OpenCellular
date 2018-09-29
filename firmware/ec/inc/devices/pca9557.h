/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef PCA9557_H_
#define PCA9557_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "inc/common/global_header.h"
#include "inc/common/i2cbus.h"

/*****************************************************************************
 *                             FUNCTION DECLARATIONS
 *****************************************************************************/
ReturnStatus PCA9557_getInput(const I2C_Dev *i2c_dev, uint8_t *inputRegValue);

ReturnStatus PCA9557_getOutput(const I2C_Dev *i2c_dev, uint8_t *outputRegValue);

ReturnStatus PCA9557_setOutput(const I2C_Dev *i2c_dev, uint8_t outputRegValue);

ReturnStatus PCA9557_getConfig(const I2C_Dev *i2c_dev, uint8_t *configValue);

ReturnStatus PCA9557_setConfig(const I2C_Dev *i2c_dev, uint8_t configValue);

ReturnStatus PCA9557_getPolarity(const I2C_Dev *i2c_dev, uint8_t *polRegValue);

ReturnStatus PCA9557_setPolarity(const I2C_Dev *i2c_dev, uint8_t polRegValue);

#endif /* PCA9557_H_ */
