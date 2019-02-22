/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_adt7481.h"
/* ======================== Constants & variables =========================== */
extern const I2C_Dev adt_invalid_bus;
extern const I2C_Dev adt_invalid_dev;
extern I2C_Dev sdr_fpga_ts;
extern uint8_t ADT7481_regs[ADT7481_REG_END];
/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    fake_I2C_init();
    fake_I2C_registerDevSimple(sdr_fpga_ts.bus, sdr_fpga_ts.slave_addr,
                               ADT7481_regs, sizeof(ADT7481_regs) + 1,
                               sizeof(ADT7481_regs[0]), sizeof(uint8_t),
                               FAKE_I2C_DEV_BIG_ENDIAN);
    // FakeGpio_registerDevSimple(ADT7481_GpioPins, ADT7481_GpioConfig);
}

void setUp(void)
{
    memset(ADT7481_regs, 0, sizeof(ADT7481_regs));
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}
/* ================================ Tests =================================== */

void test_adt7481_probe(void)
{
    POSTData postData;

    /* Correct Dev id */
    ADT7481_regs[ADT7481_REG_DEVICE_ID_R] = TEMP_ADT7481_DEV_ID;
    ADT7481_regs[ADT7481_REG_MAN_ID_R] = TEMP_ADT7481_MANF_ID;
    TEST_ASSERT_EQUAL(POST_DEV_FOUND, adt7481_probe(&sdr_fpga_ts, &postData));
    TEST_ASSERT_EQUAL(sdr_fpga_ts.bus, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(sdr_fpga_ts.slave_addr, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(TEMP_ADT7481_DEV_ID, postData.devId);
    TEST_ASSERT_EQUAL_HEX8(TEMP_ADT7481_MANF_ID, postData.manId);

    postData.i2cBus = POST_DATA_NULL;
    postData.devAddr = POST_DATA_NULL;
    postData.manId = POST_DATA_NULL;
    postData.devId = POST_DATA_NULL;
    /*  Missing device */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING,
                      adt7481_probe(&adt_invalid_dev, &postData));
    TEST_ASSERT_EQUAL(POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devId);

    /* Incorrect Dev id */
    ADT7481_regs[ADT7481_REG_DEVICE_ID_R] = ADT7481_INVALID_DEV_ID;
    ADT7481_regs[ADT7481_REG_MAN_ID_R] = TEMP_ADT7481_MANF_ID;
    TEST_ASSERT_EQUAL(POST_DEV_ID_MISMATCH,
                      adt7481_probe(&sdr_fpga_ts, &postData));
    TEST_ASSERT_EQUAL(POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devId);

    /* Incorrect Manf id */
    ADT7481_regs[ADT7481_REG_DEVICE_ID_R] = TEMP_ADT7481_DEV_ID;
    ADT7481_regs[ADT7481_REG_MAN_ID_R] = ADT7481_INVALID_MANF_ID;
    TEST_ASSERT_EQUAL(POST_DEV_ID_MISMATCH,
                      adt7481_probe(&sdr_fpga_ts, &postData));
    TEST_ASSERT_EQUAL(POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devId);
}

void test_adt7481_get_dev_id(void)
{
    uint8_t devId = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_DEVICE_ID_R] = TEMP_ADT7481_DEV_ID;

    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_dev_id(&sdr_fpga_ts, &devId));
    TEST_ASSERT_EQUAL_HEX8(TEMP_ADT7481_DEV_ID, devId);

    /* Incorrect Dev id */
    ADT7481_regs[ADT7481_REG_DEVICE_ID_R] = ADT7481_INVALID_DEV_ID;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_dev_id(&sdr_fpga_ts, &devId));
    TEST_ASSERT_EQUAL_HEX8(ADT7481_INVALID_DEV_ID, devId);
}

void test_adt7481_get_mfg_id(void)
{
    uint8_t mfgId = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_MAN_ID_R] = TEMP_ADT7481_MANF_ID;

    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_mfg_id(&sdr_fpga_ts, &mfgId));
    TEST_ASSERT_EQUAL_HEX8(TEMP_ADT7481_MANF_ID, mfgId);

    /* Incorrect Manf id */
    ADT7481_regs[ADT7481_REG_MAN_ID_R] = ADT7481_INVALID_MANF_ID;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_mfg_id(&sdr_fpga_ts, &mfgId));
    TEST_ASSERT_EQUAL_HEX8(ADT7481_INVALID_MANF_ID, mfgId);
}

void test_adt7481_get_config1(void)
{
    uint8_t configValue = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_CONFIGURATION_1_R] = ADT7481_CONFIG1_VALUE;

    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_get_config1(&sdr_fpga_ts, &configValue));
    TEST_ASSERT_EQUAL_HEX8(ADT7481_CONFIG1_VALUE, configValue);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_get_config1(&adt_invalid_dev, &configValue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_get_config1(&adt_invalid_bus, &configValue));
}

