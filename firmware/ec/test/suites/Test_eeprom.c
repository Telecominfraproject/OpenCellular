/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "unity.h"
#include "inc/devices/eeprom.h"
#include "drivers/GpioSX1509.h"
#include <string.h>
#include <stdio.h>
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"
#include "helpers/attribute.h"
#include "helpers/memory.h"
#include "drivers/OcGpio.h"
#include "helpers/array.h"
#include <string.h>

/* ============================= Fake Functions ============================= */

#include <ti/sysbios/knl/Task.h>
unsigned int s_task_sleep_ticks;
xdc_Void ti_sysbios_knl_Task_sleep__E(xdc_UInt32 nticks)
{
    s_task_sleep_ticks += nticks;
}

/* ======================== Constants & variables =========================== */

static OcGpio_Port s_fake_io_port = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

static const I2C_Dev I2C_DEV = {
    .bus = 6,
    .slave_addr = 0x50,
};
static const I2C_Dev I2C_DEV_1 = {
    .bus = 6,
    .slave_addr = 0x51,
};
static Eeprom_Cfg s_dev = {
    .i2c_dev =
            {
                    .bus = 6,
                    .slave_addr = 0x50,
            },
};

static uint16_t EEPROM_regs[] = {
    [0x00] = 0x00, /* Init */
    [0xC601] = 0x00, /* SERIAL INFO */
    [0xAC01] = 0x00, /* BOARD INFO */
    [0x0A01] = 0x00, /* DEVICE INFO */
    [0x0A02] = 0x00, /* DEVICE INFO */

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

static bool Eeprom_GpioPins[] = {
    [0x01] = 0x1, /* Pin = 1 */
    [0x02] = 0x1, /* Pin = 2 */
};

static uint32_t Eeprom_GpioConfig[] = {
    [0x01] = OCGPIO_CFG_INPUT,
    [0x02] = OCGPIO_CFG_INPUT,
};

extern const OcGpio_FnTable GpioSX1509_fnTable;

OcGpio_Port s_fake_io_exp = {
    .fn_table = &GpioSX1509_fnTable,
    .cfg =
            &(SX1509_Cfg){
                    .i2c_dev = { 6, 0x45 },
                    .pin_irq = NULL,
            },
    .object_data = &(SX1509_Obj){},
};

OcGpio_Pin pin_inven_eeprom_wp = { &s_fake_io_exp, 2, 32 };

Eeprom_Cfg eeprom_gbc_sid = {
    .i2c_dev = { 6, 0x51 },
    .pin_wp = &(OcGpio_Pin){ &s_fake_io_port, 5 },
    .type = { 0, 0 },
    .ss = 0,
};

Eeprom_Cfg eeprom_gbc_inv = {
    .i2c_dev = { 6, 0x50 },
    .pin_wp = &(OcGpio_Pin){ &s_fake_io_port, 5 },
    .type = { 0, 0 },
    .ss = 0,
};

Eeprom_Cfg eeprom_sdr_inv = {
    .i2c_dev = { 3, 0x50 },
    .pin_wp = NULL,
    .type = { 0, 0 },
    .ss = 0,
};

Eeprom_Cfg eeprom_fe_inv = {
    .i2c_dev = { 4, 0x50 },
    .pin_wp = &(OcGpio_Pin){ &s_fake_io_port, 5 },
    .type = { 0, 0 },
    .ss = 8,
};

/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    fake_I2C_init();

    FakeGpio_registerDevSimple(Eeprom_GpioPins, Eeprom_GpioConfig);

    fake_I2C_registerDevSimple(I2C_DEV.bus, I2C_DEV.slave_addr, EEPROM_regs,
                               sizeof(EEPROM_regs), sizeof(EEPROM_regs[0]),
                               sizeof(uint16_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
    fake_I2C_registerDevSimple(I2C_DEV_1.bus, I2C_DEV_1.slave_addr, EEPROM_regs,
                               sizeof(EEPROM_regs), sizeof(EEPROM_regs[0]),
                               sizeof(uint16_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
    fake_I2C_registerDevSimple(6, 0x45, SX1509_regs, sizeof(SX1509_regs),
                               sizeof(SX1509_regs[0]), sizeof(uint8_t),
                               FAKE_I2C_DEV_LITTLE_ENDIAN);
}

void setUp(void)
{
    memset(EEPROM_regs, 0, sizeof(EEPROM_regs));

    OcGpio_init(&s_fake_io_port);

    OcGpio_init(&s_fake_io_exp);
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}

/* ================================ Tests =================================== */
void test_eeprom_init(void)
{
    Eeprom_Cfg e_dev = {
        .i2c_dev = s_dev.i2c_dev,
        .pin_wp = &(OcGpio_Pin){ &s_fake_io_port, 2 },
    };

    EEPROM_regs[0x00] = 0x0505;
    Eeprom_GpioConfig[0x02] = OCGPIO_CFG_OUT_HIGH;

    eeprom_init(&e_dev);
    TEST_ASSERT_EQUAL(1, eeprom_init(&e_dev));
    TEST_ASSERT_EQUAL(OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH,
                      Eeprom_GpioConfig[0x02]);
}

void test_eeprom_read(void)
{
    uint16_t buffer;
    EEPROM_regs[0xC601] = 0x0505;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      eeprom_read(&s_dev, 0x01C6, &buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_HEX8(0x0505, buffer);
}

void test_eeprom_write(void)
{
    Eeprom_Cfg p_dev = {
        .i2c_dev = { 6, 0x50 },
        .pin_wp = NULL,
        .type = { .page_size = 64, .mem_size = (256 / 8) },
        .ss = 0,
    };

    uint16_t buffer = 0x0505;
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_write(&p_dev, 0x01C6, &buffer, 0x0A));
    TEST_ASSERT_EQUAL_HEX8(0x0505, EEPROM_regs[0xC601]);

    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_write(&p_dev, 0x01C6, &buffer, 0xCA));
    TEST_ASSERT_EQUAL_HEX8(0x05, EEPROM_regs[0xC601]);
}

void test_eeprom_disable_write(void)
{
    SX1509_regs[0x10] = 0x01;
    SX1509_regs[0x11] = 0x01;

    Eeprom_Cfg i_dev = {
        .i2c_dev = { 6, 0x45 },
        .pin_wp = &pin_inven_eeprom_wp,
        .type = { 0, 0 },
        .ss = 0,
    };

    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_disable_write(&i_dev));
    TEST_ASSERT_EQUAL(0xFF, SX1509_regs[0x11]);
}

void test_eeprom_enable_write(void)
{
    SX1509_regs[0x10] = 0x00;
    SX1509_regs[0x11] = 0x00;

    Eeprom_Cfg i_dev = {
        .i2c_dev = { 6, 0x45 },
        .pin_wp = &pin_inven_eeprom_wp,
        .type = { 0, 0 },
        .ss = 0,
    };
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_enable_write(&i_dev));
    TEST_ASSERT_EQUAL(0xFB, SX1509_regs[0x11]);
}

void test_eeprom_read_board_info(void)
{
    uint8_t rominfo = 0xff;
    EEPROM_regs[0xAC01] = 0x05;
    Eeprom_Cfg b1_dev = {
        .i2c_dev = { 6, 0x50 },
        .pin_wp = &(OcGpio_Pin){ &s_fake_io_port, 5 },
        .type = { 0, 0 },
        .ss = 0,
    };
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read_board_info(&b1_dev, &rominfo));
    TEST_ASSERT_EQUAL_HEX8(0x05, rominfo);

    Eeprom_Cfg b2_dev = {
        .i2c_dev = { 6, 0x50 },
        .pin_wp = &(OcGpio_Pin){ &s_fake_io_port, 5 },
        .type = { 0, 0 },
        .ss = 7,
    };
    EEPROM_regs[0xAC01] = 0x06;
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read_board_info(&b2_dev, &rominfo));
    TEST_ASSERT_EQUAL_HEX8(0x06, rominfo);

    Eeprom_Cfg b3_dev = {
        .i2c_dev = { 6, 0x50 },
        .pin_wp = &(OcGpio_Pin){ &s_fake_io_port, 5 },
        .type = { 0, 0 },
        .ss = 8,
    };
    EEPROM_regs[0xAC01] = 0x07;
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read_board_info(&b3_dev, &rominfo));
    TEST_ASSERT_EQUAL_HEX8(0x07, rominfo);
}
void test_eeprom_read_oc_info(void)
{
    uint8_t ocserial = 0x00;

    EEPROM_regs[0xC601] = 0x05;

    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read_oc_info(&ocserial));

    TEST_ASSERT_EQUAL(0x05, ocserial);
}

