/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef _OCMP_POWERSOURCE_H_
#define _OCMP_POWERSOURCE_H_

#include "common/inc/global/Framework.h"

SCHEMA_IMPORT const Driver_fxnTable PWRSRC_fxnTable;

static const Driver PWRSRC = {
    .name = "powerSource",
    .status =
            (Parameter[]){
                    { .name = "poeAvailability", .type = TYPE_UINT8 },
                    { .name = "poeAccessebility", .type = TYPE_UINT8 },
                    { .name = "solarAvailability", .type = TYPE_UINT8 },
                    { .name = "solarAccessebility", .type = TYPE_UINT8 },
                    { .name = "extBattAvailability", .type = TYPE_UINT8 },
                    { .name = "extBattAccessebility", .type = TYPE_UINT8 },
                    { .name = "intBattAvailability", .type = TYPE_UINT8 },
                    { .name = "intBattAccessebility", .type = TYPE_UINT8 },
                    {} },
    .fxnTable = &PWRSRC_fxnTable,
};
#endif /* _OCMP_POWERSOURCE_H_ */
