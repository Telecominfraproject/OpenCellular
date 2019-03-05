/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_ltc4015.h"

extern bool LTC4015_GpioPins[0x04];
extern const I2C_Dev I2C_DEV;
extern const LTC4015_Config fact_leadAcid_cfg;
extern const LTC4015_Config fact_lithiumIon_cfg;
extern LTC4015_Dev gbc_pwr_ext_bat_charger;
extern LTC4015_Dev gbc_pwr_int_bat_charger;
extern LTC4015_Dev gbc_pwr_invalid_bus;
extern LTC4015_Dev gbc_pwr_invalid_dev;
extern LTC4015_Dev gbc_pwr_invalid_leadAcid_cfg;
extern OcGpio_Port ec_io;
extern OcGpio_Port gbc_io_1;
extern uint32_t LTC4015_GpioConfig[0x04];
extern uint16_t LTC4015_regs[LTC4015_REG_VAILD_BIT_MESURMENT];
extern uint8_t SX1509_regs[SX1509_REG_TEST_2];

/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    FakeGpio_registerDevSimple(LTC4015_GpioPins, LTC4015_GpioConfig);
    fake_I2C_init();

    fake_I2C_registerDevSimple(gbc_pwr_int_bat_charger.cfg.i2c_dev.bus,
                               gbc_pwr_int_bat_charger.cfg.i2c_dev.slave_addr,
                               LTC4015_regs, sizeof(LTC4015_regs),
                               sizeof(LTC4015_regs[0]), sizeof(uint8_t),
                               FAKE_I2C_DEV_LITTLE_ENDIAN);

    fake_I2C_registerDevSimple(OC_CONNECT1_I2C0, BIGBROTHER_IOEXP1_ADDRESS,
                               SX1509_regs, sizeof(SX1509_regs),
                               sizeof(SX1509_regs[0]), sizeof(uint8_t),
                               FAKE_I2C_DEV_LITTLE_ENDIAN);
}

void setUp(void)
{
    memset(LTC4015_regs, 0, sizeof(LTC4015_regs));

    OcGpio_init(&gbc_io_1);

    LTC4015_init(&gbc_pwr_int_bat_charger);
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}
/* Parameters are not used as this is just used to test assigning the
   alert_handler right now.*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void OCMP_GenerateAlert(const AlertData *alert_data, unsigned int alert_id,
                        const void *data, const void *lValue,
                        OCMPActionType actionType)
{
}
#pragma GCC diagnostic pop
/* ================================ Tests =================================== */
/* Values are used in the below function are taken as per the datasheet*/
void test_ocmp_ltc4015_probe(void)
{
    POSTData postData;
    SX1509_regs[SX1509_REG_OPEN_DRAIN_A] = POST_DATA_NULL;

    /* Test with the actual value */
    LTC4015_regs[LTC4015_REG_SYSTEM_STATUS_INDICATOR] = LTC4015_CHARGER_ENABLED;
    TEST_ASSERT_EQUAL(POST_DEV_FOUND, LTC4015_fxnTable.cb_probe(
                                          &gbc_pwr_int_bat_charger, &postData));
    TEST_ASSERT_EQUAL_HEX8(SX1509_REG_OPEN_DRAIN_A_ENABLE,
                           SX1509_regs[SX1509_REG_OPEN_DRAIN_A]);
    TEST_ASSERT_EQUAL(gbc_pwr_int_bat_charger.cfg.i2c_dev.bus, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(gbc_pwr_int_bat_charger.cfg.i2c_dev.slave_addr,
                           postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(POST_MANID, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(POST_DEVID, postData.devId);

    SX1509_regs[SX1509_REG_OPEN_DRAIN_A] = POST_DATA_NULL;
    postData.i2cBus = POST_DATA_NULL;
    postData.devAddr = POST_DATA_NULL;
    postData.manId = POST_DATA_NULL;
    postData.devId = POST_DATA_NULL;
    /* Test with an incorrect value */
    LTC4015_regs[LTC4015_REG_SYSTEM_STATUS_INDICATOR] =
        ~LTC4015_CHARGER_ENABLED;
    TEST_ASSERT_EQUAL(
        POST_DEV_MISSING,
        LTC4015_fxnTable.cb_probe(&gbc_pwr_int_bat_charger, &postData));
    TEST_ASSERT_EQUAL_HEX8(SX1509_REG_OPEN_DRAIN_A_ENABLE,
                           SX1509_regs[SX1509_REG_OPEN_DRAIN_A]);
    TEST_ASSERT_EQUAL(POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devId);

    /* Test with a missing device */
    SX1509_regs[SX1509_REG_OPEN_DRAIN_A] = POST_DATA_NULL;
    TEST_ASSERT_EQUAL(POST_DEV_MISSING, LTC4015_fxnTable.cb_probe(
                                            &gbc_pwr_invalid_dev, &postData));
    TEST_ASSERT_EQUAL_HEX8(SX1509_REG_OPEN_DRAIN_A_ENABLE,
                           SX1509_regs[SX1509_REG_OPEN_DRAIN_A]);
    TEST_ASSERT_EQUAL(POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devId);

    /* Test with a missing bus */
    SX1509_regs[SX1509_REG_OPEN_DRAIN_A] = POST_DATA_NULL;
    TEST_ASSERT_EQUAL(POST_DEV_MISSING, LTC4015_fxnTable.cb_probe(
                                            &gbc_pwr_invalid_bus, &postData));
    TEST_ASSERT_EQUAL_HEX8(SX1509_REG_OPEN_DRAIN_A_ENABLE,
                           SX1509_regs[SX1509_REG_OPEN_DRAIN_A]);
    TEST_ASSERT_EQUAL(POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devId);

    /* Test for GPIO Configure Failure */
    SX1509_regs[SX1509_REG_OPEN_DRAIN_A] = POST_DATA_NULL;
    TEST_ASSERT_EQUAL(
        POST_DEV_NOSTATUS,
        LTC4015_fxnTable.cb_probe(&gbc_pwr_invalid_leadAcid_cfg, &postData));
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL,
                           SX1509_regs[SX1509_REG_OPEN_DRAIN_A]);
    TEST_ASSERT_EQUAL(POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devId);
}

void test_ocmp_ltc4015_init(void)
{
    /* Now try to init with a pin associated */
    AlertData alert_data = {
        .subsystem = 1,
        .componentId = 4,
        .deviceId = 1,
    };

    /* INIT for Lithium ION */
    TEST_ASSERT_EQUAL(POST_DEV_CFG_DONE,
                      LTC4015_fxnTable.cb_init(&gbc_pwr_int_bat_charger,
                                               &fact_lithiumIon_cfg,
                                               &alert_data));
    TEST_ASSERT_EQUAL(LTC4015_LION_INIT_BATTERY_VOLTAGE_LOW_LIMIT,
                      LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_LOW_LIMIT]);
    TEST_ASSERT_EQUAL(LTC4015_LION_INIT_BATTERY_VOLTAGE_HIGH_LIMIT,
                      LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_HIGH_LIMIT]);
    TEST_ASSERT_EQUAL(LTC4015_LION_INIT_CHARGE_CURRENT_LOW_LIMIT,
                      LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_LOW_LIMIT]);
    TEST_ASSERT_EQUAL(LTC4015_LION_INIT_INPUT_VOLTAGE_LOW_LIMIT,
                      LTC4015_regs[LTC4015_REG_INPUT_VOLTAGE_LOW_LIMIT]);
    TEST_ASSERT_EQUAL(LTC4015_LION_INIT_INPUT_CURRENT_HIGH_LIMIT,
                      LTC4015_regs[LTC4015_REG_INPUT_CURRENT_HIGH_LIMIT]);
    TEST_ASSERT_EQUAL(LTC4015_LION_INIT_INPUT_CURRENT_LIMIT_SETTING,
                      LTC4015_regs[LTC4015_REG_INPUT_CURRENT_LIMIT_SETTING]);
    TEST_ASSERT_EQUAL_HEX16(LTC4015_EVT_BVL | LTC4015_EVT_BVH |
                                LTC4015_EVT_IVL | LTC4015_EVT_ICH |
                                LTC4015_EVT_BCL,
                            LTC4015_regs[LTC4015_REG_ENABLE_LIMIT_MONITIOR]);
    TEST_ASSERT_EQUAL(LTC4015_EVT_NTCH,
                      LTC4015_regs[LTC4015_REG_ENABLE_CHARGER_STATE]);

    /* INIT for Lead Acid */
    TEST_ASSERT_EQUAL(POST_DEV_CFG_DONE, LTC4015_fxnTable.cb_init(
                                             &gbc_pwr_ext_bat_charger,
                                             &fact_leadAcid_cfg, &alert_data));
    TEST_ASSERT_EQUAL(LTC4015_LEAD_INIT_BATTERY_VOLTAGE_LOW_LIMIT,
                      LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_LOW_LIMIT]);
    TEST_ASSERT_EQUAL(LTC4015_LEAD_INIT_BATTERY_VOLTAGE_HIGH_LIMIT,
                      LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_HIGH_LIMIT]);
    TEST_ASSERT_EQUAL(LTC4015_LEAD_INIT_CHARGE_CURRENT_LOW_LIMIT,
                      LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_LOW_LIMIT]);
    TEST_ASSERT_EQUAL(LTC4015_LEAD_INIT_INPUT_VOLTAGE_LOW_LIMIT,
                      LTC4015_regs[LTC4015_REG_INPUT_VOLTAGE_LOW_LIMIT]);
    TEST_ASSERT_EQUAL(LTC4015_LEAD_INIT_INPUT_CURRENT_HIGH_LIMIT,
                      LTC4015_regs[LTC4015_REG_INPUT_CURRENT_HIGH_LIMIT]);
    TEST_ASSERT_EQUAL(LTC4015_LEAD_INIT_ICHARGE_TARGET,
                      LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_TARGET]);
    TEST_ASSERT_EQUAL(LTC4015_LEAD_INIT_VCHARGE_SETTING,
                      LTC4015_regs[LTC4015_REG_VCHARGE_SETTING]);
    TEST_ASSERT_EQUAL_HEX16(LTC4015_EVT_BVL | LTC4015_EVT_BVH |
                                LTC4015_EVT_IVL | LTC4015_EVT_ICH |
                                LTC4015_EVT_BCL,
                            LTC4015_regs[LTC4015_REG_ENABLE_LIMIT_MONITIOR]);
    TEST_ASSERT_EQUAL(LTC4015_EVT_NTCH,
                      LTC4015_regs[LTC4015_REG_ENABLE_CHARGER_STATE]);

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(POST_DEV_CFG_FAIL,
                      LTC4015_fxnTable.cb_init(&gbc_pwr_invalid_dev,
                                               &fact_lithiumIon_cfg,
                                               &alert_data));

    /* Test with a missing bus */
    TEST_ASSERT_EQUAL(POST_DEV_CFG_FAIL,
                      LTC4015_fxnTable.cb_init(&gbc_pwr_invalid_bus,
                                               &fact_lithiumIon_cfg,
                                               &alert_data));

    /* Test for _choose_battery_charge false */
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_init(&gbc_pwr_invalid_leadAcid_cfg,
                                        &fact_lithiumIon_cfg, &alert_data));
}

