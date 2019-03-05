/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_ina226.h"

extern bool INA226_GpioPins[0x05];
extern INA226_Config fact_sdr_3v_ps_cfg;
extern INA226_Dev ina226_invalid_bus;
extern INA226_Dev ina226_invalid_dev;
extern INA226_Dev sdr_fpga_ps;
extern OcGpio_Port ec_io;
extern uint16_t INA226_regs[INA226_END_REG];
extern uint32_t INA226_GpioConfig[0x05];
/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    FakeGpio_registerDevSimple(INA226_GpioPins, INA226_GpioConfig);
    fake_I2C_init();

    fake_I2C_registerDevSimple(sdr_fpga_ps.cfg.dev.bus,
                               sdr_fpga_ps.cfg.dev.slave_addr, INA226_regs,
                               sizeof(INA226_regs) + 1, sizeof(INA226_regs[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_BIG_ENDIAN);
}

void setUp(void)
{
    memset(INA226_regs, 0, sizeof(INA226_regs));

    ina226_init(&sdr_fpga_ps);
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void OCMP_GenerateAlert(const AlertData *alert_data, unsigned int alert_id,
                        const void *data, const void *lValue,
                        OCMPActionType actionType)
{
}
#pragma GCC diagnostic pop
/* ================================ Tests =================================== */
void test_probe(void)
{
    POSTData postData;
    /* Test with the actual values */
    INA226_regs[INA226_DIE_ID_REG] = INA226_DEVICE_ID;
    INA226_regs[INA226_MANF_ID_REG] = INA226_MANF_ID;
    TEST_ASSERT_EQUAL(POST_DEV_FOUND,
                      INA226_fxnTable.cb_probe(&sdr_fpga_ps, &postData));
    TEST_ASSERT_EQUAL(sdr_fpga_ps.cfg.dev.bus, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(sdr_fpga_ps.cfg.dev.slave_addr, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(INA226_MANF_ID, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(INA226_DEVICE_ID, postData.devId);

    postData.i2cBus = INA226_POST_DATA_NULL;
    postData.devAddr = INA226_POST_DATA_NULL;
    postData.manId = INA226_POST_DATA_NULL;
    postData.devId = INA226_POST_DATA_NULL;

    /* Test with an incorrect device ID */
    INA226_regs[INA226_DIE_ID_REG] = INA226_INVALID_DEVICE_ID;
    TEST_ASSERT_EQUAL(POST_DEV_ID_MISMATCH,
                      INA226_fxnTable.cb_probe(&sdr_fpga_ps, &postData));
    TEST_ASSERT_EQUAL(INA226_POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(INA226_POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(INA226_POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(INA226_POST_DATA_NULL, postData.devId);

    /* Test with an incorrect mfg ID */
    INA226_regs[INA226_DIE_ID_REG] = INA226_DEVICE_ID;
    INA226_regs[INA226_MANF_ID_REG] = INA226_INVALID_MANF_ID;
    TEST_ASSERT_EQUAL(POST_DEV_ID_MISMATCH,
                      INA226_fxnTable.cb_probe(&sdr_fpga_ps, &postData));
    TEST_ASSERT_EQUAL(INA226_POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(INA226_POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(INA226_POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(INA226_POST_DATA_NULL, postData.devId);

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING,
                      INA226_fxnTable.cb_probe(&ina226_invalid_dev, &postData));
    TEST_ASSERT_EQUAL(INA226_POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(INA226_POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(INA226_POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(INA226_POST_DATA_NULL, postData.devId);

    /* Test with a missing bus */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING,
                      INA226_fxnTable.cb_probe(&ina226_invalid_bus, &postData));
    TEST_ASSERT_EQUAL(INA226_POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(INA226_POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(INA226_POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(INA226_POST_DATA_NULL, postData.devId);
}

void test_init(void)
{
    uint16_t expCurrentVal = 0;

    INA226_regs[INA226_CONFIG_REG] = 0;
    INA226_regs[INA226_CALIBRATION_REG] = 0;
    INA226_regs[INA226_ALERT_REG] = 0;
    INA226_regs[INA226_MASK_ENABLE_REG] = 0;
    AlertData alert_data = {
        .subsystem = 7,
        .componentId = 1,
        .deviceId = 0,
    };

    TEST_ASSERT_EQUAL(POST_DEV_CFG_DONE,
                      INA226_fxnTable.cb_init(&sdr_fpga_ps, &fact_sdr_3v_ps_cfg,
                                              &alert_data));
    expCurrentVal =
        ina226_stub_set_currentLimit(fact_sdr_3v_ps_cfg.current_lim);

    /* Make sure we've reset the device */
    TEST_ASSERT_BITS_HIGH(INA226_RESET, INA226_regs[INA226_CONFIG_REG]);

    /* Make sure calibration register gets initialized */
    TEST_ASSERT_NOT_EQUAL(INA226_CAL_VALUE,
                          INA226_regs[INA226_CALIBRATION_REG]);

    /* Set CurrentLimit */
    TEST_ASSERT_EQUAL_HEX16(expCurrentVal, INA226_regs[INA226_ALERT_REG]);

    /* Make Alert is enable */
    TEST_ASSERT_EQUAL_HEX16(INA226_ALERT_MASK,
                            INA226_regs[INA226_MASK_ENABLE_REG]);

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(POST_DEV_CFG_FAIL, INA226_fxnTable.cb_init(
                                             &ina226_invalid_dev,
                                             &fact_sdr_3v_ps_cfg, &alert_data));

    /* Test with a missing bus */
    TEST_ASSERT_EQUAL(POST_DEV_CFG_FAIL, INA226_fxnTable.cb_init(
                                             &ina226_invalid_bus,
                                             &fact_sdr_3v_ps_cfg, &alert_data));

    /* Test with NULL config*/
    TEST_ASSERT_EQUAL(
        POST_DEV_CFG_FAIL,
        INA226_fxnTable.cb_init(&ina226_invalid_bus, NULL, &alert_data));
}

void test_get_status(void)
{
    uint16_t busVoltageVal = 0;
    uint16_t expBusVoltageVal = 0;
    uint16_t shuntVoltageVal = 0;
    uint16_t expShuntVoltageVal = 0;
    uint16_t currentVal = 0;
    uint16_t expCurrentVal = 0;
    uint16_t powerVal = 0;
    uint16_t expPowerVal = 0;

    INA226_regs[INA226_BUS_VOLT_REG] = 0x2580;
    TEST_ASSERT_EQUAL(
        true, INA226_fxnTable.cb_get_status(
                  &sdr_fpga_ps, INA226_STATUS_BUS_VOLTAGE, &busVoltageVal));
    expBusVoltageVal =
        ina226_stub_get_busVlotage_status(INA226_regs[INA226_BUS_VOLT_REG]);
    TEST_ASSERT_EQUAL_HEX16(expBusVoltageVal, busVoltageVal);

    INA226_regs[INA226_SHUNT_VOLT_REG] = 0x0168;
    TEST_ASSERT_EQUAL(
        true, INA226_fxnTable.cb_get_status(
                  &sdr_fpga_ps, INA226_STATUS_SHUNT_VOLTAGE, &shuntVoltageVal));
    expShuntVoltageVal =
        ina226_stub_get_shuntVlotage_status(INA226_regs[INA226_SHUNT_VOLT_REG]);
    TEST_ASSERT_EQUAL_HEX16(expShuntVoltageVal, shuntVoltageVal);

    INA226_regs[INA226_CURRENT_REG] = 0x1388;
    TEST_ASSERT_EQUAL(true, INA226_fxnTable.cb_get_status(&sdr_fpga_ps,
                                                          INA226_STATUS_CURRENT,
                                                          &currentVal));
    expCurrentVal =
        ina226_stub_get_current_status(INA226_regs[INA226_CURRENT_REG]);
    TEST_ASSERT_EQUAL_HEX16(expCurrentVal, currentVal);

    INA226_regs[INA226_POWER_REG] = 0x02A8;
    TEST_ASSERT_EQUAL(true, INA226_fxnTable.cb_get_status(
                                &sdr_fpga_ps, INA226_STATUS_POWER, &powerVal));
    expPowerVal = ina226_stub_get_power_status(INA226_regs[INA226_POWER_REG]);
    TEST_ASSERT_EQUAL_HEX16(expPowerVal, powerVal);

    INA226_regs[INA226_POWER_REG] = 0x02A8;

    /* Test with a invalid paramID */
    TEST_ASSERT_EQUAL(
        false, INA226_fxnTable.cb_get_status(
                   &sdr_fpga_ps, INA226_INVALID_STATUS_PARAMID, &powerVal));

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(false, INA226_fxnTable.cb_get_status(&ina226_invalid_dev,
                                                           INA226_STATUS_POWER,
                                                           &powerVal));

    /* Test with a missing bus */
    TEST_ASSERT_EQUAL(false, INA226_fxnTable.cb_get_status(&ina226_invalid_bus,
                                                           INA226_STATUS_POWER,
                                                           &powerVal));
}

void test_get_config(void)
{
    uint16_t currentVal = 0;
    uint16_t expCurrentVal = 0;
    INA226_regs[INA226_ALERT_REG] = 0x0000;

    TEST_ASSERT_EQUAL(
        true, INA226_fxnTable.cb_get_config(
                  &sdr_fpga_ps, INA226_CONFIG_CURRENT_LIM, &currentVal));
    expCurrentVal = ina226_stub_get_currentLimit(INA226_regs[INA226_ALERT_REG]);
    TEST_ASSERT_EQUAL(expCurrentVal, currentVal);

    /* Test with a invalid param id */
    TEST_ASSERT_EQUAL(
        false, INA226_fxnTable.cb_get_config(
                   &sdr_fpga_ps, INA226_INVALID_CONFIG_PARAMID, &currentVal));
    /* Test with a missing device */
    TEST_ASSERT_EQUAL(false, INA226_fxnTable.cb_get_config(
                                 &ina226_invalid_dev, INA226_CONFIG_CURRENT_LIM,
                                 &currentVal));

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(false, INA226_fxnTable.cb_get_config(
                                 &ina226_invalid_bus, INA226_CONFIG_CURRENT_LIM,
                                 &currentVal));
}

void test_set_config(void)
{
    uint16_t currentVal = 0x0BB8;
    uint16_t expCurrentVal = 0;
    INA226_regs[INA226_ALERT_REG] = 0x0000;
    TEST_ASSERT_EQUAL(
        true, INA226_fxnTable.cb_set_config(
                  &sdr_fpga_ps, INA226_CONFIG_CURRENT_LIM, &currentVal));
    expCurrentVal = ina226_stub_set_currentLimit(currentVal);
    TEST_ASSERT_EQUAL_HEX16(expCurrentVal, INA226_regs[INA226_ALERT_REG]);

    /* Test with a invalid param id */
    TEST_ASSERT_EQUAL(
        false, INA226_fxnTable.cb_set_config(
                   &sdr_fpga_ps, INA226_INVALID_CONFIG_PARAMID, &currentVal));

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(false, INA226_fxnTable.cb_set_config(
                                 &ina226_invalid_dev, INA226_CONFIG_CURRENT_LIM,
                                 &currentVal));

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(false, INA226_fxnTable.cb_set_config(
                                 &ina226_invalid_bus, INA226_CONFIG_CURRENT_LIM,
                                 &currentVal));
}

void test_invalid(void)
{
    uint16_t currentVal = INA226_UPPER_BOUND_VAL;
    int16_t value = INA226_LOWER_BOUND_VAL;
    uint16_t expCurrentVal = 0;
    INA226_regs[INA226_ALERT_REG] = 0x0000;
    TEST_ASSERT_EQUAL(
        true, INA226_fxnTable.cb_set_config(
                  &sdr_fpga_ps, INA226_CONFIG_CURRENT_LIM, &currentVal));
    expCurrentVal = ina226_stub_set_currentLimit(currentVal);
    TEST_ASSERT_EQUAL(expCurrentVal, INA226_regs[INA226_ALERT_REG]);

    expCurrentVal = 0;
    TEST_ASSERT_EQUAL(
        true, INA226_fxnTable.cb_set_config(&sdr_fpga_ps,
                                            INA226_CONFIG_CURRENT_LIM, &value));
    expCurrentVal = ina226_stub_set_currentLimit(value);
    TEST_ASSERT_EQUAL(expCurrentVal, INA226_regs[INA226_ALERT_REG]);
}

void test_ocmp_ina226_alert_handler(void)
{
    int16_t value = 0x0800;
    INA226_regs[INA226_CONFIG_REG] = 0;
    INA226_regs[INA226_CALIBRATION_REG] = 0;
    INA226_regs[INA226_ALERT_REG] = 0;
    INA226_regs[INA226_MASK_ENABLE_REG] = 0;
    AlertData alert_data = {
        .subsystem = 7,
        .componentId = 1,
        .deviceId = 0,
    };
    AlertData *alert_data_cp = malloc(sizeof(AlertData));
    *alert_data_cp = alert_data;
    TEST_ASSERT_EQUAL(POST_DEV_CFG_DONE,
                      INA226_fxnTable.cb_init(&sdr_fpga_ps, &fact_sdr_3v_ps_cfg,
                                              alert_data_cp));

    sdr_fpga_ps.obj.alert_cb(INA226_EVT_SOL, 4, 600, value, alert_data_cp);
    sdr_fpga_ps.obj.alert_cb(INA226_EVT_SUL, 4, 600, value, alert_data_cp);
    sdr_fpga_ps.obj.alert_cb(INA226_EVT_BOL, 4, 600, value, alert_data_cp);
    sdr_fpga_ps.obj.alert_cb(INA226_EVT_BUL, 4, 600, value, alert_data_cp);
    sdr_fpga_ps.obj.alert_cb(INA226_EVT_POL, 4, 600, value, alert_data_cp);
    sdr_fpga_ps.obj.alert_cb(INA226_EVT_COL, 4, 600, value, alert_data_cp);
    sdr_fpga_ps.obj.alert_cb(INA226_EVT_CUL, 4, 600, value, alert_data_cp);

    /* Test for memory check */
    sdr_fpga_ps.obj.alert_cb(INA226_EVT_COL, 4, 600, value, NULL);
}
