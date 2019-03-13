/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_rfwatchdog.h"

/* ======================== Constants & variables =========================== */
extern bool OcGpio_GpioPins[OC_EC_FE_TRXFE_CONN_RESET];
extern const Component sys_schema[];
extern const I2C_Dev I2C_DEV;
extern const OcGpio_FnTable GpioPCA9557_fnTable;
extern Fe_Watchdog_Cfg fe_watchdog_cfg;
extern Fe_Watchdog_Cfg fe_watchdog_invalid;
extern OcGpio_Port ec_io;
extern OcGpio_Port fe_watchdog_io;
extern RfWatchdog_Cfg fe_ch1_invalid_alert_hb;
extern RfWatchdog_Cfg fe_ch1_invalid_alert_lb;
extern RfWatchdog_Cfg fe_ch1_invalid_interrupt;
extern RfWatchdog_Cfg fe_ch1_watchdog;
extern RfWatchdog_Cfg fe_ch2_invalid_alert_hb;
extern RfWatchdog_Cfg fe_ch2_invalid_alert_lb;
extern RfWatchdog_Cfg fe_ch2_invalid_interrupt;
extern RfWatchdog_Cfg fe_ch2_watchdog;
extern RfWatchdog_Cfg fe_NULL;
extern uint8_t PCA9557_regs[PCA9557_REGS_END];
extern uint32_t OcGpio_GpioConfig[OC_EC_FE_TRXFE_CONN_RESET];

/* ============================= Fake Functions ============================= */
#include <ti/sysbios/knl/Task.h>
unsigned int s_task_sleep_ticks;
xdc_Void ti_sysbios_knl_Task_sleep__E(xdc_UInt32 nticks)
{
    s_task_sleep_ticks += nticks;
}
void test_alert(void)
{
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void OCMP_GenerateAlert(const AlertData *alert_data, unsigned int alert_id,
                        const void *data, const void *lValue,
                        OCMPActionType actionType)
{
}
#pragma GCC diagnostic pop
/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    FakeGpio_registerDevSimple(OcGpio_GpioPins, OcGpio_GpioConfig);
    fake_I2C_init();
    fake_I2C_registerDevSimple(I2C_DEV.bus, I2C_DEV.slave_addr, PCA9557_regs,
                               sizeof(PCA9557_regs), sizeof(PCA9557_regs[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_BIG_ENDIAN);
}
void setUp(void)
{
}
void tearDown(void)
{
}
void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}

/* ================================ Tests =================================== */
void test_init(void)
{
    PCA9557_regs[PCA9557_REGS_DIR_CONFIG] =
        RFWATCHDOG_DIR_CONFIG_DEFAULT_VALUE; /* Dir Config */
    OcGpio_GpioConfig[OC_EC_FE_TRXFE_CONN_RESET] = OCGPIO_CFG_OUTPUT;
    TEST_ASSERT_EQUAL(POST_DEV_CFG_DONE, RFFEWatchdogP_fxnTable.cb_init(
                                             &fe_ch1_watchdog, NULL, NULL));
    TEST_ASSERT_EQUAL_HEX8(RFWATCHDOG_CH1_DIR_CONFIG_VALUE,
                           PCA9557_regs[PCA9557_REGS_DIR_CONFIG]);
    TEST_ASSERT_EQUAL_HEX8(OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_FALLING,
                           OcGpio_GpioConfig[OC_EC_FE_TRXFE_CONN_RESET]);

    OcGpio_GpioConfig[OC_EC_FE_TRXFE_CONN_RESET] =
        RFWATCHDOG_DIR_CONFIG_DEFAULT_VALUE;
    TEST_ASSERT_EQUAL(POST_DEV_CFG_DONE, RFFEWatchdogP_fxnTable.cb_init(
                                             &fe_ch2_watchdog, NULL, NULL));
    TEST_ASSERT_EQUAL_HEX8(RFWATCHDOG_CH2_DIR_CONFIG_VALUE,
                           PCA9557_regs[PCA9557_REGS_DIR_CONFIG]);
    TEST_ASSERT_EQUAL_HEX8(OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_FALLING,
                           OcGpio_GpioConfig[OC_EC_FE_TRXFE_CONN_RESET]);

    /* Invalid Null test */
    TEST_ASSERT_EQUAL(POST_DEV_CFG_FAIL,
                      RFFEWatchdogP_fxnTable.cb_init(&fe_NULL, NULL, NULL));

    /* Invalid ch1 alert_lb test */
    TEST_ASSERT_EQUAL(
        POST_DEV_CFG_FAIL,
        RFFEWatchdogP_fxnTable.cb_init(&fe_ch1_invalid_alert_lb, NULL, NULL));

    /* Invalid ch1 alert_hb test */
    TEST_ASSERT_EQUAL(
        POST_DEV_CFG_FAIL,
        RFFEWatchdogP_fxnTable.cb_init(&fe_ch1_invalid_alert_hb, NULL, NULL));

    /* Invalid ch1 interrupt test */
    TEST_ASSERT_EQUAL(
        POST_DEV_CFG_FAIL,
        RFFEWatchdogP_fxnTable.cb_init(&fe_ch1_invalid_interrupt, NULL, NULL));

    /* Invalid ch2 alert_lb test */
    TEST_ASSERT_EQUAL(
        POST_DEV_CFG_FAIL,
        RFFEWatchdogP_fxnTable.cb_init(&fe_ch2_invalid_alert_lb, NULL, NULL));

    /* Invalid ch2 alert_hb test */
    TEST_ASSERT_EQUAL(
        POST_DEV_CFG_FAIL,
        RFFEWatchdogP_fxnTable.cb_init(&fe_ch2_invalid_alert_hb, NULL, NULL));

    /* Invalid ch2 interrupt test */
    TEST_ASSERT_EQUAL(
        POST_DEV_CFG_FAIL,
        RFFEWatchdogP_fxnTable.cb_init(&fe_ch2_invalid_interrupt, NULL, NULL));
}