void test_ocmp_ltc4015_battery_voltage_get_status(void)
{
    int16_t batteryVoltage = 0;
    int16_t expectedValue = 0;

    /* Battery Voltage(VBATSENS/cellcount) = [VBAT] <95> 192.264(uV) */
    /* Test lithium ion chemistry */
    LTC4015_regs[LTC4015_REG_VBAT_VALUE] = 15603;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_status(&gbc_pwr_int_bat_charger,
                                             LTC4015_STATUS_BATTERY_VOLTAGE,
                                             &batteryVoltage));
    expectedValue = ocmp_ltc4015_get_battery_voltage(
        &gbc_pwr_int_bat_charger, LTC4015_regs[LTC4015_REG_VBAT_VALUE]);
    TEST_ASSERT_EQUAL(expectedValue, batteryVoltage);

    /* Test lead acid chemistry */
    LTC4015_regs[LTC4015_REG_VBAT_VALUE] = 17945;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_status(&gbc_pwr_ext_bat_charger,
                                             LTC4015_STATUS_BATTERY_VOLTAGE,
                                             &batteryVoltage));
    expectedValue = ocmp_ltc4015_get_battery_voltage(
        &gbc_pwr_ext_bat_charger, LTC4015_regs[LTC4015_REG_VBAT_VALUE]);
    TEST_ASSERT_EQUAL(expectedValue, batteryVoltage);

    /* Test with a missing device */
    LTC4015_regs[LTC4015_REG_VBAT_VALUE] = 15603;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_get_status(&gbc_pwr_invalid_dev,
                                              LTC4015_STATUS_BATTERY_VOLTAGE,
                                              &batteryVoltage));

    /* Test with a missing bus */
    LTC4015_regs[LTC4015_REG_VBAT_VALUE] = 15603;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_get_status(&gbc_pwr_invalid_bus,
                                              LTC4015_STATUS_BATTERY_VOLTAGE,
                                              &batteryVoltage));
}

void test_ocmp_ltc4015_battery_current_get_status(void)
{
    int16_t batteryCurrent = 0;
    int16_t expectedValue = 0;

    /* IBAT_LO_ALERT_LIMIT = (limit*RSNSB)/1.46487uV */
    /* Test lithium ion chemistry */
    LTC4015_regs[LTC4015_REG_BATTEY_CURRENT] = 2048;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_status(&gbc_pwr_int_bat_charger,
                                             LTC4015_STATUS_BATTERY_CURRENT,
                                             &batteryCurrent));
    expectedValue = ocmp_ltc4015_get_battery_current(
        &gbc_pwr_int_bat_charger, LTC4015_regs[LTC4015_REG_BATTEY_CURRENT]);
    TEST_ASSERT_EQUAL(expectedValue, batteryCurrent);

    /* Test lead acid chemistry */
    LTC4015_regs[LTC4015_REG_BATTEY_CURRENT] = 8276;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_status(&gbc_pwr_ext_bat_charger,
                                             LTC4015_STATUS_BATTERY_CURRENT,
                                             &batteryCurrent));
    expectedValue = ocmp_ltc4015_get_battery_current(
        &gbc_pwr_ext_bat_charger, LTC4015_regs[LTC4015_REG_BATTEY_CURRENT]);
    TEST_ASSERT_EQUAL(expectedValue, batteryCurrent);

    /* Test with a missing device */
    LTC4015_regs[LTC4015_REG_BATTEY_CURRENT] = 2048;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_get_status(&gbc_pwr_invalid_dev,
                                              LTC4015_STATUS_BATTERY_CURRENT,
                                              &batteryCurrent));

    /* Test with a missing bus */
    LTC4015_regs[LTC4015_REG_BATTEY_CURRENT] = 2048;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_get_status(&gbc_pwr_invalid_bus,
                                              LTC4015_STATUS_BATTERY_CURRENT,
                                              &batteryCurrent));
}

