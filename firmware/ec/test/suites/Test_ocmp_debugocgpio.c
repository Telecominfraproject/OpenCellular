/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_debugocgpio.h"

/* ============================= Fake Functions ============================= */

unsigned int s_task_sleep_ticks;
xdc_Void ti_sysbios_knl_Task_sleep__E(xdc_UInt32 nticks)
{
    s_task_sleep_ticks += nticks;
}

GPIO_PinConfig gpioPinConfigs[OC_EC_GPIOCOUNT] = {
    /* fake define */
};

/* ======================== Constants & variables =========================== */
extern bool DEBUG_GpioPins[DEBUG_GPIO_PIN_2];
extern S_OCGPIO s_fake_pin;
extern S_OCGPIO s_pca9557_invalid_pin;
extern S_OCGPIO s_sx1509_invalid_pin;
extern S_OCGPIO_Cfg debug_sdr_ioexpanderx1E_invalid;
extern S_OCGPIO_Cfg debug_ec_gpio_pa;
extern S_OCGPIO_Cfg debug_gbc_ioexpanderx70;
extern S_OCGPIO_Cfg debug_sdr_ioexpanderx1E;
extern uint8_t PCA9557_regs[PCA9557_REGS_END];
extern uint8_t SX1509_regs[SX1509_REG_TEST_2];
extern uint32_t DEBUG_GpioConfig[DEBUG_GPIO_PIN_2];
/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    fake_I2C_init();
    fake_I2C_registerDevSimple(OC_CONNECT1_I2C0, GBC_IO_1_SLAVE_ADDR,
                               &SX1509_regs, sizeof(SX1509_regs),
                               sizeof(SX1509_regs[0]), sizeof(uint8_t),
                               FAKE_I2C_DEV_LITTLE_ENDIAN);
    fake_I2C_registerDevSimple(OC_CONNECT1_I2C3, SDR_FX3_IOEXP_ADDRESS,
                               &PCA9557_regs, sizeof(PCA9557_regs),
                               sizeof(PCA9557_regs[0]), sizeof(uint8_t),
                               FAKE_I2C_DEV_LITTLE_ENDIAN);
    FakeGpio_registerDevSimple(DEBUG_GpioPins, DEBUG_GpioConfig);
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
void test_ocgpio_get(void)
{
    S_OCGPIO *s_oc_gpio = (S_OCGPIO *)&s_fake_pin;
    s_oc_gpio->pin = DEBUG_GPIO_PIN_VALUE;
    s_oc_gpio->value = DEBUG_GPIO_DEFAULT_VALUE;

    PCA9557_regs[PCA9557_REGS_INPUT_VALUE] = DEBUG_GPIO_PIN_2;
    SX1509_regs[SX1509_REG_DATA_B] = DEBUG_GPIO_SX1509_DATA_B_VALUE;
    SX1509_regs[SX1509_REG_DATA_A] = DEBUG_GPIO_SX1509_DATA_A_VALUE;
    DEBUG_GpioPins[DEBUG_GPIO_PIN_2] = OCGPIO_CFG_OUTPUT;
    /* Native Pin */
    TEST_ASSERT_EQUAL(true, ocgpio_get(&debug_ec_gpio_pa, &s_fake_pin));
    TEST_ASSERT_EQUAL(DEBUG_GPIO_PIN_VALUE, s_oc_gpio->value);

    /* connected via SX1509 */
    TEST_ASSERT_EQUAL(true, ocgpio_get(&debug_gbc_ioexpanderx70, &s_fake_pin));
    TEST_ASSERT_EQUAL(DEBUG_GPIO_PIN_VALUE, s_oc_gpio->value);

    /* connected via PCA9557 */
    TEST_ASSERT_EQUAL(true, ocgpio_get(&debug_sdr_ioexpanderx1E, &s_fake_pin));
    TEST_ASSERT_EQUAL(DEBUG_GPIO_PIN_VALUE, s_oc_gpio->value);

    /* Invlaid Slave address */
    TEST_ASSERT_EQUAL(
        false, ocgpio_get(&debug_sdr_ioexpanderx1E_invalid, &s_fake_pin));
}
void test_ocgpio_set(void)
{
    S_OCGPIO *s_oc_gpio = (S_OCGPIO *)&s_fake_pin;
    s_oc_gpio->pin = DEBUG_GPIO_PIN_2;
    s_oc_gpio->value = DEBUG_GPIO_DEFAULT_VALUE;

    PCA9557_regs[PCA9557_REGS_OUTPUT_VALUE] = DEBUG_GPIO_DEFAULT_VALUE;
    SX1509_regs[SX1509_REG_DATA_B] = DEBUG_GPIO_DEFAULT_VALUE;
    SX1509_regs[SX1509_REG_DATA_A] = DEBUG_GPIO_DEFAULT_VALUE;
    DEBUG_GpioPins[DEBUG_GPIO_PIN_2] = OCGPIO_CFG_INPUT;
    /* Native Pin */
    TEST_ASSERT_EQUAL(true, ocgpio_set(&debug_ec_gpio_pa, &s_fake_pin));
    TEST_ASSERT_EQUAL(OCGPIO_CFG_OUTPUT, DEBUG_GpioPins[DEBUG_GPIO_PIN_2]);

    /* connected via SX1509 */
    TEST_ASSERT_EQUAL(true, ocgpio_set(&debug_gbc_ioexpanderx70, &s_fake_pin));
    TEST_ASSERT_EQUAL(DEBUG_GPIO_DEFAULT_VALUE, SX1509_regs[SX1509_REG_DATA_A]);

    /* connected via PCA9557 */
    TEST_ASSERT_EQUAL(true, ocgpio_set(&debug_sdr_ioexpanderx1E, &s_fake_pin));
    TEST_ASSERT_EQUAL(DEBUG_GPIO_DEFAULT_VALUE,
                      PCA9557_regs[PCA9557_REGS_OUTPUT_VALUE]);

    /* Invlaid Slave address */
    TEST_ASSERT_EQUAL(
        false, ocgpio_set(&debug_sdr_ioexpanderx1E_invalid, &s_fake_pin));
}
void test_ocgpio_probe(void)
{
    /* TODO: separately tracked as #117
     */
}

void test_ocgpio_init(void)
{
    TEST_ASSERT_EQUAL(POST_DEV_CFG_DONE, DEBUG_OCGPIO_fxnTable.cb_init(
                                             &debug_ec_gpio_pa, NULL, NULL));
}
