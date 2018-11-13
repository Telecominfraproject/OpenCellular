/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_ltc4015.h"

extern const I2C_Dev I2C_DEV;
extern bool LTC4015_GpioPins[1];
extern uint32_t LTC4015_GpioConfig[1];
extern OcGpio_Port gbc_io_1;
extern OcGpio_Port ec_io;
extern LTC4015_Dev gbc_pwr_int_bat_charger ;
extern LTC4015_Dev gbc_pwr_invalid_dev;
extern LTC4015_Dev gbc_pwr_invalid_bus;
extern LTC4015_Dev gbc_pwr_ext_bat_charger;
extern uint16_t LTC4015_regs[0x4A];
extern uint8_t SX1509_regs[0x7F];

/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    FakeGpio_registerDevSimple(LTC4015_GpioPins, LTC4015_GpioConfig);
    fake_I2C_init();

    fake_I2C_registerDevSimple(gbc_pwr_int_bat_charger.cfg.i2c_dev.bus,
                    gbc_pwr_int_bat_charger.cfg.i2c_dev.slave_addr,
                LTC4015_regs,
                               sizeof(LTC4015_regs), sizeof(LTC4015_regs[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_LITTLE_ENDIAN);

    fake_I2C_registerDevSimple(0, 0x45, SX1509_regs,
                               sizeof(SX1509_regs), sizeof(SX1509_regs[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
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
// Parameters are not used as this is just used to test assigning the
//   alert_handler right now.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void OCMP_GenerateAlert(const AlertData *alert_data,
                        unsigned int alert_id,
                        const void *data)
{

}
#pragma GCC diagnostic pop
/* ================================ Tests =================================== */
void test_ocmp_ltc4015_probe(void)
{
    POSTData postData;
    LTC4015_GpioConfig[0x05] = 0;

    /* Test with the actual value */
    LTC4015_regs[0x39] = LTC4015_CHARGER_ENABLED;
    TEST_ASSERT_EQUAL(POST_DEV_FOUND, LTC4015_fxnTable.cb_probe(
                &gbc_pwr_int_bat_charger, &postData));
    TEST_ASSERT_EQUAL(OCGPIO_CFG_OUTPUT, LTC4015_GpioConfig[0x05]);

     /* Test with an incorrect value */
    LTC4015_regs[0x39] = ~LTC4015_CHARGER_ENABLED;
    TEST_ASSERT_EQUAL(POST_DEV_MISSING, LTC4015_fxnTable.cb_probe(
                &gbc_pwr_int_bat_charger, &postData));
    /* Test with a missing device */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING, LTC4015_fxnTable.cb_probe(
                            &gbc_pwr_invalid_dev, &postData));

    /* Test with a missing bus */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING, LTC4015_fxnTable.cb_probe(
                            &gbc_pwr_invalid_bus, &postData));
}

void test_ocmp_ltc4015_init(void)
{
    /* Now try to init with a pin associated */
    AlertData alert_data = {
        .subsystem = 1,
        .componentId = 4,
        .deviceId = 1,
    };
    AlertData *alert_data_cp = malloc(sizeof(AlertData));
    *alert_data_cp = alert_data;
    TEST_ASSERT_EQUAL(POST_DEV_CFG_DONE, LTC4015_fxnTable.cb_init(
                                &gbc_pwr_int_bat_charger,
                                &fact_lithiumIon_cfg, alert_data_cp));
    TEST_ASSERT_EQUAL(16470, LTC4015_regs[0x01]);
    TEST_ASSERT_EQUAL(21844, LTC4015_regs[0x02]);
    TEST_ASSERT_EQUAL(2047, LTC4015_regs[0x08]);
    TEST_ASSERT_EQUAL(9830, LTC4015_regs[0x03]);
    TEST_ASSERT_EQUAL(23892, LTC4015_regs[0x07]);
    TEST_ASSERT_EQUAL(76, LTC4015_regs[0x15]);
    TEST_ASSERT_EQUAL_HEX16(LTC4015_EVT_BVL | LTC4015_EVT_BVH |
                LTC4015_EVT_IVL | LTC4015_EVT_ICH | LTC4015_EVT_BCL,
                        LTC4015_regs[0x0D]);

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(POST_DEV_CFG_FAIL, LTC4015_fxnTable.cb_init(
                        &gbc_pwr_invalid_dev,
                        &fact_lithiumIon_cfg, alert_data_cp));

    /* Test with a missing bus */
    TEST_ASSERT_EQUAL(POST_DEV_CFG_FAIL, LTC4015_fxnTable.cb_init(
                        &gbc_pwr_invalid_bus,
                        &fact_lithiumIon_cfg, alert_data_cp));
}

void test_ocmp_ltc4015_get_status(void)
{
    int16_t batteryVoltage = 0;
    int16_t batteryCurrent = 0;
    int16_t systemVoltage = 0;
    int16_t inputVoltage = 0;
    int16_t inputCurrent = 0;
    uint16_t dieTemperature = 0;
    int16_t iCharge = 0;
    int16_t buffer = 0;

    /* Test lithium ion chemistry */
    /* Battery Voltage(VBATSENS/cellcount) = [VBAT] <95> 192.264(uV) */
    LTC4015_regs[0x3A] = 15603;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_int_bat_charger,
                        LTC4015_STATUS_BATTERY_VOLTAGE, &batteryVoltage));
    TEST_ASSERT_EQUAL(8999, batteryVoltage);

    /* IBAT_LO_ALERT_LIMIT = (limit*RSNSB)/1.46487uV */
    LTC4015_regs[0x3D] = 2048;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_int_bat_charger,
                        LTC4015_STATUS_BATTERY_CURRENT, &batteryCurrent));
    TEST_ASSERT_EQUAL(100, batteryCurrent);

    /* System voltage = [VSYS] <95> 1.648mV */
    LTC4015_regs[0x3C] = 7000;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_int_bat_charger,
                        LTC4015_STATUS_SYSTEM_VOLTAGE, &systemVoltage));
    TEST_ASSERT_EQUAL(11536, systemVoltage);

    /* VIN_LO_ALERT_LIMIT = limit/1.648mV */
    LTC4015_regs[0x3B] = 3034;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_int_bat_charger,
                        LTC4015_STATUS_INPUT_VOLATGE, &inputVoltage));
    TEST_ASSERT_EQUAL(5000, inputVoltage);

    /* IIN_HI_ALERT_LIMIT = (limit*RSNSI)/1.46487uV */
    LTC4015_regs[0x3E] = 23892;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_int_bat_charger,
                        LTC4015_STATUS_INPUT_CURRENT, &inputCurrent));
    TEST_ASSERT_EQUAL(4999, inputCurrent);

    /* DIE_TEMP_HI_ALERT_LIMIT = (DIE_TEMP <96> 12010)/45.6°C */
    LTC4015_regs[0x3F] = 13606;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_int_bat_charger,
                        LTC4015_STATUS_DIE_TEMPERATURE, &dieTemperature));
    TEST_ASSERT_EQUAL(35, dieTemperature);

    /* Charger Current servo level = (ICHARGE_DAC + 1) <95> 1mV/RSNSB */
    LTC4015_regs[0x44] = 0x1F;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_int_bat_charger,
                        LTC4015_STATUS_ICHARGE_DAC, &iCharge));
    TEST_ASSERT_EQUAL(1, iCharge);

    /*Test with invalid paramId*/
    LTC4015_regs[0x44] = 0x1F;
    buffer = 0;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_int_bat_charger, 7, &buffer));
    TEST_ASSERT_EQUAL(0, buffer);
}