void test_adt7481_set_config1(void)
{
    uint8_t configValue = ADT7481_DEFAULT_VAL;
    /* write register */
    ADT7481_regs[ADT7481_REG_CONFIGURATION_1_W] = ADT7481_DEFAULT_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_set_config1(
                                     &sdr_fpga_ts, ADT7481_CONFIG1_SET_VALUE));
    TEST_ASSERT_EQUAL_HEX8(ADT7481_CONFIG1_SET_VALUE,
                           ADT7481_regs[ADT7481_REG_CONFIGURATION_1_W]);

    ADT7481_regs[ADT7481_REG_CONFIGURATION_1_R] =
        ADT7481_regs[ADT7481_REG_CONFIGURATION_1_W];
    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_get_config1(&sdr_fpga_ts, &configValue));
    TEST_ASSERT_EQUAL_HEX8(ADT7481_CONFIG1_SET_VALUE, configValue);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        adt7481_set_config1(&adt_invalid_dev, ADT7481_CONFIG1_SET_VALUE));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        adt7481_set_config1(&adt_invalid_bus, ADT7481_CONFIG1_SET_VALUE));
}

void test_adt7481_get_convo_rate(void)
{
    uint8_t convRateValue = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_COVERSION_RATE_CHANNEL_SEL_R] =
        ADT7481_COVERSION_RATE_CHANNEL_SEL;

    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_get_conv_rate(&sdr_fpga_ts, &convRateValue));
    TEST_ASSERT_EQUAL_HEX8(ADT7481_COVERSION_RATE_CHANNEL_SEL, convRateValue);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_get_conv_rate(&adt_invalid_dev, &convRateValue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_get_conv_rate(&adt_invalid_bus, &convRateValue));
}

void test_adt7481_set_convo_rate(void)
{
    uint8_t convRateValue = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_COVERSION_RATE_CHANNEL_SEL_W] = 0x00;
    TEST_ASSERT_EQUAL(
        RETURN_OK, adt7481_set_conv_rate(
                       &sdr_fpga_ts, ADT7481_SET_COVERSION_RATE_CHANNEL_SEL));
    TEST_ASSERT_EQUAL_HEX8(
        ADT7481_SET_COVERSION_RATE_CHANNEL_SEL,
        ADT7481_regs[ADT7481_REG_COVERSION_RATE_CHANNEL_SEL_W]);

    ADT7481_regs[ADT7481_REG_COVERSION_RATE_CHANNEL_SEL_R] =
        ADT7481_regs[ADT7481_REG_COVERSION_RATE_CHANNEL_SEL_W];
    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_get_conv_rate(&sdr_fpga_ts, &convRateValue));
    TEST_ASSERT_EQUAL_HEX8(ADT7481_SET_COVERSION_RATE_CHANNEL_SEL,
                           convRateValue);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        adt7481_set_conv_rate(&adt_invalid_dev,
                              ADT7481_SET_COVERSION_RATE_CHANNEL_SEL));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        adt7481_set_conv_rate(&adt_invalid_bus,
                              ADT7481_SET_COVERSION_RATE_CHANNEL_SEL));
}

void test_adt7481_status1(void)
{
    uint8_t statusValue = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_STATUS_1_R] = ADT7481_STATUS_1_R_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_get_status1(&sdr_fpga_ts, &statusValue));
    TEST_ASSERT_EQUAL_HEX8(ADT7481_STATUS_1_R_VAL, statusValue);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_get_status1(&adt_invalid_dev, &statusValue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_get_status1(&adt_invalid_bus, &statusValue));
}

void test_adt7481_status2(void)
{
    uint8_t statusValue = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_STATUS_2_R] = ADT7481_STATUS_2_R_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_get_status2(&sdr_fpga_ts, &statusValue));
    TEST_ASSERT_EQUAL_HEX8(ADT7481_STATUS_2_R_VAL, statusValue);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_get_status2(&adt_invalid_dev, &statusValue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_get_status2(&adt_invalid_bus, &statusValue));
}
void test_adt7481_local_temp_val(void)
{
    int16_t tempvalue = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_LOCAL_TEMP_R] = ADT7481_LOCAL_TEMP_R_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_get_local_temp_val(&sdr_fpga_ts, &tempvalue));
    TEST_ASSERT_EQUAL_HEX8(REG_U8_TO_TEMP(ADT7481_LOCAL_TEMP_R_VAL), tempvalue);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_get_local_temp_val(&adt_invalid_dev, &tempvalue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_get_local_temp_val(&adt_invalid_bus, &tempvalue));
}

void test_adt7481_remote1_temp_val(void)
{
    int16_t tempvalue = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_LOW_BYTE_R] =
        ADT7481_REMOTE_1_TEMP_LOW_BYTE_R_VAL;
    ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_HIGH_BYTE_R] =
        ADT7481_REMOTE_1_TEMP_HIGH_BYTE_R_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_get_remote1_temp_val(&sdr_fpga_ts, &tempvalue));
    TEST_ASSERT_EQUAL_HEX8(
        REG_U16_TO_TEMP(ADT7481_REMOTE_1_TEMP_HIGH_BYTE_R_VAL), tempvalue);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote1_temp_val(
                                        &adt_invalid_dev, &tempvalue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote1_temp_val(
                                        &adt_invalid_dev, &tempvalue));
}

void test_adt7481_remote2_temp_val(void)
{
    int8_t tempvalue = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_LOW_BYTE_R] =
        ADT7481_REMOTE_2_TEMP_LOW_BYTE_R_VAL;
    ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_HIGH_BYTE_R] =
        ADT7481_REMOTE_2_TEMP_HIGH_BYTE_R_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_get_remote2_temp_val(&sdr_fpga_ts, &tempvalue));
    TEST_ASSERT_EQUAL_HEX8(
        REG_U16_TO_TEMP(ADT7481_REMOTE_2_TEMP_HIGH_BYTE_R_VAL), tempvalue);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote2_temp_val(
                                        &adt_invalid_dev, &tempvalue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote2_temp_val(
                                        &adt_invalid_bus, &tempvalue));
}

