
/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef LTC4015_H_
#define LTC4015_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "common/inc/global/post_frame.h"
#include "drivers/OcGpio.h"
#include "inc/common/i2cbus.h"

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/

/* Mask/Enable Register Bits */
#define LTC4015_ALERT_EN_MASK \
    0xFFFF /* Bits 15-0 are the enable bits(except bit 14) */
#define LTC4015_MSK_MSRV (1 << 15) /* Measurement system results valid */
#define LTC4015_MSK_QCL (1 << 13) /* QCOUNT Low alert */
#define LTC4015_MSK_QCH (1 << 12) /* QCOUNT High alert */
#define LTC4015_MSK_BVL (1 << 11) /* Battery voltage Low alert */
#define LTC4015_MSK_BVH (1 << 10) /* Battery voltage High alert */
#define LTC4015_MSK_IVL (1 << 9) /* Input voltage Low alert */
#define LTC4015_MSK_IVH (1 << 8) /* Input voltage High alert */
#define LTC4015_MSK_SVL (1 << 7) /* System voltage Low alert */
#define LTC4015_MSK_SVH (1 << 6) /* System voltage High alert */
#define LTC4015_MSK_ICH (1 << 5) /* Input current High alert */
#define LTC4015_MSK_BCL (1 << 4) /* Battery current Low alert */
#define LTC4015_MSK_DTH (1 << 3) /* Die temperature High alert */
#define LTC4015_MSK_BSRH (1 << 2) /* BSR High alert */
#define LTC4015_MSK_NTCH (1 << 1) /* NTC ratio High alert */
#define LTC4015_MSK_NTCL (1 << 0) /* NTC ratio Low alert */

#define LTC4015_MSK_BMFA (1 << 1) /* Battery Missing Fault alert */

#define LTC4015_CHARGER_ENABLED (1 << 13)

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
/* Note: There are more chemistry settings that include fixed vs. programmable,
 * however they don't matter as much as the overall chemistry grouping for
 * selecting the correct conversion factors for the registers
 */
typedef enum LTC4015_Chem {
    LTC4015_CHEM_LI_ION,
    LTC4015_CHEM_LI_FE_PO4,
    LTC4015_CHEM_LEAD_ACID,
} LTC4015_Chem;

typedef enum LTC4015_Event {
    LTC4015_EVT_MSRV = LTC4015_MSK_MSRV, /* Measurement system results valid */
    LTC4015_EVT_QCL = LTC4015_MSK_QCL, /* QCOUNT Low alert */
    LTC4015_EVT_QCH = LTC4015_MSK_QCH, /* QCOUNT High alert */
    LTC4015_EVT_BVL = LTC4015_MSK_BVL, /* Battery voltage Low alert */
    LTC4015_EVT_BVH = LTC4015_MSK_BVH, /* Battery voltage High alert */
    LTC4015_EVT_IVL = LTC4015_MSK_IVL, /* Input voltage Low alert */
    LTC4015_EVT_IVH = LTC4015_MSK_IVH, /* Input voltage High alert */
    LTC4015_EVT_SVL = LTC4015_MSK_SVL, /* System voltage Low alert */
    LTC4015_EVT_SVH = LTC4015_MSK_SVH, /* System voltage High alert */
    LTC4015_EVT_ICH = LTC4015_MSK_ICH, /* Input current High alert */
    LTC4015_EVT_BCL = LTC4015_MSK_BCL, /* Battery current Low alert */
    LTC4015_EVT_DTH = LTC4015_MSK_DTH, /* Die temperature High alert */
    LTC4015_EVT_BSRH = LTC4015_MSK_BSRH, /* BSR High alert */
    LTC4015_EVT_NTCL = LTC4015_MSK_NTCL, /* NTC ratio High alert */
    LTC4015_EVT_NTCH = LTC4015_MSK_NTCH, /* NTC ratio Low alert */

    LTC4015_EVT_BMFA = LTC4015_MSK_BMFA, /* Battery Missing Fault alert */
} LTC4015_Event;

typedef void (*LTC4015_CallbackFn)(LTC4015_Event evt, int16_t value,
                                   void *context);

typedef struct LTC4015_HWCfg {
    I2C_Dev i2c_dev;

    /* TODO: this can be read from the IC itself */
    LTC4015_Chem
            chem; /* Battery chemistry we're controlling (verified during init) */
    uint8_t r_snsb; /* Value of SNSB resistor in milli-ohms */
    uint8_t r_snsi; /* Value of SNSI resistor in milli-ohms */

    /* TODO: this can be read from the IC itself */
    uint8_t cellcount; /* Number of cells in battery */

    OcGpio_Pin pin_lt4015_i2c_sel;

    OcGpio_Pin *pin_alert;
} LTC4015_HWCfg;