void test_eeprom_read_device_info_record(void)
{
    uint8_t recordno = 1;
    EEPROM_regs[0x0A01] = 0x4153;
    char *deviceinfo = (char *)malloc(10);

    Eeprom_Cfg c1_dev = {
        .i2c_dev = { 6, 0x50 },
        .pin_wp = &(OcGpio_Pin){ &s_fake_io_port, 5 },
        .type = { 0, 0 },
        .ss = 0,
    };
    memset(deviceinfo, 0, 10);
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read_device_info_record(
                                         &c1_dev, recordno, deviceinfo));
    TEST_ASSERT_EQUAL_STRING("SA", deviceinfo);

    uint8_t recordno1 = 1;
    EEPROM_regs[0x0A01] = 0x4153;
    char *deviceinfo1 = (char *)malloc(10);

    Eeprom_Cfg c2_dev = {
        .i2c_dev = { 6, 0x50 },
        .pin_wp = &(OcGpio_Pin){ &s_fake_io_port, 5 },
        .type = { 0, 0 },
        .ss = 7,
    };
    memset(deviceinfo1, 0, 10);
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read_device_info_record(
                                         &c2_dev, recordno1, deviceinfo1));
    TEST_ASSERT_EQUAL_STRING("SA", deviceinfo1);

    uint8_t recordno2 = 1;
    EEPROM_regs[0x0A01] = 0x4153;
    char *deviceinfo2 = (char *)malloc(10);

    Eeprom_Cfg c3_dev = {
        .i2c_dev = { 6, 0x50 },
        .pin_wp = &(OcGpio_Pin){ &s_fake_io_port, 5 },
        .type = { 0, 0 },
        .ss = 8,
    };
    memset(deviceinfo2, 0, 10);
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read_device_info_record(
                                         &c3_dev, recordno2, deviceinfo2));
    TEST_ASSERT_EQUAL_STRING("SA", deviceinfo2);
}

