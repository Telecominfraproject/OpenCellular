/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef CHARGECTRL_LTC4015_H_
#define CHARGECTRL_LTC4015_H_

/*****************************************************************************
 *                          REGISTER DEFINITIONS
 *****************************************************************************/
//  Battery voltage low alert limit                     - BITS[15:0] 0x0000
#define LTC4015_VBAT_LO_ALERT_LIMIT_SUBADDR 0x01
//  Battery voltage high alert limit                    - BITS[15:0] 0x0000
#define LTC4015_VBAT_HI_ALERT_LIMIT_SUBADDR 0x02
//  Input voltage low alert limit                       - BITS[15:0] 0x0000
#define LTC4015_VIN_LO_ALERT_LIMIT_SUBADDR 0x03
//  Input voltage high alert limit                      - BITS[15:0] 0x0000
#define LTC4015_VIN_HI_ALERT_LIMIT_SUBADDR 0x04
//  Output voltage low alert limit                      - BITS[15:0] 0x0000
#define LTC4015_VSYS_LO_ALERT_LIMIT_SUBADDR 0x05
//  Output voltage high alert limit                     - BITS[15:0] 0x0000
#define LTC4015_VSYS_HI_ALERT_LIMIT_SUBADDR 0x06
//  Input current high alert limit                      - BITS[15:0] 0x0000
#define LTC4015_IIN_HI_ALERT_LIMIT_SUBADDR 0x07
//  Charge current low alert limit                      - BITS[15:0] 0x0000
#define LTC4015_IBAT_LO_ALERT_LIMIT_SUBADDR 0x08
//  Die temperature high alert limit,                   - BITS[15:0] 0x0000
#define LTC4015_DIE_TEMP_HI_ALERT_LIMIT_SUBADDR 0x09
//  Battery series resistance high alert limit          - BITS[15:0] 0x0000
#define LTC4015_BSR_HI_ALERT_LIMIT_SUBADDR 0x0A
//  Thermistor ratio high (cold battery) alert limit    - BITS[15:0] 0x0000
#define LTC4015_NTC_RATIO_HI_ALERT_LIMIT_SUBADDR 0x0B
//  Thermistor ratio low (hot battery) alert limit      - BITS[15:0] 0x0000
#define LTC4015_NTC_RATIO_LO_ALERT_LIMIT_SUBADDR 0x0C

/*  Bit fields:
 *
 *     15 : Enable meas_sys_valid_alert
 *     14 : N/A
 *     13 : Enable coulomb counter value low alert
 *     12 : Enable coulomb counter value high alert
 *     11 : Enable battery undervoltage alert
 *     10 : Enable battery overvoltage alert
 *      9 : Enable input undervoltage alert
 *      8 : Enable input overvoltage alert
 *      7 : Enable output undervoltage alert
 *      6 : Enable output overvoltage alert
 *      5 : Enable input overcurrent alert
 *      4 : Enable battery current low alert
 *      3 : Enable die temperature high alert
 *      2 : Enable battery series resistance high alert
 *      1 : Enable thermistor ratio high (cold battery) alert
 *      0 : Enable thermistor ratio low (hot battery) alert
 */
//  Enable limit monitoring and alert notification via SMBALERT - BITS[15:0] 0x0000
#define LTC4015_EN_LIMIT_ALERTS_SUBADDR 0x0D

/*  Bit fields:
 *
 *  15:11 : N/A
 *     10 : Enable alert for lead-acid equalize charge state
 *      9 : Enable alert for absorb charge state
 *      8 : Enable alert for charger suspended state
 *      7 : Enable alert for precondition charge state
 *      6 : Enable alert for constant current constant voltage state
 *      5 : Enable alert for thermistor pause state
 *      4 : Enable alert for timer termination state
 *      3 : Enable alert for C/x termination state
 *      2 : Enable max_charge_time_fault alert
 *      1 : Enable alert for missing battery fault state
 *      0 : Enable alert for shorted battery fault state
 */
//  Enable charger state alert notification via SMBALERT - BITS[15:0] 0x0000
#define LTC4015_EN_CHARGER_STATE_ALERTS_SUBADDR 0x0E

/*  Bit fields:
 *
 *   15:4 : N/A
 *      3 : Enable alert for input undervoltage current limit active
 *      2 : Enable alert for input current limit active
 *      1 : Enable alert for constant current status
 *      0 : Enable alert for constant voltage status
 */