void test_ocmp_ltc4015_get_config(void)
{
    int16_t buffer = 0;
    uint16_t inputCurrent = 0;

    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 192.264(uV) */
    LTC4015_regs[0x01] = 13458;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_config(
                            &gbc_pwr_ext_bat_charger,
                            LTC4015_CONFIG_BATTERY_VOLTAGE_LOW, &buffer));
    TEST_ASSERT_EQUAL(10349, buffer);

    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 192.264(uV) */
    LTC4015_regs[0x02] = 17945;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_config(
                            &gbc_pwr_ext_bat_charger,
                            LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH, &buffer));
    TEST_ASSERT_EQUAL(13800, buffer);

    /* IBAT_LO_ALERT_LIMIT = (limit*1.46487uV )/RSNSB*/
    LTC4015_regs[0x08] = 8276;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_BATTERY_CURRENT_LOW, &buffer));
    TEST_ASSERT_EQUAL(4041, buffer);

    /* VIN_LO_ALERT_LIMIT = limit/1.648mV */
    LTC4015_regs[0x03] = 60;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_INPUT_VOLTAGE_LOW, &buffer));
    TEST_ASSERT_EQUAL(98, buffer);

    /* IIN_HI_ALERT_LIMIT = (limit*RSNSI)/1.46487uV */
    LTC4015_regs[0x07] = 23211;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_INPUT_CURRENT_HIGH, &buffer));
    TEST_ASSERT_EQUAL(17000, buffer);

    /* IIN_LIMIT_SETTING = (limit * RSNSI / 500uV) - 1 */
    LTC4015_regs[0x15] = 200;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_INPUT_CURRENT_LIMIT, &inputCurrent));
    TEST_ASSERT_EQUAL(50250, inputCurrent);

    /* Maximum charge current target = (ICHARGE_TARGET + 1) • 1mV/RSNSB */
    LTC4015_regs[0x1A] = 12;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_ICHARGE, &buffer));
    TEST_ASSERT_EQUAL(4333, buffer);

    /* vcharge = (VCHARGE_SETTING/105.0 + 2.0)V/cell w/o temp comp.*/
    LTC4015_regs[0x1B] = 11;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_VCHARGE, &buffer));
    TEST_ASSERT_EQUAL(12629, buffer);

    /* DIE_TEMP_HI_ALERT_LIMIT = (DIE_TEMP – 12010)/45.6°C */
    LTC4015_regs[0x09] = 15658;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_DIE_TEMPERATURE_HIGH, &buffer));
    TEST_ASSERT_EQUAL(80, buffer);

    /*Test with invalid paramId*/
    LTC4015_regs[0x09] = 15658;
    buffer = 0;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_ext_bat_charger, 9, &buffer));
    TEST_ASSERT_EQUAL(0, buffer);
}

