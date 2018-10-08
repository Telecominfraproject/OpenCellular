/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "unity.h"
#include "drivers/GpioSX1509.h"

#include "drivers/OcGpio.h"
#include "helpers/array.h"
#include "helpers/attribute.h"

#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"

#include <string.h>

/* ======================== Constants & variables =========================== */
#define I2C_BUS 2
#define I2C_ADDR 0x00

static uint8_t SX1509_regs[] = {
    [0x00] = 0x00, /* Input buffer disable register B */
    [0x01] = 0x00, /* Input buffer disable register A */
    [0x02] = 0x00, /* Output buffer long slew register B */
    [0x03] = 0x00, /* Output buffer long slew register A */
    [0x04] = 0x00, /* Output buffer low drive register B */
    [0x05] = 0x00, /* Output buffer low drive register A */
    [0x06] = 0x00, /* Pull Up register B */
    [0x07] = 0x00, /* Pull Up register A */
    [0x08] = 0x00, /* Pull Down register B */
    [0x09] = 0x00, /* Pull Down register A */
    [0x0A] = 0x00, /* Open drain register B */
    [0x0B] = 0x00, /* Open drain register A */
    [0x0C] = 0x00, /* Polarity register B */
    [0x0D] = 0x00, /* Polarity register A */
    [0x0E] = 0x00, /* Direction register B */
    [0x0F] = 0x00, /* Direction register A */
    [0x10] = 0x00, /* Data register B */
    [0x11] = 0x00, /* Data register A */
    [0x12] = 0x00, /* Interrupt mask register B */
    [0x13] = 0x00, /* Interrupt mask register A */
    [0x14] = 0x00, /* Sense High register B */
    [0x15] = 0x00, /* Sense Low register B */
    [0x16] = 0x00, /* Sense High register A */
    [0x17] = 0x00, /* Sense Low register A */
    [0x18] = 0x00, /* Interrupt source register B */
    [0x19] = 0x00, /* Interrupt source register A */
    [0x1A] = 0x00, /* Event status register B */
    [0x1B] = 0x00, /* Event status register A */
    [0x1C] = 0x00, /* Level shifter register 1 */
    [0x1D] = 0x00, /* Level shifter register 2 */
    [0x1E] = 0x00, /* Clock management register */
    [0x1F] = 0x00, /* Miscellaneous device settings register */
    [0x20] = 0x00, /* LED driver enable register B */
    [0x21] = 0x00, /* LED driver enable register A */

    [0x22] = 0x00, /* Debounce configuration register */
    [0x23] = 0x00, /* Debounce enable register B */
    [0x24] = 0x00, /* Debounce enable register A */
    [0x25] = 0x00, /* Key scan configuration register 1 */
    [0x26] = 0x00, /* Key scan configuration register 2 */
    [0x27] = 0x00, /* Key value (column) 1 */
    [0x28] = 0x00, /* Key value (row) 2 */

    [0x29] = 0x00, /* ON time register I/O[0] */
    [0x2A] = 0x00, /* ON intensity register I/O[0] */
    [0x2B] = 0x00, /* OFF time/intensity register I/O[0] */
    [0x2C] = 0x00, /* ON time register I/O[1] */
    [0x2D] = 0x00, /* ON intensity register I/O[1] */
    [0x2E] = 0x00, /* OFF time/intensity register I/O[1] */
    [0x2F] = 0x00, /* ON time register I/O[2] */
    [0x30] = 0x00, /* ON intensity register I/O[2] */
    [0x31] = 0x00, /* OFF time/intensity register I/O[2] */
    [0x32] = 0x00, /* ON time register I/O[3] */
    [0x33] = 0x00, /* ON intensity register I/O[3] */
    [0x34] = 0x00, /* OFF time/intensity register I/O[3] */
    [0x35] = 0x00, /* ON time register I/O[4] */
    [0x36] = 0x00, /* ON intensity register I/O[4] */
    [0x37] = 0x00, /* OFF time/intensity register I/O[4] */
    [0x38] = 0x00, /* Fade in register I/O[4] */
    [0x39] = 0x00, /* Fade out register I/O[4] */
    [0x3A] = 0x00, /* ON time register I/O[5] */
    [0x3B] = 0x00, /* ON intensity register I/O[5] */
    [0x3C] = 0x00, /* OFF time/intensity register I/O[5] */
    [0x3D] = 0x00, /* Fade in register I/O[5] */
    [0x3E] = 0x00, /* Fade out register I/O[5] */
    [0x3F] = 0x00, /* ON time register I/O[6] */
    [0x40] = 0x00, /* ON intensity register I/O[6] */
    [0x41] = 0x00, /* OFF time/intensity register I/O[6] */
    [0x42] = 0x00, /* Fade in register I/O[6] */
    [0x43] = 0x00, /* Fade out register I/O[6] */
    [0x44] = 0x00, /* ON time register I/O[6] */
    [0x45] = 0x00, /* ON intensity register I/O[7] */
    [0x46] = 0x00, /* OFF time/intensity register I/O[7] */
    [0x47] = 0x00, /* Fade in register I/O[7] */
    [0x48] = 0x00, /* Fade out register I/O[7] */
    [0x49] = 0x00, /* ON time register I/O[8] */
    [0x4A] = 0x00, /* ON intensity register I/O[8] */
    [0x4B] = 0x00, /* OFF time/intensity register I/O[8]  */
    [0x4C] = 0x00, /* ON time register I/O[9] */
    [0x4D] = 0x00, /* ON intensity register I/O[9] */
    [0x4E] = 0x00, /* OFF time/intensity register I/O[9] */
    [0x4F] = 0x00, /* ON time register I/O[10] */
    [0x50] = 0x00, /* ON intensity register I/O[10] */
    [0x51] = 0x00, /* OFF time/intensity register I/O[10] */
    [0x52] = 0x00, /* ON time register I/O[11] */
    [0x53] = 0x00, /* ON intensity register I/O[11] */
    [0x54] = 0x00, /* OFF time/intensity register I/O[11] */
    [0x55] = 0x00, /* ON time register I/O[12] */
    [0x56] = 0x00, /* ON intensity register I/O[12] */
    [0x57] = 0x00, /* OFF time/intensity register I/O[12] */
    [0x58] = 0x00, /* Fade in register I/O[12] */
    [0x59] = 0x00, /* Fade out register I/O[12] */
    [0x5A] = 0x00, /* ON time register I/O[13] */
    [0x5B] = 0x00, /* ON intensity register I/O[13] */
    [0x5C] = 0x00, /* OFF time/intensity register I/O[13] */
    [0x5D] = 0x00, /* Fade in register I/O[13] */
    [0x5E] = 0x00, /* Fade out register I/O[13] */
    [0x5F] = 0x00, /* ON time register I/O[14] */
    [0x60] = 0x00, /* ON intensity register I/O[14] */
    [0x61] = 0x00, /* OFF time/intensity register I/O[14] */
    [0x62] = 0x00, /* Fade in register I/O[14] */
    [0x63] = 0x00, /* Fade out register I/O[14] */
    [0x64] = 0x00, /* ON time register I/O[15] */
    [0x65] = 0x00, /* ON intensity register I/O[15] */
    [0x66] = 0x00, /* OFF time/intensity register I/O[15] */
    [0x67] = 0x00, /* Fade in register I/O[115] */
    [0x68] = 0x00, /* Fade out register I/O[15] */

    [0x69] = 0x00, /*  */
    [0x6A] = 0x00, /*  */

    [0x7D] = 0x00, /*  */
    [0x7E] = 0x00, /*  */
    [0x7F] = 0x00, /*  */
};

