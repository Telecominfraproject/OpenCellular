/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_dat.h"
/* ======================== Constants & variables =========================== */
extern const DATR5APP_Config fact_ch1_tx_gain_cfg;
extern Fe_Gain_Cfg fe_ch1_gain;
extern Fe_Gain_Cfg fe_ch1_gain_invalid;
extern uint8_t PCA9557_regs[PCA9557_REGS_END];
/* ============================= Fake Functions ============================= */
unsigned int s_task_sleep_ticks;
xdc_Void ti_sysbios_knl_Task_sleep__E(xdc_UInt32 nticks)
{
    s_task_sleep_ticks += nticks;
}
/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    fake_I2C_init();
    fake_I2C_registerDevSimple(OC_CONNECT1_I2C2, RFFE_CHANNEL1_IO_TX_ATTEN_ADDR,
                               PCA9557_regs, sizeof(PCA9557_regs),
                               sizeof(PCA9557_regs[0]), sizeof(uint8_t),
                               FAKE_I2C_DEV_LITTLE_ENDIAN);
}

void setUp(void)
{
    memset(PCA9557_regs, 0, sizeof(PCA9557_regs));
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}

void test_init(void)
{
    PCA9557_regs[PCA9557_REGS_OUTPUT_VALUE] = DAT_DEFAULT_VALUE;
    PCA9557_regs[PCA9557_REGS_POLARITY] = DAT_DEFAULT_VALUE;
    PCA9557_regs[PCA9557_REGS_DIR_CONFIG] = DAT_DEFAULT_VALUE;
    TEST_ASSERT_EQUAL(
        POST_DEV_CFG_DONE,
        DATXXR5APP_fxnTable.cb_init(&fe_ch1_gain, &fact_ch1_tx_gain_cfg, NULL));
    TEST_ASSERT_EQUAL_HEX8(DAT_INIT_OUTPUT_VALUE,
                           PCA9557_regs[PCA9557_REGS_OUTPUT_VALUE]);
    TEST_ASSERT_EQUAL_HEX8(DAT_INIT_POLARITY_VALUE,
                           PCA9557_regs[PCA9557_REGS_POLARITY]);
    TEST_ASSERT_EQUAL_HEX8(DAT_INIT_DIR_CONFIG_VALUE,
                           PCA9557_regs[PCA9557_REGS_DIR_CONFIG]);

    /* Invalid cfg Test */
    TEST_ASSERT_EQUAL(POST_DEV_CFG_FAIL,
                      DATXXR5APP_fxnTable.cb_init(&fe_ch1_gain_invalid,
                                                  &fact_ch1_tx_gain_cfg, NULL));
}

void test_probe(void)
{
    // TODO:Tracked as issue #117
}

