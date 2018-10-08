/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "i2c.h"

typedef struct I2C_Message {
    uint8_t subAddr;
    uint8_t data[];
} I2C_Message;

bool I2C_write(I2C_Handle handle, uint8_t reg, unsigned char slaveAddress,
               const void *buffer, size_t size)
{
    const size_t msg_size = sizeof(I2C_Message) + size;
    uint8_t data
            [msg_size]; // TODO: should probably use malloc instead of using stack
    I2C_Message *msg = (I2C_Message *)data;
    msg->subAddr = reg;
    memcpy(&msg->data, buffer, size);

    I2C_Transaction i2cTransaction;
    i2cTransaction.slaveAddress = slaveAddress;
    i2cTransaction.writeBuf = msg;
    i2cTransaction.writeCount = msg_size;
    i2cTransaction.readCount = 0;

    return I2C_transfer(handle, &i2cTransaction);
}

bool I2C_read(I2C_Handle handle, uint8_t reg, unsigned char slaveAddress,
              void *buffer, size_t size)
{
    I2C_Transaction i2cTransaction;
    i2cTransaction.slaveAddress = slaveAddress;
    i2cTransaction.writeBuf = &reg;
    i2cTransaction.writeCount = sizeof(reg);
    i2cTransaction.readCount = size;
    i2cTransaction.readBuf = buffer;

    return I2C_transfer(handle, &i2cTransaction);
}
