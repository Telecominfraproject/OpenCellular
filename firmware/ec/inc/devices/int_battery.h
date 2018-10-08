/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef INT_BATTERY_H_
#define INT_BATTERY_H_

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define PWR_INT_BATT_RSNSB 30 //milli ohms
#define PWR_INT_BATT_RSNSI 7 //milli ohms

/* Config parameters for Internal battery charger */
#define PWR_INTBATT_UNDERVOLTAGE_VAL 9000 //milliVolts
#define PWR_INTBATT_OVERVOLTAGE_VAL 12600 //milliVolts
#define PWR_INTBATT_INPUTUNDERVOLATGE_VAL 16200 //milliVolts
#define PWR_INTBATT_INPUTOVERCURRENT_VAL 5000 //milliAmps
#define PWR_INTBATT_LOWBATTERYCURRENT_VAL 100 //milliAmps
#define PWR_INTBATT_INPUTCURRENTLIMIT_VAL 5570 //milliAmps

#endif /* INT_BATTERY_H_ */
