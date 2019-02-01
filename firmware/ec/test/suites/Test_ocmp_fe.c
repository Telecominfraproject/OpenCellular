/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "include/test_fe.h"

extern const FE_Band_Cfg fact_ch1_band_cfg;
extern const FE_Band_Cfg fact_ch2_band_cfg;
extern FE_Ch_Band_cfg fe_ch1_bandcfg;
extern FE_Ch_Band_cfg fe_ch2_bandcfg;
/* ============================= Fake Functions ============================= */
unsigned int s_task_sleep_ticks;
xdc_Void ti_sysbios_knl_Task_sleep__E(xdc_UInt32 nticks)
{
    s_task_sleep_ticks += nticks;
}

void test_alert(void)
{
}

/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
}

void setUp(void)
{
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
}
/* ================================ Tests =================================== */
void test_ocmp_fe_init(void)
{
    /* For ch1 */
    TEST_ASSERT_EQUAL(
        POST_DEV_FOUND,
        FE_PARAM_fxnTable.cb_init(&fe_ch1_bandcfg, &fact_ch1_band_cfg, NULL));

    /* For ch2 */
    TEST_ASSERT_EQUAL(
        POST_DEV_FOUND,
        FE_PARAM_fxnTable.cb_init(&fe_ch2_bandcfg, &fact_ch2_band_cfg, NULL));
}

void test_ocmp_fe_get_config(void)
{
    rffeBand returnVal = FE_DEFAULT_VALUE;
    /* For ch1 */
    TEST_ASSERT_EQUAL(true, FE_PARAM_fxnTable.cb_get_config(
                                &fe_ch1_bandcfg, FE_CFG_BAND, &returnVal));

    /* For ch2 */
    TEST_ASSERT_EQUAL(true, FE_PARAM_fxnTable.cb_get_config(
                                &fe_ch2_bandcfg, FE_CFG_BAND, &returnVal));

    /* Invalid Param */
    TEST_ASSERT_EQUAL(false, FE_PARAM_fxnTable.cb_get_config(&fe_ch2_bandcfg,
                                                             FE_INVALID_PARAM,
                                                             &returnVal));
}

void test_ocmp_fe_set_config(void)
{
    rffeBand value = FE_DEFAULT_VALUE;
    /* For ch1 */
    TEST_ASSERT_EQUAL(true, FE_PARAM_fxnTable.cb_set_config(
                                &fe_ch1_bandcfg, FE_CFG_BAND, &value));

    /* For ch2 */
    TEST_ASSERT_EQUAL(true, FE_PARAM_fxnTable.cb_set_config(
                                &fe_ch2_bandcfg, FE_CFG_BAND, &value));

    /* Invalid Param */
    TEST_ASSERT_EQUAL(false, FE_PARAM_fxnTable.cb_set_config(
                                 &fe_ch2_bandcfg, FE_INVALID_PARAM, &value));
}
/* probe function is stack holder only. No need to create test for it */
/* rrfe_ctrl_set_band and rffe_ctrl_get_band are using static structure. No test
 * case are needed for them */
