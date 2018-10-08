/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "unity.h"
#include "inc/devices/ltc4274.h"
#include "common/inc/ocmp_wrappers/ocmp_ltc4274.h"
#include "common/inc/global/Framework.h"
#include "platform/oc-sdr/schema/schema.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"
#include <string.h>

/* ======================== Constants & variables =========================== */

extern const Component sys_schema[];

static OcGpio_Port s_fake_io_port = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

static I2C_Dev I2C_DEV = {
    .bus = 7,
    .slave_addr = 0x2F,
};

static I2C_Dev I2C_INVALID_DEV = {
    .bus = 7,
    .slave_addr = 0x52,
};

static I2C_Dev I2C_INVALID_BUS = {
    .bus = 3,
    .slave_addr = 0x2F,
};

static LTC4274_Dev s_dev = {
    .cfg =
            {
                    .i2c_dev =
                            {
                                    .bus = 7,
                                    .slave_addr = 0x2F,
                            },
                    .pin_evt = &(OcGpio_Pin){ &s_fake_io_port, 27 },
                    .reset_pin = { &s_fake_io_port, 27 },
            },
};

static LTC4274_Dev s_invalid_dev = {
    .cfg =
            {
                    .i2c_dev =
                            {
                                    .bus = 7,
                                    .slave_addr = 0x52,
                            },
            },
};

static uint8_t LTC4274_regs[] = {
    [0x00] = 0x00, /* INTERRUPT_STATUS */
    [0x01] = 0x00, /* INTERRUPT_MASK  */
    [0x02] = 0x00, /* POWER_EVENT  */
    [0x03] = 0x00, /* POWER_EVENT_COR */
    [0x04] = 0x00, /* DETECT_EVENT */
    [0x05] = 0x00, /* DETECT_EVENT_COR */
    [0x06] = 0x00, /* FAULT_EVENT  */
    [0x07] = 0x00, /* FAULT_EVENT_COR */
    [0x08] = 0x00, /* START_EVENT */
    [0x09] = 0x00, /* START_EVENT_COR */
    [0x0A] = 0x00, /* SUPPLY_EVENT */
    [0x0B] = 0x00, /* SUPPLY_EVENT_COR */
    [0x0C] = 0x00, /* STATUS */
    [0x10] = 0x00, /* POWER_STATUS */
    [0x11] = 0x00, /* PNI_STATUS */
    [0x12] = 0x00, /* OPERATION_MODE */
    [0x13] = 0x00, /* ENABLE_DUSCONNECT SENSING */
    [0x14] = 0x00, /* DETECT_CLASS_ENABLE */
    [0x15] = 0x00, /* MIDSPAN */
    [0x17] = 0x00, /* MCONF */
    [0x18] = 0x00, /* DETPB  */
    [0x19] = 0x00, /* PWRPB  */
    [0x1A] = 0x00, /* RSTPB */
    [0x1B] = 0x00, /* ID */
    [0x1E] = 0x00, /* TLIMIT  */
    [0x30] = 0x00, /* IP1LSB */
    [0x31] = 0x00, /* IP1MSB  */
    [0x32] = 0x00, /* VP1LSB */
    [0x33] = 0x00, /* VP1MSB */
    [0x41] = 0x00, /* FIRMWARE */
    [0x42] = 0x00, /* WDOG  */
    [0x43] = 0x00, /* DEVID  */
    [0x44] = 0x00, /* HP_ENABLE  */
    [0x46] = 0x00, /* HP_MODE  */
    [0x47] = 0x00, /* CUT1 */
    [0x48] = 0x00, /* LIM1 */
    [0x49] = 0x00, /* IHP_STATUS */
};

static bool LTC4274_GpioPins[] = {
    [27] = 00,
};

static uint32_t LTC7274_GpioConfig[] = {
    [27] = 00,
};

typedef enum LTC7274Status {
    LTC7274_STATUS_DETECT = 0,
    LTC7274_STATUS_CLASS,
    LTC7274_STATUS_POWERGOOD,
} LTC7274Status;

typedef enum LTC7274Config {
    LTC4274_CONFIG_OPERATING_MODE = 0,
    LTC4274_CONFIG_DETECT_ENABLE,
    LTC4274_CONFIG_INTERRUPT_MASK,
    LTC4274_CONFIG_INTERRUPT_ENABLE,
    LTC4274_CONFIG_HP_ENABLE
} LTC7274Config;

