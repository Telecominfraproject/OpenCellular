/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "unity.h"
#include "drivers/GpioPCA9557.h"

#include "drivers/OcGpio.h"
#include "helpers/array.h"
#include "helpers/attribute.h"

#include "fake/fake_I2C.h"

#include <string.h>

/* ======================== Constants & variables =========================== */
#define I2C_BUS 2
#define I2C_ADDR 0x00

static uint8_t PCA9557_regs[] = {
    [0x00] = 0x00, /* Input values */
    [0x01] = 0x00, /* Output values */
    [0x02] = 0x00, /* Polarity */
    [0x03] = 0x00, /* Dir Config */
};

static const OcGpio_Port s_pca9557_ioexp = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg =
            &(PCA9557_Cfg){
                    .i2c_dev = { I2C_BUS, I2C_ADDR },
            },
    .object_data = &(PCA9557_Obj){},
};

static const OcGpio_Port s_invalid_ioexp = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg =
            &(PCA9557_Cfg){
                    .i2c_dev = { I2C_BUS, 0x01 },
            },
    .object_data = &(PCA9557_Obj){},
};

static const OcGpio_Pin s_invalid_pin = { &s_invalid_ioexp, 0 };
static OcGpio_Pin s_test_pins[8];

/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    fake_I2C_init();

    fake_I2C_registerDevSimple(I2C_BUS, I2C_ADDR, PCA9557_regs,
                               sizeof(PCA9557_regs), sizeof(PCA9557_regs[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
}

void setUp(void)
{
    memset(PCA9557_regs, 0, sizeof(PCA9557_regs));
    OcGpio_init(&s_pca9557_ioexp);

    for (size_t i = 0; i < ARRAY_SIZE(s_test_pins); ++i) {
        s_test_pins[i] = (OcGpio_Pin){
            &s_pca9557_ioexp,
            i,
        };
    }
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}

/* ================================ Tests =================================== */
void test_OcGpio_configure(void)
{
    /* Start by setting some arbitrary input values */
    TEST_ASSERT_EQUAL(OCGPIO_SUCCESS,
                      OcGpio_configure(&s_test_pins[4], OCGPIO_CFG_INPUT));
    TEST_ASSERT_EQUAL_HEX8(0x10, PCA9557_regs[0x03]);

    TEST_ASSERT_EQUAL(OCGPIO_SUCCESS,
                      OcGpio_configure(&s_test_pins[2], OCGPIO_CFG_INPUT));
    TEST_ASSERT_EQUAL_HEX8(0x14, PCA9557_regs[0x03]);

    /* Make sure we can set all inputs properly */
    for (size_t i = 0; i < ARRAY_SIZE(s_test_pins); ++i) {
        TEST_ASSERT_EQUAL(OCGPIO_SUCCESS,
                          OcGpio_configure(&s_test_pins[i], OCGPIO_CFG_INPUT));
    }
    TEST_ASSERT_EQUAL_HEX8(0xFF, PCA9557_regs[0x03]);

    /* These pins don't have a HW-config, so polarity should be 0 */
    TEST_ASSERT_EQUAL_HEX8(0x00, PCA9557_regs[0x02]);

    /* Test some arbitrary outputs - check cfg & output default value*/
    TEST_ASSERT_EQUAL(
            OCGPIO_SUCCESS,
            OcGpio_configure(&s_test_pins[0],
                             OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH));
    TEST_ASSERT_EQUAL_HEX8(0xFE, PCA9557_regs[0x03]);
    TEST_ASSERT_EQUAL_HEX8(0x01, PCA9557_regs[0x01]);

    TEST_ASSERT_EQUAL(OCGPIO_SUCCESS,
                      OcGpio_configure(&s_test_pins[6],
                                       OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW));
    TEST_ASSERT_EQUAL_HEX8(0xBE, PCA9557_regs[0x03]);
    TEST_ASSERT_EQUAL_HEX8(0x01, PCA9557_regs[0x01]);

    /* Make sure we can set all pins to output */
    for (size_t i = 0; i < ARRAY_SIZE(s_test_pins); ++i) {
        TEST_ASSERT_EQUAL(OCGPIO_SUCCESS,
                          OcGpio_configure(&s_test_pins[i], OCGPIO_CFG_OUTPUT));
    }
    TEST_ASSERT_EQUAL_HEX8(0x00, PCA9557_regs[0x03]);

    TEST_ASSERT_EQUAL(OCGPIO_FAILURE,
                      OcGpio_configure(&s_invalid_pin, OCGPIO_CFG_OUTPUT));
}

void test_OcGpio_configure_with_hw_cfg(void)
{
    s_test_pins[1].hw_cfg = OCGPIO_CFG_INVERT;
    s_test_pins[5].hw_cfg = OCGPIO_CFG_INVERT;

    /* Make sure we can set all inputs properly */
    for (size_t i = 0; i < ARRAY_SIZE(s_test_pins); ++i) {
        TEST_ASSERT_EQUAL(OCGPIO_SUCCESS,
                          OcGpio_configure(&s_test_pins[i], OCGPIO_CFG_INPUT));
    }
    TEST_ASSERT_EQUAL_HEX8(0xFF, PCA9557_regs[0x03]);
    TEST_ASSERT_EQUAL_HEX8(0x22, PCA9557_regs[0x02]);
}

void test_GpioPCA9557_write(void)
{
    /* Make sure we can properly write to each pin */
    for (size_t i = 0; i < ARRAY_SIZE(s_test_pins); ++i) {
        PCA9557_regs[0x01] = 0x00;
        s_test_pins[i].hw_cfg = OCGPIO_CFG_POL_STD;
        TEST_ASSERT_EQUAL(OCGPIO_SUCCESS,
                          OcGpio_configure(&s_test_pins[i], OCGPIO_CFG_OUTPUT));
        TEST_ASSERT_EQUAL_HEX8(0x00, PCA9557_regs[0x01]);

        TEST_ASSERT_EQUAL(OCGPIO_SUCCESS, OcGpio_write(&s_test_pins[i], false));
        TEST_ASSERT_EQUAL_HEX8(0x00, PCA9557_regs[0x01]);

        TEST_ASSERT_EQUAL(OCGPIO_SUCCESS, OcGpio_write(&s_test_pins[i], true));
        TEST_ASSERT_EQUAL_HEX8(0x01 << i, PCA9557_regs[0x01]);

        PCA9557_regs[0x01] = 0x00;
        s_test_pins[i].hw_cfg = OCGPIO_CFG_INVERT;
        TEST_ASSERT_EQUAL(OCGPIO_SUCCESS,
                          OcGpio_configure(&s_test_pins[i], OCGPIO_CFG_OUTPUT));
        TEST_ASSERT_EQUAL_HEX8(0x01 << i, PCA9557_regs[0x01]);

        TEST_ASSERT_EQUAL(OCGPIO_SUCCESS, OcGpio_write(&s_test_pins[i], false));
        TEST_ASSERT_EQUAL_HEX8(0x01 << i, PCA9557_regs[0x01]);

        TEST_ASSERT_EQUAL(OCGPIO_SUCCESS, OcGpio_write(&s_test_pins[i], true));
        TEST_ASSERT_EQUAL_HEX8(0x00, PCA9557_regs[0x01]);
    }

    /* Test failure */
    TEST_ASSERT_EQUAL(OCGPIO_FAILURE, OcGpio_write(&s_invalid_pin, true));
}

void test_GpioPCA9557_read_input(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(s_test_pins); ++i) {
        TEST_ASSERT_EQUAL(OCGPIO_SUCCESS,
                          OcGpio_configure(&s_test_pins[i], OCGPIO_CFG_INPUT));

        PCA9557_regs[0x00] = 0x00;
        TEST_ASSERT_EQUAL(0, OcGpio_read(&s_test_pins[i]));

        PCA9557_regs[0x00] = (0x01 << i);
        TEST_ASSERT_EQUAL(1, OcGpio_read(&s_test_pins[i]));
    }

    /* Test failure */
    /* Can't use s_invalid_pin since we need to configure pin first */
    fake_I2C_unregisterDev(I2C_BUS, I2C_ADDR);
    TEST_ASSERT_EQUAL(OCGPIO_FAILURE, OcGpio_read(&s_test_pins[0]));
    fake_I2C_registerDevSimple(I2C_BUS, I2C_ADDR, PCA9557_regs,
                               sizeof(PCA9557_regs), sizeof(PCA9557_regs[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
}

void test_GpioPCA9557_read_output(void)
{
    PCA9557_regs[0x00] = 0x00; /* Input register should be ignored */
    for (size_t i = 0; i < ARRAY_SIZE(s_test_pins); ++i) {
        TEST_ASSERT_EQUAL(OCGPIO_SUCCESS,
                          OcGpio_configure(&s_test_pins[i], OCGPIO_CFG_OUTPUT));

        TEST_ASSERT_EQUAL(OCGPIO_SUCCESS, OcGpio_write(&s_test_pins[i], false));
        TEST_ASSERT_EQUAL(0, OcGpio_read(&s_test_pins[i]));

        TEST_ASSERT_EQUAL(OCGPIO_SUCCESS, OcGpio_write(&s_test_pins[i], true));
        TEST_ASSERT_EQUAL(1, OcGpio_read(&s_test_pins[i]));

        s_test_pins[i].hw_cfg = OCGPIO_CFG_INVERT;
        TEST_ASSERT_EQUAL(OCGPIO_SUCCESS,
                          OcGpio_configure(&s_test_pins[i], OCGPIO_CFG_OUTPUT));

        TEST_ASSERT_EQUAL(OCGPIO_SUCCESS, OcGpio_write(&s_test_pins[i], false));
        TEST_ASSERT_EQUAL(0, OcGpio_read(&s_test_pins[i]));

        TEST_ASSERT_EQUAL(OCGPIO_SUCCESS, OcGpio_write(&s_test_pins[i], true));
        TEST_ASSERT_EQUAL(1, OcGpio_read(&s_test_pins[i]));
    }

    /* Even failing i2c devices can be read if the pin is an output */
    TEST_ASSERT_EQUAL(0, OcGpio_read(&s_invalid_pin));
}
