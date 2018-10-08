/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef OCMP_IRIDIUM_H_
#define OCMP_IRIDIUM_H_

#include "common/inc/global/Framework.h"

SCHEMA_IMPORT const Driver_fxnTable OBC_fxnTable;
SCHEMA_IMPORT bool IRIDIUM_reset(void *driver, void *params);

static const Driver OBC_Iridium = {
    .name = "Iridium 96xx",
    .status =
            (Parameter[]){
                    { .name = "imei", .type = TYPE_UINT64 },
                    { .name = "mfg", .type = TYPE_STR, .size = 10 },
                    { .name = "model", .type = TYPE_STR, .size = 4 },
                    { .name = "signal_quality", .type = TYPE_UINT8 },
                    {
                            .name = "registration",
                            .type = TYPE_ENUM,
                            .values =
                                    (Enum_Map[]){ { 0, "Detached" },
                                                  { 1, "None" },
                                                  { 2, "Registered" },
                                                  { 3, "Registration Denied" },
                                                  {} },
                    },
                    { .name = "numberofoutgoingmessage", .type = TYPE_UINT8 },
                    { .name = "lasterror",
                      .type = TYPE_UINT8,
                      .size = 3 }, /* TODO: this is a complex type */
                    {} },
    .commands = (Command[]){ {
                                     .name = "reset",
                                     .cb_cmd = IRIDIUM_reset,
                             },
                             {} },
    .fxnTable = &OBC_fxnTable,
    .payload_fmt_union = true, /* OBC breaks serialization pattern :( */
};

#endif /* OCMP_IRIDIUM_H_ */