void test_ocmp_ltc4015_system_voltage_get_status(void)
{
    int16_t systemVoltage = 0;
    int16_t expectedValue = 0;

    /* System voltage = [VSYS] <95> 1.648mV */
    /* Test lithium ion chemistry */
    LTC4015_regs[LTC4015_REG_VSYS] = 7000;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_status(
                                &gbc_pwr_int_bat_charger,
                                LTC4015_STATUS_SYSTEM_VOLTAGE, &systemVoltage));
    expectedValue =
        ocmp_ltc4015_get_system_voltage(LTC4015_regs[LTC4015_REG_VSYS]);
    TEST_ASSERT_EQUAL(expectedValue, systemVoltage);

    /* Test lead acid chemistry */
    LTC4015_regs[LTC4015_REG_VSYS] = 12500;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_status(
                                &gbc_pwr_ext_bat_charger,
                                LTC4015_STATUS_SYSTEM_VOLTAGE, &systemVoltage));
    expectedValue =
        ocmp_ltc4015_get_system_voltage(LTC4015_regs[LTC4015_REG_VSYS]);
    TEST_ASSERT_EQUAL(expectedValue, systemVoltage);

    /* Test with a missing device */
    LTC4015_regs[LTC4015_REG_VSYS] = 12500;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_get_status(&gbc_pwr_invalid_dev,
                                              LTC4015_STATUS_SYSTEM_VOLTAGE,
                                              &systemVoltage));

    /* Test with a missing bus */
    LTC4015_regs[LTC4015_REG_VSYS] = 7000;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_get_status(&gbc_pwr_invalid_bus,
                                              LTC4015_STATUS_SYSTEM_VOLTAGE,
                                              &systemVoltage));
}

void test_ocmp_ltc4015_input_voltage_get_status(void)
{
    int16_t inputVoltage = 0;
    int16_t expectedValue = 0;

    /* VIN_LO_ALERT_LIMIT = limit/1.648mV */
    /* Test lithium ion chemistry */
    LTC4015_regs[LTC4015_REG_VIN] = 3034;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_status(
                                &gbc_pwr_int_bat_charger,
                                LTC4015_STATUS_INPUT_VOLATGE, &inputVoltage));
    expectedValue =
        ocmp_ltc4015_get_input_voltage(LTC4015_regs[LTC4015_REG_VIN]);
    TEST_ASSERT_EQUAL(expectedValue, inputVoltage);

    /* Test lead acid chemistry */
    LTC4015_regs[LTC4015_REG_VIN] = 60;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_status(
                                &gbc_pwr_ext_bat_charger,
                                LTC4015_STATUS_INPUT_VOLATGE, &inputVoltage));
    expectedValue =
        ocmp_ltc4015_get_input_voltage(LTC4015_regs[LTC4015_REG_VIN]);
    TEST_ASSERT_EQUAL(expectedValue, inputVoltage);

    /* Test with a missing device */
    LTC4015_regs[LTC4015_REG_VIN] = 60;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                                 &gbc_pwr_invalid_dev,
                                 LTC4015_STATUS_INPUT_VOLATGE, &inputVoltage));

    /* Test with a missing bus */
    LTC4015_regs[LTC4015_REG_VIN] = 3034;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                                 &gbc_pwr_invalid_bus,
                                 LTC4015_STATUS_INPUT_VOLATGE, &inputVoltage));
}

void test_ocmp_ltc4015_input_current_get_status(void)
{
    int16_t inputCurrent = 0;
    int16_t expectedValue = 0;

    /* VIN_LO_ALERT_LIMIT = limit/1.648mV */
    /* Test lithium ion chemistry */
    /* IIN_HI_ALERT_LIMIT = (limit*RSNSI)/1.46487uV */
    LTC4015_regs[LTC4015_REG_INPUT_CURRENT] = 23892;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_status(
                                &gbc_pwr_int_bat_charger,
                                LTC4015_STATUS_INPUT_CURRENT, &inputCurrent));

    expectedValue = ocmp_ltc4015_get_input_current(
        &gbc_pwr_int_bat_charger, LTC4015_regs[LTC4015_REG_INPUT_CURRENT]);
    TEST_ASSERT_EQUAL(expectedValue, inputCurrent);

    /* Test lead acid chemistry */
    LTC4015_regs[LTC4015_REG_INPUT_CURRENT] = 23211;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_status(
                                &gbc_pwr_ext_bat_charger,
                                LTC4015_STATUS_INPUT_CURRENT, &inputCurrent));
    expectedValue = ocmp_ltc4015_get_input_current(
        &gbc_pwr_ext_bat_charger, LTC4015_regs[LTC4015_REG_INPUT_CURRENT]);
    TEST_ASSERT_EQUAL(expectedValue, inputCurrent);

    /* Test with a missing device */
    LTC4015_regs[LTC4015_REG_INPUT_CURRENT] = 23892;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                                 &gbc_pwr_invalid_dev,
                                 LTC4015_STATUS_INPUT_CURRENT, &inputCurrent));

    /* Test with a missing bus */
    LTC4015_regs[LTC4015_REG_INPUT_CURRENT] = 23892;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                                 &gbc_pwr_invalid_bus,
                                 LTC4015_STATUS_INPUT_CURRENT, &inputCurrent));
}

void test_ocmp_ltc4015_dia_temperature_get_status(void)
{
    int16_t dieTemperature = 0;
    int16_t expectedValue = 0;

    /* IIN_HI_ALERT_LIMIT = (limit*RSNSI)/1.46487uV */
    /* Test lithium ion chemistry */
    LTC4015_regs[LTC4015_REG_DIE_TEMPERATURE] = 13606;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_status(&gbc_pwr_int_bat_charger,
                                             LTC4015_STATUS_DIE_TEMPERATURE,
                                             &dieTemperature));
    expectedValue = ocmp_ltc4015_get_dia_temperature(
        LTC4015_regs[LTC4015_REG_DIE_TEMPERATURE]);
    TEST_ASSERT_EQUAL(expectedValue, dieTemperature);

    /* Test lead acid chemistry */
    LTC4015_regs[LTC4015_REG_DIE_TEMPERATURE] = 15666;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_status(&gbc_pwr_int_bat_charger,
                                             LTC4015_STATUS_DIE_TEMPERATURE,
                                             &dieTemperature));
    expectedValue = ocmp_ltc4015_get_dia_temperature(
        LTC4015_regs[LTC4015_REG_DIE_TEMPERATURE]);
    TEST_ASSERT_EQUAL(expectedValue, dieTemperature);

    /* Test with a missing device */
    LTC4015_regs[LTC4015_REG_DIE_TEMPERATURE] = 15666;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_get_status(&gbc_pwr_invalid_dev,
                                              LTC4015_STATUS_DIE_TEMPERATURE,
                                              &dieTemperature));

    /* Test with a missing bus */
    LTC4015_regs[LTC4015_REG_DIE_TEMPERATURE] = 13606;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_get_status(&gbc_pwr_invalid_bus,
                                              LTC4015_STATUS_DIE_TEMPERATURE,
                                              &dieTemperature));
}

