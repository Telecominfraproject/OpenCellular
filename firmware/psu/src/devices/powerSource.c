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

#include "Board.h"

#include "common/inc/global/Framework.h"
#include "devices/i2c/threaded_int.h"
#include "inc/common/global_header.h"
#include "inc/common/i2cbus.h"
#include "inc/devices/powerSource.h"
#include "inc/subsystem/power/power.h"

#include <ti/sysbios/knl/Task.h>
#include <stdlib.h>

static tPowerSource Power_SourceInfo[PWR_SRC_MAX];

/*****************************************************************************
 **    FUNCTION NAME   : pwr_update_source_info
 **
 **    DESCRIPTION     : Update power source information.
 **
 **    ARGUMENTS       : Power Source and Power source state.
 **
 **     RETURN TYPE     : None
 **
 *****************************************************************************/
static void pwr_update_source_info(ePowerSource powerSrc, ePowerSourceState pwrState)
{

    ePowerSource itr = PWR_SRC_EXT ;
    for (; itr < PWR_SRC_MAX; itr++) {
        if (Power_SourceInfo[itr].powerSource == powerSrc) {
            Power_SourceInfo[itr].state = pwrState;
            LOGGER("POWER:INFO:: Power State updated for Power Source %d with %d.\n",
                    Power_SourceInfo[itr].powerSource,
                    Power_SourceInfo[itr].state);
        }
    }
}

/******************************************************************************
 * @fn          pwr_check_poe
 *
 * @brief       Check presence of POE.
 *
 * @args        None.
 *
 * @return      ReturnStatus
 ******************************************************************************/
static ReturnStatus pwr_check_poe(PWRSRC_Dev *pwrSrcDev)
{
    ReturnStatus ret = RETURN_NOTOK;
    uint8_t value = 0;
    ePowerSourceState status = PWR_SRC_NON_AVAILABLE;
    //For Checking POE POWER SOURCE
    value = OcGpio_read(&pwrSrcDev->cfg.pin_poe_prsnt_n);
    if ( value == 0) {
        status=PWR_SRC_AVAILABLE;
        ret = RETURN_OK;
    }
    pwr_update_source_info(PWR_SRC_POE, status);
    return ret;
}

/******************************************************************************
 * @fn          pwr_check_ext_power
 *
 * @brief       Check presence of external power.
 *
 * @args        None.
 *
 * @return      ReturnStatus
 ******************************************************************************/
static ReturnStatus pwr_check_ext_power(PWRSRC_Dev *pwrSrcDev)
{
    ReturnStatus ret = RETURN_NOTOK;
    uint8_t value = 0;
    ePowerSourceState status = PWR_SRC_NON_AVAILABLE;
    //For Checking POE POWER SOURCE
    value = OcGpio_read(&pwrSrcDev->cfg.pin_dc_present);
    if ( value == 0) {
        status=PWR_SRC_AVAILABLE;
        ret = RETURN_OK;
    }
    pwr_update_source_info(PWR_SRC_EXT, status);
    return ret;
}

/******************************************************************************
 * @fn          pwr_check_batt
 *
 * @brief       Check presence of Battery.
 *
 * @args        None.
 *
 * @return      ReturnStatus
 ******************************************************************************/
static ReturnStatus pwr_check_batt(PWRSRC_Dev *pwrSrcDev)
{
    ReturnStatus ret = RETURN_NOTOK;
    uint8_t value = 0;
    ePowerSourceState status = PWR_SRC_NON_AVAILABLE;
    //For Checking POE POWER SOURCE
    value = OcGpio_read(&pwrSrcDev->cfg.pin_int_bat_prsnt);
    if ( value == 0) {
        status=PWR_SRC_AVAILABLE;
        ret = RETURN_OK;
    }
    pwr_update_source_info(PWR_SRC_LIION_BATT, status);
    return ret;
}

