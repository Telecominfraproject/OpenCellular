/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef ADT7481_H_
#define ADT7481_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "common/inc/global/post_frame.h"
#include "inc/common/global_header.h"
#include "inc/common/i2cbus.h"
#include <stdint.h>

/*****************************************************************************
 *                            MACRO DEFINITIONS
 *****************************************************************************/
/*
 * Make the value of ADT7481_EXTENDED_FLAG flag to 1 if temperature range needs
 * to be in extended region(-64°C to +191°C) otherwise 0 if temperature region
 * is in normal range(0°C to +127°C).
 */
#define ADT7481_EXTENDED_FLAG               1

/*
 * 7 - (Mask)    - Setting this bit to 1 masks all ALERTs on the ALERT pin.
 *     Default = 0 = ALERT enabled. This applies only if Pin 8 is
 *     configured as ALERT, otherwise it has no effect.
 * 6 - (Mon/STBY) - Setting this bit to 1 places the ADT7481 in standby mode,
 *     that is, it suspends all temperature measurements(ADC). The SMBus remains
 *     active and values can be written to, and read from, the registers.However
 *     THERM and ALERT are not active in standby mode, and their states in
 *     standby mode are not reliable.
 *     Default = 0 = temperature monitoring enabled.
 * 5 - (AL/TH) - This bit selects the function of Pin 8. Default = 0 = ALERT.
 *     Setting this bit to 1 configures Pin 8 as the THERM2 pin.
 * 4 - (Reserved) - Reserved for future use.
 * 3 - (Remote 1/2)- Setting this bit to 1 enables the user to read the Remote 2
 *     values from the Remote 1 registers. When default = 0, Remote 1 temperature
 *     values and limits are read from these registers.
 * 2 - (Temp Range) - Setting this bit to 1 enables the extended temperature
 *     measurement range of -64°C to +191°C. When using the default = 0, the
 *     temperature range is 0°C to +127°C.
 * 1 - (Mask R1) - 1 - Setting this bit to 1 masks ALERTs due to the Remote 1
 *     temperature exceeding a programmed limit. Default = 0.
 * 0 - (Mask R2) - 0 - Setting this bit to 1 masks ALERTs due to the Remote 2
 *     temperature exceeding a programmed limit. Default = 0.
 */
#define ADT7481_CONFIGURATION_REG_VALUE     (ADT7481_EXTENDED_FLAG << 2)    /* Set/Clear Only Temp Range bit */

/*
 * 7   - (Averaging) - Setting this bit to 1 disables averaging of the
 *       temperature measurements at the slower conversion rates (averaging
 *       cannot take place at the three faster rates, so setting this bit has
 *       no effect). When default = 0, averaging is enabled.
 * 6   - (Reserved) - Reserved for future use. Do not write to this bit.
 * 5:4 - (Channel Selector) - These bits are used to select the temperature
 *       measurement channels:
 *       00 = Round Robin = Default = All Channels Measured
 *       01 = Local Temperature Only Measured
 *       10 = Remote 1 Temperature Only Measured
 *       11 = Remote 2 Temperature Only Measured
 * 3:0 - (Reserved) - These bits set how often the ADT7481 measures each
 *       temperature channel. Conversion rates are as follows:
 *       Conversions/sec Time           (seconds)
 *          0000 = 0.0625                   16
 *          0001 = 0.125                    8
 *          0010 = 0.25                     4
 *          0011 = 0.5                      2
 *          0100 = 1                        1
 *          0101 = 2                        500 m
 *          0110 = 4                        250 m
 *          0111 = 8 = Default              125 m
 *          1000 = 16                       62.5 m
 *          1001 = 32                       31.25 m
 *          1010 = 64                       15.5 m
 *          1011 = Continuous Measurements  73 m (Averaging Enabled)
 */
#define ADT7481_CONVERSION_RATE_REG_VALUE   0x07 /* Set conversion rate to 125ms(default) */

/* ADT7481 Manufacturer Id and Device Id */
#define TEMP_ADT7481_MANF_ID                0x41
#define TEMP_ADT7481_DEV_ID                 0x81

/*
 * Enumeration of Temperature limit registers
 */
typedef enum {
    CONF_TEMP_ADT7481_LOW_LIMIT_REG = 1,
    CONF_TEMP_ADT7481_HIGH_LIMIT_REG,
    CONF_TEMP_ADT7481_THERM_LIMIT_REG
} eTempSensorADT7481ConfigParamsId;

/*****************************************************************************
 *                          FUNCTION DECLARATIONS
 *****************************************************************************/