void test_ocmp_ltc4015_icharge_dac_get_status(void)
{
    int16_t iCharge = 0;
    int16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    LTC4015_regs[LTC4015_REG_CHARGE_CUEERNT_DAC_CONTROL] = 31;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_status(
                                &gbc_pwr_int_bat_charger,
                                LTC4015_STATUS_ICHARGE_DAC, &iCharge));
    expectedValue = ocmp_ltc4015_get_icharge_dac(
        &gbc_pwr_int_bat_charger,
        LTC4015_regs[LTC4015_REG_CHARGE_CUEERNT_DAC_CONTROL]);
    TEST_ASSERT_EQUAL(expectedValue, iCharge);

    /* Test lead acid chemistry */
    LTC4015_regs[LTC4015_REG_CHARGE_CUEERNT_DAC_CONTROL] = 31;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_status(
                                &gbc_pwr_ext_bat_charger,
                                LTC4015_STATUS_ICHARGE_DAC, &iCharge));
    expectedValue = ocmp_ltc4015_get_icharge_dac(
        &gbc_pwr_ext_bat_charger,
        LTC4015_regs[LTC4015_REG_CHARGE_CUEERNT_DAC_CONTROL]);
    TEST_ASSERT_EQUAL(expectedValue, iCharge);

    /* Test with a missing device */
    LTC4015_regs[LTC4015_REG_CHARGE_CUEERNT_DAC_CONTROL] = 31;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_get_status(
                   &gbc_pwr_invalid_dev, LTC4015_STATUS_ICHARGE_DAC, &iCharge));

    /* Test with a missing bus */
    LTC4015_regs[LTC4015_REG_CHARGE_CUEERNT_DAC_CONTROL] = 31;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_get_status(
                   &gbc_pwr_invalid_bus, LTC4015_STATUS_ICHARGE_DAC, &iCharge));
}

void test_ocmp_ltc4015_invalid_param_get_status(void)
{
    int16_t buffer = 0;

    /*Test with invalid paramId*/
    LTC4015_regs[LTC4015_REG_CHARGE_CUEERNT_DAC_CONTROL] = 31;
    buffer = 0;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                                 &gbc_pwr_int_bat_charger, 7, &buffer));
    TEST_ASSERT_EQUAL(0, buffer);
}

void test_ocmp_ltc4015_get_cfg_battery_voltage_low(void)
{
    int16_t batteryVoltageLow = 0;
    int16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_LOW_LIMIT] = 15603;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_config(&gbc_pwr_int_bat_charger,
                                             LTC4015_CONFIG_BATTERY_VOLTAGE_LOW,
                                             &batteryVoltageLow));
    expectedValue = ocmp_ltc4015_get_battery_voltage(
        &gbc_pwr_int_bat_charger,
        LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_LOW_LIMIT]);
    TEST_ASSERT_EQUAL(expectedValue, batteryVoltageLow);

    /* Test lead acid chemistry */
    LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_LOW_LIMIT] = 13458;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_BATTERY_VOLTAGE_LOW,
                                             &batteryVoltageLow));
    expectedValue = ocmp_ltc4015_get_battery_voltage(
        &gbc_pwr_ext_bat_charger,
        LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_LOW_LIMIT]);
    TEST_ASSERT_EQUAL(expectedValue, batteryVoltageLow);

    /* Test with a missing device */
    LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_LOW_LIMIT] = 13458;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                                 &gbc_pwr_invalid_dev,
                                 LTC4015_CONFIG_BATTERY_VOLTAGE_LOW,
                                 &batteryVoltageLow));

    /* Test with a missing bus */
    LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_LOW_LIMIT] = 13458;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                                 &gbc_pwr_invalid_bus,
                                 LTC4015_CONFIG_BATTERY_VOLTAGE_LOW,
                                 &batteryVoltageLow));
}

void test_ocmp_ltc4015_get_cfg_battery_voltage_high(void)
{
    int16_t batteryVoltageHigh = 0;
    int16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_HIGH_LIMIT] = 21845;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_config(
                                &gbc_pwr_int_bat_charger,
                                LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH,
                                &batteryVoltageHigh));
    expectedValue = ocmp_ltc4015_get_battery_voltage(
        &gbc_pwr_int_bat_charger,
        LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_HIGH_LIMIT]);
    TEST_ASSERT_EQUAL(expectedValue, batteryVoltageHigh);

    /* Test lead acid chemistry */
    LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_HIGH_LIMIT] = 17945;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_config(
                                &gbc_pwr_ext_bat_charger,
                                LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH,
                                &batteryVoltageHigh));
    expectedValue = ocmp_ltc4015_get_battery_voltage(
        &gbc_pwr_ext_bat_charger,
        LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_HIGH_LIMIT]);
    TEST_ASSERT_EQUAL(expectedValue, batteryVoltageHigh);

    /* Test with a missing device */
    LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_HIGH_LIMIT] = 17945;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                                 &gbc_pwr_invalid_dev,
                                 LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH,
                                 &batteryVoltageHigh));

    /* Test with a missing bus */
    LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_HIGH_LIMIT] = 17945;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                                 &gbc_pwr_invalid_bus,
                                 LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH,
                                 &batteryVoltageHigh));
}

void test_ocmp_ltc4015_get_cfg_battery_current_low(void)
{
    int16_t batteryCurrentLow = 0;
    int16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_LOW_LIMIT] = 2048;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_config(&gbc_pwr_int_bat_charger,
                                             LTC4015_CONFIG_BATTERY_CURRENT_LOW,
                                             &batteryCurrentLow));
    expectedValue = ocmp_ltc4015_get_cfg_battery_current_low(
        &gbc_pwr_int_bat_charger,
        LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_LOW_LIMIT]);
    TEST_ASSERT_EQUAL(expectedValue, batteryCurrentLow);

    /* Test lead acid chemistry */
    LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_LOW_LIMIT] = 8276;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_BATTERY_CURRENT_LOW,
                                             &batteryCurrentLow));
    expectedValue = ocmp_ltc4015_get_cfg_battery_current_low(
        &gbc_pwr_ext_bat_charger,
        LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_LOW_LIMIT]);
    TEST_ASSERT_EQUAL(expectedValue, batteryCurrentLow);

    /* Test with a missing device */
    LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_LOW_LIMIT] = 8276;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                                 &gbc_pwr_invalid_dev,
                                 LTC4015_CONFIG_BATTERY_CURRENT_LOW,
                                 &batteryCurrentLow));

    /* Test with a missing bus */
    LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_LOW_LIMIT] = 8276;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                                 &gbc_pwr_invalid_bus,
                                 LTC4015_CONFIG_BATTERY_CURRENT_LOW,
                                 &batteryCurrentLow));
}

