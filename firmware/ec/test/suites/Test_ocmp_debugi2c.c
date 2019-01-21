/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_debugI2c.h"

extern S_I2C_Cfg debug_I2C1;
extern S_I2C_Cfg I2C_INVALID_DEV;
extern S_OCI2C s_oci2c;
extern S_OCI2C s_oci2c_invalid;
extern S_OCI2C s_oci2c_2byte;
extern uint8_t DEBUG_I2C_regs[DEBUG_I2C_END];
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
    fake_I2C_registerDevSimple(debug_I2C1.bus, s_oci2c.slaveAddress,
                               &DEBUG_I2C_regs, sizeof(DEBUG_I2C_regs),
                               sizeof(DEBUG_I2C_regs[0]), sizeof(uint8_t),
                               FAKE_I2C_DEV_LITTLE_ENDIAN);
}

void setUp(void)
{
    memset(DEBUG_I2C_regs, 0, sizeof(DEBUG_I2C_regs));
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}
/* ================================ Tests =================================== */
void test_i2c_read(void)
{
    DEBUG_I2C_regs[DEBUG_I2C_INTERRUPT_MASK] = DEBUG_I2C_READ_WRITE_VALUE;
    TEST_ASSERT_EQUAL(true, i2c_read(&debug_I2C1, &s_oci2c));
    TEST_ASSERT_EQUAL_HEX8(DEBUG_I2C_READ_WRITE_VALUE, s_oci2c.reg_value);

    /* Invalid bus */
    TEST_ASSERT_EQUAL(false, i2c_read(&I2C_INVALID_DEV, &s_oci2c));
    TEST_ASSERT_EQUAL(false, i2c_read(&debug_I2C1, &s_oci2c_invalid));
}

void test_i2c_write(void)
{
    DEBUG_I2C_regs[DEBUG_I2C_INTERRUPT_MASK] = DEBUG_I2C_DEFAULT_VALUE;
    s_oci2c.reg_value = DEBUG_I2C_READ_WRITE_VALUE;
    TEST_ASSERT_EQUAL(true, i2c_write(&debug_I2C1, &s_oci2c));
    TEST_ASSERT_EQUAL_HEX8(DEBUG_I2C_READ_WRITE_VALUE,
                           DEBUG_I2C_regs[DEBUG_I2C_INTERRUPT_MASK]);

    /* Invalid bus */
    TEST_ASSERT_EQUAL(false, i2c_write(&I2C_INVALID_DEV, &s_oci2c));
    TEST_ASSERT_EQUAL(false, i2c_write(&debug_I2C1, &s_oci2c_invalid));
}
