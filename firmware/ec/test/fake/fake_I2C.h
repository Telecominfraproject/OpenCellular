/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef FAKE_I2C_H_
#define FAKE_I2C_H_

#include <stddef.h>
#include <stdint.h>

typedef enum Fake_I2C_Endianness {
    FAKE_I2C_DEV_LITTLE_ENDIAN = __ORDER_LITTLE_ENDIAN__,
    FAKE_I2C_DEV_BIG_ENDIAN = __ORDER_BIG_ENDIAN__
} Fake_I2C_Endianness;

void fake_I2C_init(void);

void fake_I2C_deinit(void);

/*!
 * Register a simple i2c device - I2C_transfer simply reads/writes to a table
 * in RAM. Only basic bounds checking is performed to prevent overflow
 *
 * @param bus Index of the I2C bus this device is on
 * @param addr 7-bit address of the i2c device
 * @param reg_table Pointer to memory to represent registers
 * @param tbl_size Size of the register table
 * @param reg_size Size of each register in the device
 * @param addr_size Size of register addresses (typically 1-2B)
 */
void fake_I2C_registerDevSimple(unsigned int bus, uint8_t addr, void *reg_table,
                                size_t tbl_size, size_t reg_size,
                                size_t addr_size,
                                Fake_I2C_Endianness endianness);

/*!
 * Remove a previously registered device
 *
 * @param bus Index of the I2C bus this device is on
 * @param addr 7-bit address of the i2c device
 */
void fake_I2C_unregisterDev(unsigned int bus, uint8_t addr);

#endif
