/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "fake/fake_GPIO.h"
#include "helpers/array.h"
#include "helpers/attribute.h"
#include <string.h>
#include "unity.h"

/* ======================== Constants & variables =========================== */

static OcGpio_Port s_fake_io_port = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

static bool OcGpio_GpioPins[] = {
    [0x01] = 0x1, /* Pin = 1 */
    [0x02] = 0x1, /* Pin = 2 */
};

static uint32_t OcGpio_GpioConfig[] = {
    [0x01] = OCGPIO_CFG_INPUT,
    [0x02] = OCGPIO_CFG_INPUT,
};

static OcGpio_Pin s_fake_pin = {
    .port = &s_fake_io_port,
    .idx = 1,
};
/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    FakeGpio_registerDevSimple(OcGpio_GpioPins, OcGpio_GpioConfig);
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

void test_ocgpio_init(void)
{
    TEST_ASSERT_EQUAL(OCGPIO_SUCCESS, OcGpio_init(&s_fake_io_port));
}

void test_ocgpio_read(void)
{
    OcGpio_GpioPins[1] = 1;
    TEST_ASSERT_EQUAL(1, OcGpio_read(&s_fake_pin));
}

void test_ocgpio_write(void)
{
    OcGpio_GpioPins[1] = 1;
    TEST_ASSERT_EQUAL(OCGPIO_SUCCESS, OcGpio_write(&s_fake_pin, 0));
    TEST_ASSERT_EQUAL(0, OcGpio_GpioPins[1]);
}

void test_ocgpio_configure(void)
{
    OcGpio_GpioPins[1] = 1;
    TEST_ASSERT_EQUAL(OCGPIO_SUCCESS, OcGpio_configure(&s_fake_pin, 8));
    TEST_ASSERT_EQUAL(8, OcGpio_GpioConfig[1]);
}