void test_ocmp_ltc4015_set_config(void)
{
    int16_t buffer = 0;
    uint32_t inputCurrent = 0;

    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 192.264(uV) */
    buffer = 10350;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_BATTERY_VOLTAGE_LOW, &buffer));
    TEST_ASSERT_EQUAL(13458, LTC4015_regs[0x01]);

    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 128.176(uV) */
    buffer = 13800;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH, &buffer));
    TEST_ASSERT_EQUAL(17944, LTC4015_regs[0x02]);

    /* IBAT_LO_ALERT_LIMIT = (limit*1.46487uV )/RSNSB*/
    buffer = 4041;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_BATTERY_CURRENT_LOW, &buffer));
    TEST_ASSERT_EQUAL(8275, LTC4015_regs[0x08]);

    /* VIN_LO_ALERT_LIMIT = limit/1.648mV */
    buffer = 98;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_INPUT_VOLTAGE_LOW, &buffer));
    TEST_ASSERT_EQUAL(59, LTC4015_regs[0x03]);

    /* IIN_HI_ALERT_LIMIT = (limit*RSNSI)/1.46487uV */
    buffer = 17000;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_INPUT_CURRENT_HIGH, &buffer));
    TEST_ASSERT_EQUAL(23210, LTC4015_regs[0x07]);

    /* IIN_LIMIT_SETTING = (limit * RSNSI / 500uV) - 1 */
    inputCurrent = 50250;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_INPUT_CURRENT_LIMIT, &inputCurrent));
    TEST_ASSERT_EQUAL(200, LTC4015_regs[0x15]);

    /* Maximum charge current target = (ICHARGE_TARGET + 1) • 1mV/RSNSB */
    buffer = 4333;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_ICHARGE, &buffer));
    TEST_ASSERT_EQUAL(12, LTC4015_regs[0x1A]);

    /* vcharge = (VCHARGE_SETTING/105.0 + 2.0)V/cell w/o temp comp.*/
    buffer = 12629;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_VCHARGE, &buffer));
    TEST_ASSERT_EQUAL(11, LTC4015_regs[0x1B]);

    /* DIE_TEMP_HI_ALERT_LIMIT = (DIE_TEMP – 12010)/45.6°C */
    buffer = 80;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_DIE_TEMPERATURE_HIGH, &buffer));
    TEST_ASSERT_EQUAL(15658, LTC4015_regs[0x09]);

    /*Test with invalid paramId*/
    buffer = 80;
    LTC4015_regs[0x09] = 0;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger, 9, &buffer));
    TEST_ASSERT_EQUAL(0, LTC4015_regs[0x09]);
}

