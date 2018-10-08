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
#include "inc/subsystem/rffe/rffe_powermonitor.h"

#include "inc/common/i2cbus.h"
#include "inc/subsystem/rffe/rffe.h"
#include "inc/subsystem/rffe/rffe_ctrl.h"
#include "inc/utils/util.h"

#include <ti/sysbios/knl/Task.h>

/* RFFE device config */
extern void *fe_ch1_ads7830;
extern void *fe_ch2_ads7830;

/*****************************************************************************
 *                             HANDLES DEFINITION
 *****************************************************************************/
/* Global Task Configuration Variables */
static Task_Struct rffePowerMonitorTask;
static Char rffePowerMonitorTaskStack[RFFEPOWERMONITOR_TASK_STACK_SIZE];

/*****************************************************************************
 **    FUNCTION NAME   : rffe_powermonitor_read_adcpower
 **
 **    DESCRIPTION     : Read the RF Power from the Power detector.
 **
 **    NOTE            : This function isn't explicitly thread safe, but
 **                      since we use a single i2c transaction to talk to the
 **                      device, we're good
 **
 **    ARGUMENTS       : Channel Address, Configuration Value and
 **                      Power to be read.
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
static ReturnStatus rffe_powermonitor_read_adcpower(const I2C_Dev *i2c_dev,
                                                    uint8_t adcConfigValue,
                                                    uint16_t *rfpower)
{
    DEBUG("RFPOWERMONITOR:INFO:: Configuring ADS7830 device 0x%x with "
          "Command Byte Value 0x%x\n",
          i2c_dev->slave_addr, adcConfigValue);

    I2C_Handle adcHandle = i2c_get_handle(i2c_dev->bus);
    if (!adcHandle) {
        LOGGER_ERROR("I2CBUS:ERROR:: Failed to get I2C Bus for RF ADS 0x%x.\n",
                     i2c_dev->slave_addr);
        return RETURN_NOTOK;
    }

    /* To read from the ADC, we first pass a config value to tell it which
     * pin(s) to use, then we read an 8-bit value back - we can just treat the
     * config as a register address :) */
    *rfpower = 0; /* Zero for safety - we only read one byte from the device */
    ReturnStatus status = i2c_reg_read(adcHandle, i2c_dev->slave_addr,
                                       adcConfigValue, rfpower, 1);
    if (status != RETURN_OK) {
        LOGGER_ERROR("RFPOWERMONITOR:ERROR:: Failed reading power value from "
                     "ADS7830 device 0x%x.\n",
                     i2c_dev->slave_addr);
        return RETURN_NOTOK;
    }

    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : rffe_powermonitor_read_power
 **
 **    DESCRIPTION     : Read the RF FE Transmitting Power for the requested
 **                      channel.
 **
 **    ARGUMENTS       : Channel and Power to be read
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
ReturnStatus rffe_powermonitor_read_power(const I2C_Dev *i2c_dev,
                                          eRffeStatusParamId rfPowerSelect,
                                          uint16_t *rfpower)
{
    ReturnStatus status = RETURN_OK;
    uint8_t adcConfigValue = 0x00;
    rffeBand band;
    eRffePowerDetect rfPowerDetect;

    if (!i2c_dev) {
        DEBUG("RFPOWERMONITOR:ERROR:: Invalid channel Access\n");
        return RETURN_NOTOK;
    }

    /* Get the RF Band Configuration for the requested RF Channel */
    //status = rffe_ctrl_get_band(rfchannel, &band);

    /* TODO: not ideal, but it's ultimately hardcoded. this makes our lives
     * a bit easier for now */
    band = RFFE_BAND8_900;

    if ((band == RFFE_BAND2_1900) || (band == RFFE_BAND3_1800)) {
        if (rfPowerSelect == RFFE_STAT_FW_POWER) {
            rfPowerDetect = RFFE_HB_F_POWER;
        } else if (rfPowerSelect == RFFE_STAT_REV_POWER) {
            rfPowerDetect = RFFE_HB_R_POWER;
        }
    } else if ((band == RFFE_BAND8_900) || (band == RFFE_BAND5_850)) {
        if (rfPowerSelect == RFFE_STAT_FW_POWER) {
            rfPowerDetect = RFFE_LB_F_POWER;
        } else if (rfPowerSelect == RFFE_STAT_REV_POWER) {
            rfPowerDetect = RFFE_LB_R_POWER;
        }
    }

    /*   Bit 7 - Single-Ended/Differential Inputs(SD)
     *            0 : Differential Inputs
     *            1 : Single-Ended Inputs
     * Bit 6:4 - Channel Selections(C2:C0)
     * Bit 3:2 - Power-Down(PD1:PD0)
     *            0 : Power-Down Selection
     *            X : Unused
     * Bit 1:0 - Unused
     */

    /* Configure ADC in Single Ended Inputs(Bit 7 = 1) and
     * Power Down selection as Internal Reference OFF and A/D Converter ON
     * (Bit 3 = 0 and Bit 2 = 1 ) */
    adcConfigValue = 0x84;

    /* Select the ADC channel using Channel Selection Control Table(Datasheet)
     * and shift the value by 4 and then or with ADC Configuration value */
    if (rfPowerDetect == RFFE_HB_F_POWER) {
        adcConfigValue |= (0x01 << 4);
    } else if (rfPowerDetect == RFFE_LB_F_POWER) {
        adcConfigValue |= (0x02 << 4);
    } else if (rfPowerDetect == RFFE_HB_R_POWER) {
        adcConfigValue |= (0x05 << 4);
    } else if (rfPowerDetect == RFFE_LB_R_POWER) {
        adcConfigValue |= (0x06 << 4);
    } else {
        DEBUG("RFPOWERMONITOR:ERROR:: Invalid RF Power Read Access\n");
        return RETURN_NOTOK;
    }
    status = rffe_powermonitor_read_adcpower(i2c_dev, adcConfigValue, rfpower);
    if (status == RETURN_OK) {
        DEBUG("RFPOWERMONITOR:INFO:: RF Forward/Reverse Power is %d\n",
              *rfpower);
        return status;
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : rffe_powermonitor_task_fxn
 **
 **    DESCRIPTION     : Monitors and Controls RF FE Power.
 **
 **    ARGUMENTS       : a0, a1 - not used
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void rffe_powermonitor_task_fxn(UArg a0, UArg a1)
{
    /* TODO: future enhancement - once we can safely use i2c transfers with
     * callback mode, we don't really need this thread (if we need to maximize
     * speed & efficiency while reducing latency) */
    while (true) {
        uint16_t rfPower;
        ReturnStatus status;

        /* Read RF FE Forward Power on channel 1 */
        status = rffe_powermonitor_read_power(fe_ch1_ads7830,
                                              RFFE_STAT_FW_POWER, &rfPower);
        if (status == RETURN_OK) {
            DEBUG("RFPOWERMONITOR:INFO:: RF Channel 1 Forward Power is %d.\n",
                  rfPower);
        }

        /* Read RF FE Forward Power on channel 2 */
        status = rffe_powermonitor_read_power(fe_ch2_ads7830,
                                              RFFE_STAT_FW_POWER, &rfPower);
        if (status == RETURN_OK) {
            DEBUG("RFPOWERMONITOR:INFO:: RF Channel 2 Forward Power is %d.\n",
                  rfPower);
        }
        Task_sleep(30000);
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : rffe_powermonitor_createtask
 **
 **    DESCRIPTION     : Creates task for RF FE Power Monitoring Task.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
void rffe_powermonitor_createtask(void)
{
    Task_Params taskParams;
    /* Configure RF Power Monitor task */
    Task_Params_init(&taskParams);
    taskParams.stack = rffePowerMonitorTaskStack;
    taskParams.stackSize = RFFEPOWERMONITOR_TASK_STACK_SIZE;
    taskParams.priority = RFFEPOWERMONITOR_TASK_PRIORITY;
    Task_construct(&rffePowerMonitorTask, rffe_powermonitor_task_fxn,
                   &taskParams, NULL);

    DEBUG("RFPOWERMONITOR:INFO:: Creating a RF FE Power Monitoring Task.\n");
}
