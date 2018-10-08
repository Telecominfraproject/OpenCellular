/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "unity.h"

#include "inc/devices/sx1509.h"

#include "fake/fake_I2C.h"

#include <string.h>

/* ======================== Constants & variables =========================== */
#define I2C_BUS 7
#define I2C_ADDR 0x00

static const I2C_Dev s_sx1509_dev = {
    .bus = I2C_BUS,
    .slave_addr = I2C_ADDR,
};

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

/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    fake_I2C_init();

    fake_I2C_registerDevSimple(I2C_BUS, I2C_ADDR, SX1509_regs,
                               sizeof(SX1509_regs), sizeof(SX1509_regs[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_BIG_ENDIAN);
}

void setUp(void)
{
    memset(SX1509_regs, 0, sizeof(SX1509_regs));
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}

/* ================================ Tests =================================== */
void test_ioexp_led_input(void)
{
    /* Test a couple of different arbitrary input values */
    uint8_t input_val = 0xff;

    SX1509_regs[0x11] = 0xF2;
    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_get_data(&s_sx1509_dev, SX1509_REG_A,
                                                    &input_val));
    TEST_ASSERT_EQUAL_HEX8(0xF2, input_val);

    SX1509_regs[0x10] = 0x04;
    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_get_data(&s_sx1509_dev, SX1509_REG_B,
                                                    &input_val));
    TEST_ASSERT_EQUAL_HEX8(0x04, input_val);
}

void test_ioexp_led_output(void)
{
    /* Test getting and setting output values */
    uint8_t output_val = 0xff;

    SX1509_regs[0x11] = 0x0C;
    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_get_data(&s_sx1509_dev, SX1509_REG_A,
                                                    &output_val));
    TEST_ASSERT_EQUAL_HEX8(0x0C, output_val);

    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_set_data(&s_sx1509_dev, SX1509_REG_A,
                                                    0xAA, 0x00));
    TEST_ASSERT_EQUAL_HEX8(0xAA, SX1509_regs[0x11]);

    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_get_data(&s_sx1509_dev, SX1509_REG_A,
                                                    &output_val));
    TEST_ASSERT_EQUAL_HEX8(0xAA, output_val);
}

void test_ioexp_led_on_time(void)
{
    /* Test setting on time value */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_set_on_time(&s_sx1509_dev, 0,
                                            0x10)); // ON time register I/O[0]
    TEST_ASSERT_EQUAL_HEX8(0x10, SX1509_regs[0x29]);
}

void test_ioexp_led_off_time(void)
{
    /* Test setting off time value */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_set_off_time(&s_sx1509_dev, 0,
                                             0x80)); // OFF time register I/O[0]
    TEST_ASSERT_EQUAL_HEX8(0x80, SX1509_regs[0x2B]);
}

void test_ioexp_led_software_reset(void)
{
    /* Test software reset */
    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_software_reset(&s_sx1509_dev));
}

void test_ioexp_led_inputbuffer(void)
{
    /* Test setting input buffer values */
    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_config_inputbuffer(
                                         &s_sx1509_dev, SX1509_REG_AB, 0x55,
                                         0xAA)); // LSB(Reg A), LSB(Reg B)
    TEST_ASSERT_EQUAL_HEX8(0x55, SX1509_regs[0x01]); // Reg A
    TEST_ASSERT_EQUAL_HEX8(0xAA, SX1509_regs[0x00]); // Reg B

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_config_inputbuffer(&s_sx1509_dev, SX1509_REG_B,
                                                   0x7F, 0x00)); // Reg B
    TEST_ASSERT_EQUAL_HEX8(0x7F, SX1509_regs[0x00]);
}

void test_ioexp_led_pullup(void)
{
    /* Test setting pull up values */
    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_config_pullup(
                                         &s_sx1509_dev, SX1509_REG_AB, 0x27,
                                         0x82)); // LSB(Reg A), MSB(Reg B)
    TEST_ASSERT_EQUAL_HEX8(0x27, SX1509_regs[0x07]); // Reg A
    TEST_ASSERT_EQUAL_HEX8(0x82, SX1509_regs[0x06]); // Reg B

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_config_pullup(&s_sx1509_dev, SX1509_REG_A, 0x23,
                                              0x00)); // Reg A
    TEST_ASSERT_EQUAL_HEX8(0x23, SX1509_regs[0x07]);
}