void test_ocmp_ltc4015_upper_boundary_value(void)
{
    uint32_t buffer = 0;
    uint32_t inputCurrent = 0;

    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 192.264(uV) */
    buffer = 66539;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_BATTERY_VOLTAGE_LOW, &buffer));
    TEST_ASSERT_EQUAL(1304, LTC4015_regs[0x01]);

    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 128.176(uV) */
    buffer =  66539;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH, &buffer));
    TEST_ASSERT_EQUAL(1304, LTC4015_regs[0x02]);

    /* IBAT_LO_ALERT_LIMIT = (limit*1.46487uV )/RSNSB*/
    buffer = 66539;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_BATTERY_CURRENT_LOW, &buffer));
    TEST_ASSERT_EQUAL(2054, LTC4015_regs[0x08]);

    /* VIN_LO_ALERT_LIMIT = limit/1.648mV */
    buffer = 66539;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_INPUT_VOLTAGE_LOW, &buffer));
    TEST_ASSERT_EQUAL(608, LTC4015_regs[0x03]);

    /* IIN_HI_ALERT_LIMIT = (limit*RSNSI)/1.46487uV */
    buffer = 66539;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_INPUT_CURRENT_HIGH, &buffer));
    TEST_ASSERT_EQUAL(1369, LTC4015_regs[0x07]);

    /* IIN_LIMIT_SETTING = (limit * RSNSI / 500uV) - 1 */
    inputCurrent = 66539;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_INPUT_CURRENT_LIMIT, &inputCurrent));
    TEST_ASSERT_EQUAL(3, LTC4015_regs[0x15]);

    /* Maximum charge current target = (ICHARGE_TARGET + 1) • 1mV/RSNSB */
    buffer = 66539;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_ICHARGE, &buffer));
    TEST_ASSERT_EQUAL(2, LTC4015_regs[0x1A]);

    /* vcharge = (VCHARGE_SETTING/105.0 + 2.0)V/cell w/o temp comp.*/
    buffer = 66539;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_VCHARGE, &buffer));
    TEST_ASSERT_EQUAL(0, LTC4015_regs[0x1B]);

    /* DIE_TEMP_HI_ALERT_LIMIT = (DIE_TEMP – 12010)/45.6°C */
    buffer = 66539;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_DIE_TEMPERATURE_HIGH, &buffer));
    TEST_ASSERT_EQUAL(57746, LTC4015_regs[0x09]);
}