typedef enum LTC7274Alert {
    LTC4274_ALERT_NO_ACTIVE = 0,
    LTC4274_ALERT_POWER_ENABLE,
    LTC4274_ALERT_POWERGOOD,
    LTC4274_ALERT_DISCONNECT,
    LTC4274_ALERT_DETECTION,
    LTC4274_ALERT_CLASS,
    LTC4274_ALERT_TCUT,
    LTC4274_ALERT_TSTART,
    LTC4274_ALERT_SUPPLY
} LTC7274Alert;

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
    fake_I2C_registerDevSimple(I2C_DEV.bus, I2C_DEV.slave_addr, &LTC4274_regs,
                               sizeof(LTC4274_regs), sizeof(LTC4274_regs[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
    FakeGpio_registerDevSimple(LTC4274_GpioPins, LTC7274_GpioConfig);
}

void setUp(void)
{
    memset(LTC4274_regs, 0, sizeof(LTC4274_regs));
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}

/* ================================ Tests =================================== */
// Parameters are not used as this is just used to test assigning the
//   alert_handler right now.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void OCMP_GenerateAlert(const AlertData *alert_data, unsigned int alert_id,
                        const void *data)
{
    return;
}
#pragma GCC diagnostic pop

void test_probe(void)
{
    POSTData postData;

    /* Correct Dev id */
    LTC4274_regs[0x1B] = (0x0c << 3);
    LTC4274_GpioPins[27] = 1;
    TEST_ASSERT_EQUAL(POST_DEV_FOUND,
                      LTC4274_fxnTable.cb_probe(&s_dev, &postData));
    TEST_ASSERT_EQUAL(LTC7274_GpioConfig[27],
                      OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);
    TEST_ASSERT_EQUAL(0, LTC4274_GpioPins[27]);

    /*  Missing device */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING,
                      LTC4274_fxnTable.cb_probe(&s_invalid_dev, &postData));

    /* Incorrect Dev id */
    LTC4274_regs[0x1B] = (0x0D << 3);
    TEST_ASSERT_EQUAL(POST_DEV_ID_MISMATCH,
                      LTC4274_fxnTable.cb_probe(&s_dev, &postData));
}

void test_get_status(void)
{
    uint8_t value = 0xFF;

    /* success values */
    LTC4274_regs[0x04] = 0xFF;
    LTC4274_regs[0x0C] = 0x01;
    TEST_ASSERT_EQUAL(true, LTC4274_fxnTable.cb_get_status(
                                    &I2C_DEV, LTC7274_STATUS_DETECT, &value));
    TEST_ASSERT_EQUAL_HEX8(0x01, value);

    LTC4274_regs[0x04] = 0xFF;
    LTC4274_regs[0x0C] = 0x2B;
    TEST_ASSERT_EQUAL(true, LTC4274_fxnTable.cb_get_status(
                                    &I2C_DEV, LTC7274_STATUS_CLASS, &value));
    TEST_ASSERT_EQUAL_HEX8(0x02, value);

    LTC4274_regs[0x04] = 0xFF;
    LTC4274_regs[0x10] = 0x00;
    TEST_ASSERT_EQUAL(true,
                      LTC4274_fxnTable.cb_get_status(
                              &I2C_DEV, LTC7274_STATUS_POWERGOOD, &value));
    TEST_ASSERT_EQUAL_HEX8(0x01, value);

    /* invalid paramid */
    LTC4274_regs[0x04] = 0xFF;
    LTC4274_regs[0x0C] = 0x01;
    TEST_ASSERT_EQUAL(false,
                      LTC4274_fxnTable.cb_get_status(&I2C_DEV, 0XFF, &value));

    /* invalid dev-id */
    TEST_ASSERT_EQUAL(false,
                      LTC4274_fxnTable.cb_get_status(
                              &I2C_INVALID_DEV, LTC7274_STATUS_CLASS, &value));
    TEST_ASSERT_EQUAL(
            false, LTC4274_fxnTable.cb_get_status(
                           &I2C_INVALID_DEV, LTC7274_STATUS_POWERGOOD, &value));
    TEST_ASSERT_EQUAL(
            false, LTC4274_fxnTable.cb_get_status(
                           &I2C_INVALID_DEV, LTC7274_STATUS_POWERGOOD, &value));
    /* invalid bus */
    TEST_ASSERT_EQUAL(false,
                      LTC4274_fxnTable.cb_get_status(
                              &I2C_INVALID_BUS, LTC7274_STATUS_CLASS, &value));
    TEST_ASSERT_EQUAL(
            false, LTC4274_fxnTable.cb_get_status(
                           &I2C_INVALID_BUS, LTC7274_STATUS_POWERGOOD, &value));
    TEST_ASSERT_EQUAL(
            false, LTC4274_fxnTable.cb_get_status(
                           &I2C_INVALID_BUS, LTC7274_STATUS_POWERGOOD, &value));
}

