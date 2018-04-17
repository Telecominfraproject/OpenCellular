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

#include "devices/i2c/threaded_int.h"
#include "inc/devices/pca9557.h"
#include "inc/subsystem/rffe/rffe.h"
#include "inc/utils/util.h"
#include "registry/Framework.h"

#include <stdlib.h>
#include <string.h>

/*****************************************************************************
 *                           GPIO CONFIGURATION
 *****************************************************************************/
extern const void *sys_config[];

static void _rffe_watchdog_handler(void *context)
{
    RfWatchdog_Cfg *cfg = context;
    if (OcGpio_read(cfg->pin_alert_lb) > 0) {
        OCMP_GenerateAlert(context, 0, NULL);
    }
    if (OcGpio_read(cfg->pin_alert_hb) > 0) {
        OCMP_GenerateAlert(context, 1, NULL);
    }
}

static ePostCode _rffe_watchdog_init(void *driver, const void *config,
                                     const void *alert_token)
{
    RfWatchdog_Cfg *cfg = driver;
    if (OcGpio_configure(cfg->pin_alert_lb, OCGPIO_CFG_INPUT)) {
        return POST_DEV_CFG_FAIL;
    }
    if (OcGpio_configure(cfg->pin_alert_hb, OCGPIO_CFG_INPUT)) {
        return POST_DEV_CFG_FAIL;
    }
    if (OcGpio_configure(cfg->pin_interrupt, OCGPIO_CFG_INPUT |
                                          OCGPIO_CFG_INT_FALLING)) {
        return POST_DEV_CFG_FAIL;
    }

    ThreadedInt_Init(cfg->pin_interrupt, _rffe_watchdog_handler, cfg);
    return POST_DEV_CFG_DONE;
}

Driver RFFEWatchdog = {
    .name = "RFFE Watchdog",
    .alerts = (Parameter[]){
        { .name = "LB_R_PWR" },
        { .name = "HB_R_PWR" },
        {}
    },
    .cb_init = _rffe_watchdog_init,
};

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
ReturnStatus rffe_ctrl_get_band(rffeChannel channel, rffeBand *band)
{
    ReturnStatus status = RETURN_OK;

    // TODO: Using RFFE_IO_BOARD_CFG_ADDR we should find Band Config.
    *band = RFFE_BAND8_900;

    DEBUG("RFFECTRL:INFO:: Channel %s RF Band Configuration is %d .\n",
          ((channel == 0) ? "1" : "2"), *band);
    return status;
}

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
ReturnStatus rffe_ctrl_configure_power_amplifier(rffeChannel channel,
                                                 rffePaCtrlType rfPACtrl)
{
    ReturnStatus status = RETURN_OK;
    rffeBand band = RFFE_SHUTDOWN;
    const Fe_Cfg *fe_cfg = sys_config[OC_SS_RF];
    const Fe_Ch2_Gain_Cfg *fe_ch1_rf_band_sel = &fe_cfg->fe_ch2_gain_cfg;
    const Fe_Ch2_Lna_Cfg *fe_ch1_rf_pwr = &fe_cfg->fe_ch2_lna_cfg;
    const Fe_Watchdog_Cfg *fe_ch2_rf_pwr = &fe_cfg->fe_watchdog_cfg;

    /* Get the RF Band Configuration for the requested RF Channel */
    status = rffe_ctrl_get_band(channel, &band);
    if (status == RETURN_OK) {
        DEBUG("RFFECTRL:INFO:: RF Channel %s Band is 0x%x.\n",
              ((channel == 0) ? "1" : "2"), band);
    } else {
        return status;
    }

    DEBUG("RFFECTRL:INFO:: %s Channel %s for Band %d.\n",
          ((rfPACtrl == 0) ? "Deactiavting" : "Activating"),
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
    return status;
}

bool RFFE_enablePA(void *driver, void *params)
{
    return (rffe_ctrl_configure_power_amplifier((rffeChannel)driver,
                    RFFE_ACTIVATE_PA) == RETURN_OK);
}

bool RFFE_disablePA(void *driver, void *params)
{
    return (rffe_ctrl_configure_power_amplifier((rffeChannel)driver,
                    RFFE_DEACTIVATE_PA) == RETURN_OK);
}