void test_ocmp_ltc4015_get_cfg_input_voltage_low(void)
{
    int16_t inputVoltageLow = 0;
    int16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    LTC4015_regs[LTC4015_REG_INPUT_VOLTAGE_LOW_LIMIT] = 60;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_config(&gbc_pwr_int_bat_charger,
                                             LTC4015_CONFIG_INPUT_VOLTAGE_LOW,
                                             &inputVoltageLow));
    expectedValue = ocmp_ltc4015_get_cfg_input_voltage_low(
        LTC4015_regs[LTC4015_REG_INPUT_VOLTAGE_LOW_LIMIT]);
    TEST_ASSERT_EQUAL(expectedValue, inputVoltageLow);

    /* Test lead acid chemistry */
    LTC4015_regs[LTC4015_REG_INPUT_VOLTAGE_LOW_LIMIT] = 60;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_INPUT_VOLTAGE_LOW,
                                             &inputVoltageLow));
    expectedValue = ocmp_ltc4015_get_cfg_input_voltage_low(
        LTC4015_regs[LTC4015_REG_INPUT_VOLTAGE_LOW_LIMIT]);
    TEST_ASSERT_EQUAL(expectedValue, inputVoltageLow);

    /* Test with a missing device */
    LTC4015_regs[LTC4015_REG_INPUT_VOLTAGE_LOW_LIMIT] = 60;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_get_config(&gbc_pwr_invalid_dev,
                                              LTC4015_CONFIG_INPUT_VOLTAGE_LOW,
                                              &inputVoltageLow));

    /* Test with a missing bus */
    LTC4015_regs[LTC4015_REG_INPUT_VOLTAGE_LOW_LIMIT] = 60;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_get_config(&gbc_pwr_invalid_bus,
                                              LTC4015_CONFIG_INPUT_VOLTAGE_LOW,
                                              &inputVoltageLow));
}

void test_ocmp_ltc4015_get_cfg_input_current_high(void)
{
    int16_t inputCurrentHigh = 0;
    int16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    LTC4015_regs[LTC4015_REG_INPUT_CURRENT_HIGH_LIMIT] = 23211;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_config(&gbc_pwr_int_bat_charger,
                                             LTC4015_CONFIG_INPUT_CURRENT_HIGH,
                                             &inputCurrentHigh));
    expectedValue = ocmp_ltc4015_get_cfg_input_current_high(
        &gbc_pwr_int_bat_charger,
        LTC4015_regs[LTC4015_REG_INPUT_CURRENT_HIGH_LIMIT]);
    TEST_ASSERT_EQUAL(expectedValue, inputCurrentHigh);

    /* Test lead acid chemistry */
    LTC4015_regs[LTC4015_REG_INPUT_CURRENT_HIGH_LIMIT] = 23211;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_INPUT_CURRENT_HIGH,
                                             &inputCurrentHigh));
    expectedValue = ocmp_ltc4015_get_cfg_input_current_high(
        &gbc_pwr_ext_bat_charger,
        LTC4015_regs[LTC4015_REG_INPUT_CURRENT_HIGH_LIMIT]);
    TEST_ASSERT_EQUAL(expectedValue, inputCurrentHigh);

    /* Test with a missing device */
    LTC4015_regs[LTC4015_REG_INPUT_CURRENT_HIGH_LIMIT] = 23211;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_get_config(&gbc_pwr_invalid_dev,
                                              LTC4015_CONFIG_INPUT_CURRENT_HIGH,
                                              &inputCurrentHigh));

    /* Test with a missing bus */
    LTC4015_regs[LTC4015_REG_INPUT_CURRENT_HIGH_LIMIT] = 23211;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_get_config(&gbc_pwr_invalid_bus,
                                              LTC4015_CONFIG_INPUT_CURRENT_HIGH,
                                              &inputCurrentHigh));
}

void test_ocmp_ltc4015_get_cfg_input_current_limit(void)
{
    uint16_t inputCurrentLimit = 0;
    uint16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    LTC4015_regs[LTC4015_REG_INPUT_CURRENT_LIMIT_SETTING] = 76;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_config(&gbc_pwr_int_bat_charger,
                                             LTC4015_CONFIG_INPUT_CURRENT_LIMIT,
                                             &inputCurrentLimit));
    expectedValue = ocmp_ltc4015_get_cfg_input_current_limit(
        &gbc_pwr_int_bat_charger,
        LTC4015_regs[LTC4015_REG_INPUT_CURRENT_LIMIT_SETTING]);
    TEST_ASSERT_EQUAL(expectedValue, inputCurrentLimit);

    /* Test lead acid chemistry */
    LTC4015_regs[LTC4015_REG_INPUT_CURRENT_LIMIT_SETTING] = 200;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_INPUT_CURRENT_LIMIT,
                                             &inputCurrentLimit));
    expectedValue = ocmp_ltc4015_get_cfg_input_current_limit(
        &gbc_pwr_ext_bat_charger,
        LTC4015_regs[LTC4015_REG_INPUT_CURRENT_LIMIT_SETTING]);
    TEST_ASSERT_EQUAL(expectedValue, inputCurrentLimit);

    /* Test with a missing device */
    LTC4015_regs[LTC4015_REG_INPUT_CURRENT_LIMIT_SETTING] = 200;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                                 &gbc_pwr_invalid_dev,
                                 LTC4015_CONFIG_INPUT_CURRENT_LIMIT,
                                 &inputCurrentLimit));

    /* Test with a missing bus */
    LTC4015_regs[LTC4015_REG_INPUT_CURRENT_LIMIT_SETTING] = 200;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                                 &gbc_pwr_invalid_bus,
                                 LTC4015_CONFIG_INPUT_CURRENT_LIMIT,
                                 &inputCurrentLimit));
}

void test_ocmp_ltc4015_get_cfg_icharge(void)
{
    uint16_t icharge = 0;
    uint16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_TARGET] = 8;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_config(&gbc_pwr_int_bat_charger,
                                             LTC4015_CONFIG_ICHARGE, &icharge));
    expectedValue = ocmp_ltc4015_get_cfg_icharge(
        &gbc_pwr_int_bat_charger,
        LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_TARGET]);
    TEST_ASSERT_EQUAL(expectedValue, icharge);

    /* Test lead acid chemistry */
    LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_TARGET] = 11;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_ICHARGE, &icharge));
    expectedValue = ocmp_ltc4015_get_cfg_icharge(
        &gbc_pwr_ext_bat_charger,
        LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_TARGET]);
    TEST_ASSERT_EQUAL(expectedValue, icharge);

    /* Test with a missing device */
    LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_TARGET] = 12;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_get_config(
                   &gbc_pwr_invalid_dev, LTC4015_CONFIG_ICHARGE, &icharge));

    /* Test with a missing bus */
    LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_TARGET] = 12;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_get_config(
                   &gbc_pwr_invalid_bus, LTC4015_CONFIG_ICHARGE, &icharge));
}