void test_eeprom_write_device_info_record(void)
{
    uint8_t recordno = 1;
    char *deviceinfo = (char *)malloc(10);
    memset(deviceinfo, 0, 10);

    strcpy(deviceinfo, "SA");

    Eeprom_Cfg d1_dev = {
        .i2c_dev = { 6, 0x51 },
        .pin_wp = &(OcGpio_Pin){ &s_fake_io_port, 5 },
        .type = { 0, 0 },
        .ss = 0,
    };

    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_write_device_info_record(
                                         &d1_dev, recordno, deviceinfo));
    TEST_ASSERT_EQUAL(0x4153, EEPROM_regs[0x0A01]);
    strcpy(deviceinfo, "SB");

    Eeprom_Cfg d2_dev = {
        .i2c_dev = { 6, 0x51 },
        .pin_wp = &(OcGpio_Pin){ &s_fake_io_port, 5 },
        .type = { 0, 0 },
        .ss = 0,
    };

    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_write_device_info_record(
                                         &d2_dev, recordno, deviceinfo));
    TEST_ASSERT_EQUAL(0x4253, EEPROM_regs[0x0A01]);

    strcpy(deviceinfo, "SC");
    Eeprom_Cfg d3_dev = {
        .i2c_dev = { 6, 0x51 },
        .pin_wp = &(OcGpio_Pin){ &s_fake_io_port, 5 },
        .type = { 0, 0 },
        .ss = 0,
    };

    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_write_device_info_record(
                                         &d3_dev, recordno, deviceinfo));
    TEST_ASSERT_EQUAL(0x4353, EEPROM_regs[0x0A01]);
}