//  Enable charge status alert notification via SMBALERT - BITS[15:0] 0x0000
#define LTC4015_EN_CHARGE_STATUS_ALERTS_SUBADDR 0x0F

// Coulomb counter QCOUNT low alert limit, same format as QCOUNT (0x13)  - BITS[15:0] : 0x0000
#define LTC4015_QCOUNT_LO_ALERT_LIMIT_SUBADDR 0x10
// Coulomb counter QCOUNT high alert limit, same format as QCOUNT (0x13) - BITS[15:0] : 0x0000
#define LTC4015_QCOUNT_HI_ALERT_LIMIT_SUBADDR 0x11
// Coulomb counter prescale factor                                       - BITS[15:0] : 0x0200
#define LTC4015_QCOUNT_PRESCALE_FACTOR_SUBADDR 0x12
// Coulomb counter value                                                 - BITS[15:0] : 0x8000
#define LTC4015_QCOUNT_SUBADDR 0x13

/*  Bit fields:
 *
 *   15:9 : N/A
 *      8 : Suspend battery charger operation
 *    7:6 : N/A
 *      5 : Perform a battery series resistance measurement
 *      4 : Force measurement system to operate
 *      3 : Enable Maximum Power Point Tracking
 *      2 : Enable coulomb counter
 *    1:0 : N/A
 */
// Configuration Settings  - BITS[15:0] : 0x0000
#define LTC4015_CONFIG_BITS_SUBADDR 0x14

// Input current limit setting = (IIN_LIMIT_SETTING + 1) • 500uV / RSNSI - BITS[5:0] : 0x3F
#define LTC4015_IIN_LIMIT_SETTING_SUBADDR 0x15
// UVCLFB input undervoltage limit = (VIN_UVCL_SETTING + 1) • 4.6875mV   - BITS[7:0] : 0xFF
#define LTC4015_VIN_UVCL_SETTING_SUBADDR 0x16
#define LTC4015_RESERVED_0X17_SUBADDR 0x17
#define LTC4015_RESERVED_0X18_SUBADDR 0x18
// Write 0x534D to arm ship mode. Once armed, ship mode cannot be disarmed.
#define LTC4015_ARM_SHIP_MODE_SUBADDR 0x19
// Maximum charge current target = (ICHARGE_TARGET + 1) • 1mV/RSNSB      - BITS[4:0]
#define LTC4015_ICHARGE_TARGET_SUBADDR 0x1A
// Charge voltage target                                                 - BITS[5:0]
#define LTC4015_VCHARGE_SETTING_SUBADDR 0x1B
// Two’s complement Low IBAT threshold for C/x termination               - BITS[15:0]
#define LTC4015_C_OVER_X_THRESHOLD_SUBADDR 0x1C
// Time in seconds with battery charger in the CV state before timer termination
// occurs (lithium chemistries only)
#define LTC4015_MAX_CV_TIME_SUBADDR 0x1D
// Time in seconds before a max_charge_time fault is declared. Set to zero to
// disable max_charge_time fault
#define LTC4015_MAX_CHARGE_TIME_SUBADDR 0x1E
// Value of NTC_RATIO for transition between JEITA regions 2 and 1 (off) - BITS[15:0] : 0x3F00
#define LTC4015_JEITA_T1_SUBADDR 0x1F
// Value of NTC_RATIO for transition between JEITA regions 3 and 2       - BITS[15:0] : 0x372A
#define LTC4015_JEITA_T2_SUBADDR 0x20
// Value of NTC_RATIO for transition between JEITA regions 4 and 3       - BITS[15:0] : 0x1F27
#define LTC4015_JEITA_T3_SUBADDR 0x21
// Value of NTC_RATIO for transition between JEITA regions 5 and 4       - BITS[15:0] : 0x1BCC
#define LTC4015_JEITA_T4_SUBADDR 0x22
// Value of NTC_RATIO for transition between JEITA regions 6 and 5       - BITS[15:0] : 0x18B9
#define LTC4015_JEITA_T5_SUBADDR 0x23
// Value of NTC_RATIO for transition between JEITA regions 7 (off) and 6 - BITS[15:0] : 0x136D
#define LTC4015_JEITA_T6_SUBADDR 0x24

