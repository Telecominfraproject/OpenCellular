/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef COMMON_INC_OCMP_WRAPPERS_OCMP_MAC_H_
#define COMMON_INC_OCMP_WRAPPERS_OCMP_MAC_H_

#include "common/inc/global/Framework.h"

#define OC_MAC_ADDRESS_SIZE 13

SCHEMA_IMPORT const Driver_fxnTable MAC_fxnTable;

static const Driver Driver_MAC = {
    .name = "MAC",
    .config = (Parameter[]){ { .name = "address",
                               .type = TYPE_STR,
                               .size = OC_MAC_ADDRESS_SIZE + 1 } },
    .fxnTable = &MAC_fxnTable,
};

#endif /* COMMON_INC_OCMP_WRAPPERS_OCMP_MAC_H_ */
