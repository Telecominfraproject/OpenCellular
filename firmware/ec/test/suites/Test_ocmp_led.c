/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "common/inc/ocmp_wrappers/ocmp_led.h"
#include "include/test_led.h"

extern const I2C_Dev s_sx1509_left_dev;
extern const I2C_Dev s_sx1509_right_dev;
extern HciLedCfg led_hci_ioexp;
extern uint8_t LED_GpioPins[OC_EC_HCI_LED_RESET];
extern uint8_t SX1509_regs[SX1509_REG_TEST_2];
extern uint8_t SX1509_right_regs[SX1509_REG_TEST_2];
extern uint32_t LED_GpioConfig[OC_EC_HCI_LED_RESET];
HciLedCfg ledDriver;
/* ============================= Boilerplate ================================ */
unsigned int s_task_sleep_ticks;

xdc_Void ti_sysbios_knl_Task_sleep__E(xdc_UInt32 nticks)
{
    s_task_sleep_ticks += nticks;
}

void post_update_POSTData(POSTData *pData, uint8_t I2CBus, uint8_t devAddress,
                          uint16_t manId, uint16_t devId)
{
    pData->i2cBus = I2CBus;
    pData->devAddr = devAddress;
    pData->manId = manId;
    pData->devId = devId;
}

void suite_setUp(void)
{
    fake_I2C_init();
    fake_I2C_registerDevSimple(OC_CONNECT1_I2C8, LED_SX1509_LEFT_ADDRESS,
                               SX1509_regs, sizeof(SX1509_regs),
                               sizeof(SX1509_regs[0]), sizeof(uint8_t),
                               FAKE_I2C_DEV_BIG_ENDIAN);
    fake_I2C_registerDevSimple(OC_CONNECT1_I2C8, LED_SX1509_RIGHT_ADDRESS,
                               SX1509_right_regs, sizeof(SX1509_right_regs),
                               sizeof(SX1509_right_regs[0]), sizeof(uint8_t),
                               FAKE_I2C_DEV_BIG_ENDIAN);
    FakeGpio_registerDevSimple(LED_GpioPins, LED_GpioConfig);
}

