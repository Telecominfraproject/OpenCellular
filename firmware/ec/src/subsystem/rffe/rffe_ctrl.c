/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
//*****************************************************************************
//                                HEADER FILES
//*****************************************************************************
#include "inc/subsystem/rffe/rffe_ctrl.h"

#include "common/inc/global/Framework.h"
#include "devices/i2c/threaded_int.h"
#include "inc/devices/pca9557.h"
#include "inc/subsystem/rffe/rffe.h"
#include "inc/utils/util.h"

#include <stdlib.h>
#include <string.h>

static FE_Band_Cfg FE_BandCfg[RFFE_MAX_CHANNEL];

typedef enum FE_ParamCfg {
    FE_CFG_BAND = 0,
} FE_ParamCfg;

/*****************************************************************************
 **    FUNCTION NAME   : rffe_ctrl_configure_power_amplifier
 **
 **    DESCRIPTION     : Activate or Deactivate Power Amplifiers for the
 **                      requested band.
 **
 **    ARGUMENTS       : Channel and PA Configuration
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
ReturnStatus rffe_ctrl_configure_power_amplifier(Fe_Ch_Pwr_Cfg *pwrCfg,
                                                 rffePaCtrlType rfPACtrl)
{
    ReturnStatus status = RETURN_OK;
    rffeBand band = RFFE_SHUTDOWN;
    rffeChannel channel = pwrCfg->channel;
    const Fe_Ch2_Gain_Cfg *fe_ch1_rf_band_sel =
            pwrCfg->fe_Rffecfg->fe_ch2_gain_cfg;
    const Fe_Ch2_Lna_Cfg *fe_ch1_rf_pwr = pwrCfg->fe_Rffecfg->fe_ch2_lna_cfg;
    const Fe_Watchdog_Cfg *fe_ch2_rf_pwr = pwrCfg->fe_Rffecfg->fe_watchdog_cfg;

    /* Get the RF Band Configuration for the requested RF Channel */
    status = rffe_ctrl_get_band(channel, &band);
    if (status == true) {
        DEBUG("RFFECTRL:INFO:: RF Channel %s Band is 0x%x.\n",
              ((channel == 0) ? "1" : "2"), band);
    } else {
        return status;
    }

    DEBUG("RFFECTRL:INFO:: %s Channel %s for Band %d.\n",
          ((rfPACtrl == 0) ? "Deactivating" : "Activating"),
          ((channel == 0) ? "1" : "2"), band);

    if (channel == RFFE_CHANNEL1) {
        if (rfPACtrl == RFFE_ACTIVATE_PA) {
            OcGpio_write(&fe_ch1_rf_band_sel->pin_ch1_2g_lb_band_sel_l, true);
            OcGpio_write(&fe_ch1_rf_pwr->pin_ch1_rf_pwr_off, true);
        } else if (rfPACtrl == RFFE_DEACTIVATE_PA) {
            OcGpio_write(&fe_ch1_rf_pwr->pin_ch1_rf_pwr_off, false);
        }

    } else if (channel == RFFE_CHANNEL2) {
        if (rfPACtrl == RFFE_ACTIVATE_PA) {
            OcGpio_write(&fe_ch2_rf_pwr->pin_ch2_rf_pwr_off, true);
        } else if (rfPACtrl == RFFE_DEACTIVATE_PA) {
            OcGpio_write(&fe_ch2_rf_pwr->pin_ch2_rf_pwr_off, false);
        }
    }
    return RETURN_OK;
}

bool RFFE_enablePA(void *driver, void *params)
{
    Fe_Ch_Pwr_Cfg *channel = (Fe_Ch_Pwr_Cfg *)driver;
    return (rffe_ctrl_configure_power_amplifier(channel, RFFE_ACTIVATE_PA) ==
            RETURN_OK);
}

bool RFFE_disablePA(void *driver, void *params)
{
    Fe_Ch_Pwr_Cfg *channel = (Fe_Ch_Pwr_Cfg *)driver;
    return (rffe_ctrl_configure_power_amplifier(channel, RFFE_DEACTIVATE_PA) ==
            RETURN_OK);
}