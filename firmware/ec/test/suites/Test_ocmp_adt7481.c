/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "unity.h"
#include "inc/devices/adt7481.h"
#include "common/inc/ocmp_wrappers/ocmp_adt7481.h"
#include "common/inc/global/Framework.h"
#include "platform/oc-sdr/schema/schema.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"
#include <string.h>

/* ======================== Constants & variables =========================== */

extern const Component sys_schema[];

static I2C_Dev I2C_DEV = {
    .bus = 7,
    .slave_addr = 0x2F,
};

static I2C_Dev s_invalid_dev = {
    .bus = 7,
    .slave_addr = 0x52,
};

static I2C_Dev s_invalid_bus = {
    .bus = 3,
    .slave_addr = 0x2F,
};

typedef enum Adt7481Status {
    ADT7481_STATUS_TEMPERATURE = 0,
} Adt7481Status;

typedef enum Adt7481SConfig {
    ADT7481_CONFIG_LIM_LOW = 0,
    ADT7481_CONFIG_LIM_HIGH,
    ADT7481_CONFIG_LIM_CRIT,
} Adt7481SConfig;

typedef enum Adt7481SAlert {
    ADT7481_ALERT_LOW = 0,
    ADT7481_ALERT_HIGH,
    ADT7481_ALERT_CRITICAL
} Adt7481SAlert;

static uint8_t ADT7481_regs[] = {
    [0x00] = 0x00, /* Local Temperature Value R */
    [0x01] = 0x00, /* Remote 1 Temperature Value High Byte R*/
    [0x02] = 0x00, /* Status Register 1 R */
    [0x03] = 0x00, /* Configuration Register 1 R*/
    [0x04] = 0x00, /* Conversion Rate/Channel Selector R*/
    [0x05] = 0x00, /* Local Temperature High Limit R*/
    [0x06] = 0x00, /* Local Temperature Low Limit R*/
    [0x07] = 0x00, /* Remote 1 Temp High Limit High Byte R*/
    [0x08] = 0x00, /* Remote 1 Temp Low Limit High Byte R*/
    [0x09] = 0x00, /* Configuration Register W*/
    [0x0A] = 0x00, /* Conversion Rate/Channel Selector W */
    [0x0B] = 0x00, /* Local Temperature High Limit  W*/
    [0x0C] = 0x00, /* Local Temperature Low Limit W*/
    [0x0D] = 0x00, /* Remote 1 Temp High Limit High Byte W*/
    [0x0E] = 0x00, /* Remote 1 Temp Low Limit High Byte W*/
    [0x0F] = 0x00, /* One-Shot W*/
    [0x10] = 0x00, /* Remote 1 Temperature Value Low Byte R*/
    [0x11] = 0x00, /* Remote 1 Temperature Offset High Byte R*/
    [0x12] = 0x00, /* Remote 1 Temperature Offset Low Byte R*/
    [0x13] = 0x00, /* Remote 1 Temp High Limit Low Byte R*/
    [0x14] = 0x00, /* Remote 1 Temp Low Limit Low Byte R*/
    [0x19] = 0x00, /* Remote 1 THERM Limit R */
    [0x20] = 0x00, /* Local THERM Limit R*/
    [0x21] = 0x00, /* THERM Hysteresis R*/
    [0x22] = 0x00, /* Consecutive ALERT R*/
    [0x23] = 0x00, /* Status Register 2 R */
    [0x24] = 0x00, /* Configuration 2 Register R*/
    [0x30] = 0x00, /* Remote 2 Temperature Value High Byte R */
    [0x31] = 0x00, /* Remote 2 Temp High Limit High Byte R*/
    [0x32] = 0x00, /* Remote 2 Temp Low Limit High Byte R*/
    [0x33] = 0x00, /* Remote 2 Temperature Value Low Byte R*/
    [0x34] = 0x00, /* Remote 2 Temperature Offset High Byte R*/
    [0x35] = 0x00, /* Remote 2 Temperature Offset Low Byte R*/
    [0x36] = 0x00, /* Remote 2 Temp High Limit Low Byte R */
    [0x37] = 0x00, /* Remote 2 Temp Low Limit Low Byte R*/
    [0x39] = 0x00, /* Remote 2 THERM Limit R*/
    [0x3D] = 0x00, /* Device ID R */
    [0x3E] = 0x00, /* Manufacturer ID R */
};

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

