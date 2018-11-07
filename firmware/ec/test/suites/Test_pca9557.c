/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "unity.h"
#include "inc/devices/pca9557.h"

#include "fake/fake_I2C.h"

#include <string.h>

/* ======================== Constants & variables =========================== */
static uint8_t PCA9557_regs[] = {
    [0x00] = 0x00, /* Input values */
    [0x01] = 0x00, /* Output values */
    [0x02] = 0x00, /* Polarity */
    [0x03] = 0x00, /* Dir Config */
};

static const I2C_Dev pca9557_dev = {
    .bus = 2,
    .slave_addr = 0x00,
};

/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    fake_I2C_init();

    fake_I2C_registerDevSimple(pca9557_dev.bus, pca9557_dev.slave_addr,
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

/* ================================ Tests =================================== */
void test_PCA9557_input(void)
{
    /* Test a couple of different arbitrary input values */
    uint8_t input_val = 0xff;

    PCA9557_regs[0x00] = 0x05;
    TEST_ASSERT_EQUAL(RETURN_OK, PCA9557_getInput(&pca9557_dev, &input_val));
    TEST_ASSERT_EQUAL_HEX8(0x05, input_val);

    PCA9557_regs[0x00] = 0xAA;
    TEST_ASSERT_EQUAL(RETURN_OK, PCA9557_getInput(&pca9557_dev, &input_val));
    TEST_ASSERT_EQUAL_HEX8(0xAA, input_val);
}

void test_PCA9557_output(void)
{
    /* Test getting and setting output values */
    uint8_t output_val = 0xff;

    PCA9557_regs[0x01] = 0x0C;
    TEST_ASSERT_EQUAL(RETURN_OK, PCA9557_getOutput(&pca9557_dev, &output_val));
    TEST_ASSERT_EQUAL_HEX8(0x0C, output_val);

    TEST_ASSERT_EQUAL(RETURN_OK, PCA9557_setOutput(&pca9557_dev, 0x11));
    TEST_ASSERT_EQUAL_HEX8(0x11, PCA9557_regs[0x01]);

    TEST_ASSERT_EQUAL(RETURN_OK, PCA9557_getOutput(&pca9557_dev, &output_val));
    TEST_ASSERT_EQUAL_HEX8(0x11, output_val);
}

void test_PCA9557_polarity(void)
{
    /* Test getting and setting direction config values */
    uint8_t polarity_val = 0xff;

    PCA9557_regs[0x02] = 0xFB;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      PCA9557_getPolarity(&pca9557_dev, &polarity_val));
    TEST_ASSERT_EQUAL_HEX8(0xFB, polarity_val);

    TEST_ASSERT_EQUAL(RETURN_OK, PCA9557_setPolarity(&pca9557_dev, 0x56));
    TEST_ASSERT_EQUAL_HEX8(0x56, PCA9557_regs[0x02]);

    TEST_ASSERT_EQUAL(RETURN_OK,
                      PCA9557_getPolarity(&pca9557_dev, &polarity_val));
    TEST_ASSERT_EQUAL_HEX8(0x56, polarity_val);
}

void test_PCA9557_config(void)
{
    /* Test getting and setting direction config values */
    uint8_t config_val = 0xff;

    PCA9557_regs[0x03] = 0xAB;
    TEST_ASSERT_EQUAL(RETURN_OK, PCA9557_getConfig(&pca9557_dev, &config_val));
    TEST_ASSERT_EQUAL_HEX8(0xAB, config_val);

    TEST_ASSERT_EQUAL(RETURN_OK, PCA9557_setConfig(&pca9557_dev, 0xCD));
    TEST_ASSERT_EQUAL_HEX8(0xCD, PCA9557_regs[0x03]);

    TEST_ASSERT_EQUAL(RETURN_OK, PCA9557_getConfig(&pca9557_dev, &config_val));
    TEST_ASSERT_EQUAL_HEX8(0xCD, config_val);
}

void test_PCA9557_not_present(void)
{
    /* Ensure that we fail properly if the device isn't on the bus */
    uint8_t dummy_val;
    I2C_Dev invalid_dev = pca9557_dev;
    invalid_dev.slave_addr = 0x01;

    TEST_ASSERT_EQUAL(RETURN_NOTOK, PCA9557_getInput(&invalid_dev, &dummy_val));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      PCA9557_getOutput(&invalid_dev, &dummy_val));
    TEST_ASSERT_EQUAL(RETURN_NOTOK, PCA9557_setOutput(&invalid_dev, dummy_val));
    TEST_ASSERT_EQUAL(RETURN_NOTOK, PCA9557_setConfig(&invalid_dev, dummy_val));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      PCA9557_getConfig(&invalid_dev, &dummy_val));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      PCA9557_setPolarity(&invalid_dev, dummy_val));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      PCA9557_getPolarity(&invalid_dev, &dummy_val));
}