ReturnStatus adt7481_get_dev_id(const I2C_Dev *i2c_dev,
                                uint8_t *devID);
ReturnStatus adt7481_get_mfg_id(const I2C_Dev *i2c_dev,
                                uint8_t *mfgID);
ePostCode adt7481_probe(const I2C_Dev *i2c_dev,
                              POSTData *postData);
ReturnStatus adt7481_get_config1(const I2C_Dev *i2c_dev,
                                 uint8_t *configValue);
ReturnStatus adt7481_set_config1(const I2C_Dev *i2c_dev,
                                 uint8_t configValue);
ReturnStatus adt7481_get_conv_rate(const I2C_Dev *i2c_dev,
                                   uint8_t *convRateValue);
ReturnStatus adt7481_set_conv_rate(const I2C_Dev *i2c_dev,
                                   uint8_t convRateValue);
ReturnStatus adt7481_get_status1(const I2C_Dev *i2c_dev,
                                 uint8_t *statusValue);
ReturnStatus adt7481_get_status2(const I2C_Dev *i2c_dev,
                                 uint8_t *statusValue);
ReturnStatus adt7481_get_local_temp_val(const I2C_Dev *i2c_dev,
                                        int16_t *tempValue);
ReturnStatus adt7481_get_remote1_temp_val(const I2C_Dev *i2c_dev,
                                          int16_t *tempValue);
ReturnStatus adt7481_get_remote2_temp_val(const I2C_Dev *i2c_dev,
                                          int8_t *tempValue);
ReturnStatus adt7481_get_local_temp_limit(const I2C_Dev *i2c_dev,
                                          eTempSensorADT7481ConfigParamsId limitToConfig,
                                          int16_t* tempLimitValue);
ReturnStatus adt7481_set_local_temp_limit(const I2C_Dev *i2c_dev,
                                          eTempSensorADT7481ConfigParamsId limitToConfig,
                                          int16_t tempLimitValue);
ReturnStatus adt7481_get_remote2_temp_low_limit(const I2C_Dev *i2c_dev,
                                                int8_t* tempLimitValue);
ReturnStatus adt7481_get_remote2_temp_high_limit(const I2C_Dev *i2c_dev,
                                                 int8_t* tempLimitValue);
ReturnStatus adt7481_get_remote2_temp_therm_limit(const I2C_Dev *i2c_dev,
                                                  int8_t* tempLimitValue);
ReturnStatus adt7481_get_remote1_temp_limit(const I2C_Dev *i2c_dev,
                                            eTempSensorADT7481ConfigParamsId limitToConfig,
                                            int8_t* tempLimitValue);
ReturnStatus adt7481_set_remote2_temp_low_limit(const I2C_Dev *i2c_dev,
                                                int8_t tempLimitValue);
ReturnStatus adt7481_set_remote2_temp_high_limit(const I2C_Dev *i2c_dev,
                                                 int8_t tempLimitValue);
ReturnStatus adt7481_set_remote2_temp_therm_limit(const I2C_Dev *i2c_dev,
                                                  int8_t tempLimitValue);
ReturnStatus adt7481_set_remote1_temp_limit(const I2C_Dev *i2c_dev,
                                            eTempSensorADT7481ConfigParamsId limitToConfig,
                                            int8_t tempLimitValue);
ReturnStatus adt7481_get_remote2_temp_limit(const I2C_Dev *i2c_dev,
                                            eTempSensorADT7481ConfigParamsId limitToConfig,
                                            int8_t* tempLimitValue);
ReturnStatus adt7481_set_remote2_temp_limit(const I2C_Dev *i2c_dev,
                                            eTempSensorADT7481ConfigParamsId limitToConfig,
                                            int8_t tempLimitValue);
ReturnStatus adt7481_get_remote1_temp_offset(const I2C_Dev *i2c_dev,
                                             int16_t* tempOffsetValue);
ReturnStatus adt7481_set_remote1_temp_offset(const I2C_Dev *i2c_dev,
                                             int16_t tempOffsetValue);
ReturnStatus adt7481_get_remote2_temp_offset(const I2C_Dev *i2c_dev,
                                             int16_t* tempOffsetValue);
ReturnStatus adt7481_set_remote2_temp_offset(const I2C_Dev *i2c_dev,
                                             int8_t tempOffsetValue);

ReturnStatus adt7481_get_therm_hysteresis(const I2C_Dev *i2c_dev,
                                          int8_t* tempHysteresisValue);
ReturnStatus adt7481_set_therm_hysteresis(const I2C_Dev *i2c_dev,
                                          int8_t tempHysteresisValue);

#endif /* ADT7481_H_ */