void setUp(void)
{
    memset(SX1509_regs, 0, sizeof(SX1509_regs));
    memset(SX1509_right_regs, 0, sizeof(SX1509_right_regs));
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}
/* ================================ Tests =================================== */
void test_ocmp_led_probe(void)
{
    HciLedCfg *invalidLedDriver = NULL;
    POSTData postData;
    POSTData *invalidPostData = NULL;

    TEST_ASSERT_EQUAL(POST_DEV_FOUND,
                      LED_fxnTable.cb_probe(&led_hci_ioexp, &postData));
    TEST_ASSERT_EQUAL(OC_CONNECT1_I2C8, postData.i2cBus);
    TEST_ASSERT_EQUAL(LED_SX1509_LEFT_ADDRESS, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(LED_POST_MANID, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(LED_POST_DEVID, postData.devId);

    postData.i2cBus = LED_POST_DATA_NULL;
    postData.devAddr = LED_POST_DATA_NULL;
    postData.manId = LED_POST_DATA_NULL;
    postData.devId = LED_POST_DATA_NULL;

    /* Invalid config test */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING,
                      LED_fxnTable.cb_probe(invalidLedDriver, &postData));
    TEST_ASSERT_EQUAL(LED_POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(LED_POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(LED_POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(LED_POST_DATA_NULL, postData.devId);

    /* Invalid post data test */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING,
                      LED_fxnTable.cb_probe(&led_hci_ioexp, invalidPostData));
    TEST_ASSERT_EQUAL(LED_POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(LED_POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(LED_POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(LED_POST_DATA_NULL, postData.devId);
}

void test_ocmp_init_probe(void)
{
    HciLedCfg *invalidLedDriver = NULL;
    SX1509_regs[SX1509_REG_DATA_A] = LED_DEFAULT_VALUE;
    SX1509_regs[SX1509_REG_DATA_B] = LED_DEFAULT_VALUE;
    SX1509_right_regs[SX1509_REG_DATA_A] = LED_DEFAULT_VALUE;
    SX1509_right_regs[SX1509_REG_DATA_B] = LED_DEFAULT_VALUE;

    TEST_ASSERT_EQUAL(POST_DEV_CFG_DONE,
                      LED_fxnTable.cb_init(&led_hci_ioexp, NULL, NULL));
    TEST_ASSERT_EQUAL_HEX8(SX1509_SOFT_RESET_REG_VALUE_2,
                           SX1509_regs[SX1509_REG_RESET]);
    TEST_ASSERT_EQUAL_HEX8(SX1509_SOFT_RESET_REG_VALUE_2,
                           SX1509_right_regs[SX1509_REG_RESET]);
    TEST_ASSERT_EQUAL_HEX8(LED_REG_INPUT_DISABLE_B_VALUE,
                           SX1509_regs[SX1509_REG_INPUT_DISABLE_B]);
    TEST_ASSERT_EQUAL_HEX8(LED_REG_INPUT_DISABLE_B_VALUE,
                           SX1509_right_regs[SX1509_REG_INPUT_DISABLE_B]);
    TEST_ASSERT_EQUAL_HEX8(LED_REG_PULL_UP_B_VALUE,
                           SX1509_regs[SX1509_REG_PULL_UP_B]);
    TEST_ASSERT_EQUAL_HEX8(LED_REG_PULL_UP_B_VALUE,
                           SX1509_right_regs[SX1509_REG_PULL_UP_B]);
    TEST_ASSERT_EQUAL_HEX8(LED_REG_OPEN_DRAIN_B_VALUE,
                           SX1509_regs[SX1509_REG_OPEN_DRAIN_B]);
    TEST_ASSERT_EQUAL_HEX8(LED_REG_OPEN_DRAIN_B_VALUE,
                           SX1509_right_regs[SX1509_REG_OPEN_DRAIN_B]);
    TEST_ASSERT_EQUAL_HEX8(LED_CLOCK_VALUE, SX1509_regs[SX1509_REG_CLOCK]);
    TEST_ASSERT_EQUAL_HEX8(LED_CLOCK_VALUE,
                           SX1509_right_regs[SX1509_REG_CLOCK]);
    TEST_ASSERT_EQUAL_HEX8(REG_MISC_VALUE, SX1509_regs[SX1509_REG_MISC]);
    TEST_ASSERT_EQUAL_HEX8(REG_MISC_VALUE, SX1509_right_regs[SX1509_REG_MISC]);
    TEST_ASSERT_EQUAL_HEX8(LED_DRIVER_ENABLE_B_VALUE,
                           SX1509_regs[SX1509_REG_LED_DRIVER_ENABLE_B]);
    TEST_ASSERT_EQUAL_HEX8(LED_DRIVER_ENABLE_B_VALUE,
                           SX1509_right_regs[SX1509_REG_LED_DRIVER_ENABLE_B]);
    TEST_ASSERT_EQUAL_HEX8(LED_GREEN, SX1509_regs[SX1509_REG_DATA_A]);
    TEST_ASSERT_EQUAL_HEX8(0xD5, SX1509_regs[SX1509_REG_DATA_B]);
    TEST_ASSERT_EQUAL_HEX8(LED_GREEN, SX1509_right_regs[SX1509_REG_DATA_A]);
    TEST_ASSERT_EQUAL_HEX8(0xD5, SX1509_right_regs[SX1509_REG_DATA_B]);

    /* Invalid config test */
    TEST_ASSERT_EQUAL(POST_DEV_CFG_FAIL,
                      LED_fxnTable.cb_init(invalidLedDriver, NULL, NULL));
}

void test_ocmp_led_testpattern_control(void)
{
    HciLedCfg *invalidLedDriver = NULL;
    uint8_t param = HCI_LED_OFF;
    SX1509_regs[SX1509_REG_DATA_A] = LED_GREEN;
    SX1509_regs[SX1509_REG_DATA_B] = LED_GREEN;
    SX1509_right_regs[SX1509_REG_DATA_A] = LED_GREEN;
    SX1509_right_regs[SX1509_REG_DATA_B] = LED_GREEN;

    /* For HCI_LED_OFF */
    TEST_ASSERT_EQUAL(true, led_testpattern_control(&led_hci_ioexp, &param));
    TEST_ASSERT_EQUAL_HEX8(LED_OFF, SX1509_regs[SX1509_REG_DATA_A]);
    TEST_ASSERT_EQUAL_HEX8(LED_OFF, SX1509_regs[SX1509_REG_DATA_B]);
    TEST_ASSERT_EQUAL_HEX8(LED_OFF, SX1509_right_regs[SX1509_REG_DATA_A]);
    TEST_ASSERT_EQUAL_HEX8(LED_OFF, SX1509_right_regs[SX1509_REG_DATA_B]);

    /* For HCI_LED_RED */
    param = HCI_LED_RED;
    TEST_ASSERT_EQUAL(true, led_testpattern_control(&led_hci_ioexp, &param));
    TEST_ASSERT_EQUAL_HEX8(LED_RED, SX1509_regs[SX1509_REG_DATA_A]);
    TEST_ASSERT_EQUAL_HEX8(LED_RED, SX1509_regs[SX1509_REG_DATA_B]);
    TEST_ASSERT_EQUAL_HEX8(LED_RED, SX1509_right_regs[SX1509_REG_DATA_A]);
    TEST_ASSERT_EQUAL_HEX8(LED_RED, SX1509_right_regs[SX1509_REG_DATA_B]);

    /* For HCI_LED_GREEN */
    param = HCI_LED_GREEN;
    TEST_ASSERT_EQUAL(true, led_testpattern_control(&led_hci_ioexp, &param));
    TEST_ASSERT_EQUAL_HEX8(LED_GREEN, SX1509_regs[SX1509_REG_DATA_A]);
    TEST_ASSERT_EQUAL_HEX8(LED_GREEN, SX1509_regs[SX1509_REG_DATA_B]);
    TEST_ASSERT_EQUAL_HEX8(LED_GREEN, SX1509_right_regs[SX1509_REG_DATA_A]);
    TEST_ASSERT_EQUAL_HEX8(LED_GREEN, SX1509_right_regs[SX1509_REG_DATA_B]);

    /* Inavlid param Id */
    param = LED_INVALID_PARAM;
    TEST_ASSERT_EQUAL(false, led_testpattern_control(&led_hci_ioexp, &param));

    /* Invalid config test */
    param = HCI_LED_GREEN;
    TEST_ASSERT_EQUAL(false, led_testpattern_control(invalidLedDriver, &param));
}
