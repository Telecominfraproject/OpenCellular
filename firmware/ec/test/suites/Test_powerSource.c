/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "unity.h"
#include "inc/devices/powerSource.h"

#include "drivers/GpioSX1509.h"
#include "drivers/OcGpio.h"
#include "helpers/array.h"
#include "helpers/attribute.h"
#include "helpers/memory.h"

#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"

#include <string.h>

/* ======================== Constants & variables =========================== */
#define I2C_BUS 5
#define I2C_ADDR 0x71

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

extern const OcGpio_FnTable GpioSX1509_fnTable;

static bool PWR_GpioPins[] = {
    [0x1E] = 0x1, /* pin_solar_aux_prsnt_n =30*/
    [0x55] = 0x1, /* pin_poe_prsnt_n = 85 */
};

static uint32_t PWR_GpioConfig[] = {
    [0x1E] = OCGPIO_CFG_INPUT,
    [0x55] = OCGPIO_CFG_INPUT,
};

/* ============================= Boilerplate ================================ */
static OcGpio_Port s_fake_io_port = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

static OcGpio_Port s_fake_io_exp = {
    .fn_table = &GpioSX1509_fnTable,
    .cfg =
            &(SX1509_Cfg){
                    .i2c_dev = { I2C_BUS, I2C_ADDR },
            },
    .object_data = &(SX1509_Obj){},
};

