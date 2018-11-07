/**
 * Copyright (c) 2018-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef OC_MDIO_H_
#define OC_MDIO_H_

#include <stdint.h>

typedef struct __attribute__((packed, aligned(1))) {
    uint16_t reg_address;
    uint16_t reg_value;
} S_OCMDIO;

typedef struct S_MDIO_Cfg {
    unsigned int port;
} S_MDIO_Cfg;

#endif /* INC_DEVICES_OC_MDIO_H_ */