void test_ocmp_ltc4015_lower_boundary_value(void)
{
    int16_t buffer = 0;
    int16_t inputCurrent = 0;

    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 192.264(uV) */
    buffer = -345;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_BATTERY_VOLTAGE_LOW, &buffer));
    TEST_ASSERT_EQUAL(65088, LTC4015_regs[0x01]);

    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 128.176(uV) */
    buffer =  -345;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH, &buffer));
    TEST_ASSERT_EQUAL(65088, LTC4015_regs[0x02]);

    /* IBAT_LO_ALERT_LIMIT = (limit*1.46487uV )/RSNSB*/
    buffer = -345;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_BATTERY_CURRENT_LOW, &buffer));
    TEST_ASSERT_EQUAL(64830, LTC4015_regs[0x08]);

    /* VIN_LO_ALERT_LIMIT = limit/1.648mV */
    buffer = -345;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_INPUT_VOLTAGE_LOW, &buffer));
    TEST_ASSERT_EQUAL(65327, LTC4015_regs[0x03]);

    /* IIN_HI_ALERT_LIMIT = (limit*RSNSI)/1.46487uV */
    buffer = -345;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_INPUT_CURRENT_HIGH, &buffer));
    TEST_ASSERT_EQUAL(65065, LTC4015_regs[0x07]);

    /* IIN_LIMIT_SETTING = (limit * RSNSI / 500uV) - 1 */
    inputCurrent = -345;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_INPUT_CURRENT_LIMIT, &inputCurrent));
    TEST_ASSERT_EQUAL(259, LTC4015_regs[0x15]);

    /* Maximum charge current target = (ICHARGE_TARGET + 1) • 1mV/RSNSB */
    buffer = -345;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_ICHARGE, &buffer));
    TEST_ASSERT_EQUAL(195, LTC4015_regs[0x1A]);

    /* vcharge = (VCHARGE_SETTING/105.0 + 2.0)V/cell w/o temp comp.*/
    buffer = -345;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_VCHARGE, &buffer));
    TEST_ASSERT_EQUAL(931, LTC4015_regs[0x1B]);

    /* DIE_TEMP_HI_ALERT_LIMIT = (DIE_TEMP – 12010)/45.6°C */
    buffer = -345;
    TEST_ASSERT_EQUAL(true, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_ext_bat_charger,
                        LTC4015_CONFIG_DIE_TEMPERATURE_HIGH, &buffer));
    TEST_ASSERT_EQUAL(61814, LTC4015_regs[0x09]);
}

void test_ocmp_ltc4015_get_status_for_invalid_device(void)
{
    int16_t batteryVoltage = 0;
    int16_t batteryCurrent = 0;
    int16_t systemVoltage = 0;
    int16_t inputVoltage = 0;
    int16_t inputCurrent = 0;
    uint16_t dieTemperature = 0;
    int16_t iCharge = 0;

    /* Test lithium ion chemistry */
    /* Battery Voltage(VBATSENS/cellcount) = [VBAT] <95> 192.264(uV) */
    LTC4015_regs[0x3A] = 15603;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_invalid_dev,
                        LTC4015_STATUS_BATTERY_VOLTAGE, &batteryVoltage));

    /* IBAT_LO_ALERT_LIMIT = (limit*RSNSB)/1.46487uV */
    LTC4015_regs[0x3D] = 2048;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_invalid_dev,
                        LTC4015_STATUS_BATTERY_CURRENT, &batteryCurrent));

    /* System voltage = [VSYS] <95> 1.648mV */
    LTC4015_regs[0x3C] = 7000;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_invalid_dev,
                        LTC4015_STATUS_SYSTEM_VOLTAGE, &systemVoltage));

    /* VIN_LO_ALERT_LIMIT = limit/1.648mV */
    LTC4015_regs[0x3B] = 3034;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_invalid_dev,
                        LTC4015_STATUS_INPUT_VOLATGE, &inputVoltage));

    /* IIN_HI_ALERT_LIMIT = (limit*RSNSI)/1.46487uV */
    LTC4015_regs[0x3E] = 23892;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_invalid_dev,
                        LTC4015_STATUS_INPUT_CURRENT, &inputCurrent));

    /* DIE_TEMP_HI_ALERT_LIMIT = (DIE_TEMP <96> 12010)/45.6°C */
    LTC4015_regs[0x3F] = 13606;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_invalid_dev,
                        LTC4015_STATUS_DIE_TEMPERATURE, &dieTemperature));

    /* Charger Current servo level = (ICHARGE_DAC + 1) <95> 1mV/RSNSB */
    LTC4015_regs[0x44] = 0x1F;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_invalid_dev,
                        LTC4015_STATUS_ICHARGE_DAC, &iCharge));
}

