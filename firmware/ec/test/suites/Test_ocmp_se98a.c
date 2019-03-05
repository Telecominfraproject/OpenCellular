/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_se98a.h"

extern bool SE98A_GpioPins[0x05];
extern const SE98A_Config fact_ap_se98a_ts1_cfg;
extern OcGpio_Port gbc_io_0;
extern SE98A_Dev gbc_gpp_ap_ts1;
extern SE98A_Dev s_invalid_bus;
extern SE98A_Dev s_invalid_device;
extern uint16_t SE98A_regs[SE98A_REG_DEVICE_ID];
extern uint32_t SE98A_GpioConfig[0x05];

/* ============================= Fake Functions ============================= */
unsigned int s_task_sleep_ticks;
xdc_Void ti_sysbios_knl_Task_sleep__E(xdc_UInt32 nticks)
{
    s_task_sleep_ticks += nticks;
}

/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    FakeGpio_registerDevSimple(SE98A_GpioPins, SE98A_GpioConfig);
    fake_I2C_init();
    fake_I2C_init();
    fake_I2C_registerDevSimple(gbc_gpp_ap_ts1.cfg.dev.bus,
                               gbc_gpp_ap_ts1.cfg.dev.slave_addr, SE98A_regs,
                               sizeof(SE98A_regs) + 2, sizeof(SE98A_regs[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_BIG_ENDIAN);
}

void setUp(void)
{
    memset(SE98A_regs, 0, sizeof(SE98A_regs));
    SE98A_regs[SE98A_REG_CAPABILITY] = 0x0037;
    SE98A_regs[SE98A_REG_MFG_ID] = SE98A_MFG_ID;
    SE98A_regs[SE98A_REG_DEVICE_ID] = SE98A_DEVICE_ID;

    s_task_sleep_ticks = 0;
    OcGpio_init(&gbc_io_0);
    se98a_init(&gbc_gpp_ap_ts1);
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}
/* ================================ Tests =================================== */
/* Parameters are not used as this is just used to test assigning the
 *  alert_handler right now. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void OCMP_GenerateAlert(const AlertData *alert_data, unsigned int alert_id,
                        const void *data, const void *lValue,
                        OCMPActionType actionType)
{
}
#pragma GCC diagnostic pop
/* ================================ Tests =================================== */
void test_ocmp_se98a_probe(void)
{
    uint8_t devId = 0;
    /* Test with the actual values
     * (dev id is hi-byte)
     * (1131h = NXP Semiconductors PCI-SIG) */
    POSTData postData;
    SE98A_regs[SE98A_REG_DEVICE_ID] = SE98A_DEVICE_ID;
    SE98A_regs[SE98A_REG_MFG_ID] = SE98A_MFG_ID;
    TEST_ASSERT_EQUAL(POST_DEV_FOUND,
                      SE98_fxnTable.cb_probe(&gbc_gpp_ap_ts1, &postData));
    devId = ocmp_se98a_dev_id(SE98A_regs[SE98A_REG_DEVICE_ID]);
    TEST_ASSERT_EQUAL(gbc_gpp_ap_ts1.cfg.dev.bus, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(gbc_gpp_ap_ts1.cfg.dev.slave_addr, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(SE98A_MFG_ID, postData.manId);
    TEST_ASSERT_EQUAL_HEX16(devId, postData.devId);

    /* Test with an incorrect device ID */
    postData.i2cBus = POST_DATA_NULL;
    postData.devAddr = POST_DATA_NULL;
    postData.manId = POST_DATA_NULL;
    postData.devId = POST_DATA_NULL;
    SE98A_regs[SE98A_REG_DEVICE_ID] = SE98A_INVALID_DEVICE_ID;
    TEST_ASSERT_EQUAL(POST_DEV_ID_MISMATCH,
                      SE98_fxnTable.cb_probe(&gbc_gpp_ap_ts1, &postData));
    TEST_ASSERT_EQUAL(POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devId);

    /* Test with an incorrect mfg ID */
    SE98A_regs[SE98A_REG_DEVICE_ID] = SE98A_DEVICE_ID;
    SE98A_regs[SE98A_REG_MFG_ID] = SE98A_INVALID_MFG_ID;
    TEST_ASSERT_EQUAL(POST_DEV_ID_MISMATCH,
                      SE98_fxnTable.cb_probe(&gbc_gpp_ap_ts1, &postData));
    TEST_ASSERT_EQUAL(POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devId);

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING,
                      SE98_fxnTable.cb_probe(&s_invalid_device, &postData));
    TEST_ASSERT_EQUAL(POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devId);

    /* Test with a missing bus */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING,
                      SE98_fxnTable.cb_probe(&s_invalid_bus, &postData));
    TEST_ASSERT_EQUAL(POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devId);
}

void test_ocmp_se98a_init(void)
{
    int16_t expTempLowLimit = 0;
    int16_t expTempHighLimit = 0;
    int16_t expTempCriticLimit = 0;

    SE98A_regs[SE98A_REG_CONFIG] = SE98A_DEFAULT_INIT_VALUE;
    SE98A_regs[SE98A_REG_HIGH_LIMIT] = SE98A_DEFAULT_INIT_VALUE;
    SE98A_regs[SE98A_REG_LOW_LIMIT] = SE98A_DEFAULT_INIT_VALUE;
    SE98A_regs[SE98A_REG_CRITICAL_LIMIT] = SE98A_DEFAULT_INIT_VALUE;

    AlertData alert_data = {
        .subsystem = GPP_SUBSYSTEM,
        .componentId = AP_COMPONENET,
        .deviceId = GPP_TEMP_SENS_DEVICE_ID,
    };

    /* Init with a pin associated */
    TEST_ASSERT_EQUAL(POST_DEV_CFG_DONE,
                      SE98_fxnTable.cb_init(&gbc_gpp_ap_ts1,
                                            &fact_ap_se98a_ts1_cfg,
                                            &alert_data));
    expTempLowLimit =
        ocmp_se98a_set_temp_limit(fact_ap_se98a_ts1_cfg.limits[0]);
    expTempHighLimit =
        ocmp_se98a_set_temp_limit(fact_ap_se98a_ts1_cfg.limits[1]);
    expTempCriticLimit =
        ocmp_se98a_set_temp_limit(fact_ap_se98a_ts1_cfg.limits[2]);

    /* Test that the enable alert flag is set */
    TEST_ASSERT_BITS_HIGH(SE98A_CFG_EOCTL, SE98A_regs[SE98A_REG_CONFIG]);

    /* Test temprature values */
    TEST_ASSERT_EQUAL_HEX16(expTempLowLimit, SE98A_regs[SE98A_REG_LOW_LIMIT]);
    TEST_ASSERT_EQUAL_HEX16(expTempHighLimit, SE98A_regs[SE98A_REG_HIGH_LIMIT]);
    TEST_ASSERT_EQUAL_HEX16(expTempCriticLimit,
                            SE98A_regs[SE98A_REG_CRITICAL_LIMIT]);

    /* Test with null config */
    SE98A_regs[SE98A_REG_CONFIG] = SE98A_DEFAULT_INIT_VALUE;
    SE98A_regs[SE98A_REG_HIGH_LIMIT] = SE98A_DEFAULT_INIT_VALUE;
    SE98A_regs[SE98A_REG_LOW_LIMIT] = SE98A_DEFAULT_INIT_VALUE;
    SE98A_regs[SE98A_REG_CRITICAL_LIMIT] = SE98A_DEFAULT_INIT_VALUE;

    TEST_ASSERT_EQUAL(
        POST_DEV_CFG_DONE,
        SE98_fxnTable.cb_init(&gbc_gpp_ap_ts1, NULL, &alert_data));

    TEST_ASSERT_EQUAL_HEX16(SE98A_DEFAULT_INIT_VALUE,
                            SE98A_regs[SE98A_REG_LOW_LIMIT]);
    TEST_ASSERT_EQUAL_HEX16(SE98A_DEFAULT_INIT_VALUE,
                            SE98A_regs[SE98A_REG_HIGH_LIMIT]);
    TEST_ASSERT_EQUAL_HEX16(SE98A_DEFAULT_INIT_VALUE,
                            SE98A_regs[SE98A_REG_CRITICAL_LIMIT]);

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(
        POST_DEV_CFG_FAIL,
        SE98_fxnTable.cb_init(&s_invalid_device, NULL, &alert_data));

    /* Test with a missing bus */
    TEST_ASSERT_EQUAL(POST_DEV_CFG_FAIL,
                      SE98_fxnTable.cb_init(&s_invalid_bus, NULL, &alert_data));
}

/* Values are used in the below function are taken as per the datasheet*/
void test_ocmp_se98a_get_status(void)
{
    /*
     * [15..13] Trip Status
     * [12..5] 8-bit integer part
     * [4..1] fractional part
     * [0] RFU
     */
    int8_t statusTemp = 0;
    int8_t expStatusTemp = 0;
    Se98aStatus paramId = SE98A_STATUS_TEMPERATURE;
    SE98A_regs[SE98A_REG_MEASURED_TEMP] = 0x019C;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_get_status(&gbc_gpp_ap_ts1,
                                                        paramId, &statusTemp));
    expStatusTemp =
        ocmp_se98a_get_temp_value(SE98A_regs[SE98A_REG_MEASURED_TEMP]);
    TEST_ASSERT_EQUAL(expStatusTemp, statusTemp);

    statusTemp = 0;
    expStatusTemp = 0;
    SE98A_regs[SE98A_REG_MEASURED_TEMP] = 0x1E64;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_get_status(&gbc_gpp_ap_ts1,
                                                        paramId, &statusTemp));
    expStatusTemp =
        ocmp_se98a_get_temp_value(SE98A_regs[SE98A_REG_MEASURED_TEMP]);
    TEST_ASSERT_EQUAL(expStatusTemp, statusTemp);

    statusTemp = 0;
    expStatusTemp = 0;
    SE98A_regs[SE98A_REG_MEASURED_TEMP] = 0x07C0;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_get_status(&gbc_gpp_ap_ts1,
                                                        paramId, &statusTemp));
    expStatusTemp =
        ocmp_se98a_get_temp_value(SE98A_regs[SE98A_REG_MEASURED_TEMP]);
    TEST_ASSERT_EQUAL(expStatusTemp, statusTemp);

    statusTemp = 0;
    expStatusTemp = 0;
    SE98A_regs[SE98A_REG_MEASURED_TEMP] = 0x1FF0;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_get_status(&gbc_gpp_ap_ts1,
                                                        paramId, &statusTemp));
    expStatusTemp =
        ocmp_se98a_get_temp_value(SE98A_regs[SE98A_REG_MEASURED_TEMP]);
    TEST_ASSERT_EQUAL(expStatusTemp, statusTemp);

    statusTemp = 0;
    expStatusTemp = 0;
    SE98A_regs[SE98A_REG_MEASURED_TEMP] = 0x1C90;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_get_status(&gbc_gpp_ap_ts1,
                                                        paramId, &statusTemp));
    expStatusTemp =
        ocmp_se98a_get_temp_value(SE98A_regs[SE98A_REG_MEASURED_TEMP]);
    TEST_ASSERT_EQUAL(expStatusTemp, statusTemp);

    /* The device shouldn't return temperatures larger than 125, so we only
     * support int8s - everything else is rounded for now */
    statusTemp = 0;
    expStatusTemp = 0;
    SE98A_regs[SE98A_REG_MEASURED_TEMP] = 0x17E0;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_get_status(&gbc_gpp_ap_ts1,
                                                        paramId, &statusTemp));
    expStatusTemp =
        ocmp_se98a_get_temp_value(SE98A_regs[SE98A_REG_MEASURED_TEMP]);
    TEST_ASSERT_EQUAL(expStatusTemp, statusTemp);

    statusTemp = 0;
    expStatusTemp = 0;
    SE98A_regs[SE98A_REG_MEASURED_TEMP] = 0x0B40;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_get_status(&gbc_gpp_ap_ts1,
                                                        paramId, &statusTemp));
    expStatusTemp =
        ocmp_se98a_get_temp_value(SE98A_regs[SE98A_REG_MEASURED_TEMP]);
    TEST_ASSERT_EQUAL(expStatusTemp, statusTemp);

    /* Make sure we mask the status/RFU bits out */
    statusTemp = 0;
    expStatusTemp = 0;
    SE98A_regs[SE98A_REG_MEASURED_TEMP] = 0xFC91;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_get_status(&gbc_gpp_ap_ts1,
                                                        paramId, &statusTemp));
    expStatusTemp =
        ocmp_se98a_get_temp_value(SE98A_regs[SE98A_REG_MEASURED_TEMP]);
    TEST_ASSERT_EQUAL(expStatusTemp, statusTemp);

    /* Test with a missing device */
    statusTemp = 0;
    expStatusTemp = 0;
    SE98A_regs[SE98A_REG_MEASURED_TEMP] = 0xFC91;
    TEST_ASSERT_EQUAL(false, SE98_fxnTable.cb_get_status(&s_invalid_device,
                                                         paramId, &statusTemp));
    /* Test with a missing bus */
    statusTemp = 0;
    expStatusTemp = 0;
    SE98A_regs[SE98A_REG_MEASURED_TEMP] = 0xFC91;
    TEST_ASSERT_EQUAL(false, SE98_fxnTable.cb_get_status(&s_invalid_bus,
                                                         paramId, &statusTemp));
    /* Test with a invalid paramid */
    statusTemp = 0;
    expStatusTemp = 0;
    SE98A_regs[SE98A_REG_MEASURED_TEMP] = 0xFC91;
    TEST_ASSERT_EQUAL(
        false, SE98_fxnTable.cb_get_status(&gbc_gpp_ap_ts1, 1, &statusTemp));
}

