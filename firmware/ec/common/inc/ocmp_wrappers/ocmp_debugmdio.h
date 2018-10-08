/**
 * Copyright (c) 2018-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef OCMP_MDIO_H_
#define OCMP_MDIO_H_

#include "common/inc/global/Framework.h"

SCHEMA_IMPORT bool mdio_read(void *driver, void *data);
SCHEMA_IMPORT bool mdio_write(void *driver, void *data);

static const Driver OC_MDIO = {
    .name = "OC_MDIO",
    .argList = (Parameter[]){ { .name = "reg_address", .type = TYPE_UINT16 },
                              { .name = "reg_values", .type = TYPE_UINT16 },
                              {} },
    .commands = (Command[]){ {
                                     .name = "get",
                                     .cb_cmd = mdio_read,
                             },
                             {
                                     .name = "set",
                                     .cb_cmd = mdio_write,
                             },
                             {} },
};

#endif /* INC_DEVICES_OCMP_WRAPPERS_OCMP_MDIO_H_ */