void test_ocmp_ltc4015_get_status_for_invalid_bus(void)
{
    int16_t batteryVoltage = 0;
    int16_t batteryCurrent = 0;
    int16_t systemVoltage = 0;
    int16_t inputVoltage = 0;
    int16_t inputCurrent = 0;
    uint16_t dieTemperature = 0;
    int16_t iCharge = 0;

    /* Test lithium ion chemistry */
    /* Battery Voltage(VBATSENS/cellcount) = [VBAT] <95> 192.264(uV) */
    LTC4015_regs[0x3A] = 15603;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_invalid_bus,
                        LTC4015_STATUS_BATTERY_VOLTAGE, &batteryVoltage));

    /* IBAT_LO_ALERT_LIMIT = (limit*RSNSB)/1.46487uV */
    LTC4015_regs[0x3D] = 2048;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_invalid_bus,
                        LTC4015_STATUS_BATTERY_CURRENT, &batteryCurrent));

    /* System voltage = [VSYS] <95> 1.648mV */
    LTC4015_regs[0x3C] = 7000;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_invalid_bus,
                        LTC4015_STATUS_SYSTEM_VOLTAGE, &systemVoltage));

    /* VIN_LO_ALERT_LIMIT = limit/1.648mV */
    LTC4015_regs[0x3B] = 3034;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_invalid_bus,
                        LTC4015_STATUS_INPUT_VOLATGE, &inputVoltage));

    /* IIN_HI_ALERT_LIMIT = (limit*RSNSI)/1.46487uV */
    LTC4015_regs[0x3E] = 23892;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_invalid_bus,
                        LTC4015_STATUS_INPUT_CURRENT, &inputCurrent));

    /* DIE_TEMP_HI_ALERT_LIMIT = (DIE_TEMP <96> 12010)/45.6°C */
    LTC4015_regs[0x3F] = 13606;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_invalid_bus,
                        LTC4015_STATUS_DIE_TEMPERATURE, &dieTemperature));

    /* Charger Current servo level = (ICHARGE_DAC + 1) <95> 1mV/RSNSB */
    LTC4015_regs[0x44] = 0x1F;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_status(
                        &gbc_pwr_invalid_bus,
                        LTC4015_STATUS_ICHARGE_DAC, &iCharge));
}