/* Helper to let us run through the various limits we can get */
static void
test_ocmp_se98a_get_x_limit(eTempSensor_ConfigParamsId limitToConfig,
                            uint8_t reg_addr)
{
    /* Register map:
     * [15..13] RFU
     * [12] sign
     * [11..4] 9-bit integer part
     * [3..2] fractional part (0.5, 0.25)
     * [1..0] RFU
     */
    int8_t limit = 0;
    int16_t expTempLimit = 0;

    SE98A_regs[reg_addr] = 0x0000;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_get_config(&gbc_gpp_ap_ts1,
                                                        limitToConfig, &limit));
    expTempLimit = ocmp_se98a_get_temp_value(SE98A_regs[reg_addr]);
    TEST_ASSERT_EQUAL(expTempLimit, limit);

    limit = 0;
    expTempLimit = 0;
    SE98A_regs[reg_addr] = 1 << 4;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_get_config(&gbc_gpp_ap_ts1,
                                                        limitToConfig, &limit));
    expTempLimit = ocmp_se98a_get_temp_value(SE98A_regs[reg_addr]);
    TEST_ASSERT_EQUAL(expTempLimit, limit);

    limit = 0;
    expTempLimit = 0;
    SE98A_regs[reg_addr] = 75 << 4;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_get_config(&gbc_gpp_ap_ts1,
                                                        limitToConfig, &limit));
    expTempLimit = ocmp_se98a_get_temp_value(SE98A_regs[reg_addr]);
    TEST_ASSERT_EQUAL(expTempLimit, limit);

    limit = 0;
    expTempLimit = 0;
    SE98A_regs[reg_addr] = 0x019C;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_get_config(&gbc_gpp_ap_ts1,
                                                        limitToConfig, &limit));
    expTempLimit = ocmp_se98a_get_temp_value(SE98A_regs[reg_addr]);
    TEST_ASSERT_EQUAL(expTempLimit, limit);

    limit = 0;
    expTempLimit = 0;
    SE98A_regs[reg_addr] = 0x1B50;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_get_config(&gbc_gpp_ap_ts1,
                                                        limitToConfig, &limit));
    expTempLimit = ocmp_se98a_get_temp_value(SE98A_regs[reg_addr]);
    TEST_ASSERT_EQUAL(expTempLimit, limit);

    limit = 0;
    expTempLimit = 0;
    SE98A_regs[reg_addr] = 0x07F0;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_get_config(&gbc_gpp_ap_ts1,
                                                        limitToConfig, &limit));
    expTempLimit = ocmp_se98a_get_temp_value(SE98A_regs[reg_addr]);
    TEST_ASSERT_EQUAL(expTempLimit, limit);

    limit = 0;
    expTempLimit = 0;
    SE98A_regs[reg_addr] = 0x1800;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_get_config(&gbc_gpp_ap_ts1,
                                                        limitToConfig, &limit));
    expTempLimit = ocmp_se98a_get_temp_value(SE98A_regs[reg_addr]);
    TEST_ASSERT_EQUAL(expTempLimit, limit);

    /* Make sure we mask the RFU bits out */
    limit = 0;
    expTempLimit = 0;
    SE98A_regs[reg_addr] = 0x07FC;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_get_config(&gbc_gpp_ap_ts1,
                                                        limitToConfig, &limit));
    expTempLimit = ocmp_se98a_get_temp_value(SE98A_regs[reg_addr]);
    TEST_ASSERT_EQUAL(expTempLimit, limit);

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(false, SE98_fxnTable.cb_get_config(
                                 &s_invalid_device, limitToConfig, &limit));

    /* Test with a missing bus */
    TEST_ASSERT_EQUAL(false, SE98_fxnTable.cb_get_config(
                                 &s_invalid_bus, limitToConfig, &limit));

    /* Test with a invalid paramId */
    TEST_ASSERT_EQUAL(false,
                      SE98_fxnTable.cb_get_config(&gbc_gpp_ap_ts1, 4, &limit));
}