/*  Bit Fields:
 *
 *   15:10 : N/A
 *    9:5  : vcharge_jeita_6
 *    4:0  : vcharge_jeita_5
 */
// VCHARGE values for JEITA temperature regions 6 and 5
#define LTC4015_VCHARGE_JEITA_6_5_SUBADDR 0x25

/*  Bit Fields:
 *
 *      15 : N/A
 *   14:10 : vcharge_jeita_4
 *    9:5  : vcharge_jeita_3
 *    4:0  : vcharge_jeita_4
 */
// VCHARGE values for JEITA temperature regions 4, 3, and 2
#define LTC4015_VCHARGE_JEITA_4_3_2_SUBADDR 0x26

/*  Bit Fields:
 *
 *   15:10 : N/A
 *    9:5  : icharge_jeita_6
 *    4:0  : icharge_jeita_5
 */
// ICHARGE_TARGET values for JEITA temperature regions 6 and 5      - BITS[15:0] : 0x01EF
#define LTC4015_ICHARGE_JEITA_6_5_SUBADDR 0x27

/*  Bit Fields:
 *
 *      15 : N/A
 *   14:10 : icharge_jeita_4
 *    9:5  : icharge_jeita_3
 *    4:0  : icharge_jeita_4
 */
// ICHARGE_TARGET value for JEITA temperature regions 4, 3, and 2   - BITS[15:0] : 0x7FEF
#define LTC4015_ICHARGE_JEITA_4_3_2_SUBADDR 0x28

/*  Bit Fields:
 *
 *    15:3 : N/A
 *       2 : Enable C/x termination
 *       1 : Enable lead acid charge voltage temperature compensation
 *       0 : Enable JEITA temperature profile
 */
// Battery charger configuration settings
#define LTC4015_CHARGER_CONFIG_BITS_SUBADDR 0x29

// LiFePO4/lead-acid absorb voltage adder, bits 15:6 are reserved
#define LTC4015_VABSORB_DELTA_SUBADDR 0x2A
// Maximum time for LiFePO4/lead-acid absorb charge
#define LTC4015_MAX_ABSORB_TIME_SUBADDR 0x2B
// Lead-acid equalize charge voltage adder, bits 15:6 are reserved - BITS[15:0] : 0x002A
#define LTC4015_VEQUALIZE_DELTA_SUBADDR 0x2C
// Lead-acid equalization time                                     - BITS[15:0] : 0x0E10
#define LTC4015_EQUALIZE_TIME_SUBADDR 0x2D
// LiFeP04 recharge threshold                                      - BITS[15:0] : 0x4410
#define LTC4015_LIFEPO4_RECHARGE_THRESHOLD_SUBADDR 0x2E
#define LTC4015_RESERVED_0X2F_SUBADDR 0x2F
// For lithium chemistries, indicates the time (in sec) that the battery has
// been charging
#define LTC4015_MAX_CHARGE_TIMER_SUBADDR 0x30
// For lithium chemistries, indicates the time (in sec) that the battery has
// been in constant-voltage regulation
#define LTC4015_CV_TIMER_SUBADDR 0x31
// For LiFePO4 and lead-acid batteries, indicates the time (in sec) that the
// battery has been in absorb phase
#define LTC4015_ABSORB_TIMER_SUBADDR 0x32
// For lead-acid batteries, indicates the time (in sec) that the battery has
// been in EQUALIZE phase
#define LTC4015_EQUALIZE_TIMER_SUBADDR 0x33

/*  Bit Fields:
 *
 *    15:3 : N/A
 *      10 : Indicates battery charger is in lead-acid equalization charge state
 *       9 : Indicates battery charger is in absorb charge state
 *       8 : Indicates battery charger is in charger suspended state
 *       7 : Indicates battery charger is in precondition charge state
 *       6 : Indicates battery charger is in CC-CV state
 *       5 : Indicates battery charger is in thermistor pause state
 *       4 : Indicates battery charger is in timer termination state
 *       3 : Indicates battery charger is in C/x termination state
 *       2 : indicates battery charger is in max_charge_time_fault state
 *       1 : Indicates battery charger is in missing battery fault state
 *       0 : Indicates battery charger is in shorted battery fault state
 */
// Real time battery charger state indicator. Individual bits are mutually
// exclusive. Bits 15:11 are reserved.
#define LTC4015_CHARGER_STATE_SUBADDR 0x34