void suite_setUp(void)
{
    FakeGpio_registerDevSimple(PWR_GpioPins, PWR_GpioConfig);
    fake_I2C_init();
    fake_I2C_registerDevSimple(I2C_BUS, I2C_ADDR, SX1509_regs,
                               sizeof(SX1509_regs), sizeof(SX1509_regs[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
}

void setUp(void)
{
    memset(SX1509_regs, 0, sizeof(SX1509_regs));
    OcGpio_init(&s_fake_io_exp);
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit();
}

static PWRSRC_Dev p_dev = {
    .cfg =
            {
                    /* SOLAR_AUX_PRSNT_N */
                    .pin_solar_aux_prsnt_n = { &s_fake_io_port, 0x1E },
                    /* POE_PRSNT_N */
                    .pin_poe_prsnt_n = { &s_fake_io_port, 0x55 },
                    /* INT_BAT_PRSNT */
                    .pin_int_bat_prsnt = { &s_fake_io_exp, 11 },
                    /* EXT_BAT_PRSNT */
                    .pin_ext_bat_prsnt = { &s_fake_io_exp, 12 },
            },
};
/* ================================ Tests =================================== */

void test_pwr_process_get_status_parameters_data_poeavailable(void)
{
    uint8_t powerStatus = 0;
    uint8_t index = 0x00; //PoE Availability
    PWR_GpioPins[0x55] = 0x0; //PoE Enable
    PWR_GpioPins[0x1E] = 0x1; //Aux/solar Disable
    SX1509_regs[0x10] = 0x18; //Int/Ext Battery Disable
    SX1509_regs[0x11] = 0x00;

    pwr_source_init();
    pwr_get_source_info(&p_dev);
    pwr_process_get_status_parameters_data(index, &powerStatus);

    TEST_ASSERT_EQUAL(1, powerStatus);
}

void test_pwr_process_get_status_parameters_data_poeaccessible(void)
{
    uint8_t powerStatus = 0;
    uint8_t index = 0x01; //PoE Accessibility
    PWR_GpioPins[0x55] = 0x0; //PoE Enable
    PWR_GpioPins[0x1E] = 0x1; //Aux/solar Disable
    SX1509_regs[0x10] = 0x18; //Int/Ext Battery Disable
    SX1509_regs[0x11] = 0x00;

    pwr_source_init();
    pwr_get_source_info(&p_dev);
    pwr_process_get_status_parameters_data(index, &powerStatus);

    TEST_ASSERT_EQUAL(1, powerStatus);
}

void test_pwr_process_get_status_parameters_data_solaravailable(void)
{
    uint8_t powerStatus = 0;
    uint8_t index = 0x02; //SOLAR Availability
    PWR_GpioPins[0x55] = 0x1; //PoE Disable
    PWR_GpioPins[0x1E] = 0x0; //Aux/solar Enable
    SX1509_regs[0x10] = 0x18; //Int/Ext Battery Disable
    SX1509_regs[0x11] = 0x00;

    pwr_source_init();
    pwr_get_source_info(&p_dev);
    pwr_process_get_status_parameters_data(index, &powerStatus);

    TEST_ASSERT_EQUAL(1, powerStatus);
}

void test_pwr_process_get_status_parameters_data_solaraccessible(void)
{
    uint8_t powerStatus = 0;
    uint8_t index = 0x03; //SOLAR Accessibility
    PWR_GpioPins[0x55] = 0x1; //PoE Disable
    PWR_GpioPins[0x1E] = 0x0; //Aux/solar Enable
    SX1509_regs[0x10] = 0x18; //Int/Ext Battery Disable
    SX1509_regs[0x11] = 0x00;

    pwr_source_init();
    pwr_get_source_info(&p_dev);
    pwr_process_get_status_parameters_data(index, &powerStatus);

    TEST_ASSERT_EQUAL(1, powerStatus);
}

void test_pwr_process_get_status_parameters_data_extavailable(void)
{
    uint8_t powerStatus = 0;
    uint8_t index = 0x04; //Ext Batt availability
    PWR_GpioPins[0x55] = 0x1; //PoE Disable
    PWR_GpioPins[0x1E] = 0x1; //Aux/solar Disable
    SX1509_regs[0x10] = 0x08; //Int Batt OFF, Ext batt ON
    SX1509_regs[0x11] = 0x00;

    pwr_source_init();
    pwr_get_source_info(&p_dev);
    pwr_process_get_status_parameters_data(index, &powerStatus);

    TEST_ASSERT_EQUAL(1, powerStatus);
}

void test_pwr_process_get_status_parameters_data_extaccessible(void)
{
    uint8_t powerStatus = 0;
    uint8_t index = 0x05; //Ext Batt accessibility
    PWR_GpioPins[0x55] = 0x1; //PoE Disable
    PWR_GpioPins[0x1E] = 0x1; //Aux/solar Disable
    SX1509_regs[0x10] = 0x08; //Int Batt OFF, Ext batt ON
    SX1509_regs[0x11] = 0x00;

    pwr_source_init();
    pwr_get_source_info(&p_dev);
    pwr_process_get_status_parameters_data(index, &powerStatus);

    TEST_ASSERT_EQUAL(1, powerStatus);
}

void test_pwr_process_get_status_parameters_data_intavailable(void)
{
    uint8_t powerStatus = 0;
    uint8_t index = 0x06; //Int Batt Availability
    PWR_GpioPins[0x55] = 0x1; //PoE Disable
    PWR_GpioPins[0x1E] = 0x1; //Aux/solar Disable
    SX1509_regs[0x10] = 0x10; //Ext Batt OFF, Int batt ON
    SX1509_regs[0x11] = 0x00;

    pwr_source_init();
    pwr_get_source_info(&p_dev);
    pwr_process_get_status_parameters_data(index, &powerStatus);

    TEST_ASSERT_EQUAL(1, powerStatus);
}

void test_pwr_process_get_status_parameters_data_intaccessible(void)
{
    uint8_t powerStatus = 0;
    uint8_t index = 0x07; //Int Batt Accessibility
    PWR_GpioPins[0x55] = 0x1; //PoE Disable
    PWR_GpioPins[0x1E] = 0x1; //Aux/solar Disable
    SX1509_regs[0x10] = 0x10; //Ext Batt OFF, Int batt ON
    SX1509_regs[0x11] = 0x00;

    pwr_source_init();
    pwr_get_source_info(&p_dev);
    pwr_process_get_status_parameters_data(index, &powerStatus);

    TEST_ASSERT_EQUAL(1, powerStatus);
}