void test_ocmp_ltc4015_get_cfg_vcharge(void)
{
    uint16_t vcharge = 0;
    uint16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    LTC4015_regs[LTC4015_REG_VCHARGE_SETTING] = 31;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_config(&gbc_pwr_int_bat_charger,
                                             LTC4015_CONFIG_VCHARGE, &vcharge));
    expectedValue = ocmp_ltc4015_get_cfg_vcharge(
        &gbc_pwr_int_bat_charger, LTC4015_regs[LTC4015_REG_VCHARGE_SETTING]);
    TEST_ASSERT_EQUAL(expectedValue, vcharge);

    /* Test lead acid chemistry */
    LTC4015_regs[LTC4015_REG_VCHARGE_SETTING] = 60;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_get_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_VCHARGE, &vcharge));
    expectedValue = ocmp_ltc4015_get_cfg_vcharge(
        &gbc_pwr_ext_bat_charger, LTC4015_regs[LTC4015_REG_VCHARGE_SETTING]);
    TEST_ASSERT_EQUAL(expectedValue, vcharge);

    /* Test with a missing device */
    LTC4015_regs[LTC4015_REG_VCHARGE_SETTING] = 11;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_get_config(
                   &gbc_pwr_invalid_dev, LTC4015_CONFIG_VCHARGE, &vcharge));

    /* Test with a missing bus */
    LTC4015_regs[LTC4015_REG_VCHARGE_SETTING] = 11;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_get_config(
                   &gbc_pwr_invalid_bus, LTC4015_CONFIG_VCHARGE, &vcharge));
}

void test_ocmp_ltc4015_get_cfg_die_temperature_high(void)
{
    int16_t dieTempHigh = 0;
    int16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    LTC4015_regs[LTC4015_REG_DIE_TEMP_HIGH_LIMIT] = 15658;
    TEST_ASSERT_EQUAL(true,
                      LTC4015_fxnTable.cb_get_config(
                          &gbc_pwr_int_bat_charger,
                          LTC4015_CONFIG_DIE_TEMPERATURE_HIGH, &dieTempHigh));
    expectedValue = ocmp_ltc4015_get_cfg_die_temperature_high(
        LTC4015_regs[LTC4015_REG_DIE_TEMP_HIGH_LIMIT]);
    TEST_ASSERT_EQUAL(expectedValue, dieTempHigh);

    /* Test lead acid chemistry */
    LTC4015_regs[LTC4015_REG_DIE_TEMP_HIGH_LIMIT] = 11554;
    TEST_ASSERT_EQUAL(true,
                      LTC4015_fxnTable.cb_get_config(
                          &gbc_pwr_ext_bat_charger,
                          LTC4015_CONFIG_DIE_TEMPERATURE_HIGH, &dieTempHigh));
    expectedValue = ocmp_ltc4015_get_cfg_die_temperature_high(
        LTC4015_regs[LTC4015_REG_DIE_TEMP_HIGH_LIMIT]);
    TEST_ASSERT_EQUAL(expectedValue, dieTempHigh);

    /* Test with a missing device */
    LTC4015_regs[LTC4015_REG_DIE_TEMP_HIGH_LIMIT] = 15658;
    TEST_ASSERT_EQUAL(false,
                      LTC4015_fxnTable.cb_get_config(
                          &gbc_pwr_invalid_dev,
                          LTC4015_CONFIG_DIE_TEMPERATURE_HIGH, &dieTempHigh));

    /* Test with a missing bus */
    LTC4015_regs[LTC4015_REG_DIE_TEMP_HIGH_LIMIT] = 15658;
    TEST_ASSERT_EQUAL(false,
                      LTC4015_fxnTable.cb_get_config(
                          &gbc_pwr_invalid_bus,
                          LTC4015_CONFIG_DIE_TEMPERATURE_HIGH, &dieTempHigh));
}

void test_ocmp_ltc4015_get_cfg_invalid_param(void)
{
    int16_t buffer = 0;

    /*Test with invalid paramId*/
    LTC4015_regs[LTC4015_REG_DIE_TEMP_HIGH_LIMIT] = 15658;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                                 &gbc_pwr_ext_bat_charger, 9, &buffer));
    TEST_ASSERT_EQUAL(0, buffer);
}

void test_ocmp_ltc4015_set_cfg_battery_voltage_low(void)
{
    int16_t batteryVolLow = 0;
    uint16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    batteryVolLow = 9000;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_set_config(&gbc_pwr_int_bat_charger,
                                             LTC4015_CONFIG_BATTERY_VOLTAGE_LOW,
                                             &batteryVolLow));
    expectedValue = ocmp_ltc4015_set_cfg_battery_voltage(
        &gbc_pwr_int_bat_charger, batteryVolLow);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_LOW_LIMIT]);

    /* Test lead acid chemistry */
    batteryVolLow = 10350;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_set_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_BATTERY_VOLTAGE_LOW,
                                             &batteryVolLow));
    expectedValue = ocmp_ltc4015_set_cfg_battery_voltage(
        &gbc_pwr_ext_bat_charger, batteryVolLow);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_LOW_LIMIT]);

    /* Make sure too-low values don't do anything bad */
    batteryVolLow = 100;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_set_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_BATTERY_VOLTAGE_LOW,
                                             &batteryVolLow));
    expectedValue = ocmp_ltc4015_set_cfg_battery_voltage(
        &gbc_pwr_ext_bat_charger, batteryVolLow);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_LOW_LIMIT]);

    /* Test with a missing device */
    batteryVolLow = 10350;
    TEST_ASSERT_EQUAL(false,
                      LTC4015_fxnTable.cb_set_config(
                          &gbc_pwr_invalid_dev,
                          LTC4015_CONFIG_BATTERY_VOLTAGE_LOW, &batteryVolLow));

    /* Test with a missing bus */
    batteryVolLow = 10350;
    TEST_ASSERT_EQUAL(false,
                      LTC4015_fxnTable.cb_set_config(
                          &gbc_pwr_invalid_bus,
                          LTC4015_CONFIG_BATTERY_VOLTAGE_LOW, &batteryVolLow));
}

void test_ocmp_ltc4015_set_cfg_battery_voltage_high(void)
{
    int16_t batteryVolHigh = 0;
    int16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    batteryVolHigh = 12600;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                                &gbc_pwr_int_bat_charger,
                                LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH,
                                &batteryVolHigh));
    expectedValue = ocmp_ltc4015_set_cfg_battery_voltage(
        &gbc_pwr_int_bat_charger, batteryVolHigh);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_HIGH_LIMIT]);

    /* Test lead acid chemistry */
    batteryVolHigh = 13800;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                                &gbc_pwr_ext_bat_charger,
                                LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH,
                                &batteryVolHigh));
    expectedValue = ocmp_ltc4015_set_cfg_battery_voltage(
        &gbc_pwr_ext_bat_charger, batteryVolHigh);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_HIGH_LIMIT]);

    /*  Make sure too-high values don't do anything bad */
    batteryVolHigh = 35500;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                                &gbc_pwr_ext_bat_charger,
                                LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH,
                                &batteryVolHigh));
    expectedValue = ocmp_ltc4015_set_cfg_battery_voltage(
        &gbc_pwr_ext_bat_charger, batteryVolHigh);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_BATTERY_VOLTAGE_HIGH_LIMIT]);

    /* Test with a missing device */
    batteryVolHigh = 13800;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                                 &gbc_pwr_invalid_dev,
                                 LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH,
                                 &batteryVolHigh));

    /* Test with a missing bus */
    batteryVolHigh = 13800;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                                 &gbc_pwr_invalid_bus,
                                 LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH,
                                 &batteryVolHigh));
}