/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    fake_I2C_init();
    fake_I2C_registerDevSimple(I2C_DEV.bus, I2C_DEV.slave_addr, ADT7481_regs,
                               sizeof(ADT7481_regs), sizeof(ADT7481_regs[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_BIG_ENDIAN);
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

void test_probe(void)
{
    POSTData postData;

    /* Correct Dev id */
    ADT7481_regs[0x3D] = 0x81; /* Device ID */
    ADT7481_regs[0x3E] = 0x41; /* MFG ID */

    TEST_ASSERT_EQUAL(POST_DEV_FOUND,
                      ADT7481_fxnTable.cb_probe(&I2C_DEV, &postData));

    /*  Invalid device */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING,
                      ADT7481_fxnTable.cb_probe(&s_invalid_dev, &postData));
    /*  Invalid bus */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING,
                      ADT7481_fxnTable.cb_probe(&s_invalid_bus, &postData));
    /* Incorrect Dev id */
    ADT7481_regs[0x3D] = 0x80; /* Device ID */
    ADT7481_regs[0x3E] = 0x40; /* MFG ID */
    TEST_ASSERT_EQUAL(POST_DEV_ID_MISMATCH,
                      ADT7481_fxnTable.cb_probe(&I2C_DEV, &postData));
}

void test_get_status(void)
{
    uint8_t tempvalue = 0xff;
    ADT7481_regs[0x30] = 0x73;

    /* ADT7481_STATUS_TEMPERATURE */
    TEST_ASSERT_EQUAL(
            true, ADT7481_fxnTable.cb_get_status(
                          &I2C_DEV, ADT7481_STATUS_TEMPERATURE, &tempvalue));
    TEST_ASSERT_EQUAL_HEX8(0x33, tempvalue);

    /* Invalid device */
    TEST_ASSERT_EQUAL(false, ADT7481_fxnTable.cb_get_status(
                                     &s_invalid_dev, ADT7481_STATUS_TEMPERATURE,
                                     &tempvalue));
    /* Invalid bus */
    TEST_ASSERT_EQUAL(false, ADT7481_fxnTable.cb_get_status(
                                     &s_invalid_bus, ADT7481_STATUS_TEMPERATURE,
                                     &tempvalue));
    /* Invalid parameter */
    TEST_ASSERT_EQUAL(false,
                      ADT7481_fxnTable.cb_get_status(&I2C_DEV, 40, &tempvalue));
}

void test_set_config(void)
{
    int8_t limit = 0x62;

    ADT7481_regs[0x32] = 0x00;
    TEST_ASSERT_EQUAL(true, ADT7481_fxnTable.cb_set_config(
                                    &I2C_DEV, ADT7481_CONFIG_LIM_LOW, &limit));
    TEST_ASSERT_EQUAL_HEX8(0xA2, ADT7481_regs[0x32]);

    ADT7481_regs[0x31] = 0x00;
    TEST_ASSERT_EQUAL(true, ADT7481_fxnTable.cb_set_config(
                                    &I2C_DEV, ADT7481_CONFIG_LIM_HIGH, &limit));
    TEST_ASSERT_EQUAL_HEX8(0xA2, ADT7481_regs[0x31]);

    ADT7481_regs[0x39] = 0x00;
    TEST_ASSERT_EQUAL(true, ADT7481_fxnTable.cb_set_config(
                                    &I2C_DEV, ADT7481_CONFIG_LIM_CRIT, &limit));
    TEST_ASSERT_EQUAL_HEX8(0xA2, ADT7481_regs[0x39]);

    /* Invalid Device */
    TEST_ASSERT_EQUAL(false,
                      ADT7481_fxnTable.cb_set_config(
                              &s_invalid_dev, ADT7481_CONFIG_LIM_LOW, &limit));
    /* Invalid bus */
    TEST_ASSERT_EQUAL(false,
                      ADT7481_fxnTable.cb_set_config(
                              &s_invalid_bus, ADT7481_CONFIG_LIM_LOW, &limit));
    /* Invalid Parameter */
    TEST_ASSERT_EQUAL(false,
                      ADT7481_fxnTable.cb_set_config(&I2C_DEV, 40, &limit));
}

