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

SCHEMA_IMPORT bool PWR_post_get_results(void **getpostResult);
SCHEMA_IMPORT bool PWR_post_enable(void **postActivate);
SCHEMA_IMPORT const Driver_fxnTable PWRSRC_fxnTable;

static const Driver PWRSRC = {
    .name = "powerSource",
    .status =
        (Parameter[]){
                       { .name = "extPowerAvailability", .type = TYPE_UINT8 },
                       { .name = "extPowerAccessebility", .type = TYPE_UINT8 },
                       { .name = "poeAvailability", .type = TYPE_UINT8 },
                       { .name = "poeAccessebility", .type = TYPE_UINT8 },
                       { .name = "battAvailability", .type = TYPE_UINT8 },
                       { .name = "battAccessebility", .type = TYPE_UINT8 },
                       {}
    },
    .config = (Parameter[]){
        {}
    },
    .alerts = (Parameter[]){
        {}
    },
    .post = (Post[]){
        {
            .name = "results",
            .cb_postCmd = PWR_post_get_results,
        },
        {
            .name = "enable",
            .cb_postCmd = PWR_post_enable,
        },
        {}
    },
    .fxnTable = &PWRSRC_fxnTable,
};
#endif /* _OCMP_POWERSOURCE_H_ */