void test_ocmp_se98a_temp_sens_get_limit(void)
{
    /* Register:
     *  SE98A_REG_HIGH_LIM = SE98A_REG_HIGH_LIMIT
     *  SE98A_REG_LOW_LIM  = SE98A_REG_LOW_LIMIT
     *  SE98A_REG_CRIT_LIM = SE98A_REG_CRITICAL_LIMIT
     */
    test_ocmp_se98a_get_x_limit(SE98A_CONFIG_LIM_LOW, SE98A_REG_LOW_LIMIT);
    test_ocmp_se98a_get_x_limit(SE98A_CONFIG_LIM_HIGH, SE98A_REG_HIGH_LIMIT);
    test_ocmp_se98a_get_x_limit(SE98A_CONFIG_LIM_CRIT,
                                SE98A_REG_CRITICAL_LIMIT);
}

/* Helper to let us run through the various limits we can set */
static void
test_ocmp_se98a_set_x_limit(eTempSensor_ConfigParamsId limitToConfig,
                            uint8_t reg_addr)
{
    /* Register map:
     * [15..13] RFU
     * [12]     SIGN (2's complement)
     * [11..4]  Integer part (8 bits)
     * [3..2]   Fractional part (0.5, 0.25)
     * [1..0]   RFU
     */
    int8_t limit = 0;
    int16_t expTempLimit = 0;

    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_set_config(&gbc_gpp_ap_ts1,
                                                        limitToConfig, &limit));
    expTempLimit = ocmp_se98a_set_temp_limit(limit);
    TEST_ASSERT_EQUAL_HEX16(expTempLimit, SE98A_regs[reg_addr]);

    limit = 1;
    expTempLimit = 0;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_set_config(&gbc_gpp_ap_ts1,
                                                        limitToConfig, &limit));
    expTempLimit = ocmp_se98a_set_temp_limit(limit);
    TEST_ASSERT_EQUAL_HEX16(expTempLimit, SE98A_regs[reg_addr]);

    limit = 75;
    expTempLimit = 0;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_set_config(&gbc_gpp_ap_ts1,
                                                        limitToConfig, &limit));
    expTempLimit = ocmp_se98a_set_temp_limit(limit);
    TEST_ASSERT_EQUAL_HEX16(expTempLimit, SE98A_regs[reg_addr]);

    limit = -75;
    expTempLimit = 0;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_set_config(&gbc_gpp_ap_ts1,
                                                        limitToConfig, &limit));
    expTempLimit = ocmp_se98a_set_temp_limit(limit);
    TEST_ASSERT_EQUAL_HEX16(expTempLimit, SE98A_regs[reg_addr]);

    limit = 127;
    expTempLimit = 0;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_set_config(&gbc_gpp_ap_ts1,
                                                        limitToConfig, &limit));
    expTempLimit = ocmp_se98a_set_temp_limit(limit);
    TEST_ASSERT_EQUAL_HEX16(expTempLimit, SE98A_regs[reg_addr]);

    limit = -128;
    expTempLimit = 0;
    TEST_ASSERT_EQUAL(true, SE98_fxnTable.cb_set_config(&gbc_gpp_ap_ts1,
                                                        limitToConfig, &limit));
    expTempLimit = ocmp_se98a_set_temp_limit(limit);
    TEST_ASSERT_EQUAL_HEX16(expTempLimit, SE98A_regs[reg_addr]);

    /* Test with a missing device */
    SE98A_regs[reg_addr] = 0x0000;
    limit = 20;
    expTempLimit = 0;
    TEST_ASSERT_EQUAL(false, SE98_fxnTable.cb_set_config(
                                 &s_invalid_device, limitToConfig, &limit));
    /* Test with a missing device */
    SE98A_regs[reg_addr] = 0x0000;
    limit = 20;
    expTempLimit = 0;
    TEST_ASSERT_EQUAL(false, SE98_fxnTable.cb_set_config(
                                 &s_invalid_bus, limitToConfig, &limit));

    /* Test with a missing device */
    SE98A_regs[reg_addr] = 0x0000;
    limit = 20;
    expTempLimit = 0;
    TEST_ASSERT_EQUAL(false,
                      SE98_fxnTable.cb_set_config(&gbc_gpp_ap_ts1, 4, &limit));
}