void test_ioexp_led_pulldown(void)
{
    /* Test setting pull down values */
    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_config_pulldown(
                                         &s_sx1509_dev, SX1509_REG_AB, 0x32,
                                         0x5F)); // LSB(Reg A), MSB(Reg B)
    TEST_ASSERT_EQUAL_HEX8(0x32, SX1509_regs[0x09]); // Reg A
    TEST_ASSERT_EQUAL_HEX8(0x5F, SX1509_regs[0x08]); // Reg B

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_config_pulldown(&s_sx1509_dev, SX1509_REG_A,
                                                0xFF, 0x00)); // Reg A
    TEST_ASSERT_EQUAL_HEX8(0xFF, SX1509_regs[0x09]);
}

void test_ioexp_led_opendrain(void)
{
    /* Test setting open drain values */
    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_config_opendrain(
                                         &s_sx1509_dev, SX1509_REG_AB, 0x45,
                                         0x54)); // LSB(Reg A), MSB(Reg B)
    TEST_ASSERT_EQUAL_HEX8(0x45, SX1509_regs[0x0B]); // Reg A
    TEST_ASSERT_EQUAL_HEX8(0x54, SX1509_regs[0x0A]); // Reg B

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_config_opendrain(&s_sx1509_dev, SX1509_REG_B,
                                                 0x00, 0x00)); // Reg A
    TEST_ASSERT_EQUAL_HEX8(0x00, SX1509_regs[0x0A]);
}

void test_ioexp_led_data_direction(void)
{
    /* Test setting data direction values */
    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_config_data_direction(
                                         &s_sx1509_dev, SX1509_REG_AB, 0xAB,
                                         0xD9)); // LSB(Reg A), MSB(Reg B)
    TEST_ASSERT_EQUAL_HEX8(0xAB, SX1509_regs[0x0F]); // Reg A
    TEST_ASSERT_EQUAL_HEX8(0xD9, SX1509_regs[0x0E]); // Reg B

    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_config_data_direction(
                                         &s_sx1509_dev, SX1509_REG_B, 0x98,
                                         0x00)); // Reg A
    TEST_ASSERT_EQUAL_HEX8(0x98, SX1509_regs[0x0E]);
}

void test_ioexp_led_polarity(void)
{
    /* Test setting polarity values */
    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_config_polarity(
                                         &s_sx1509_dev, SX1509_REG_AB, 0x67,
                                         0xCD)); // LSB(Reg A), MSB(Reg B)
    TEST_ASSERT_EQUAL_HEX8(0x67, SX1509_regs[0x0D]); // Reg A
    TEST_ASSERT_EQUAL_HEX8(0xCD, SX1509_regs[0x0C]); // Reg B

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_config_polarity(&s_sx1509_dev, SX1509_REG_A,
                                                0xAB, 0x00)); // Reg A
    TEST_ASSERT_EQUAL_HEX8(0xAB, SX1509_regs[0x0D]);
}

void test_ioexp_led_clock(void)
{
    /* Test setting clock settings */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_config_clock(&s_sx1509_dev,
                                             SX1509_INTERNAL_CLOCK_2MHZ,
                                             SX1509_CLOCK_OSC_IN));
    TEST_ASSERT_EQUAL_HEX8(0x40, SX1509_regs[0x1E]); //0100 0000

    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_config_clock(&s_sx1509_dev,
                                                        SX1509_EXTERNAL_CLOCK,
                                                        SX1509_CLOCK_OSC_IN));
    TEST_ASSERT_EQUAL_HEX8(0x20, SX1509_regs[0x1E]); //0010 0000

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_config_clock(&s_sx1509_dev,
                                             SX1509_INTERNAL_CLOCK_2MHZ,
                                             SX1509_CLOCK_OSC_OUT));
    TEST_ASSERT_EQUAL_HEX8(0x50, SX1509_regs[0x1E]); //0101 0000

    //How to configure clock frequency on OSCOUT pin?
}

void test_ioexp_led_misc(void)
{
    /* Test setting misc settings */
    TEST_ASSERT_EQUAL(
            RETURN_OK,
            ioexp_led_config_misc(&s_sx1509_dev,
                                  0x24)); //Clkx-1MHz, Fading-Linear(Bank A, B)
    TEST_ASSERT_EQUAL_HEX8(0x24, SX1509_regs[0x1F]);

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_config_misc(
                              &s_sx1509_dev,
                              0x54)); //Clkx-125KHz, Fading-Linear(Bank A, B)
    TEST_ASSERT_EQUAL_HEX8(0x54, SX1509_regs[0x1F]); //0010 0000

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_config_misc(
                              &s_sx1509_dev,
                              0xAC)); //Clkx-1MHz, Fading-Loarithmic(Bank A, B)
    TEST_ASSERT_EQUAL_HEX8(0xAC, SX1509_regs[0x1F]); //0101 0000

    //How to configure multiple things on RegMisc settings?
}