static void
test_adt7481_get_temp_x_limit(eTempSensorADT7481ConfigParamsId limitToConfig,
                              uint8_t reg_addr)
{
    int16_t limit;

    ADT7481_regs[reg_addr] = ADT7481_TEMP_LIMIT1;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_local_temp_limit(
                                     &sdr_fpga_ts, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(REG_U8_TO_TEMP(ADT7481_TEMP_LIMIT1), limit);

    ADT7481_regs[reg_addr] = ADT7481_TEMP_LIMIT2;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_local_temp_limit(
                                     &sdr_fpga_ts, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(REG_U8_TO_TEMP(ADT7481_TEMP_LIMIT2), limit);

    ADT7481_regs[reg_addr] = ADT7481_TEMP_LIMIT3;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_local_temp_limit(
                                     &sdr_fpga_ts, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(REG_U8_TO_TEMP(ADT7481_TEMP_LIMIT3), limit);

    ADT7481_regs[reg_addr] = ADT7481_TEMP_LIMIT4;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_local_temp_limit(
                                     &sdr_fpga_ts, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(REG_U8_TO_TEMP(ADT7481_TEMP_LIMIT4), limit);
}

void test_adt7481_get_local_temp_limit(void)
{
    test_adt7481_get_temp_x_limit(CONF_TEMP_ADT7481_LOW_LIMIT_REG,
                                  ADT7481_REG_LOCAL_TEMP_LOW_LIMIT_R);
    test_adt7481_get_temp_x_limit(CONF_TEMP_ADT7481_HIGH_LIMIT_REG,
                                  ADT7481_REG_LOCAL_TEMP_HIGH_LIMIT_R);
    // test_adt7481_get_temp_x_limit(CONF_TEMP_ADT7481_THERM_LIMIT_REG,
    // ADT7481_REG_LOCAL_THERM_LIMIT_R);
    test_adt7481_get_temp_x_limit(CONF_TEMP_ADT7481_THERM_LIMIT_REG, 0x20);
}

void test_adt7481_get_invalid_local_temp_limit(void)
{
    int16_t limit;
    ADT7481_regs[ADT7481_REG_LOCAL_TEMP_LOW_LIMIT_R] = ADT7481_TEMP_LIMIT1;
    /* Invalid param */

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        adt7481_get_local_temp_limit(&sdr_fpga_ts,
                                     CONF_TEMP_ADT7481_INVALID_PARAM, &limit));

    /* Invalid dev */
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        adt7481_get_local_temp_limit(
            &adt_invalid_dev, ADT7481_REG_LOCAL_TEMP_LOW_LIMIT_R, &limit));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        adt7481_get_local_temp_limit(
            &adt_invalid_bus, ADT7481_REG_LOCAL_TEMP_LOW_LIMIT_R, &limit));
}

static void
test_adt7481_set_temp_x_limit(eTempSensorADT7481ConfigParamsId limitToConfig,
                              uint8_t reg_addr)
{
    TEST_ASSERT_EQUAL(
        RETURN_OK, adt7481_set_local_temp_limit(&sdr_fpga_ts, limitToConfig,
                                                ADT7481_SET_LOCAL_TEMP_LIMIT1));
    TEST_ASSERT_EQUAL(TEMP_TO_REG_U8(ADT7481_SET_LOCAL_TEMP_LIMIT1),
                      ADT7481_regs[reg_addr]);

    TEST_ASSERT_EQUAL(
        RETURN_OK, adt7481_set_local_temp_limit(&sdr_fpga_ts, limitToConfig,
                                                ADT7481_SET_LOCAL_TEMP_LIMIT2));
    TEST_ASSERT_EQUAL(TEMP_TO_REG_U8(ADT7481_SET_LOCAL_TEMP_LIMIT2),
                      ADT7481_regs[reg_addr]);

    TEST_ASSERT_EQUAL(
        RETURN_OK, adt7481_set_local_temp_limit(&sdr_fpga_ts, limitToConfig,
                                                ADT7481_SET_LOCAL_TEMP_LIMIT3));
    TEST_ASSERT_EQUAL(TEMP_TO_REG_U8(ADT7481_SET_LOCAL_TEMP_LIMIT3),
                      ADT7481_regs[reg_addr]);

    TEST_ASSERT_EQUAL(
        RETURN_OK, adt7481_set_local_temp_limit(&sdr_fpga_ts, limitToConfig,
                                                ADT7481_SET_LOCAL_TEMP_LIMIT4));
    TEST_ASSERT_EQUAL(TEMP_TO_REG_U8(ADT7481_SET_LOCAL_TEMP_LIMIT4),
                      ADT7481_regs[reg_addr]);
}

void test_adt7481_set_local_temp_limit(void)
{
    test_adt7481_set_temp_x_limit(CONF_TEMP_ADT7481_LOW_LIMIT_REG,
                                  ADT7481_REG_LOCAL_TEMP_LOW_LIMIT_W);
    test_adt7481_set_temp_x_limit(CONF_TEMP_ADT7481_HIGH_LIMIT_REG,
                                  ADT7481_REG_LOCAL_TEMP_HIGH_LIMIT_W);
    test_adt7481_set_temp_x_limit(CONF_TEMP_ADT7481_THERM_LIMIT_REG,
                                  ADT7481_REG_LOCAL_THERM_LIMIT_R);
}

void test_adt7481_set_invalid_local_temp_limit(void)
{
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_set_local_temp_limit(
                          &sdr_fpga_ts, CONF_TEMP_ADT7481_INVALID_PARAM,
                          ADT7481_SET_LOCAL_TEMP_LIMIT4));

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_set_local_temp_limit(
                          &adt_invalid_dev, CONF_TEMP_ADT7481_LOW_LIMIT_REG,
                          ADT7481_SET_LOCAL_TEMP_LIMIT4));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_set_local_temp_limit(
                          &adt_invalid_bus, CONF_TEMP_ADT7481_LOW_LIMIT_REG,
                          ADT7481_SET_LOCAL_TEMP_LIMIT4));
}

