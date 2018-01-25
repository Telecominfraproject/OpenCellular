/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef RFFE_CTRL_H_
#define RFFE_CTRL_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "inc/common/global_header.h"
#include "inc/subsystem/rffe/rffe.h"
#include "src/registry/Framework.h"

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define RFFE_CHANNEL1_IO_TX_ATTEN_ADDR              0x18
#define RFFE_CHANNEL1_IO_RX_ATTEN_ADDR              0x1A
#define RFFE_CHANNEL2_IO_TX_ATTEN_ADDR              0x1C
#define RFFE_CHANNEL2_IO_RX_ATTEN_ADDR              0x1D
#define RFFE_IO_REVPOWER_ALERT_ADDR                 0x1B
#define RFFE_IO_BOARD_CFG_ADDR                      0x19

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
/* RFFE Band Type */
typedef enum {
    RFFE_BAND2_1900 = 1,
    RFFE_BAND3_1800,
    RFFE_BAND8_900,
    RFFE_BAND5_850,
    RFFE_BYPASS1,
    RFFE_BYPASS2,
    RFFE_DISABLE,
    RFFE_SHUTDOWN
} rffeBand;

/* RF Channel Type */
typedef enum rfChannel {
    RFFE_CHANNEL1 = 0,
    RFFE_CHANNEL2
} rffeChannel;

/* Power Amplifier Control Type */
typedef enum rfPACtrl {
    RFFE_DEACTIVATE_PA = 0,
    RFFE_ACTIVATE_PA
} rffePaCtrlType;

typedef struct RfWatchdog_Cfg {
    void *alert_token;
    OcGpio_Pin *pin_alert_lb;
    OcGpio_Pin *pin_alert_hb;
    OcGpio_Pin *pin_interrupt;
} RfWatchdog_Cfg;

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
ReturnStatus rffe_ctrl_get_band(rffeChannel channel, rffeBand *band);
ReturnStatus rffe_ctrl_configure_power_amplifier(rffeChannel channel,
                                                 rffePaCtrlType rfPACtrl);

bool RFFE_enablePA(void *driver, void *params);
bool RFFE_disablePA(void *driver, void *params);

extern Driver RFFEWatchdog;

#endif /* RFFE_CTRL_H_ */
