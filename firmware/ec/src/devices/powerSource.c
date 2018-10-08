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
#include "inc/devices/powerSource.h"

#include "Board.h"
#include "inc/common/global_header.h"
#include "inc/common/i2cbus.h"
#include "inc/subsystem/power/power.h"

#include <ti/sysbios/knl/Task.h>
#include <stdlib.h>
/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
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
static void pwr_update_source_info(ePowerSource powerSrc,
                                   ePowerSourceState pwrState)
{
    ePowerSource itr = PWR_SRC_AUX_OR_SOLAR;
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
 **    FUNCTION NAME   : pwr_source_inuse
 **
 **    DESCRIPTION     : Give info about currently used power source.
 **
 **    ARGUMENTS       : None bool
 **
 **    RETURN TYPE     : None
 **
 ******************************************************************************/
static ReturnStatus pwr_source_inuse(ePowerSource *inUse)
{
    ReturnStatus ret = RETURN_NOTOK;
    ePowerSource itr = PWR_SRC_AUX_OR_SOLAR;
    for (; itr < PWR_SRC_MAX; itr++) {
        if (Power_SourceInfo[itr].state == PWR_SRC_AVAILABLE) {
            *inUse = itr;
            ret = RETURN_OK;
            break;
        }
    }
    return ret;
}

void pwr_source_config(PWRSRC_Dev *driver)
{
    //Configuring GPIOS
    OcGpio_configure(&driver->cfg.pin_solar_aux_prsnt_n, OCGPIO_CFG_INPUT);
    OcGpio_configure(&driver->cfg.pin_poe_prsnt_n, OCGPIO_CFG_INPUT);
    OcGpio_configure(&driver->cfg.pin_int_bat_prsnt, OCGPIO_CFG_INPUT);
    OcGpio_configure(&driver->cfg.pin_ext_bat_prsnt, OCGPIO_CFG_INPUT);
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
void pwr_source_init(void)
{
    ePowerSource itr = PWR_SRC_AUX_OR_SOLAR;
    for (; itr < PWR_SRC_MAX; itr++) {
        Power_SourceInfo[itr].powerSource = itr;
        Power_SourceInfo[itr].state = PWR_SRC_NON_AVAILABLE;
    }
}

/******************************************************************************
 * @fn          pwr_check_aux_or_solar
 *
 * @brief       Check presence of auxilary or solar power source.
 *
 * @args        None.
 *
 * @return      ReturnStatus
 */
static ReturnStatus pwr_check_aux_or_solar(PWRSRC_Dev *pwrSrcDev)
{
    ReturnStatus ret = RETURN_NOTOK;
    ePowerSourceState status = PWR_SRC_NON_AVAILABLE;
    //For Checking SOLAR POWER SOURCE
    uint8_t value = 0;
    value = OcGpio_read(&pwrSrcDev->cfg.pin_solar_aux_prsnt_n);
    if (value == 0) {
        status = PWR_SRC_AVAILABLE;
        ret = RETURN_OK;
    }
    pwr_update_source_info(PWR_SRC_AUX_OR_SOLAR, status);
    return ret;
}

/******************************************************************************
 * @fn          pwr_check_poe
 *
 * @brief       Check presence of POE.
 *
 * @args        None.
 *
 * @return      ReturnStatus
 */
static ReturnStatus pwr_check_poe(PWRSRC_Dev *pwrSrcDev)
{
    ReturnStatus ret = RETURN_NOTOK;
    uint8_t value = 0;
    ePowerSourceState status = PWR_SRC_NON_AVAILABLE;
    //For Checking POE POWER SOURCE
    value = OcGpio_read(&pwrSrcDev->cfg.pin_poe_prsnt_n);
    if (value == 0) {
        status = PWR_SRC_AVAILABLE;
        ret = RETURN_OK;
    }
    pwr_update_source_info(PWR_SRC_POE, status);
    return ret;
}

/******************************************************************************
 * @fn          pwr_check_int_batt
 *
 * @brief       Check presence of internal battery.
 *
 * @args        None.
 *
 * @return      ReturnStatus
 */
static ReturnStatus pwr_check_int_batt(PWRSRC_Dev *pwrSrcDev)
{
    ReturnStatus ret = RETURN_NOTOK;
    uint8_t value = 0;
    ePowerSourceState status = PWR_SRC_NON_AVAILABLE;

    //For Checking INTERNAL BATTERY SOURCE
    value = OcGpio_read(&pwrSrcDev->cfg.pin_int_bat_prsnt);
    if (value == 0) { /* If read fails, we'll get a negative value */
        status = PWR_SRC_AVAILABLE;
        ret = RETURN_OK;
    }

    pwr_update_source_info(PWR_SRC_LIION_BATT, status);
    return ret;
}

/******************************************************************************
 * @fn          pwr_check_ext_batt
 *
 * @brief       Check presence of external battery.
 *
 * @args        None.
 *
 * @return      ReturnStatus
 */
static ReturnStatus pwr_check_ext_batt(PWRSRC_Dev *pwrSrcDev)
{
    ReturnStatus ret = RETURN_NOTOK;
    uint8_t value = 0;
    ePowerSourceState status = PWR_SRC_NON_AVAILABLE;

    value = OcGpio_read(&pwrSrcDev->cfg.pin_ext_bat_prsnt);
    if (value == 0) { /* If read fails, we'll get a negative value */
        status = PWR_SRC_AVAILABLE;
        ret = RETURN_OK;
    }

    pwr_update_source_info(PWR_SRC_LEAD_ACID_BATT, status);
    return ret;
}
/******************************************************************************
 **    FUNCTION NAME   : pwr_check_presence_of_source
 **
 **    DESCRIPTION     : check for power source available.
 **
 **    ARGUMENTS       : None bool
 **
 **    RETURN TYPE     : None
 **
 ******************************************************************************/
static void pwr_check_presence_of_source(PWRSRC_Dev *pwrSrcDev)
{
    ReturnStatus ret = RETURN_NOTOK;
    ret = pwr_check_aux_or_solar(pwrSrcDev);
    LOGGER("POWER:INFO:: Power Source Aux/Solar %s.\n",
           ((ret == RETURN_OK) ? "available" : "not available"));

    ret = pwr_check_poe(pwrSrcDev);
    LOGGER("POWER:INFO:: Power Source POE %s.\n",
           ((ret == RETURN_OK) ? "available" : "not available"));

    ret = pwr_check_int_batt(pwrSrcDev);
    LOGGER("POWER:INFO:: Power Source INTERNAL BATTERY %s.\n",
           ((ret == RETURN_OK) ? "available" : "not available"));

    ret = pwr_check_ext_batt(pwrSrcDev);
    LOGGER("POWER:INFO:: Power Source EXTERNAL BATTERY %s.\n",
           ((ret == RETURN_OK) ? "available" : "not available"));

    return;
}

/*****************************************************************************
 **    FUNCTION NAME   : pwr_get_source_info
 **
 **    DESCRIPTION     : initialize power source information.
 **
 **    ARGUMENTS       :  None
 **
 **     RETURN TYPE     : None
 **
 *****************************************************************************/
void pwr_get_source_info(PWRSRC_Dev *pwrSrcDev)
{
    ReturnStatus status = RETURN_NOTOK;
    ePowerSource powerSource = PWR_SRC_AUX_OR_SOLAR;
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
 **    ARGUMENTS       : Pointer to OCMPMessageFrame structure
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
ReturnStatus
pwr_process_get_status_parameters_data(ePower_StatusParamId paramIndex,
                                       uint8_t *pPowerStatusData)
{
    ReturnStatus status = RETURN_OK;
    switch (paramIndex) {
        case PWR_STAT_POE_AVAILABILITY: {
            if ((Power_SourceInfo[PWR_SRC_POE].state == PWR_SRC_ACTIVE) ||
                (Power_SourceInfo[PWR_SRC_POE].state == PWR_SRC_AVAILABLE))
                *pPowerStatusData = 1;
            break;
        }
        case PWR_STAT_POE_ACCESSIBILITY: {
            if (Power_SourceInfo[PWR_SRC_POE].state == PWR_SRC_ACTIVE)
                *pPowerStatusData = 1;
            break;
        }
        case PWR_STAT_SOLAR_AVAILABILITY: {
            if ((Power_SourceInfo[PWR_SRC_AUX_OR_SOLAR].state ==
                 PWR_SRC_ACTIVE) ||
                (Power_SourceInfo[PWR_SRC_AUX_OR_SOLAR].state ==
                 PWR_SRC_AVAILABLE))
                *pPowerStatusData = 1;
            break;
        }
        case PWR_STAT_SOLAR_ACCESSIBILITY: {
            if (Power_SourceInfo[PWR_SRC_AUX_OR_SOLAR].state == PWR_SRC_ACTIVE)
                *pPowerStatusData = 1;
            break;
        }
        case PWR_STAT_EXTBATT_AVAILABILITY: {
            if ((Power_SourceInfo[PWR_SRC_LEAD_ACID_BATT].state ==
                 PWR_SRC_ACTIVE) ||
                (Power_SourceInfo[PWR_SRC_LEAD_ACID_BATT].state ==
                 PWR_SRC_AVAILABLE))
                *pPowerStatusData = 1;
            break;
        }
        case PWR_STAT_EXTBATT_ACCESSIBILITY: {
            if (Power_SourceInfo[PWR_SRC_LEAD_ACID_BATT].state ==
                PWR_SRC_ACTIVE)
                *pPowerStatusData = 1;
            break;
        }
        case PWR_STAT_INTBATT_AVAILABILITY: {
            if ((Power_SourceInfo[PWR_SRC_LIION_BATT].state ==
                 PWR_SRC_ACTIVE) ||
                (Power_SourceInfo[PWR_SRC_LIION_BATT].state ==
                 PWR_SRC_AVAILABLE))
                *pPowerStatusData = 1;
            break;
        }
        case PWR_STAT_INTBATT_ACCESSIBILITY: {
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
