/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef OCMP_ETH_SW_H_
#define OCMP_ETH_SW_H_
#include "common/inc/global/Framework.h"

SCHEMA_IMPORT const Driver_fxnTable eth_fxnTable;
SCHEMA_IMPORT bool ETHERNET_reset(void *driver, void *params);
SCHEMA_IMPORT bool ETHERNET_enLoopBk(void *driver, void *params);
SCHEMA_IMPORT bool ETHERNET_disLoopBk(void *driver, void *params);
SCHEMA_IMPORT bool ETHERNET_enPktGen(void *driver, void *params);
SCHEMA_IMPORT bool ETHERNET_disPktGen(void *driver, void *params);
SCHEMA_IMPORT bool ETHERNET_tivaClient(void *driver, void *params);

static const Driver ETH_SW = {
    .name = "Marvel_88E6071",
    .status = (Parameter[]){ { .name = "speed", .type = TYPE_UINT8 },
                             { .name = "duplex", .type = TYPE_UINT8 },
                             { .name = "autoneg_on", .type = TYPE_UINT8 },
                             { .name = "sleep_mode_en", .type = TYPE_UINT8 },
                             { .name = "autoneg_complete", .type = TYPE_UINT8 },
                             { .name = "link_up", .type = TYPE_UINT8 },
                             {} },
    .config = (Parameter[]){ { .name = "speed", .type = TYPE_UINT8 },
                             { .name = "duplex", .type = TYPE_UINT8 },
                             { .name = "powerDown", .type = TYPE_UINT8 },
                             { .name = "enable_sleepMode", .type = TYPE_UINT8 },
                             { .name = "enable_interrupt", .type = TYPE_UINT8 },
                             { .name = "switch_reset", .type = TYPE_UINT8 },
                             { .name = "restart_autoneg", .type = TYPE_UINT8 },
                             {} },
    .alerts = (Parameter[]){ { .name = "speed", .type = TYPE_UINT8 },
                             { .name = "duplex", .type = TYPE_UINT8 },
                             { .name = "autoneg_complete", .type = TYPE_UINT8 },
                             { .name = "crossover_det", .type = TYPE_UINT8 },
                             { .name = "energy_det", .type = TYPE_UINT8 },
                             { .name = "polarity_change", .type = TYPE_UINT8 },
                             { .name = "jabber_det", .type = TYPE_UINT8 },
                             {} },
    .commands = (Command[]){ {
                                     .name = "reset",
                                     .cb_cmd = ETHERNET_reset,
                             },
                             {
                                     .name = "en_loopBk",
                                     .cb_cmd = ETHERNET_enLoopBk,
                             },
                             {
                                     .name = "dis_loopBk",
                                     .cb_cmd = ETHERNET_disLoopBk,
                             },
                             {
                                     .name = "en_pktGen",
                                     .cb_cmd = ETHERNET_enPktGen,
                             },
                             {
                                     .name = "dis_pktGen",
                                     .cb_cmd = ETHERNET_disPktGen,
                             },
                             {
                                     .name = "en_tivaClient",
                                     .cb_cmd = ETHERNET_tivaClient,
                             },
                             {} },
    .fxnTable = &eth_fxnTable,
};

#endif /* OCMP_ETH_SW_H_ */