void test_ocmp_ltc4015_set_cfg_battery_current_low(void)
{
    int16_t batteryCurrentLow = 0;
    int16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    batteryCurrentLow = 100;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_set_config(&gbc_pwr_int_bat_charger,
                                             LTC4015_CONFIG_BATTERY_CURRENT_LOW,
                                             &batteryCurrentLow));
    expectedValue = ocmp_ltc4015_set_cfg_battery_current_low(
        &gbc_pwr_int_bat_charger, batteryCurrentLow);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_LOW_LIMIT]);

    /* Test lead acid chemistry */
    batteryCurrentLow = 2000;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_set_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_BATTERY_CURRENT_LOW,
                                             &batteryCurrentLow));
    expectedValue = ocmp_ltc4015_set_cfg_battery_current_low(
        &gbc_pwr_ext_bat_charger, batteryCurrentLow);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_LOW_LIMIT]);

    /* Make sure too-low values don't do anything bad */
    batteryCurrentLow = 20;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_set_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_BATTERY_CURRENT_LOW,
                                             &batteryCurrentLow));
    expectedValue = ocmp_ltc4015_set_cfg_battery_current_low(
        &gbc_pwr_ext_bat_charger, batteryCurrentLow);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_LOW_LIMIT]);

    /* Test with a missing device */
    batteryCurrentLow = 4041;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                                 &gbc_pwr_invalid_dev,
                                 LTC4015_CONFIG_BATTERY_CURRENT_LOW,
                                 &batteryCurrentLow));

    /* Test with a missing bus */
    batteryCurrentLow = 4041;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                                 &gbc_pwr_invalid_bus,
                                 LTC4015_CONFIG_BATTERY_CURRENT_LOW,
                                 &batteryCurrentLow));
}

void test_ocmp_ltc4015_set_cfg_input_voltage_low(void)
{
    int16_t inputVoltageLow = 0;
    int16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    inputVoltageLow = 16200;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_set_config(&gbc_pwr_int_bat_charger,
                                             LTC4015_CONFIG_INPUT_VOLTAGE_LOW,
                                             &inputVoltageLow));
    expectedValue = ocmp_ltc4015_set_cfg_input_voltage_low(inputVoltageLow);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_INPUT_VOLTAGE_LOW_LIMIT]);

    /* Test lead acid chemistry */
    inputVoltageLow = 16200;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_set_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_INPUT_VOLTAGE_LOW,
                                             &inputVoltageLow));
    expectedValue = ocmp_ltc4015_set_cfg_input_voltage_low(inputVoltageLow);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_INPUT_VOLTAGE_LOW_LIMIT]);

    /* Make sure too-low values don't do anything bad */
    inputVoltageLow = 100;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_set_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_INPUT_VOLTAGE_LOW,
                                             &inputVoltageLow));
    expectedValue = ocmp_ltc4015_set_cfg_input_voltage_low(inputVoltageLow);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_INPUT_VOLTAGE_LOW_LIMIT]);

    /* Test with a missing device */
    inputVoltageLow = 16200;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_set_config(&gbc_pwr_invalid_dev,
                                              LTC4015_CONFIG_INPUT_VOLTAGE_LOW,
                                              &inputVoltageLow));

    /* Test with a missing bus */
    inputVoltageLow = 16200;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_set_config(&gbc_pwr_invalid_bus,
                                              LTC4015_CONFIG_INPUT_VOLTAGE_LOW,
                                              &inputVoltageLow));
}

void test_ocmp_ltc4015_set_input_current_high(void)
{
    int16_t inputCurrentHigh = 0;
    int16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    inputCurrentHigh = 17000;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_set_config(&gbc_pwr_int_bat_charger,
                                             LTC4015_CONFIG_INPUT_CURRENT_HIGH,
                                             &inputCurrentHigh));
    expectedValue = ocmp_ltc4015_set_cfg_input_current_high(
        &gbc_pwr_int_bat_charger, inputCurrentHigh);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_INPUT_CURRENT_HIGH_LIMIT]);

    /* Test lead acid chemistry */
    inputCurrentHigh = 17000;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_set_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_INPUT_CURRENT_HIGH,
                                             &inputCurrentHigh));
    expectedValue = ocmp_ltc4015_set_cfg_input_current_high(
        &gbc_pwr_ext_bat_charger, inputCurrentHigh);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_INPUT_CURRENT_HIGH_LIMIT]);

    /* Make sure too-low values don't do anything bad */
    inputCurrentHigh = 100;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_set_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_INPUT_CURRENT_HIGH,
                                             &inputCurrentHigh));
    expectedValue = ocmp_ltc4015_set_cfg_input_current_high(
        &gbc_pwr_ext_bat_charger, inputCurrentHigh);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_INPUT_CURRENT_HIGH_LIMIT]);

    /* Test with a missing device */
    inputCurrentHigh = 17000;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_set_config(&gbc_pwr_invalid_dev,
                                              LTC4015_CONFIG_INPUT_CURRENT_HIGH,
                                              &inputCurrentHigh));

    /* Test with a missing bus */
    inputCurrentHigh = 17000;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_set_config(&gbc_pwr_invalid_bus,
                                              LTC4015_CONFIG_INPUT_CURRENT_HIGH,
                                              &inputCurrentHigh));
}

void test_ocmp_ltc4015_set_input_current_limit(void)
{
    int16_t inputCurrentLimit = 0;
    int16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    inputCurrentLimit = 16500;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_set_config(&gbc_pwr_int_bat_charger,
                                             LTC4015_CONFIG_INPUT_CURRENT_LIMIT,
                                             &inputCurrentLimit));
    expectedValue = ocmp_ltc4015_set_cfg_input_current_limit(
        &gbc_pwr_int_bat_charger, inputCurrentLimit);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_INPUT_CURRENT_LIMIT_SETTING]);

    /* Test lead acid chemistry */
    inputCurrentLimit = 16500;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_set_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_INPUT_CURRENT_LIMIT,
                                             &inputCurrentLimit));
    expectedValue = ocmp_ltc4015_set_cfg_input_current_limit(
        &gbc_pwr_ext_bat_charger, inputCurrentLimit);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_INPUT_CURRENT_LIMIT_SETTING]);

    /* Make sure too-low values don't do anything bad */
    inputCurrentLimit = 400;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_set_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_INPUT_CURRENT_LIMIT,
                                             &inputCurrentLimit));
    expectedValue = ocmp_ltc4015_set_cfg_input_current_limit(
        &gbc_pwr_ext_bat_charger, inputCurrentLimit);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_INPUT_CURRENT_LIMIT_SETTING]);

    /* Test with a missing device */
    inputCurrentLimit = 16500;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                                 &gbc_pwr_invalid_dev,
                                 LTC4015_CONFIG_INPUT_CURRENT_LIMIT,
                                 &inputCurrentLimit));

    /* Test with a missing bus */
    inputCurrentLimit = 16500;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                                 &gbc_pwr_invalid_bus,
                                 LTC4015_CONFIG_INPUT_CURRENT_LIMIT,
                                 &inputCurrentLimit));
}