/*  Bit Fields:
 *
 *    15:4 : N/A
 *       3 : Indicates the input undervoltage control loop is actively
 *           controlling power delivery based on VIN_UVCL_SETTING
 *       2 : Indicates the input current limit control loop is actively
 *           controlling power delivery based on IIN_LIMIT[_DAC][_SETTING]
 *       1 : Indicates the charge current control loop is actively controlling
 *           power delivery based on ICHARGE_DAC
 *       0 : Indicates the battery voltage control loop is actively controlling
 *           power delivery based on VCHARGE_DAC
 */
// Charge status indicator. Individual bits are mutually exclusive. Only active
// in charging states.
#define LTC4015_CHARGE_STATUS_SUBADDR 0x35

/*  Bit Fields:
 *
 *      15 : Indicates that measurement system results have become valid.
 *      14 : N/A
 *      13 : Indicates QCOUNT has fallen below QCOUNT_LO_ALERT_LIMIT
 *      12 : Indicates QCOUNT has exceeded QCOUNT_HI_ALERT_LIMIT
 *      11 : Indicates VBAT has fallen below VBAT_LO_ALERT_LIMIT
 *      10 : Indicates VBAT has exceeded VBAT_HI_ALERT_LIMIT
 *       9 : Indicates VIN has fallen below VIN_LO_ALERT_LIMIT
 *       8 : Indicates VIN has exceeded VIN_HI_ALERT_LIMIT
 *       7 : Indicates VSYS has fallen below VSYS_LO_ALERT_LIMIT
 *       6 : Indicates VSYS has exceeded VSYS_HI_ALERT_LIMIT
 *       5 : Indicates IIN has exceeded IIN_HI_ALERT_LIMIT
 *       4 : Indicates IBAT has fallen below IBAT_LO_ALERT_LIMIT
 *       3 : Indicates DIE_TEMP has exceeded DIE_TEMP_HI_ALERT_LIMIT
 *       2 : Indicates BSR has exceeded BSR_HI_ALERT_LIMIT
 *       1 : Indicates NTC_RATIO has exceeded NTC_RATIO_HI_ALERT_LIMIT
 *       0 : Indicates NTC_RATIO has fallen below NTC_RATIO_LO_ALERT_LIMIT
 */
// Limit alert register.Individual bits are enabled by EN_LIMIT_ALERTS (0x0D).
// Writing 0 to any bit clears that alert. Once set, alert bits remain high
// until cleared or disabled.
#define LTC4015_LIMIT_ALERTS_SUBADDR 0x36

/*  Bit Fields:
 *
 *   15:11 : N/A
 *      10 : Alert indicates charger has entered equalize charge state
 *       9 : Alert indicates charger has entered absorb charge state
 *       8 : Alert indicates charger has been suspended
 *       7 : Alert indicates charger has entered preconditioning charge state
 *       6 : Alert indicates charger has entered CC-CV charge state
 *       5 : Alert indicates charger has entered thermistor pause state
 *       4 : Alert indicates timer termination has occurred
 *       3 : Alert indicates C/x termination has occurred
 *       2 : Alert indicates charger has entered max_charge_time_fault state
 *       1 : Alert indicates battery missing fault has occurred
 *       0 : Alert indicates battery short fault has occurred
 */
// Charger state alert register. Individual bits are enabled by EN_CHARGER_STATE_ALERTS (0x0E).
#define LTC4015_CHARGER_STATE_ALERTS_SUBADDR 0x37

/*  Bit Fields:
 *
 *   15:4 : N/A
 *      3 : Alert indicates that vin_uvcl_active has occurred
 *      2 : Alert indicates iin_limit_active has occurred
 *      1 : Alert indicates constant_current has occurred
 *      0 : Alert indicates constant_voltage has occurred
 */
// Alerts that CHARGE_STATUS indicators have occurred.
// Individual bits are enabled by EN_CHARGE_STATUS_ALERTS (0x0F)
#define LTC4015_CHARGE_STATUS_ALERTS_SUBADDR 0x38