void test_set_get_config(void)
{
    int16_t atten = DAT_AATN_VALUE_1;
    int16_t readValue = DAT_DEFAULT_VALUE;

    PCA9557_regs[PCA9557_REGS_OUTPUT_VALUE] = DAT_ATTN1_OUTPUT_VALUE_SET;
    TEST_ASSERT_EQUAL(true, DATXXR5APP_fxnTable.cb_set_config(
                                &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &atten));
    TEST_ASSERT_EQUAL(true,
                      DATXXR5APP_fxnTable.cb_get_config(
                          &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &readValue));
    TEST_ASSERT_EQUAL(DAT_AATN_VALUE_1, readValue);
    TEST_ASSERT_EQUAL_HEX8(DAT_ATTN1_OUTPUT_VALUE_GET,
                           PCA9557_regs[PCA9557_REGS_OUTPUT_VALUE]);

    atten = DAT_AATN_VALUE_0;
    TEST_ASSERT_EQUAL(true, DATXXR5APP_fxnTable.cb_set_config(
                                &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &atten));
    TEST_ASSERT_EQUAL(true,
                      DATXXR5APP_fxnTable.cb_get_config(
                          &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &readValue));
    TEST_ASSERT_EQUAL(DAT_AATN_VALUE_0, readValue);
    TEST_ASSERT_EQUAL_HEX8(DAT_ATTN0_OUTPUT_VALUE_GET,
                           PCA9557_regs[PCA9557_REGS_OUTPUT_VALUE]);

    atten = DAT_AATN_VALUE_2;
    TEST_ASSERT_EQUAL(true, DATXXR5APP_fxnTable.cb_set_config(
                                &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &atten));
    TEST_ASSERT_EQUAL(true,
                      DATXXR5APP_fxnTable.cb_get_config(
                          &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &readValue));
    TEST_ASSERT_EQUAL(DAT_AATN_VALUE_2, readValue);
    TEST_ASSERT_EQUAL_HEX8(DAT_ATTN2_OUTPUT_VALUE_GET,
                           PCA9557_regs[PCA9557_REGS_OUTPUT_VALUE]);

    atten = DAT_AATN_VALUE_4;
    TEST_ASSERT_EQUAL(true, DATXXR5APP_fxnTable.cb_set_config(
                                &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &atten));
    TEST_ASSERT_EQUAL(true,
                      DATXXR5APP_fxnTable.cb_get_config(
                          &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &readValue));
    TEST_ASSERT_EQUAL(DAT_AATN_VALUE_4, readValue);
    TEST_ASSERT_EQUAL_HEX8(DAT_ATTN4_OUTPUT_VALUE_GET,
                           PCA9557_regs[PCA9557_REGS_OUTPUT_VALUE]);

    atten = DAT_AATN_VALUE_8;
    TEST_ASSERT_EQUAL(true, DATXXR5APP_fxnTable.cb_set_config(
                                &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &atten));
    TEST_ASSERT_EQUAL(true,
                      DATXXR5APP_fxnTable.cb_get_config(
                          &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &readValue));
    TEST_ASSERT_EQUAL(DAT_AATN_VALUE_8, readValue);
    TEST_ASSERT_EQUAL_HEX8(DAT_ATTN8_OUTPUT_VALUE_GET,
                           PCA9557_regs[PCA9557_REGS_OUTPUT_VALUE]);

    atten = DAT_AATN_VALUE_16;
    TEST_ASSERT_EQUAL(true, DATXXR5APP_fxnTable.cb_set_config(
                                &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &atten));
    TEST_ASSERT_EQUAL(true,
                      DATXXR5APP_fxnTable.cb_get_config(
                          &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &readValue));
    TEST_ASSERT_EQUAL(DAT_AATN_VALUE_16, readValue);
    TEST_ASSERT_EQUAL_HEX8(DAT_ATTN16_OUTPUT_VALUE_GET,
                           PCA9557_regs[PCA9557_REGS_OUTPUT_VALUE]);

    atten = DAT_AATN_VALUE_32;
    TEST_ASSERT_EQUAL(true, DATXXR5APP_fxnTable.cb_set_config(
                                &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &atten));
    TEST_ASSERT_EQUAL(true,
                      DATXXR5APP_fxnTable.cb_get_config(
                          &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &readValue));
    TEST_ASSERT_EQUAL(DAT_AATN_VALUE_32, readValue);
    TEST_ASSERT_EQUAL_HEX8(DAT_ATTN32_OUTPUT_VALUE_GET,
                           PCA9557_regs[PCA9557_REGS_OUTPUT_VALUE]);

    atten = DAT_AATN_VALUE_41;
    TEST_ASSERT_EQUAL(true, DATXXR5APP_fxnTable.cb_set_config(
                                &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &atten));
    TEST_ASSERT_EQUAL(true,
                      DATXXR5APP_fxnTable.cb_get_config(
                          &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &readValue));
    TEST_ASSERT_EQUAL(DAT_AATN_VALUE_41, readValue);
    TEST_ASSERT_EQUAL_HEX8(DAT_ATTN41_OUTPUT_VALUE_GET,
                           PCA9557_regs[PCA9557_REGS_OUTPUT_VALUE]);

    atten = DAT_AATN_VALUE_51;
    TEST_ASSERT_EQUAL(true, DATXXR5APP_fxnTable.cb_set_config(
                                &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &atten));
    TEST_ASSERT_EQUAL(true,
                      DATXXR5APP_fxnTable.cb_get_config(
                          &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &readValue));
    TEST_ASSERT_EQUAL(DAT_AATN_VALUE_51, readValue);
    TEST_ASSERT_EQUAL_HEX8(DAT_ATTN51_OUTPUT_VALUE_GET,
                           PCA9557_regs[PCA9557_REGS_OUTPUT_VALUE]);

    atten = DAT_AATN_VALUE_63;
    TEST_ASSERT_EQUAL(true, DATXXR5APP_fxnTable.cb_set_config(
                                &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &atten));
    TEST_ASSERT_EQUAL(true,
                      DATXXR5APP_fxnTable.cb_get_config(
                          &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &readValue));
    TEST_ASSERT_EQUAL(DAT_AATN_VALUE_63, readValue);
    TEST_ASSERT_EQUAL_HEX8(DAT_ATTN63_OUTPUT_VALUE_GET,
                           PCA9557_regs[PCA9557_REGS_OUTPUT_VALUE]);

    /* Invalid Parameter */
    TEST_ASSERT_EQUAL(false, DATXXR5APP_fxnTable.cb_set_config(
                                 &fe_ch1_gain, DAT_INVALID_PARAM, &atten));
    TEST_ASSERT_EQUAL(false, DATXXR5APP_fxnTable.cb_get_config(
                                 &fe_ch1_gain, DAT_INVALID_PARAM, &atten));
    /* out of bound value */
    atten = DAT_AATN_VALUE_64;
    /* TODO: The below function should return false. The attenuation range is 0
    to 63(ie 0.5dB to 31.5dB). Fix is needed for this. Tracked as issue #118 */
    TEST_ASSERT_EQUAL(true, DATXXR5APP_fxnTable.cb_set_config(
                                &fe_ch1_gain, DAT_CONFIG_ATTENUATION, &atten));
    TEST_ASSERT_EQUAL_HEX8(DAT_ATTN64_OUTPUT_VALUE_GET,
                           PCA9557_regs[PCA9557_REGS_OUTPUT_VALUE]);
}