void test_ocmp_ltc4015_set_cfg_icharge(void)
{
    uint16_t icharge = 0;
    uint16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    icharge = 4321;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_set_config(&gbc_pwr_int_bat_charger,
                                             LTC4015_CONFIG_ICHARGE, &icharge));
    expectedValue =
        ocmp_ltc4015_set_cfg_icharge(&gbc_pwr_int_bat_charger, icharge);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_TARGET]);

    /* Test lead acid chemistry */
    icharge = 5000;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_set_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_ICHARGE, &icharge));
    expectedValue =
        ocmp_ltc4015_set_cfg_icharge(&gbc_pwr_ext_bat_charger, icharge);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_TARGET]);

    /* Make sure low values are handled properly (iCharge * Rsnsb < 1000) */
    icharge = 100;
    TEST_ASSERT_EQUAL(
        true, LTC4015_fxnTable.cb_set_config(&gbc_pwr_ext_bat_charger,
                                             LTC4015_CONFIG_ICHARGE, &icharge));
    expectedValue =
        ocmp_ltc4015_set_cfg_icharge(&gbc_pwr_ext_bat_charger, icharge);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_CHARGE_CURRENT_TARGET]);

    /* Test with a missing device */
    icharge = 5000;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_set_config(
                   &gbc_pwr_invalid_dev, LTC4015_CONFIG_ICHARGE, &icharge));

    /* Test with a missing bus */
    icharge = 5000;
    TEST_ASSERT_EQUAL(
        false, LTC4015_fxnTable.cb_set_config(
                   &gbc_pwr_invalid_bus, LTC4015_CONFIG_ICHARGE, &icharge));
}

void test_ocmp_ltc4015_set_vcharge(void)
{
    int16_t vChargeSetting = 0;
    int16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    vChargeSetting = 12300;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                                &gbc_pwr_int_bat_charger,
                                LTC4015_CONFIG_VCHARGE, &vChargeSetting));
    expectedValue =
        ocmp_ltc4015_set_cfg_vcharge(&gbc_pwr_int_bat_charger, vChargeSetting);
    TEST_ASSERT_EQUAL(expectedValue, LTC4015_regs[LTC4015_REG_VCHARGE_SETTING]);

    /* Test lead acid chemistry */
    vChargeSetting = 12600;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                                &gbc_pwr_ext_bat_charger,
                                LTC4015_CONFIG_VCHARGE, &vChargeSetting));
    expectedValue =
        ocmp_ltc4015_set_cfg_vcharge(&gbc_pwr_ext_bat_charger, vChargeSetting);
    TEST_ASSERT_EQUAL(expectedValue, LTC4015_regs[LTC4015_REG_VCHARGE_SETTING]);

    /* Make sure too-low values don't do anything bad */
    vChargeSetting = 6000;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                                &gbc_pwr_ext_bat_charger,
                                LTC4015_CONFIG_VCHARGE, &vChargeSetting));
    expectedValue =
        ocmp_ltc4015_set_cfg_vcharge(&gbc_pwr_ext_bat_charger, vChargeSetting);
    TEST_ASSERT_EQUAL(expectedValue, LTC4015_regs[LTC4015_REG_VCHARGE_SETTING]);

    /* Test with a missing device */
    vChargeSetting = 12629;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                                 &gbc_pwr_invalid_dev, LTC4015_CONFIG_VCHARGE,
                                 &vChargeSetting));

    /* Test with a missing bus */
    vChargeSetting = 12629;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                                 &gbc_pwr_invalid_bus, LTC4015_CONFIG_VCHARGE,
                                 &vChargeSetting));
}

void test_ocmp_ltc4015_set_die_temperature_high(void)
{
    int16_t dieTempAlertLimit = 0;
    int16_t expectedValue = 0;

    /* Test lithium ion chemistry */
    dieTempAlertLimit = 80;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                                &gbc_pwr_int_bat_charger,
                                LTC4015_CONFIG_DIE_TEMPERATURE_HIGH,
                                &dieTempAlertLimit));
    expectedValue =
        ocmp_ltc4015_set_cfg_die_temperature_high(dieTempAlertLimit);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_DIE_TEMP_HIGH_LIMIT]);

    /* Test lead acid chemistry */
    dieTempAlertLimit = -10;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                                &gbc_pwr_ext_bat_charger,
                                LTC4015_CONFIG_DIE_TEMPERATURE_HIGH,
                                &dieTempAlertLimit));
    expectedValue =
        ocmp_ltc4015_set_cfg_die_temperature_high(dieTempAlertLimit);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_DIE_TEMP_HIGH_LIMIT]);

    /* Make sure too-low values don't do anything bad */
    dieTempAlertLimit = 0;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                                &gbc_pwr_ext_bat_charger,
                                LTC4015_CONFIG_DIE_TEMPERATURE_HIGH,
                                &dieTempAlertLimit));
    expectedValue =
        ocmp_ltc4015_set_cfg_die_temperature_high(dieTempAlertLimit);
    TEST_ASSERT_EQUAL(expectedValue,
                      LTC4015_regs[LTC4015_REG_DIE_TEMP_HIGH_LIMIT]);

    /* Test with a missing device */
    dieTempAlertLimit = 80;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                                 &gbc_pwr_invalid_dev,
                                 LTC4015_CONFIG_DIE_TEMPERATURE_HIGH,
                                 &dieTempAlertLimit));

    /* Test with a missing bus */
    dieTempAlertLimit = 80;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                                 &gbc_pwr_invalid_bus, LTC4015_CONFIG_VCHARGE,
                                 &dieTempAlertLimit));
}

void test_ocmp_ltc4015_set_cfg_invalid_param(void)
{
    int16_t buffer = 80;

    /* Invalid param test */
    LTC4015_regs[LTC4015_REG_DIE_TEMP_HIGH_LIMIT] = 0;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                                 &gbc_pwr_ext_bat_charger, 9, &buffer));
}

void test_ocmp_ltc4015_alert_handler(void)
{
    int16_t value = 0x0800;
    AlertData alert_data = {
        .subsystem = 1,
        .componentId = 4,
        .deviceId = 1,
    };
    AlertData *alert_data_cp = malloc(sizeof(AlertData));
    *alert_data_cp = alert_data;
    TEST_ASSERT_EQUAL(POST_DEV_CFG_DONE,
                      LTC4015_fxnTable.cb_init(&gbc_pwr_int_bat_charger,
                                               &fact_lithiumIon_cfg,
                                               alert_data_cp));

    gbc_pwr_int_bat_charger.obj.alert_cb(LTC4015_EVT_BVL, value, alert_data_cp);
    gbc_pwr_int_bat_charger.obj.alert_cb(LTC4015_EVT_BVH, value, alert_data_cp);
    gbc_pwr_int_bat_charger.obj.alert_cb(LTC4015_EVT_BCL, value, alert_data_cp);
    gbc_pwr_int_bat_charger.obj.alert_cb(LTC4015_EVT_IVL, value, alert_data_cp);
    gbc_pwr_int_bat_charger.obj.alert_cb(LTC4015_EVT_ICH, value, alert_data_cp);
    gbc_pwr_int_bat_charger.obj.alert_cb(LTC4015_EVT_DTH, value, alert_data_cp);

    /* Test for memory check */
    gbc_pwr_int_bat_charger.obj.alert_cb(LTC4015_EVT_BVL, value, NULL);

    /* Default case test */
    gbc_pwr_int_bat_charger.obj.alert_cb(LTC4015_EVT_BMFA, value,
                                         alert_data_cp);
}