void test_set_config(void)
{
    uint8_t value = 0x00;

    /* success values */
    LTC4274_regs[0x12] = 0x00;
    value = 0x51;
    TEST_ASSERT_EQUAL(true,
                      LTC4274_fxnTable.cb_set_config(
                              &I2C_DEV, LTC4274_CONFIG_OPERATING_MODE, &value));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_regs[0x12], value);

    LTC4274_regs[0x14] = 0xFF;
    value = 0x53;
    TEST_ASSERT_EQUAL(true,
                      LTC4274_fxnTable.cb_set_config(
                              &I2C_DEV, LTC4274_CONFIG_DETECT_ENABLE, &value));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_regs[0x14], value);

    LTC4274_regs[0x01] = 0xFF;
    value = 0x54;
    TEST_ASSERT_EQUAL(true,
                      LTC4274_fxnTable.cb_set_config(
                              &I2C_DEV, LTC4274_CONFIG_INTERRUPT_MASK, &value));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_regs[0x01], value);

    LTC4274_regs[0x17] = 0xFF;
    value = true;
    TEST_ASSERT_EQUAL(
            true, LTC4274_fxnTable.cb_set_config(
                          &I2C_DEV, LTC4274_CONFIG_INTERRUPT_ENABLE, &value));
    TEST_ASSERT_EQUAL_HEX8(0x80, LTC4274_regs[0x17]);

    LTC4274_regs[0x44] = 0xFF;
    value = 0x56;
    TEST_ASSERT_EQUAL(true,
                      LTC4274_fxnTable.cb_set_config(
                              &I2C_DEV, LTC4274_CONFIG_HP_ENABLE, &value));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_regs[0x44], value);

    /* Invalid paramid */
    TEST_ASSERT_EQUAL(false,
                      LTC4274_fxnTable.cb_set_config(&I2C_DEV, 0xFF, &value));

    TEST_ASSERT_EQUAL(false, LTC4274_fxnTable.cb_set_config(
                                     &I2C_INVALID_DEV,
                                     LTC4274_CONFIG_OPERATING_MODE, &value));
    TEST_ASSERT_EQUAL(false, LTC4274_fxnTable.cb_set_config(
                                     &I2C_INVALID_DEV,
                                     LTC4274_CONFIG_DETECT_ENABLE, &value));
    TEST_ASSERT_EQUAL(false, LTC4274_fxnTable.cb_set_config(
                                     &I2C_INVALID_DEV,
                                     LTC4274_CONFIG_INTERRUPT_MASK, &value));
    TEST_ASSERT_EQUAL(false, LTC4274_fxnTable.cb_set_config(
                                     &I2C_INVALID_DEV,
                                     LTC4274_CONFIG_INTERRUPT_ENABLE, &value));
    TEST_ASSERT_EQUAL(
            false, LTC4274_fxnTable.cb_set_config(
                           &I2C_INVALID_DEV, LTC4274_CONFIG_HP_ENABLE, &value));

    /* invalid bus */
    TEST_ASSERT_EQUAL(false, LTC4274_fxnTable.cb_set_config(
                                     &I2C_INVALID_BUS,
                                     LTC4274_CONFIG_OPERATING_MODE, &value));
    TEST_ASSERT_EQUAL(false, LTC4274_fxnTable.cb_set_config(
                                     &I2C_INVALID_BUS,
                                     LTC4274_CONFIG_DETECT_ENABLE, &value));
    TEST_ASSERT_EQUAL(false, LTC4274_fxnTable.cb_set_config(
                                     &I2C_INVALID_BUS,
                                     LTC4274_CONFIG_INTERRUPT_MASK, &value));
    TEST_ASSERT_EQUAL(false, LTC4274_fxnTable.cb_set_config(
                                     &I2C_INVALID_BUS,
                                     LTC4274_CONFIG_INTERRUPT_ENABLE, &value));
    TEST_ASSERT_EQUAL(
            false, LTC4274_fxnTable.cb_set_config(
                           &I2C_INVALID_BUS, LTC4274_CONFIG_HP_ENABLE, &value));
}