void test_adt7481_remote1_temp_low_limit(void)
{
    int8_t tempvalue = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_LOW_LIMIT_LOW_BYTE_R] =
        ADT7481_REMOTE_1_TEMP_LOW_LIMIT_LOW_BYTE_R_VAL;
    ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_LOW_LIMIT_R] =
        ADT7481_REMOTE_1_TEMP_LOW_LIMIT_R_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_remote1_temp_low_limit(
                                     &sdr_fpga_ts, &tempvalue));
    TEST_ASSERT_EQUAL_HEX8(
        REG_U16_TO_TEMP(ADT7481_REMOTE_1_TEMP_LOW_LIMIT_R_VAL), tempvalue);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote1_temp_low_limit(
                                        &adt_invalid_dev, &tempvalue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote1_temp_low_limit(
                                        &adt_invalid_bus, &tempvalue));
}

void test_adt7481_remote1_temp_high_limit(void)
{
    int8_t tempvalue = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_HIGH_LIMIT_LOW_BYTE_R] =
        ADT7481_REMOTE_1_TEMP_HIGH_LIMIT_LOW_BYTE_R_VAl;
    ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_HIGH_LIMIT_R] =
        ADT7481_REMOTE_1_TEMP_HIGH_LIMIT_R_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_remote1_temp_high_limit(
                                     &sdr_fpga_ts, &tempvalue));
    TEST_ASSERT_EQUAL_HEX8(
        REG_U16_TO_TEMP(ADT7481_REMOTE_1_TEMP_HIGH_LIMIT_R_VAL), tempvalue);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote1_temp_high_limit(
                                        &adt_invalid_dev, &tempvalue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote1_temp_high_limit(
                                        &adt_invalid_bus, &tempvalue));
}

void test_adt7481_remote1_temp_therm_limit(void)
{
    int8_t tempvalue = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_REMOTE_1_THERM_LIMIT_R] =
        ADT7481_REMOTE_1_THER_LIMIT_R_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_remote1_temp_therm_limit(
                                     &sdr_fpga_ts, &tempvalue));
    TEST_ASSERT_EQUAL_HEX8(REG_U8_TO_TEMP(ADT7481_REMOTE_1_THER_LIMIT_R_VAL),
                           tempvalue);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote1_temp_therm_limit(
                                        &adt_invalid_dev, &tempvalue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote1_temp_therm_limit(
                                        &adt_invalid_bus, &tempvalue));
}

static void test_adt7481_get_remote1_temp_x_limit(
    eTempSensorADT7481ConfigParamsId limitToConfig, uint8_t reg_addr)
{
    int8_t limit = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_LOW_LIMIT_LOW_BYTE_R] =
        ADT7481_REMOTE_1_TEMP_LOW_LIMIT_LOW_BYTE_R_VAL;
    ADT7481_regs[reg_addr] = ADT7481_TEMP_LIMIT1;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_remote1_temp_limit(
                                     &sdr_fpga_ts, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(REG_U16_TO_TEMP(ADT7481_TEMP_LIMIT1), limit);

    ADT7481_regs[reg_addr] = ADT7481_TEMP_LIMIT2;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_remote1_temp_limit(
                                     &sdr_fpga_ts, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(REG_U16_TO_TEMP(ADT7481_TEMP_LIMIT2), limit);

    ADT7481_regs[reg_addr] = ADT7481_TEMP_LIMIT3;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_remote1_temp_limit(
                                     &sdr_fpga_ts, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(REG_U16_TO_TEMP(ADT7481_TEMP_LIMIT3), limit);

    ADT7481_regs[reg_addr] = ADT7481_TEMP_LIMIT4;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_remote1_temp_limit(
                                     &sdr_fpga_ts, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(REG_U16_TO_TEMP(ADT7481_TEMP_LIMIT4), limit);
}

void test_adt7481_get_remote1_temp_limit(void)
{
    test_adt7481_get_remote1_temp_x_limit(
        CONF_TEMP_ADT7481_LOW_LIMIT_REG, ADT7481_REG_REMOTE_1_TEMP_LOW_LIMIT_R);
    test_adt7481_get_remote1_temp_x_limit(
        CONF_TEMP_ADT7481_HIGH_LIMIT_REG,
        ADT7481_REG_REMOTE_1_TEMP_HIGH_LIMIT_R);
    test_adt7481_get_remote1_temp_x_limit(CONF_TEMP_ADT7481_THERM_LIMIT_REG,
                                          ADT7481_REG_REMOTE_1_THERM_LIMIT_R);
}

void test_adt7481_get_invalid_remote1_temp_limit(void)
{
    int8_t limit = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_LOW_LIMIT_LOW_BYTE_R] =
        ADT7481_REMOTE_1_TEMP_LOW_LIMIT_LOW_BYTE_R_VAL;
    ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_LOW_LIMIT_R] = ADT7481_TEMP_LIMIT1;
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        adt7481_get_remote1_temp_limit(
            &sdr_fpga_ts, CONF_TEMP_ADT7481_INVALID_PARAM, &limit));

    /* Invalid dev */
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        adt7481_get_remote1_temp_limit(
            &adt_invalid_dev, ADT7481_REG_REMOTE_1_TEMP_LOW_LIMIT_R, &limit));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        adt7481_get_remote1_temp_limit(
            &adt_invalid_bus, ADT7481_REG_REMOTE_1_TEMP_LOW_LIMIT_R, &limit));
}

