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

#include "common/inc/global/Framework.h"

SCHEMA_IMPORT bool i2c_read(void *driver, void *data);
SCHEMA_IMPORT bool i2c_write(void *driver, void *data);

static const Driver OC_I2C = {
    .name = "OC_I2C",
    .argList = (Parameter[]){ { .name = "slave_address", .type = TYPE_UINT8 },
                              { .name = "no_of_bytes", .type = TYPE_UINT8 },
                              { .name = "reg_address", .type = TYPE_UINT8 },
                              { .name = "reg_values", .type = TYPE_UINT16 },
                              {} },
    .commands = (Command[]){ {
                                     .name = "get",
                                     .cb_cmd = i2c_read,
                             },
                             {
                                     .name = "set",
                                     .cb_cmd = i2c_write,
                             },
                             {} },
};

#endif /* INC_DEVICES_OCMP_WRAPPERS_OCMP_I2C_H_ */