/******************************************************************************
 **    FUNCTION NAME   : pwr_check_presence_of_source
 **
 **    DESCRIPTION     : check for power source available.
 **
 **    ARGUMENTS       : pointer to powersource config
 **
 **    RETURN TYPE     : None
 **
 ******************************************************************************/
static void pwr_check_presence_of_source(PWRSRC_Dev *pwrSrcDev)
{
    ReturnStatus ret = RETURN_NOTOK;
    ret =  pwr_check_ext_power(pwrSrcDev);
        LOGGER("POWER:INFO:: Power Source External %s.\n",
                    ((ret == RETURN_OK) ? "available" : "not available"));

    ret = pwr_check_poe(pwrSrcDev);
    LOGGER("POWER:INFO:: Power Source POE %s.\n",
                ((ret == RETURN_OK) ? "available" : "not available"));

    ret = pwr_check_batt(pwrSrcDev);
    LOGGER("POWER:INFO:: Power Source BATTERY %s.\n",
                ((ret == RETURN_OK) ? "available" : "not available"));

    return ;
}

/******************************************************************************
 **    FUNCTION NAME   : pwr_source_inuse
 **
 **    DESCRIPTION     : Give info about currently used power source.
 **
 **    ARGUMENTS       : output pointer for storing powersource
 **
 **    RETURN TYPE     : RETURN_OK or RETURN_NOTOK
 **
 ******************************************************************************/
static ReturnStatus pwr_source_inuse(ePowerSource *inUse)
{
    ReturnStatus ret = RETURN_NOTOK;
    ePowerSource itr = PWR_SRC_EXT ;
    for ( ; itr < PWR_SRC_MAX; itr++) {
        if (Power_SourceInfo[itr].state == PWR_SRC_AVAILABLE) {
            *inUse = itr;
            ret = RETURN_OK;
            break;
        }

    }
    return ret;
}

/*****************************************************************************
 **    FUNCTION NAME   : pwr_source_init
 **
 **    DESCRIPTION     : initialize power source information.
 **
 **    ARGUMENTS       :  None
 **
 **     RETURN TYPE    : None
 **
 *****************************************************************************/
void pwr_source_init(PWRSRC_Dev *dev, void *alert_token)
{
    ePowerSource itr = PWR_SRC_EXT ;
    const uint32_t pin_cfg = OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_BOTH_EDGES;
    AlertData *alert_data_cp = alert_token;

    pinConfig *dc_presence = malloc(sizeof(pinConfig));
    pinConfig *poe_presence = malloc(sizeof(pinConfig));
    pinConfig *battery_presence = malloc(sizeof(pinConfig));

    if (!(dc_presence && poe_presence && battery_presence)) {
        LOGGER("POWER:ERROR:: Failed to get memory.\n");
        return;
    }
    for (; itr < PWR_SRC_MAX; itr++) {
        Power_SourceInfo[itr].powerSource = itr;
        Power_SourceInfo[itr].state = PWR_SRC_NON_AVAILABLE;
    }

    if (&dev->cfg.pin_dc_present) {
        if (OcGpio_configure(&dev->cfg.pin_dc_present, pin_cfg) < OCGPIO_SUCCESS) {
            return ;
        }
        dc_presence->subSystem = alert_data_cp->subsystem;
        dc_presence->alertPin = &dev->cfg.pin_dc_present;
        ThreadedInt_Init(dc_presence, NULL, (void *)dev);
    }
    if (&dev->cfg.pin_poe_prsnt_n) {
        if (OcGpio_configure(&dev->cfg.pin_poe_prsnt_n, pin_cfg) < OCGPIO_SUCCESS) {
            return ;
        }
        poe_presence->subSystem = alert_data_cp->subsystem;
        poe_presence->alertPin = &dev->cfg.pin_poe_prsnt_n;
        ThreadedInt_Init(poe_presence, NULL, (void *)dev);
    }
    if (&dev->cfg.pin_int_bat_prsnt) {
        if (OcGpio_configure(&dev->cfg.pin_int_bat_prsnt, pin_cfg) < OCGPIO_SUCCESS) {
            return ;
        }
        battery_presence->subSystem = alert_data_cp->subsystem;
        battery_presence->alertPin = &dev->cfg.pin_int_bat_prsnt;
        ThreadedInt_Init(battery_presence, NULL, (void *)dev);
    }

    return;
}