void test_adt7481_set_remote1_temp_low_limit(void)
{
    ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_LOW_LIMIT_W] = 0x00;
    ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_LOW_LIMIT_LOW_BYTE_R] = 0x00;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_set_remote1_temp_low_limit(
                                     &sdr_fpga_ts, ADT7481_REMOTE_TEMP_LIMIT));
    TEST_ASSERT_EQUAL_HEX8(TEMP_TO_REG_U16(ADT7481_REMOTE_TEMP_LIMIT) >> 8,
                           ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_LOW_LIMIT_W]);
    TEST_ASSERT_EQUAL_HEX8(
        (uint8_t)(TEMP_TO_REG_U16(ADT7481_REMOTE_TEMP_LIMIT)),
        ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_LOW_LIMIT_LOW_BYTE_R]);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_set_remote1_temp_low_limit(
                          &adt_invalid_dev, ADT7481_REMOTE_TEMP_LIMIT));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_set_remote1_temp_low_limit(
                          &adt_invalid_bus, ADT7481_REMOTE_TEMP_LIMIT));
}

void test_adt7481_set_remote1_temp_high_limit(void)
{
    ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_HIGH_LIMIT_W] = 0x00;
    ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_HIGH_LIMIT_LOW_BYTE_R] = 0x00;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_set_remote1_temp_high_limit(
                                     &sdr_fpga_ts, ADT7481_REMOTE_TEMP_LIMIT));
    TEST_ASSERT_EQUAL_HEX8(
        (uint8_t)(TEMP_TO_REG_U16(ADT7481_REMOTE_TEMP_LIMIT) >> 8),
        ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_HIGH_LIMIT_W]);
    TEST_ASSERT_EQUAL_HEX8(
        (uint8_t)(TEMP_TO_REG_U16(ADT7481_REMOTE_TEMP_LIMIT)),
        ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_HIGH_LIMIT_LOW_BYTE_R]);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_set_remote1_temp_high_limit(
                          &adt_invalid_dev, ADT7481_REMOTE_TEMP_LIMIT));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_set_remote1_temp_high_limit(
                          &adt_invalid_bus, ADT7481_REMOTE_TEMP_LIMIT));
}

void test_adt7481_set_remote1_temp_therm_limit(void)
{
    ADT7481_regs[ADT7481_REG_REMOTE_1_THERM_LIMIT_R] = 0x00;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_set_remote1_temp_therm_limit(
                                     &sdr_fpga_ts, ADT7481_REMOTE_TEMP_LIMIT));
    TEST_ASSERT_EQUAL_HEX8(TEMP_TO_REG_U8(ADT7481_REMOTE_TEMP_LIMIT),
                           ADT7481_regs[ADT7481_REG_REMOTE_1_THERM_LIMIT_R]);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_set_remote1_temp_therm_limit(
                          &adt_invalid_dev, ADT7481_REMOTE_TEMP_LIMIT));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_set_remote1_temp_therm_limit(
                          &adt_invalid_bus, ADT7481_REMOTE_TEMP_LIMIT));
}

void test_adt7481_set_remote1_temp_limit(void)
{
    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_set_remote1_temp_limit(
                          &sdr_fpga_ts, CONF_TEMP_ADT7481_LOW_LIMIT_REG,
                          ADT7481_REMOTE_TEMP_LIMIT));
    TEST_ASSERT_EQUAL_HEX8(TEMP_TO_REG_U16(ADT7481_REMOTE_TEMP_LIMIT) >> 8,
                           ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_LOW_LIMIT_W]);

    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_set_remote1_temp_limit(
                          &sdr_fpga_ts, CONF_TEMP_ADT7481_HIGH_LIMIT_REG,
                          ADT7481_REMOTE_TEMP_LIMIT));
    TEST_ASSERT_EQUAL_HEX8(
        (uint8_t)(TEMP_TO_REG_U16(ADT7481_REMOTE_TEMP_LIMIT) >> 8),
        ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_HIGH_LIMIT_W]);

    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_set_remote1_temp_limit(
                          &sdr_fpga_ts, CONF_TEMP_ADT7481_THERM_LIMIT_REG,
                          ADT7481_REMOTE_TEMP_LIMIT));
    TEST_ASSERT_EQUAL(TEMP_TO_REG_U8(ADT7481_REMOTE_TEMP_LIMIT),
                      ADT7481_regs[ADT7481_REG_REMOTE_1_THERM_LIMIT_R]);

    /* Invalid param */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_set_remote1_temp_limit(
                          &sdr_fpga_ts, CONF_TEMP_ADT7481_INVALID_PARAM,
                          ADT7481_REMOTE_TEMP_LIMIT));

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_set_remote1_temp_limit(
                          &adt_invalid_dev, CONF_TEMP_ADT7481_THERM_LIMIT_REG,
                          ADT7481_REMOTE_TEMP_LIMIT));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_set_remote1_temp_limit(
                          &adt_invalid_bus, CONF_TEMP_ADT7481_THERM_LIMIT_REG,
                          ADT7481_REMOTE_TEMP_LIMIT));
}