void test_ocmp_ltc4015_get_config_for_invalid_device(void)
{
    int16_t buffer = 0;
    uint16_t inputCurrent = 0;

    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 192.264(uV) */
    LTC4015_regs[0x01] = 13458;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_invalid_dev,
                        LTC4015_CONFIG_BATTERY_VOLTAGE_LOW, &buffer));

    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 192.264(uV) */
    LTC4015_regs[0x02] = 17945;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_invalid_dev,
                        LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH, &buffer));

    /* IBAT_LO_ALERT_LIMIT = (limit*1.46487uV )/RSNSB*/
    LTC4015_regs[0x08] = 8276;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_invalid_dev,
                        LTC4015_CONFIG_BATTERY_CURRENT_LOW, &buffer));

    /* VIN_LO_ALERT_LIMIT = limit/1.648mV */
    LTC4015_regs[0x03] = 60;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_invalid_dev,
                        LTC4015_CONFIG_INPUT_VOLTAGE_LOW, &buffer));

    /* IIN_HI_ALERT_LIMIT = (limit*RSNSI)/1.46487uV */
    LTC4015_regs[0x07] = 23211;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_invalid_dev,
                        LTC4015_CONFIG_INPUT_CURRENT_HIGH, &buffer));

    /* IIN_LIMIT_SETTING = (limit * RSNSI / 500uV) - 1 */
    LTC4015_regs[0x15] = 200;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_invalid_dev,
                        LTC4015_CONFIG_INPUT_CURRENT_LIMIT, &inputCurrent));

    /* Maximum charge current target = (ICHARGE_TARGET + 1) • 1mV/RSNSB */
    LTC4015_regs[0x1A] = 12;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_invalid_dev,
                        LTC4015_CONFIG_ICHARGE, &buffer));

    /* vcharge = (VCHARGE_SETTING/105.0 + 2.0)V/cell w/o temp comp.*/
    LTC4015_regs[0x1B] = 11;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_invalid_dev,
                        LTC4015_CONFIG_VCHARGE, &buffer));

    /* DIE_TEMP_HI_ALERT_LIMIT = (DIE_TEMP – 12010)/45.6°C */
    LTC4015_regs[0x09] = 15658;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_invalid_dev,
                        LTC4015_CONFIG_DIE_TEMPERATURE_HIGH, &buffer));
}


void test_ocmp_ltc4015_get_config_for_invalid_bus(void)
{
    int16_t buffer = 0;
    uint16_t inputCurrent = 0;

    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 192.264(uV) */
    LTC4015_regs[0x01] = 13458;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_invalid_bus,
                        LTC4015_CONFIG_BATTERY_VOLTAGE_LOW, &buffer));

    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 192.264(uV) */
    LTC4015_regs[0x02] = 17945;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_invalid_bus,
                        LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH, &buffer));

    /* IBAT_LO_ALERT_LIMIT = (limit*1.46487uV )/RSNSB*/
    LTC4015_regs[0x08] = 8276;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_invalid_bus,
                        LTC4015_CONFIG_BATTERY_CURRENT_LOW, &buffer));

    /* VIN_LO_ALERT_LIMIT = limit/1.648mV */
    LTC4015_regs[0x03] = 60;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_invalid_bus,
                        LTC4015_CONFIG_INPUT_VOLTAGE_LOW, &buffer));

    /* IIN_HI_ALERT_LIMIT = (limit*RSNSI)/1.46487uV */
    LTC4015_regs[0x07] = 23211;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_invalid_bus,
                        LTC4015_CONFIG_INPUT_CURRENT_HIGH, &buffer));

    /* IIN_LIMIT_SETTING = (limit * RSNSI / 500uV) - 1 */
    LTC4015_regs[0x15] = 200;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_invalid_bus,
                        LTC4015_CONFIG_INPUT_CURRENT_LIMIT, &inputCurrent));

    /* Maximum charge current target = (ICHARGE_TARGET + 1) • 1mV/RSNSB */
    LTC4015_regs[0x1A] = 12;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_invalid_bus,
                        LTC4015_CONFIG_ICHARGE, &buffer));

    /* vcharge = (VCHARGE_SETTING/105.0 + 2.0)V/cell w/o temp comp.*/
    LTC4015_regs[0x1B] = 11;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_invalid_bus,
                        LTC4015_CONFIG_VCHARGE, &buffer));

    /* DIE_TEMP_HI_ALERT_LIMIT = (DIE_TEMP – 12010)/45.6°C */
    LTC4015_regs[0x09] = 15658;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_get_config(
                        &gbc_pwr_invalid_bus,
                        LTC4015_CONFIG_DIE_TEMPERATURE_HIGH, &buffer));
}