void test_get_config(void)
{
    int8_t limit = 0xFF;
    ADT7481_regs[0x31] = 0xA2;
    ADT7481_regs[0x32] = 0xA2;
    ADT7481_regs[0x39] = 0xA2;

    TEST_ASSERT_EQUAL(true, ADT7481_fxnTable.cb_get_config(
                                    &I2C_DEV, ADT7481_CONFIG_LIM_LOW, &limit));
    TEST_ASSERT_EQUAL_HEX8(0x62, limit);

    limit = 0xFF;
    TEST_ASSERT_EQUAL(true, ADT7481_fxnTable.cb_get_config(
                                    &I2C_DEV, ADT7481_CONFIG_LIM_HIGH, &limit));
    TEST_ASSERT_EQUAL_HEX8(0x62, limit);

    limit = 0xFF;
    TEST_ASSERT_EQUAL(true, ADT7481_fxnTable.cb_get_config(
                                    &I2C_DEV, ADT7481_CONFIG_LIM_CRIT, &limit));
    TEST_ASSERT_EQUAL_HEX8(0x62, limit);

    /* Invalid Device */
    TEST_ASSERT_EQUAL(false,
                      ADT7481_fxnTable.cb_get_config(
                              &s_invalid_dev, ADT7481_CONFIG_LIM_LOW, &limit));
    /* Invalid bus */
    TEST_ASSERT_EQUAL(false,
                      ADT7481_fxnTable.cb_get_config(
                              &s_invalid_bus, ADT7481_CONFIG_LIM_LOW, &limit));
    /* Invalid Parameter */
    TEST_ASSERT_EQUAL(false,
                      ADT7481_fxnTable.cb_get_config(&I2C_DEV, 40, &limit));
}

void test_init(void)
{
    const ADT7481_Config fact_sdr_fpga_adt7481_cfg = {
        .lowlimit = -20,
        .highlimit = 75,
        .critlimit = 85,
    };
    TEST_ASSERT_EQUAL(POST_DEV_CFG_DONE,
                      ADT7481_fxnTable.cb_init(
                              &I2C_DEV, &fact_sdr_fpga_adt7481_cfg, NULL));
    TEST_ASSERT_EQUAL_HEX8(0x2C, ADT7481_regs[0x32]);
    TEST_ASSERT_EQUAL_HEX8(0x8B, ADT7481_regs[0x31]);
    TEST_ASSERT_EQUAL_HEX8(0x95, ADT7481_regs[0x39]);

    TEST_ASSERT_EQUAL_HEX8(ADT7481_CONFIGURATION_REG_VALUE, ADT7481_regs[0x09]);
    TEST_ASSERT_EQUAL_HEX8(ADT7481_CONVERSION_RATE_REG_VALUE,
                           ADT7481_regs[0x0A]);

    /* Invalid Device */
    TEST_ASSERT_EQUAL(POST_DEV_CFG_FAIL,
                      ADT7481_fxnTable.cb_init(&s_invalid_dev,
                                               ADT7481_CONFIG_LIM_LOW, NULL));
    TEST_ASSERT_EQUAL(POST_DEV_CFG_FAIL,
                      ADT7481_fxnTable.cb_init(&s_invalid_bus,
                                               ADT7481_CONFIG_LIM_LOW, NULL));
    /* Invalid Parameter */
    TEST_ASSERT_EQUAL(POST_DEV_CFG_FAIL,
                      ADT7481_fxnTable.cb_init(&I2C_DEV, NULL, NULL));
}