void test_adt7481_set_remote2_temp_low_limit(void)
{
    uint8_t tempvalue = ADT7481_REMOTE_TEMP_LIMIT;
    ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_LOW_LIMIT_R] = 0x00;
    ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_LOW_LIMIT_LOW_BYTE_R] = 0x00;
    TEST_ASSERT_EQUAL(
        RETURN_OK, adt7481_set_remote2_temp_low_limit(&sdr_fpga_ts, tempvalue));
    TEST_ASSERT_EQUAL_HEX8(
        TEMP_TO_REG_U16(ADT7481_REMOTE_TEMP_LIMIT),
        ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_LOW_LIMIT_LOW_BYTE_R]);
    TEST_ASSERT_EQUAL_HEX8(
        (uint8_t)(TEMP_TO_REG_U16(ADT7481_REMOTE_TEMP_LIMIT) >> 8),
        ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_LOW_LIMIT_R]);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_set_remote2_temp_low_limit(
                                        &adt_invalid_dev, tempvalue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_set_remote2_temp_low_limit(
                                        &adt_invalid_bus, tempvalue));
}

void test_adt7481_set_remote2_temp_high_limit(void)
{
    uint8_t tempvalue = ADT7481_REMOTE_TEMP_LIMIT;
    ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_HIGH_LIMIT_R] = 0x00;
    ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_HIGH_LIMIT_LOW_BYTE_R] = 0x00;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_set_remote2_temp_high_limit(
                                     &sdr_fpga_ts, tempvalue));
    TEST_ASSERT_EQUAL_HEX8(
        (uint8_t)(TEMP_TO_REG_U16(ADT7481_REMOTE_TEMP_LIMIT) >> 8),
        ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_HIGH_LIMIT_R]);
    TEST_ASSERT_EQUAL_HEX8(
        TEMP_TO_REG_U16(ADT7481_REMOTE_TEMP_LIMIT),
        ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_HIGH_LIMIT_LOW_BYTE_R]);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_set_remote2_temp_high_limit(
                                        &adt_invalid_dev, tempvalue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_set_remote2_temp_high_limit(
                                        &adt_invalid_bus, tempvalue));
}

void test_adt7481_set_remote2_temp_therm_limit(void)
{
    uint8_t tempvalue = ADT7481_REMOTE_TEMP_LIMIT;
    ADT7481_regs[ADT7481_REG_REMOTE_2_THERM_LIMIT_R] = 0x00;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_set_remote2_temp_therm_limit(
                                     &sdr_fpga_ts, tempvalue));
    TEST_ASSERT_EQUAL_HEX8(TEMP_TO_REG_U8(ADT7481_REMOTE_TEMP_LIMIT),
                           ADT7481_regs[ADT7481_REG_REMOTE_2_THERM_LIMIT_R]);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_set_remote2_temp_therm_limit(
                                        &adt_invalid_dev, tempvalue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_set_remote2_temp_therm_limit(
                                        &adt_invalid_bus, tempvalue));
}

void test_adt7481_set_remote2_temp_limit(void)
{
    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_set_remote2_temp_limit(
                          &sdr_fpga_ts, CONF_TEMP_ADT7481_LOW_LIMIT_REG,
                          ADT7481_REMOTE_TEMP_LIMIT));
    TEST_ASSERT_EQUAL_HEX8(
        TEMP_TO_REG_U16(ADT7481_REMOTE_TEMP_LIMIT),
        ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_LOW_LIMIT_LOW_BYTE_R]);
    TEST_ASSERT_EQUAL_HEX8(
        (uint8_t)(TEMP_TO_REG_U16(ADT7481_REMOTE_TEMP_LIMIT) >> 8),
        ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_LOW_LIMIT_R]);

    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_set_remote2_temp_limit(
                          &sdr_fpga_ts, CONF_TEMP_ADT7481_HIGH_LIMIT_REG,
                          ADT7481_REMOTE_TEMP_LIMIT));
    TEST_ASSERT_EQUAL_HEX8(
        (uint8_t)(TEMP_TO_REG_U16(ADT7481_REMOTE_TEMP_LIMIT) >> 8),
        ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_HIGH_LIMIT_R]);
    TEST_ASSERT_EQUAL_HEX8(
        TEMP_TO_REG_U16(ADT7481_REMOTE_TEMP_LIMIT),
        ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_HIGH_LIMIT_LOW_BYTE_R]);

    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_set_remote2_temp_limit(
                          &sdr_fpga_ts, CONF_TEMP_ADT7481_THERM_LIMIT_REG,
                          ADT7481_REMOTE_TEMP_LIMIT));
    TEST_ASSERT_EQUAL_HEX8(TEMP_TO_REG_U8(ADT7481_REMOTE_TEMP_LIMIT),
                           ADT7481_regs[ADT7481_REG_REMOTE_2_THERM_LIMIT_R]);

    /* Invalid param */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_set_remote2_temp_limit(
                          &sdr_fpga_ts, CONF_TEMP_ADT7481_INVALID_PARAM,
                          ADT7481_REMOTE_TEMP_LIMIT));

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_set_remote2_temp_limit(
                          &adt_invalid_dev, CONF_TEMP_ADT7481_THERM_LIMIT_REG,
                          ADT7481_REMOTE_TEMP_LIMIT));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      adt7481_set_remote1_temp_limit(
                          &adt_invalid_bus, CONF_TEMP_ADT7481_THERM_LIMIT_REG,
                          ADT7481_REMOTE_TEMP_LIMIT));
}