void test_ioexp_led_enable_leddriver(void)
{
    /* Test setting led driver values */
    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_enable_leddriver(
                                         &s_sx1509_dev, SX1509_REG_AB, 0x52,
                                         0xF8)); // LSB(Reg A), MSB(Reg B)
    TEST_ASSERT_EQUAL_HEX8(0x52, SX1509_regs[0x21]); // Reg A
    TEST_ASSERT_EQUAL_HEX8(0xF8, SX1509_regs[0x20]); // Reg B

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_enable_leddriver(&s_sx1509_dev, SX1509_REG_B,
                                                 0x9C, 0x00)); // Reg B
    TEST_ASSERT_EQUAL_HEX8(0x9C, SX1509_regs[0x20]);
}

void test_ioexp_led_testregister_1(void)
{
    /* Test a couple of different arbitrary input values */
    uint8_t input_val = 0xff;

    SX1509_regs[0x7E] = 0x00;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_read_testregister_1(&s_sx1509_dev, &input_val));
    TEST_ASSERT_EQUAL_HEX8(0x00, input_val);

    SX1509_regs[0x7E] = 0x04;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_read_testregister_1(&s_sx1509_dev, &input_val));
    TEST_ASSERT_EQUAL_HEX8(0x04, input_val);
}

void test_ioexp_led_interrupt(void)
{
    /* Test setting interrupt values */
    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_config_interrupt(
                                         &s_sx1509_dev, SX1509_REG_AB, 0x27,
                                         0x28)); // LSB(Reg A), MSB(Reg B)
    TEST_ASSERT_EQUAL_HEX8(0x27, SX1509_regs[0x13]); // Reg A
    TEST_ASSERT_EQUAL_HEX8(0x28, SX1509_regs[0x12]); // Reg B

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_config_interrupt(&s_sx1509_dev, SX1509_REG_A,
                                                 0x38, 0x00)); // Reg A
    TEST_ASSERT_EQUAL_HEX8(0x38, SX1509_regs[0x13]);
}

void test_ioexp_led_edge_sense_A(void)
{
    /* Test setting Edge sense A values */
    /* Rising edge */
    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_config_edge_sense_A(
                                         &s_sx1509_dev, SX1509_REG_AB, 0x55,
                                         0x55)); // Low(Reg A), High(Reg A)
    TEST_ASSERT_EQUAL_HEX8(0x55, SX1509_regs[0x17]); // Low Reg A
    TEST_ASSERT_EQUAL_HEX8(0x55, SX1509_regs[0x16]); // High Reg A

    /* Falling edge */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_config_edge_sense_A(&s_sx1509_dev, SX1509_REG_A,
                                                    0xAA, 0x00)); // Low Reg A
    TEST_ASSERT_EQUAL_HEX8(0xAA, SX1509_regs[0x17]);

    /* Both edges */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_config_edge_sense_A(&s_sx1509_dev, SX1509_REG_B,
                                                    0xFF, 0x00)); // High Reg A
    TEST_ASSERT_EQUAL_HEX8(0xFF, SX1509_regs[0x16]);
}

void test_ioexp_led_edge_sense_B(void)
{
    /* Test setting Edge sense A values */
    /* Falling edge */
    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_config_edge_sense_B(
                                         &s_sx1509_dev, SX1509_REG_AB, 0xAA,
                                         0xAA)); // Low(Reg B), High(Reg B)
    TEST_ASSERT_EQUAL_HEX8(0xAA, SX1509_regs[0x15]); // Low Reg B
    TEST_ASSERT_EQUAL_HEX8(0xAA, SX1509_regs[0x14]); // High Reg B

    /* Rising edge */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_config_edge_sense_B(&s_sx1509_dev, SX1509_REG_B,
                                                    0x55, 0x00)); // High Reg B
    TEST_ASSERT_EQUAL_HEX8(0x55, SX1509_regs[0x14]);

    /* Both edges */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_config_edge_sense_B(&s_sx1509_dev, SX1509_REG_A,
                                                    0xFF, 0x00)); // Low Reg B
    TEST_ASSERT_EQUAL_HEX8(0xFF, SX1509_regs[0x15]);
}

void test_ioexp_led_debounce_time(void)
{
    /* Test setting Debounce time values */
    /* Debounce Time(2ms) */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_config_debounce_time(&s_sx1509_dev, 2));
    TEST_ASSERT_EQUAL_HEX8(0x02, SX1509_regs[0x22]);

    /* Debounce Time(32ms) */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_config_debounce_time(&s_sx1509_dev, 32));
    TEST_ASSERT_EQUAL_HEX8(0x06, SX1509_regs[0x22]);
}

