/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#pragma once

#ifndef HELPERS_I2C_H_
#define HELPERS_I2C_H_

#include <ti/drivers/I2C.h>

bool I2C_write(I2C_Handle handle, uint8_t reg, unsigned char slaveAddress,
               const void *buffer, size_t size);

bool I2C_read(I2C_Handle handle, uint8_t reg, unsigned char slaveAddress,
              void *buffer, size_t size);

#endif /* HELPERS_I2C_H_ */