void test_adt7481_set_remote1_temp_offset(void)
{
    uint16_t tempvalue = ADT7481_REMOTE_TEMP_LIMIT;
    ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_OFFSET_HIGH_BYTE_R] = 0x00;
    ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_OFFSET_LOW_BYTE_R] = 0x00;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_set_remote1_temp_offset(&sdr_fpga_ts, tempvalue));
    TEST_ASSERT_EQUAL(
        (uint8_t)(TEMP_TO_REG_U16(ADT7481_REMOTE_TEMP_LIMIT) >> 8),
        ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_OFFSET_HIGH_BYTE_R]);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_set_remote1_temp_offset(
                                        &adt_invalid_dev, tempvalue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_set_remote1_temp_offset(
                                        &adt_invalid_bus, tempvalue));
}

void test_adt7481_set_remote2_temp_offset(void)
{
    uint8_t tempvalue = ADT7481_REMOTE_TEMP_LIMIT;
    ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_OFFSET_HIGH_BYTE_R] = 0x00;
    ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_OFFSET_LOW_BYTE_R] = 0x00;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_set_remote2_temp_offset(&sdr_fpga_ts, tempvalue));
    TEST_ASSERT_EQUAL(
        (TEMP_TO_REG_U16(ADT7481_REMOTE_TEMP_LIMIT) >> 8),
        ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_OFFSET_HIGH_BYTE_R]);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_set_remote2_temp_offset(
                                        &adt_invalid_dev, tempvalue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_set_remote2_temp_offset(
                                        &adt_invalid_bus, tempvalue));
}

void test_adt7481_remote2_temp_low_limit(void)
{
    int8_t tempvalue = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_LOW_LIMIT_R] =
        ADT7481_REMOTE_2_TEMP_LIMIT;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_remote2_temp_low_limit(
                                     &sdr_fpga_ts, &tempvalue));
    TEST_ASSERT_EQUAL_HEX8(REG_U16_TO_TEMP(ADT7481_REMOTE_2_TEMP_LIMIT),
                           tempvalue);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote2_temp_low_limit(
                                        &adt_invalid_dev, &tempvalue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote2_temp_low_limit(
                                        &adt_invalid_bus, &tempvalue));
}

void test_adt7481_remote2_temp_high_limit(void)
{
    int8_t tempvalue = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_HIGH_LIMIT_R] =
        ADT7481_REMOTE_2_TEMP_LIMIT;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_remote2_temp_high_limit(
                                     &sdr_fpga_ts, &tempvalue));
    TEST_ASSERT_EQUAL_HEX8(REG_U16_TO_TEMP(ADT7481_REMOTE_2_TEMP_LIMIT),
                           tempvalue);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote2_temp_high_limit(
                                        &adt_invalid_dev, &tempvalue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote2_temp_high_limit(
                                        &adt_invalid_bus, &tempvalue));
}

void test_adt7481_remote2_temp_therm_limit(void)
{
    int8_t tempvalue = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_REMOTE_2_THERM_LIMIT_R] =
        ADT7481_REMOTE_2_TEMP_LIMIT;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_remote2_temp_therm_limit(
                                     &sdr_fpga_ts, &tempvalue));
    TEST_ASSERT_EQUAL_HEX8(REG_U16_TO_TEMP(ADT7481_REMOTE_2_TEMP_LIMIT),
                           tempvalue);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote2_temp_therm_limit(
                                        &adt_invalid_dev, &tempvalue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote2_temp_therm_limit(
                                        &adt_invalid_bus, &tempvalue));
}