static const OcGpio_Port s_sx1509_ioexp = {
    .fn_table = &GpioSX1509_fnTable,
    .cfg =
            &(SX1509_Cfg){
                    .i2c_dev = { I2C_BUS, I2C_ADDR },
            },
    .object_data = &(SX1509_Obj){},
};

static OcGpio_Pin s_test_pins[16];

/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    fake_I2C_init();

    fake_I2C_registerDevSimple(I2C_BUS, I2C_ADDR, SX1509_regs,
                               sizeof(SX1509_regs), sizeof(SX1509_regs[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
}

void setUp(void)
{
    memset(SX1509_regs, 0, sizeof(SX1509_regs));

    SX1509_regs[0x00] = SX1509_regs[0x01] = 0xFF; /* Input buffer disable */
    SX1509_regs[0x0E] = SX1509_regs[0x0F] = 0xFF; /* Pin Direction */

    OcGpio_init(&s_sx1509_ioexp);

    for (size_t i = 0; i < ARRAY_SIZE(s_test_pins); ++i) {
        s_test_pins[i] = (OcGpio_Pin){
            &s_sx1509_ioexp,
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
static void _test_cfg_helper(uint32_t pin_cfg)
{
    for (size_t i = 0; i < ARRAY_SIZE(s_test_pins); ++i) {
        TEST_ASSERT_EQUAL(OCGPIO_SUCCESS,
                          OcGpio_configure(&s_test_pins[i], pin_cfg));
    }

    /* Check PU config */
    TEST_ASSERT_EQUAL_HEX8(0x04, SX1509_regs[0x06]); /* B */
    TEST_ASSERT_EQUAL_HEX8(0x82, SX1509_regs[0x07]); /* A */

    /* Check PD config */
    TEST_ASSERT_EQUAL_HEX8(0x11, SX1509_regs[0x08]); /* B */
    TEST_ASSERT_EQUAL_HEX8(0x20, SX1509_regs[0x09]); /* A */

    /* Check polarity */
    TEST_ASSERT_EQUAL_HEX8(0x03, SX1509_regs[0x0C]); /* B */
    TEST_ASSERT_EQUAL_HEX8(0x80, SX1509_regs[0x0D]); /* A */
}

void test_OcGpio_configure_inputs(void)
{
    s_test_pins[1].hw_cfg = OCGPIO_CFG_IN_PU;
    s_test_pins[5].hw_cfg = OCGPIO_CFG_IN_PD;
    s_test_pins[7].hw_cfg = OCGPIO_CFG_IN_PU | OCGPIO_CFG_INVERT;
    s_test_pins[8].hw_cfg = OCGPIO_CFG_IN_PD | OCGPIO_CFG_INVERT;
    s_test_pins[9].hw_cfg = OCGPIO_CFG_INVERT;
    s_test_pins[10].hw_cfg = OCGPIO_CFG_IN_PU;
    s_test_pins[12].hw_cfg = OCGPIO_CFG_IN_PD;

    /* Make sure we can set all inputs properly */
    _test_cfg_helper(OCGPIO_CFG_INPUT);

    /* Make sure they're all inputs */
    TEST_ASSERT_EQUAL_HEX8(0xFF, SX1509_regs[0x0E]); /* B */
    TEST_ASSERT_EQUAL_HEX8(0xFF, SX1509_regs[0x0F]); /* A */

    /* Check input buffer is enabled (0) for each input */
    TEST_ASSERT_EQUAL_HEX8(0x00, SX1509_regs[0x00]); /* B */
    TEST_ASSERT_EQUAL_HEX8(0x00, SX1509_regs[0x01]); /* A */
}

void test_OcGpio_configure_outputs(void)
{
    s_test_pins[1].hw_cfg = OCGPIO_CFG_OUT_OD_PU;
    s_test_pins[5].hw_cfg = OCGPIO_CFG_OUT_OD_PD;
    s_test_pins[7].hw_cfg = OCGPIO_CFG_OUT_OD_PU | OCGPIO_CFG_INVERT;
    s_test_pins[8].hw_cfg = OCGPIO_CFG_OUT_OD_PD | OCGPIO_CFG_INVERT;
    s_test_pins[9].hw_cfg = OCGPIO_CFG_INVERT;
    s_test_pins[10].hw_cfg = OCGPIO_CFG_OUT_OD_PU;
    s_test_pins[12].hw_cfg = OCGPIO_CFG_OUT_OD_PD;
    s_test_pins[14].hw_cfg = OCGPIO_CFG_OUT_OD_NOPULL;

    /* Make sure we can set all inputs properly */
    _test_cfg_helper(OCGPIO_CFG_OUTPUT);

    /* Make sure they're all outputs */
    TEST_ASSERT_EQUAL_HEX8(0x00, SX1509_regs[0x0E]); /* B */
    TEST_ASSERT_EQUAL_HEX8(0x00, SX1509_regs[0x0F]); /* A */

    /* Check input buffer is disabled (1) for each output */
    TEST_ASSERT_EQUAL_HEX8(0xFF, SX1509_regs[0x00]); /* B */
    TEST_ASSERT_EQUAL_HEX8(0xFF, SX1509_regs[0x01]); /* A */

    /* Check OD */
    TEST_ASSERT_EQUAL_HEX8(0x55, SX1509_regs[0x0A]); /* B */
    TEST_ASSERT_EQUAL_HEX8(0xA2, SX1509_regs[0x0B]); /* A */
}