void test_ocmp_se98a_temp_sens_set_limit(void)
{
    /* Register:
     *  SE98A_REG_HIGH_LIM = SE98A_REG_HIGH_LIMIT
     *  SE98A_REG_LOW_LIM  = SE98A_REG_LOW_LIMIT
     *  SE98A_REG_CRIT_LIM = SE98A_REG_CRITICAL_LIMIT
     */
    test_ocmp_se98a_set_x_limit(SE98A_CONFIG_LIM_LOW, SE98A_REG_LOW_LIMIT);
    test_ocmp_se98a_set_x_limit(SE98A_CONFIG_LIM_HIGH, SE98A_REG_HIGH_LIMIT);
    test_ocmp_se98a_set_x_limit(SE98A_CONFIG_LIM_CRIT,
                                SE98A_REG_CRITICAL_LIMIT);
}

void test_ocmp_se98a_alert_handler(void)
{
    int16_t value = 0x0000;

    AlertData alert_data = {
        .subsystem = GPP_SUBSYSTEM,
        .componentId = AP_COMPONENET,
        .deviceId = GPP_TEMP_SENS_DEVICE_ID,
    };

    TEST_ASSERT_EQUAL(POST_DEV_CFG_DONE,
                      SE98_fxnTable.cb_init(&gbc_gpp_ap_ts1,
                                            &fact_ap_se98a_ts1_cfg,
                                            &alert_data));

    gbc_gpp_ap_ts1.obj.alert_cb(SE98A_EVT_ACT, 4, 23, value, &alert_data);
    gbc_gpp_ap_ts1.obj.alert_cb(SE98A_EVT_AAW, 4, 23, value, &alert_data);
    gbc_gpp_ap_ts1.obj.alert_cb(SE98A_EVT_BAW, 4, 23, value, &alert_data);

    /* Test for memory check */
    gbc_gpp_ap_ts1.obj.alert_cb(SE98A_EVT_ACT, 4, 23, value, NULL);

    /* Default case test */
    gbc_gpp_ap_ts1.obj.alert_cb(SE98A_EVT_DEFAULT, 4, 23, value, &alert_data);
}
