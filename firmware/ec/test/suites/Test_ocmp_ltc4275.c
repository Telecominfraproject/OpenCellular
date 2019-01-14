/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "include/test_ltc4275.h"

extern LTC4275_Dev gbc_pwr_pd;
extern LTC4275_Dev *ltc4275_invalid_cfg;
extern OcGpio_Port ec_io;
extern tPower_PDStatus_Info PDStatus_Info;
extern uint8_t LTC4275_GpioPins[LTC4275_PD_PWRGD_ALERT];
extern uint32_t LTC4275_GpioConfig[LTC4275_PD_PWRGD_ALERT];
/* ============================= Fake Functions ============================= */
unsigned int s_task_sleep_ticks;

xdc_Void ti_sysbios_knl_Task_sleep__E(xdc_UInt32 nticks)
{
    s_task_sleep_ticks += nticks;
}

void test_alert(void)
{
}
/* Parameters are not used as this is just used to test assigning the
   alert_handler right now.*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void OCMP_GenerateAlert(const AlertData *alert_data, unsigned int alert_id,
                        const void *data)
{
}
#pragma GCC diagnostic pop
/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    FakeGpio_registerDevSimple(LTC4275_GpioPins, LTC4275_GpioConfig);
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
void test_ocmp_ltc4275_init(void)
{
    LTC4275_GpioPins[LTC4275_PD_PWRGD_ALERT] = LTC4275_POWERGOOD;
    LTC4275_GpioPins[LTC4275_PWR_PD_NT2P] = LTC4275_CLASSTYPE_1;
    LTC4275_GpioConfig[LTC4275_PD_PWRGD_ALERT] = OCGPIO_CFG_INPUT;
    AlertData alert_data = {
        .subsystem = 1,
        .componentId = 6,
        .deviceId = 0,
    };

    TEST_ASSERT_EQUAL(POST_DEV_CFG_DONE,
                      LTC4275_fxnTable.cb_init(&gbc_pwr_pd, NULL, &alert_data));
    TEST_ASSERT_EQUAL(LTC4275_POWERGOOD,
                      PDStatus_Info.pdStatus.powerGoodStatus);
    TEST_ASSERT_EQUAL(LTC4275_CLASSTYPE_2, PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_BOTH_EDGES,
                      LTC4275_GpioConfig[LTC4275_PD_PWRGD_ALERT]);

    /* Invalid config */
    TEST_ASSERT_EQUAL(
        POST_DEV_CFG_FAIL,
        LTC4275_fxnTable.cb_init(ltc4275_invalid_cfg, NULL, &alert_data));
}