/*****************************************************************************
 **    FUNCTION NAME   : pwr_get_source_info
 **
 **    DESCRIPTION     : initialize power source information.
 **
 **    ARGUMENTS       :  power source config
 **
 **     RETURN TYPE     : None
 **
 *****************************************************************************/
void pwr_get_source_info(PWRSRC_Dev *pwrSrcDev)
{
    ReturnStatus status = RETURN_NOTOK;
    ePowerSource powerSource = PWR_SRC_EXT;
    /* Check the presence of power sources*/
    pwr_check_presence_of_source(pwrSrcDev);

    /* Find the primary power source and update Power Source info for same.*/
    status = pwr_source_inuse(&powerSource);
    if (status != RETURN_OK) {
        LOGGER("POWER:ERROR:: Failed to get current power source.\n");
    } else {
        LOGGER("POWER:INFO:: Current Power source is 0x%x.\n", powerSource);
        pwr_update_source_info(powerSource, PWR_SRC_ACTIVE);
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : pwr_process_get_status_parameters_data
 **
 **    DESCRIPTION     : Get Power Status Message.
 **
 **    ARGUMENTS       : parameter id ,Pointer to OCMPMessageFrame payload
 **
 **    RETURN TYPE     : RETURN_OK or RETURN_NOTOK
 **
 *****************************************************************************/
ReturnStatus
pwr_process_get_status_parameters_data(ePower_StatusParamId paramIndex,
                                       uint8_t *pPowerStatusData)
{
    ReturnStatus status = RETURN_OK;
    switch (paramIndex) {
        case PWR_STAT_EXT_PWR_AVAILABILITY: {
            if ((Power_SourceInfo[PWR_SRC_POE].state == PWR_SRC_ACTIVE) ||
                    (Power_SourceInfo[PWR_SRC_POE].state == PWR_SRC_AVAILABLE))
                *pPowerStatusData = 1;
            break;
        }
        case PWR_STAT_EXT_PWR_ACTIVE: {
            if (Power_SourceInfo[PWR_SRC_POE].state == PWR_SRC_ACTIVE)
                *pPowerStatusData = 1;
            break;
        }
        case PWR_STAT_POE_AVAILABILITY: {
            if ((Power_SourceInfo[PWR_SRC_POE].state == PWR_SRC_ACTIVE) ||
                (Power_SourceInfo[PWR_SRC_POE].state == PWR_SRC_AVAILABLE))
                *pPowerStatusData = 1;
            break;
        }
        case PWR_STAT_POE_ACTIVE: {
            if (Power_SourceInfo[PWR_SRC_POE].state == PWR_SRC_ACTIVE)
                *pPowerStatusData = 1;
            break;
        }
        case PWR_STAT_BATT_AVAILABILITY: {
            if ((Power_SourceInfo[PWR_SRC_LIION_BATT].state ==
                 PWR_SRC_ACTIVE) ||
                (Power_SourceInfo[PWR_SRC_LIION_BATT].state ==
                 PWR_SRC_AVAILABLE))
                *pPowerStatusData = 1;
            break;
        }
        case PWR_STAT_BATT_ACTIVE: {
            if (Power_SourceInfo[PWR_SRC_LIION_BATT].state == PWR_SRC_ACTIVE)
                *pPowerStatusData = 1;
            break;
        }
        default: {
            LOGGER("POWER::ERROR: Invalid Power param status.\n");
        }
    }
    return status;
}