void test_ioexp_led_enable_debounce(void)
{
    /* Test enabling debounce values */
    TEST_ASSERT_EQUAL(RETURN_OK, ioexp_led_enable_debounce(
                                         &s_sx1509_dev, SX1509_REG_AB, 0x4B,
                                         0x2C)); // LSB(Reg A), MSB(Reg B)
    TEST_ASSERT_EQUAL_HEX8(0x4B, SX1509_regs[0x24]); // Reg A
    TEST_ASSERT_EQUAL_HEX8(0x2C, SX1509_regs[0x23]); // Reg B

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_enable_debounce(&s_sx1509_dev, SX1509_REG_B,
                                                0x61, 0x00)); // Reg B
    TEST_ASSERT_EQUAL_HEX8(0x61, SX1509_regs[0x23]);
}

void test_ioexp_led_interrupt_source(void)
{
    /* Test getting interrupt source values */
    uint16_t intPins = 0xffff;
    SX1509_regs[0X18] = 0x5F; //MSB
    SX1509_regs[0X19] = 0X7C; //LSB
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_get_interrupt_source(&s_sx1509_dev, &intPins));
    TEST_ASSERT_EQUAL_HEX16(0x5F7C, intPins);

    SX1509_regs[0X18] = 0xAA; //MSB
    SX1509_regs[0X19] = 0X55; //LSB
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_get_interrupt_source(&s_sx1509_dev, &intPins));
    TEST_ASSERT_EQUAL_HEX16(0xAA55, intPins);
}

void test_ioexp_led_clear_interrupt_source(void)
{
    /* Test clearing interrupt source values */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ioexp_led_clear_interrupt_source(&s_sx1509_dev));
    TEST_ASSERT_EQUAL_HEX8(0xFF, SX1509_regs[0x18]); //MSB
    TEST_ASSERT_EQUAL_HEX8(0xFF, SX1509_regs[0x19]); //LSB
}

void test_ioexp_led_not_present(void)
{
    I2C_Dev invalid_dev = s_sx1509_dev;
    invalid_dev.slave_addr = 0x01;

    /* Ensure that we fail properly if the device isn't on the bus */
    uint8_t dummy_val;
    TEST_ASSERT_EQUAL(
            RETURN_NOTOK,
            ioexp_led_get_data(&invalid_dev, SX1509_REG_A, &dummy_val));
    TEST_ASSERT_EQUAL(
            RETURN_NOTOK,
            ioexp_led_set_data(&invalid_dev, SX1509_REG_A, dummy_val, 0x00));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ioexp_led_set_on_time(&invalid_dev, 0, dummy_val));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ioexp_led_set_off_time(&invalid_dev, 0, dummy_val));
    TEST_ASSERT_EQUAL(RETURN_NOTOK, ioexp_led_software_reset(&invalid_dev));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ioexp_led_config_inputbuffer(&invalid_dev, SX1509_REG_A,
                                                   dummy_val, 0x00));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ioexp_led_config_pullup(&invalid_dev, SX1509_REG_A,
                                              dummy_val, 0x00));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ioexp_led_config_pulldown(&invalid_dev, SX1509_REG_A,
                                                dummy_val, 0x00));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ioexp_led_config_opendrain(&invalid_dev, SX1509_REG_A,
                                                 dummy_val, 0x00));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ioexp_led_config_data_direction(
                              &invalid_dev, SX1509_REG_A, dummy_val, 0x00));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ioexp_led_config_polarity(&invalid_dev, SX1509_REG_A,
                                                dummy_val, 0x00));
    TEST_ASSERT_EQUAL(RETURN_NOTOK, ioexp_led_config_clock(&invalid_dev, 0, 1));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ioexp_led_config_misc(&invalid_dev, dummy_val));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ioexp_led_enable_leddriver(&invalid_dev, SX1509_REG_A,
                                                 dummy_val, 0x00));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ioexp_led_read_testregister_1(&invalid_dev, &dummy_val));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ioexp_led_config_interrupt(&invalid_dev, SX1509_REG_A,
                                                 dummy_val, 0x00));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ioexp_led_config_edge_sense_A(&invalid_dev, SX1509_REG_A,
                                                    dummy_val, 0x00));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ioexp_led_config_edge_sense_B(&invalid_dev, SX1509_REG_A,
                                                    dummy_val, 0x00));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ioexp_led_config_debounce_time(&invalid_dev, 0));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ioexp_led_enable_debounce(&invalid_dev, SX1509_REG_A,
                                                dummy_val, 0x00));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ioexp_led_get_interrupt_source(&invalid_dev,
                                                     (uint16_t *)&dummy_val));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ioexp_led_clear_interrupt_source(&invalid_dev));
}
