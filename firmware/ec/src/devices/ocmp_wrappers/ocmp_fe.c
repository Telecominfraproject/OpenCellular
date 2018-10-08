/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/ocmp_wrappers/ocmp_fe-param.h"

#include "inc/subsystem/rffe/rffe_ctrl.h"

static FE_Band_Cfg FE_BandCfg[RFFE_MAX_CHANNEL];
typedef enum FE_ParamCfg {
    FE_CFG_BAND = 0,
} FE_ParamCfg;

/*****************************************************************************
 **    FUNCTION NAME   : rffe_ctrl_set_band
 **
 **    DESCRIPTION     : Set the RF Band.
 **
 **    ARGUMENTS       : Channel and Band to be read
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
bool rffe_ctrl_set_band(rffeChannel channel, rffeBand band)
{
    // TODO: Using RFFE_IO_BOARD_CFG_ADDR we should find Band Config.
    FE_BandCfg[channel].band = band;

    DEBUG("RFFECTRL:INFO:: Channel %s RF Band Configuration is %d .\n",
          ((channel == 0) ? "1" : "2"), band);
    return true;
}

/*****************************************************************************
 **    FUNCTION NAME   : rffe_ctrl_get_band
 **
 **    DESCRIPTION     : Gets the RF Band.
 **
 **    ARGUMENTS       : Channel and Band to be read
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
bool rffe_ctrl_get_band(rffeChannel channel, rffeBand *band)
{
    // TODO: Using RFFE_IO_BOARD_CFG_ADDR we should find Band Config.
    *band = FE_BandCfg[channel].band;

    DEBUG("RFFECTRL:INFO:: Channel %s RF Band Configuration is %d .\n",
          ((channel == 0) ? "1" : "2"), *band);
    return true;
}

bool static _get_config(void *driver, unsigned int param_id, void *return_buf)
{
    bool ret = false;
    FE_Ch_Band_cfg *driverCfg = driver;
    switch (param_id) {
        case FE_CFG_BAND: {
            ret = rffe_ctrl_get_band(driverCfg->channel, return_buf);
            break;
        }
        default: {
            LOGGER_ERROR("FE_PARAM::Unknown config param %d\n", param_id);
            ret = false;
        }
    }
    return ret;
}

bool static _set_config(void *driver, unsigned int param_id, void *return_buf)
{
    bool ret = false;
    FE_Ch_Band_cfg *driverCfg = driver;
    rffeBand *band = (rffeBand *)return_buf;
    switch (param_id) {
        case FE_CFG_BAND: {
            rffeChannel *cfg = driver;
            ret = rffe_ctrl_set_band(driverCfg->channel, *band);
            break;
        }
        default: {
            LOGGER_ERROR("FE_PARAM::Unknown config param %d\n", param_id);
            ret = false;
        }
    }
    return ret;
}

static ePostCode _probe(void *driver)
{
    return POST_DEV_FOUND;
}

static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    FE_Ch_Band_cfg *driverCfg = (FE_Ch_Band_cfg *)driver;
    FE_Band_Cfg *cfg = (FE_Band_Cfg *)config;
    rffe_ctrl_set_band(driverCfg->channel, cfg->band);
    return POST_DEV_FOUND;
}

const Driver_fxnTable FE_PARAM_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,
    .cb_init = _init,
    .cb_get_config = _get_config,
    .cb_set_config = _set_config,
};