typedef struct LTC4015_Obj {
    LTC4015_CallbackFn alert_cb;
    void *cb_context;
} LTC4015_Obj;

typedef struct LTC4015_Dev {
    LTC4015_HWCfg cfg;
    LTC4015_Obj obj;
} LTC4015_Dev;

/*****************************************************************************
 *                             FUNCTION DECLARATIONS
 *****************************************************************************/
ReturnStatus LTC4015_cfg_icharge(LTC4015_Dev *dev, uint16_t max_chargeCurrent);

ReturnStatus LTC4015_get_cfg_icharge(LTC4015_Dev *dev,
                                     uint16_t *max_chargeCurrent);

ReturnStatus LTC4015_cfg_vcharge(LTC4015_Dev *dev,
                                 uint16_t charge_voltageLevel);

ReturnStatus LTC4015_get_cfg_vcharge(LTC4015_Dev *dev,
                                     uint16_t *charge_voltageLevel);

ReturnStatus LTC4015_cfg_battery_voltage_low(LTC4015_Dev *dev,
                                             int16_t underVoltage);

ReturnStatus LTC4015_get_cfg_battery_voltage_low(LTC4015_Dev *dev,
                                                 int16_t *underVolatage);

ReturnStatus LTC4015_cfg_battery_voltage_high(LTC4015_Dev *dev,
                                              int16_t overVoltage);

ReturnStatus LTC4015_get_cfg_battery_voltage_high(LTC4015_Dev *dev,
                                                  int16_t *overVoltage);

ReturnStatus LTC4015_cfg_input_voltage_low(LTC4015_Dev *dev,
                                           int16_t inputUnderVoltage);

ReturnStatus LTC4015_get_cfg_input_voltage_low(LTC4015_Dev *dev,
                                               int16_t *inpUnderVoltage);

ReturnStatus LTC4015_cfg_input_current_high(LTC4015_Dev *dev,
                                            int16_t inputOvercurrent);

ReturnStatus LTC4015_get_cfg_input_current_high(LTC4015_Dev *dev,
                                                int16_t *inpOverCurrent);

ReturnStatus LTC4015_cfg_battery_current_low(LTC4015_Dev *dev,
                                             int16_t lowbattCurrent);

ReturnStatus LTC4015_get_cfg_battery_current_low(LTC4015_Dev *dev,
                                                 int16_t *lowbattCurrent);

ReturnStatus LTC4015_cfg_die_temperature_high(LTC4015_Dev *dev,
                                              int16_t dieTemp);

ReturnStatus LTC4015_get_cfg_die_temperature_high(LTC4015_Dev *dev,
                                                  int16_t *dieTemp);

ReturnStatus LTC4015_cfg_input_current_limit(LTC4015_Dev *dev,
                                             uint16_t inputCurrentLimit);

ReturnStatus LTC4015_get_cfg_input_current_limit(LTC4015_Dev *dev,
                                                 uint16_t *currentLimit);

ReturnStatus LTC4015_get_die_temperature(LTC4015_Dev *dev, int16_t *dieTemp);

ReturnStatus LTC4015_get_battery_current(LTC4015_Dev *dev, int16_t *iBatt);

ReturnStatus LTC4015_get_input_current(LTC4015_Dev *dev, int16_t *iIn);

ReturnStatus LTC4015_get_battery_voltage(LTC4015_Dev *dev, int16_t *vbat);

ReturnStatus LTC4015_get_input_voltage(LTC4015_Dev *dev, int16_t *vIn);

ReturnStatus LTC4015_get_system_voltage(LTC4015_Dev *dev, int16_t *vSys);

ReturnStatus LTC4015_get_icharge_dac(LTC4015_Dev *dev, int16_t *ichargeDac);

ReturnStatus LTC4015_get_bat_presence(LTC4015_Dev *dev, bool *present);

ReturnStatus LTC4015_init(LTC4015_Dev *dev);

void LTC4015_setAlertHandler(LTC4015_Dev *dev, LTC4015_CallbackFn alert_cb,
                             void *cb_context);

ReturnStatus LTC4015_enableLimitAlerts(LTC4015_Dev *dev, uint16_t alert_mask);

ReturnStatus LTC4015_enableChargerStateAlerts(LTC4015_Dev *dev,
                                              uint16_t alert_mask);

ePostCode LTC4015_probe(LTC4015_Dev *dev, POSTData *postData);

#endif /* LTC4015_H_ */