/*  Bit Fields:
 *
 *   15:14 : N/A
 *      13 : Indicates that the battery charger is active
 *      12 : N/A
 *      11 : Indicates the MPPT pin is set to enable Maximum Power Point Tracking
 *      10 : Indicates a rising edge has been detected at the EQ pin, and an
 *           equalize charge is queued
 *       9 : Indicates DRVCC voltage is above switching regulator undervoltage
 *           lockout level (4.3V typ)
 *       8 : Indicates an invalid combination of CELLS pin settings
 *       7 : N/A
 *       6 : Indicates all system conditions are met to allow battery charger
 *           operation.
 *       5 : Indicates no resistor has been detected at the RT pin
 *       4 : Indicates die temperature is greater than thermal shutdown level
 *           (160C typical)
 *       3 : Indicates VIN voltage is greater than overvoltage lockout level
 *           (38.6V typical)
 *       2 : Indicates VIN voltage is sufficiently greater than BATSENSE for
 *           switching regulator operation (200mV typical)
 *       1 : Indicates INTVCC voltage is above switching regulator undervoltage
 *           lockout level (4.3V typ)
 *       0 : Indicates INTVCC voltage is greater than measurement system lockout
 *           level (2.8V typical)
 */
// Real time system status indicator bits
#define LTC4015_SYSTEM_STATUS_SUBADDR 0x39

// Two’s complement ADC measurement result for the BATSENS pin.
// VBATSENS/cellcount = [VBAT] • 192.264uV for lithium chemistries.
// VBATSENS/cellcount = [VBAT] • 128.176uV for lead-acid.
#define LTC4015_VBAT_SUBADDR 0x3A

// Two’s complement ADC measurement result for VIN.
// VVIN = [VIN] • 1.648mV
#define LTC4015_VIN_SUBADDR 0x3B

// Two’s complement ADC measurement result for VSYS.
// VSYS = [VSYS] • 1.648mV
#define LTC4015_VSYS_SUBADDR 0x3C

// Two’s complement ADC measurement result for (VCSP – VCSN).
// Charge current (into the battery) is represented as a positive number.
// Battery current = [IBAT] • 1.46487uV/RSNSB
#define LTC4015_IBAT_SUBADDR 0x3D

// Two’s complement ADC measurement result for (VCLP – VCLN).
// Input current = [IIN] • 1.46487uV/RSNSI
#define LTC4015_IIN_SUBADDR 0x3E

// Two’s complement ADC measurement result for die temperature.
// Temperature = (DIE_TEMP – 12010)/45.6°C
#define LTC4015_DIE_TEMP_SUBADDR 0x3F

// Two’s complement ADC measurement result for NTC thermistor ratio.
// RNTC = NTC_RATIO • RNTCBIAS/(21845.0 – NTC_RATIO)
#define LTC4015_NTC_RATIO_SUBADDR 0x40

// Calculated battery series resistance.
// For lithium chemistries, series resistance/cellcount = BSR • RSNSB/500.0
// For lead-acid chemistries, series resistance/cellcount = BSR • RSNSB/750.0
#define LTC4015_BSR_SUBADDR 0x41

// JEITA temperature region of the NTC thermistor (Li Only).
// Active only when EN_JEITA=1 (Only Bits[2:0] used)
#define LTC4015_JEITA_REGION_SUBADDR 0x42

/*  Bit Fields:
 *
 *   15:12 : N/A
 *    11:8 : programmed battery chemistry
 *     7:4 : Reserved
 *     3:0 : Cell count as set by CELLS pins
 */
// Readout of CHEM and CELLS pin settings
#define LTC4015_CHEM_CELLS_SUBADDR 0x43
// Charge current control DAC control bits      (Only Bits[4:0] used)
#define LTC4015_ICHARGE_DAC_SUBADDR 0x44
// Charge voltage control DAC control bits      (Only Bits[5:0] used)
#define LTC4015_VCHARGE_DAC_SUBADDR 0x45
// Input current limit control DAC control word (Only Bits[5:0] used)
#define LTC4015_IIN_LIMIT_DAC_SUBADDR 0x46
// Digitally filtered two’s complement ADC measurement result for battery voltage
#define LTC4015_VBAT_FILT_SUBADDR 0x47
// This 16-bit two's complement word is the value of IBAT (0x3D) used in calculating BSR.
#define LTC4015_ICHARGE_BSR_SUBADDR 0x48
#define LTC4015_RESERVED_0X49_SUBADDR 0x49
// Measurement valid bit, bit 0 is a 1 when the telemetry(ADC) system is ready
#define LTC4015_MEAS_SYS_VALID_SUBADDR 0x4A

#endif /* CHARGECTRL_LTC4015_H_ */
