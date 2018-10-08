/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef EXT_BATTERY_H_
#define EXT_BATTERY_H_

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define PWR_LEAD_ACID_BATT_DEV_TEMP_SENS_ADDR 0x18

#define PWR_EXT_BATT_RSNSB 3 //milli ohms
#define PWR_EXT_BATT_RSNSI 2 //milli ohms

/*
 * External Battery Temperature sensors Low, High and Critical Temeprature Alert Limits
 */
#define PWR_EXT_BATT_TEMP_LOW_LIMIT -20 //(in Celcius)
#define PWR_EXT_BATT_TEMP_HIGH_LIMIT 75 //(in Celcius)
#define PWR_EXT_BATT_TEMP_CRITICAL_LIMIT 80 //(in Celcius)
#define PWR_EXT_BATT_DIE_TEMP_LIMIT 60

/* Config parameters for External battery charger */
#define PWR_EXTBATT_ICHARGE_VAL 10660 //milliAmps
#define PWR_EXTBATT_VCHARGE_VAL 12000 //milliVolts
#define PWR_EXTBATT_UNDERVOLTAGE_VAL 9500 //milliVolts
#define PWR_EXTBATT_OVERVOLTAGE_VAL 13800 //milliVolts
#define PWR_EXTBATT_INPUTBATTUNDERVOLATGE_VAL 16200 //milliVolts
#define PWR_EXTBATT_INPUTHICURRENT_VAL 17000 //milliAmps
#define PWR_EXTBATT_LOWBATTCURRENT_VAL 100 //milliAmps
#define PWR_EXTBATT_INPUTCURRENTLIMIT_VAL 16500 //milliAmps

#endif /* EXT_BATTERY_H_ */
