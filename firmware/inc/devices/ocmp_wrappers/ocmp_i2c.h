/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef OCMP_I2C_H_
#define OCMP_I2C_H_

typedef struct __attribute__ ((packed, aligned(1))){
    uint8_t    slaveAddress;
    uint8_t    number_of_bytes;
    uint8_t    reg_address;
    uint16_t   reg_value;
}S_OCI2C;

typedef struct S_I2C_Cfg {
    unsigned int bus;
}S_I2C_Cfg;
extern const Driver OC_I2C;

#endif /* INC_DEVICES_OCMP_WRAPPERS_OCMP_I2C_H_ */