void test_ocmp_ltc4015_set_config_for_invalid_device(void)
{
    int16_t buffer = 0;
    uint32_t inputCurrent = 0;

    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 192.264(uV) */
    buffer = 10350;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_invalid_dev,
                        LTC4015_CONFIG_BATTERY_VOLTAGE_LOW, &buffer));

    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 128.176(uV) */
    buffer = 13800;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_invalid_dev,
                        LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH, &buffer));

    /* IBAT_LO_ALERT_LIMIT = (limit*1.46487uV )/RSNSB*/
    buffer = 4041;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_invalid_dev,
                        LTC4015_CONFIG_BATTERY_CURRENT_LOW, &buffer));

    /* VIN_LO_ALERT_LIMIT = limit/1.648mV */
    buffer = 98;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_invalid_dev,
                        LTC4015_CONFIG_INPUT_VOLTAGE_LOW, &buffer));

    /* IIN_HI_ALERT_LIMIT = (limit*RSNSI)/1.46487uV */
    buffer = 17000;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_invalid_dev,
                        LTC4015_CONFIG_INPUT_CURRENT_HIGH, &buffer));

    /* IIN_LIMIT_SETTING = (limit * RSNSI / 500uV) - 1 */
    inputCurrent = 50250;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_invalid_dev,
                        LTC4015_CONFIG_INPUT_CURRENT_LIMIT, &inputCurrent));

    /* Maximum charge current target = (ICHARGE_TARGET + 1) • 1mV/RSNSB */
    buffer = 4333;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_invalid_dev,
                        LTC4015_CONFIG_ICHARGE, &buffer));

    /* vcharge = (VCHARGE_SETTING/105.0 + 2.0)V/cell w/o temp comp.*/
    buffer = 12629;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_invalid_dev,
                        LTC4015_CONFIG_VCHARGE, &buffer));

    /* DIE_TEMP_HI_ALERT_LIMIT = (DIE_TEMP – 12010)/45.6°C */
    buffer = 80;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_invalid_dev,
                        LTC4015_CONFIG_DIE_TEMPERATURE_HIGH, &buffer));
}

void test_ocmp_ltc4015_set_config_for_invalid_bus(void)
{
    int16_t buffer = 0;
    uint32_t inputCurrent = 0;

    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 192.264(uV) */
    buffer = 10350;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_invalid_bus,
                        LTC4015_CONFIG_BATTERY_VOLTAGE_LOW, &buffer));

    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 128.176(uV) */
    buffer = 13800;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_invalid_bus,
                        LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH, &buffer));

    /* IBAT_LO_ALERT_LIMIT = (limit*1.46487uV )/RSNSB*/
    buffer = 4041;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_invalid_bus,
                        LTC4015_CONFIG_BATTERY_CURRENT_LOW, &buffer));

    /* VIN_LO_ALERT_LIMIT = limit/1.648mV */
    buffer = 98;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_invalid_bus,
                        LTC4015_CONFIG_INPUT_VOLTAGE_LOW, &buffer));

    /* IIN_HI_ALERT_LIMIT = (limit*RSNSI)/1.46487uV */
    buffer = 17000;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_invalid_bus,
                        LTC4015_CONFIG_INPUT_CURRENT_HIGH, &buffer));

    /* IIN_LIMIT_SETTING = (limit * RSNSI / 500uV) - 1 */
    inputCurrent = 50250;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_invalid_bus,
                        LTC4015_CONFIG_INPUT_CURRENT_LIMIT, &inputCurrent));

    /* Maximum charge current target = (ICHARGE_TARGET + 1) • 1mV/RSNSB */
    buffer = 4333;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_invalid_bus,
                        LTC4015_CONFIG_ICHARGE, &buffer));

    /* vcharge = (VCHARGE_SETTING/105.0 + 2.0)V/cell w/o temp comp.*/
    buffer = 12629;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_invalid_bus,
                        LTC4015_CONFIG_VCHARGE, &buffer));

    /* DIE_TEMP_HI_ALERT_LIMIT = (DIE_TEMP – 12010)/45.6°C */
    buffer = 80;
    TEST_ASSERT_EQUAL(false, LTC4015_fxnTable.cb_set_config(
                        &gbc_pwr_invalid_bus,
                        LTC4015_CONFIG_DIE_TEMPERATURE_HIGH, &buffer));
}
