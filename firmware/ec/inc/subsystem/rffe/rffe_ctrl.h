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
#include "common/inc/global/Framework.h"
#include "inc/common/global_header.h"
#include "inc/subsystem/rffe/rffe.h"

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define RFFE_IO_BOARD_CFG_ADDR 0x19

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
/* RF Channel Type */
typedef enum rfChannel {
    RFFE_CHANNEL1 = 0,
    RFFE_CHANNEL2,
    RFFE_MAX_CHANNEL
} rffeChannel;

typedef struct FE_Ch_Band_cfg {
    rffeChannel channel;
} FE_Ch_Band_cfg;

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

typedef struct FE_Band_Cfg {
    rffeBand band;
} FE_Band_Cfg;

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

typedef struct Fe_Ch_Pwr_Cfg {
    rffeChannel channel;
    Fe_Cfg *fe_Rffecfg;
} Fe_Ch_Pwr_Cfg;
/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
bool rffe_ctrl_set_band(rffeChannel channel, rffeBand band);
bool rffe_ctrl_get_band(rffeChannel channel, rffeBand *band);
ReturnStatus rffe_ctrl_configure_power_amplifier(Fe_Ch_Pwr_Cfg *channel,
                                                 rffePaCtrlType rfPACtrl);
bool RFFE_enablePA(void *driver, void *params);
bool RFFE_disablePA(void *driver, void *params);
void _rffe_watchdog_handler(void *context);

#endif /* RFFE_CTRL_H_ */