void test_get_config(void)
{
    uint8_t value = 0x00;

    /* success values */
    LTC4274_regs[0x12] = 0x51;
    TEST_ASSERT_EQUAL(true,
                      LTC4274_fxnTable.cb_get_config(
                              &I2C_DEV, LTC4274_CONFIG_OPERATING_MODE, &value));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_regs[0x12], value);

    LTC4274_regs[0x14] = 0x53;
    TEST_ASSERT_EQUAL(true,
                      LTC4274_fxnTable.cb_get_config(
                              &I2C_DEV, LTC4274_CONFIG_DETECT_ENABLE, &value));
    TEST_ASSERT_EQUAL_HEX8((LTC4274_regs[0x14] & 07), value);

    LTC4274_regs[0x01] = 0x54;
    TEST_ASSERT_EQUAL(true,
                      LTC4274_fxnTable.cb_get_config(
                              &I2C_DEV, LTC4274_CONFIG_INTERRUPT_MASK, &value));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_regs[0x01], value);

    LTC4274_regs[0x17] = 0x80;
    TEST_ASSERT_EQUAL(
            true, LTC4274_fxnTable.cb_get_config(
                          &I2C_DEV, LTC4274_CONFIG_INTERRUPT_ENABLE, &value));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_regs[0x17], value);

    LTC4274_regs[0x44] = 0x56;
    TEST_ASSERT_EQUAL(true,
                      LTC4274_fxnTable.cb_get_config(
                              &I2C_DEV, LTC4274_CONFIG_HP_ENABLE, &value));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_regs[0x44], value);

    /* Invalid paramid */
    TEST_ASSERT_EQUAL(false,
                      LTC4274_fxnTable.cb_get_config(&I2C_DEV, 0xFF, &value));

    TEST_ASSERT_EQUAL(false, LTC4274_fxnTable.cb_get_config(
                                     &I2C_INVALID_DEV,
                                     LTC4274_CONFIG_OPERATING_MODE, &value));
    TEST_ASSERT_EQUAL(false, LTC4274_fxnTable.cb_get_config(
                                     &I2C_INVALID_DEV,
                                     LTC4274_CONFIG_DETECT_ENABLE, &value));
    TEST_ASSERT_EQUAL(false, LTC4274_fxnTable.cb_get_config(
                                     &I2C_INVALID_DEV,
                                     LTC4274_CONFIG_INTERRUPT_MASK, &value));
    TEST_ASSERT_EQUAL(false, LTC4274_fxnTable.cb_get_config(
                                     &I2C_INVALID_DEV,
                                     LTC4274_CONFIG_INTERRUPT_ENABLE, &value));
    TEST_ASSERT_EQUAL(
            false, LTC4274_fxnTable.cb_get_config(
                           &I2C_INVALID_DEV, LTC4274_CONFIG_HP_ENABLE, &value));
    /* Invalid bus */
    TEST_ASSERT_EQUAL(false, LTC4274_fxnTable.cb_get_config(
                                     &I2C_INVALID_BUS,
                                     LTC4274_CONFIG_OPERATING_MODE, &value));
    TEST_ASSERT_EQUAL(false, LTC4274_fxnTable.cb_get_config(
                                     &I2C_INVALID_BUS,
                                     LTC4274_CONFIG_DETECT_ENABLE, &value));
    TEST_ASSERT_EQUAL(false, LTC4274_fxnTable.cb_get_config(
                                     &I2C_INVALID_BUS,
                                     LTC4274_CONFIG_INTERRUPT_MASK, &value));
    TEST_ASSERT_EQUAL(false, LTC4274_fxnTable.cb_get_config(
                                     &I2C_INVALID_BUS,
                                     LTC4274_CONFIG_INTERRUPT_ENABLE, &value));
    TEST_ASSERT_EQUAL(
            false, LTC4274_fxnTable.cb_get_config(
                           &I2C_INVALID_BUS, LTC4274_CONFIG_HP_ENABLE, &value));
}

void test_init(void)
{
    const int alert_token;

    const LTC4274_Config fact_ltc4274_cfg = {
        .operatingMode = LTC4274_AUTO_MODE,
        .detectEnable = LTC4274_DETECT_ENABLE,
        .interruptMask = LTC4274_INTERRUPT_MASK,
        .interruptEnable = true,
        .pseHpEnable = LTC4274_HP_ENABLE,
    };

    TEST_ASSERT_EQUAL(
            POST_DEV_CFG_DONE,
            LTC4274_fxnTable.cb_init(&s_dev, &fact_ltc4274_cfg, &alert_token));

    TEST_ASSERT_EQUAL_HEX8(LTC4274_regs[0x12], LTC4274_AUTO_MODE);
    TEST_ASSERT_EQUAL_HEX8(LTC4274_regs[0x14], LTC4274_DETECT_ENABLE);
    TEST_ASSERT_EQUAL_HEX8(LTC4274_regs[0x01], LTC4274_INTERRUPT_MASK);
    TEST_ASSERT_EQUAL_HEX8(0x80, LTC4274_regs[0x17]);
    TEST_ASSERT_EQUAL_HEX8(LTC4274_regs[0x44], LTC4274_HP_ENABLE);
    TEST_ASSERT_EQUAL(OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_FALLING,
                      LTC7274_GpioConfig[27]);

    TEST_ASSERT_EQUAL(POST_DEV_CFG_FAIL,
                      LTC4274_fxnTable.cb_init(
                              &s_invalid_dev, &fact_ltc4274_cfg, &alert_token));
    TEST_ASSERT_EQUAL(POST_DEV_CFG_DONE,
                      LTC4274_fxnTable.cb_init(&s_dev, NULL, &alert_token));
}