void test_ocmp_ltc4275_probe(void)
{
    POSTData postData;
    LTC4275_GpioPins[LTC4275_PD_PWRGD_ALERT] = LTC4275_POWERGOOD_NOTOK;
    LTC4275_GpioConfig[LTC4275_PD_PWRGD_ALERT] = OCGPIO_CFG_OUTPUT;
    LTC4275_GpioConfig[LTC4275_PWR_PD_NT2P] = OCGPIO_CFG_OUTPUT;
    TEST_ASSERT_EQUAL(POST_DEV_MISSING,
                      LTC4275_fxnTable.cb_probe(&gbc_pwr_pd, &postData));
    TEST_ASSERT_EQUAL(LTC4275_CLASSTYPE_UNKOWN,
                      PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(LTC4275_STATE_NOTOK, PDStatus_Info.state);
    TEST_ASSERT_EQUAL(LTC4275_DISCONNECT_ALERT, PDStatus_Info.pdalert);
    TEST_ASSERT_EQUAL(LTC4275_POWERGOOD_NOTOK,
                      PDStatus_Info.pdStatus.powerGoodStatus);
    TEST_ASSERT_EQUAL(OCGPIO_CFG_INPUT,
                      LTC4275_GpioConfig[LTC4275_PD_PWRGD_ALERT]);
    TEST_ASSERT_EQUAL(OCGPIO_CFG_INPUT,
                      LTC4275_GpioConfig[LTC4275_PWR_PD_NT2P]);
    TEST_ASSERT_EQUAL_HEX8(LTC4275_POST_DATA, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(LTC4275_POST_DATA, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(LTC4275_POST_DATA, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(LTC4275_POST_DATA, postData.devId);

    postData.i2cBus = POST_DATA_NULL;
    postData.devAddr = POST_DATA_NULL;
    postData.manId = POST_DATA_NULL;
    postData.devId = POST_DATA_NULL;
    LTC4275_GpioPins[LTC4275_PD_PWRGD_ALERT] = LTC4275_POWERGOOD;
    LTC4275_GpioConfig[LTC4275_PD_PWRGD_ALERT] = OCGPIO_CFG_OUTPUT;
    LTC4275_GpioConfig[LTC4275_PWR_PD_NT2P] = OCGPIO_CFG_OUTPUT;
    TEST_ASSERT_EQUAL(POST_DEV_FOUND,
                      LTC4275_fxnTable.cb_probe(&gbc_pwr_pd, &postData));
    TEST_ASSERT_EQUAL(LTC4275_CLASSTYPE_UNKOWN,
                      PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(LTC4275_STATE_NOTOK, PDStatus_Info.state);
    TEST_ASSERT_EQUAL(LTC4275_DISCONNECT_ALERT, PDStatus_Info.pdalert);
    TEST_ASSERT_EQUAL(LTC4275_POWERGOOD,
                      PDStatus_Info.pdStatus.powerGoodStatus);
    TEST_ASSERT_EQUAL(OCGPIO_CFG_INPUT,
                      LTC4275_GpioConfig[LTC4275_PD_PWRGD_ALERT]);
    TEST_ASSERT_EQUAL(OCGPIO_CFG_INPUT,
                      LTC4275_GpioConfig[LTC4275_PWR_PD_NT2P]);
    TEST_ASSERT_EQUAL_HEX8(LTC4275_POST_DATA, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(LTC4275_POST_DATA, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(LTC4275_POST_DATA, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(LTC4275_POST_DATA, postData.devId);

    /* Invalid config */
    postData.i2cBus = POST_DATA_NULL;
    postData.devAddr = POST_DATA_NULL;
    postData.manId = POST_DATA_NULL;
    postData.devId = POST_DATA_NULL;
    TEST_ASSERT_EQUAL(POST_DEV_MISSING, LTC4275_fxnTable.cb_probe(
                                            ltc4275_invalid_cfg, &postData));
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devId);
}

void test_ocmp_ltc4275_get_status(void)
{
    ePDPowerState val = 0;
    LTC4275_GpioPins[LTC4275_PD_PWRGD_ALERT] = LTC4275_POWERGOOD;
    TEST_ASSERT_EQUAL(true, LTC4275_fxnTable.cb_get_status(
                                &gbc_pwr_pd, LTC4275_STATUS_POWERGOOD, &val));
    TEST_ASSERT_EQUAL(LTC4275_POWERGOOD, val);

    val = 0;
    LTC4275_GpioPins[LTC4275_PWR_PD_NT2P] = LTC4275_CLASSTYPE_1;
    TEST_ASSERT_EQUAL(true, LTC4275_fxnTable.cb_get_status(
                                &gbc_pwr_pd, LTC4275_STATUS_CLASS, &val));
    TEST_ASSERT_EQUAL(LTC4275_CLASSTYPE_2, val);

    /*Invalid ParamID*/
    LTC4275_GpioPins[LTC4275_PD_PWRGD_ALERT] = LTC4275_POWERGOOD;
    TEST_ASSERT_EQUAL(false, LTC4275_fxnTable.cb_get_status(
                                 &gbc_pwr_pd, LTC4275_INVALID_PARAM_ID, &val));

    /* Invalid config */
    LTC4275_GpioPins[LTC4275_PWR_PD_NT2P] = LTC4275_CLASSTYPE_2;
    TEST_ASSERT_EQUAL(
        false, LTC4275_fxnTable.cb_get_status(ltc4275_invalid_cfg,
                                              LTC4275_STATUS_CLASS, &val));
}

void test_ocmp_ltc4275_alert_handler(void)
{
    AlertData alert_data = {
        .subsystem = 1,
        .componentId = 6,
        .deviceId = 0,
    };
    AlertData *alert_data_cp = malloc(sizeof(AlertData));
    *alert_data_cp = alert_data;
    TEST_ASSERT_EQUAL(POST_DEV_CFG_DONE, LTC4275_fxnTable.cb_init(
                                             &gbc_pwr_pd, NULL, alert_data_cp));

    gbc_pwr_pd.obj.alert_cb(LTC4275_CONNECT_EVT, alert_data_cp);
    gbc_pwr_pd.obj.alert_cb(LTC4275_DISCONNECT_EVT, alert_data_cp);
    gbc_pwr_pd.obj.alert_cb(LTC4275_INCOMPATIBLE_EVT, alert_data_cp);

    /* Test for memory check */
    gbc_pwr_pd.obj.alert_cb(LTC4275_CONNECT_EVT, NULL);

    /* Default case test */
    gbc_pwr_pd.obj.alert_cb(LTC4275_DEFAULT_EVT, alert_data_cp);
}
