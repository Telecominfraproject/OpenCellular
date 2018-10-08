/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef OCMP_TESTMODULE_H_
#define OCMP_TESTMODULE_H_

#include "common/inc/global/Framework.h"

SCHEMA_IMPORT const Driver_fxnTable G510_fxnTable;
SCHEMA_IMPORT bool TestMod_cmdEnable(void *driver, void *params);
SCHEMA_IMPORT bool TestMod_cmdDisable(void *driver, void *params);
SCHEMA_IMPORT bool TestMod_cmdDisconnect(void *driver, void *params);
SCHEMA_IMPORT bool TestMod_cmdConnect(void *driver, void *params);
SCHEMA_IMPORT bool TestMod_cmdSendSms(void *driver, void *params);
SCHEMA_IMPORT bool TestMod_cmdDial(void *driver, void *params);
SCHEMA_IMPORT bool TestMod_cmdAnswer(void *driver, void *params);
SCHEMA_IMPORT bool TestMod_cmdHangup(void *driver, void *params);
SCHEMA_IMPORT bool TestMod_cmdReset(void *driver, void *params);

static const Driver Testmod_G510 = {
    .name = "Fibocom G510",
    .status =
            (Parameter[]){
                    { .name = "imei", .type = TYPE_UINT64 },
                    { .name = "imsi", .type = TYPE_UINT64 },
                    { .name = "mfg", .type = TYPE_STR, .size = 10 },
                    { .name = "model", .type = TYPE_STR, .size = 5 },
                    { .name = "rssi", .type = TYPE_UINT8 },
                    { .name = "ber", .type = TYPE_UINT8 },
                    {
                            .name = "registration",
                            .type = TYPE_ENUM,
                            .values =
                                    (Enum_Map[]){
                                            { 0,
                                              "Not Registered, Not Searching" },
                                            { 1, "Registered, Home Network" },
                                            { 2, "Not Registered, Searching" },
                                            { 3, "Registration Denied" },
                                            { 4, "Status Unknown" },
                                            { 5, "Registered, Roaming" },
                                            {} },
                    },
                    { .name = "network_operatorinfo",
                      .type = TYPE_UINT8,
                      .size = 3 }, /* TODO: this is a complex type */
                    { .name = "cellid", .type = TYPE_UINT32 },
                    { .name = "bsic", .type = TYPE_UINT8 },
                    { .name = "lasterror",
                      .type = TYPE_UINT8,
                      .size = 3 }, /* TODO: this is a complex type */
                    {} },
    .alerts =
            (Parameter[]){
                    {
                            .name = "Call State Changed",
                            .type = TYPE_ENUM,
                            .values = (Enum_Map[]){ { 0, "Ringing" },
                                                    { 1, "Call End" },
                                                    {} },
                    },
                    /* TODO: var len str */
                    { .name = "Incoming SMS", .type = TYPE_STR, .size = 20 },
                    {} },
    .commands =
            (Command[]){ { .name = "disconnect_nw",
                           .cb_cmd = TestMod_cmdDisconnect },
                         { .name = "connect_nw", .cb_cmd = TestMod_cmdConnect },
                         { .name = "send", .cb_cmd = TestMod_cmdSendSms },
                         { .name = "dial", .cb_cmd = TestMod_cmdDial },
                         {
                                 .name = "answer",
                                 .cb_cmd = TestMod_cmdAnswer,
                         },
                         {
                                 .name = "hangup",
                                 .cb_cmd = TestMod_cmdHangup,
                         },
                         {
                                 .name = "enable",
                                 .cb_cmd = TestMod_cmdEnable,
                         },
                         {
                                 .name = "disable",
                                 .cb_cmd = TestMod_cmdDisable,
                         },
                         {} },
    .fxnTable = &G510_fxnTable,
    .payload_fmt_union = true, /* Testmodule breaks serialization pattern :( */
};

#endif /* OCMP_TESTMODULE_H_ */