static void test_adt7481_get_remote2_temp_x_limit(
    eTempSensorADT7481ConfigParamsId limitToConfig, uint8_t reg_addr)
{
    int8_t limit;
    ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_LOW_LIMIT_LOW_BYTE_R] = 0x67;
    ADT7481_regs[reg_addr] = ADT7481_TEMP_LIMIT1;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_remote2_temp_limit(
                                     &sdr_fpga_ts, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(REG_U16_TO_TEMP(ADT7481_TEMP_LIMIT1), limit);

    ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_HIGH_LIMIT_LOW_BYTE_R] = 0x67;
    ADT7481_regs[reg_addr] = ADT7481_TEMP_LIMIT2;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_remote2_temp_limit(
                                     &sdr_fpga_ts, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(REG_U16_TO_TEMP(ADT7481_TEMP_LIMIT2), limit);

    ADT7481_regs[reg_addr] = ADT7481_TEMP_LIMIT3;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_remote2_temp_limit(
                                     &sdr_fpga_ts, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(REG_U16_TO_TEMP(ADT7481_TEMP_LIMIT3), limit);

    ADT7481_regs[reg_addr] = ADT7481_TEMP_LIMIT4;
    TEST_ASSERT_EQUAL(RETURN_OK, adt7481_get_remote2_temp_limit(
                                     &sdr_fpga_ts, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(REG_U16_TO_TEMP(ADT7481_TEMP_LIMIT4), limit);
}

void test_adt7481_get_remote2_temp_limit(void)
{
    test_adt7481_get_remote2_temp_x_limit(
        CONF_TEMP_ADT7481_LOW_LIMIT_REG, ADT7481_REG_REMOTE_2_TEMP_LOW_LIMIT_R);
    test_adt7481_get_remote2_temp_x_limit(
        CONF_TEMP_ADT7481_HIGH_LIMIT_REG,
        ADT7481_REG_REMOTE_2_TEMP_HIGH_LIMIT_R);
    test_adt7481_get_remote2_temp_x_limit(CONF_TEMP_ADT7481_THERM_LIMIT_REG,
                                          ADT7481_REG_REMOTE_2_THERM_LIMIT_R);
}

void test_adt7481_get_invalid_remote2_temp_limit(void)
{
    int8_t limit;
    ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_LOW_LIMIT_R] = ADT7481_TEMP_LIMIT1;
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        adt7481_get_remote2_temp_limit(
            &sdr_fpga_ts, CONF_TEMP_ADT7481_INVALID_PARAM, &limit));

    /* Invalid dev */
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        adt7481_get_remote2_temp_limit(
            &adt_invalid_dev, CONF_TEMP_ADT7481_LOW_LIMIT_REG, &limit));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        adt7481_get_remote2_temp_limit(
            &adt_invalid_bus, CONF_TEMP_ADT7481_LOW_LIMIT_REG, &limit));
}

void test_adt7481_get_remote1_temp_offset(void)
{
    int16_t tempvalue = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_OFFSET_LOW_BYTE_R] =
        ADT7481_REMOTE_1_TEMP_OFFSET_LOW_BYTE_R;
    ADT7481_regs[ADT7481_REG_REMOTE_1_TEMP_OFFSET_HIGH_BYTE_R] =
        ADT7481_REMOTE_1_TEMP_OFFSET_HIGH_BYTE_R;
    TEST_ASSERT_EQUAL(
        RETURN_OK, adt7481_get_remote1_temp_offset(&sdr_fpga_ts, &tempvalue));
    TEST_ASSERT_EQUAL_HEX8(
        REG_U16_TO_TEMP(ADT7481_REMOTE_1_TEMP_OFFSET_HIGH_BYTE_R), tempvalue);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote1_temp_offset(
                                        &adt_invalid_dev, &tempvalue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote1_temp_offset(
                                        &adt_invalid_bus, &tempvalue));
}

void test_adt7481_get_remote2_temp_offset(void)
{
    int16_t tempvalue = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_REMOTE_2_TEMP_OFFSET_HIGH_BYTE_R] =
        ADT7481_REMOTE_1_TEMP_OFFSET_HIGH_BYTE_R;
    TEST_ASSERT_EQUAL(
        RETURN_OK, adt7481_get_remote2_temp_offset(&sdr_fpga_ts, &tempvalue));
    TEST_ASSERT_EQUAL_HEX8(
        REG_U16_TO_TEMP(ADT7481_REMOTE_1_TEMP_OFFSET_HIGH_BYTE_R), tempvalue);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote2_temp_offset(
                                        &adt_invalid_dev, &tempvalue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_remote2_temp_offset(
                                        &adt_invalid_bus, &tempvalue));
}

void test_adt7481_get_therm_hysteresis(void)
{
    int8_t tempvalue = ADT7481_DEFAULT_VAL;
    ADT7481_regs[ADT7481_REG_THERM_HYSTERESIS_R] =
        ADT7481_REMOTE_1_TEMP_OFFSET_HIGH_BYTE_R;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_get_therm_hysteresis(&sdr_fpga_ts, &tempvalue));
    TEST_ASSERT_EQUAL_HEX8(
        REG_U16_TO_TEMP(ADT7481_REMOTE_1_TEMP_OFFSET_HIGH_BYTE_R), tempvalue);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_therm_hysteresis(
                                        &adt_invalid_dev, &tempvalue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_get_therm_hysteresis(
                                        &adt_invalid_bus, &tempvalue));
}

void test_adt7481_set_therm_hysteresis(void)
{
    int8_t tempvalue = ADT7481_REMOTE_1_TEMP_OFFSET_HIGH_BYTE_R;
    ADT7481_regs[ADT7481_REG_THERM_HYSTERESIS_R] = ADT7481_DEFAULT_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      adt7481_set_therm_hysteresis(&sdr_fpga_ts, tempvalue));
    TEST_ASSERT_EQUAL_HEX8(TEMP_TO_REG_U8(tempvalue),
                           ADT7481_regs[ADT7481_REG_THERM_HYSTERESIS_R]);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_set_therm_hysteresis(
                                        &adt_invalid_dev, tempvalue));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, adt7481_set_therm_hysteresis(
                                        &adt_invalid_bus, tempvalue));
}
