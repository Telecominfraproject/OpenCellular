/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "drivers/PinGroup.h"
#include "drivers/GpioPCA9557.h"
#include "fake/fake_I2C.h"
#include "fake/fake_GPIO.h"
#include "inc/devices/dat-xxr5a-pp.h"
#include "inc/subsystem/rffe/rffe_ctrl.h"
#include "helpers/array.h"
#include "helpers/attribute.h"
#include "common/inc/global/OC_CONNECT1.h"
#include <string.h>
#include "unity.h"

/* ======================== Constants & variables =========================== */
#define I2C_BUS 2
#define I2C_ADDR 0x18
static uint8_t PCA9557_regs[] = {
    [0x00] = 0x00, /* Input values */
    [0x01] = 0x00, /* Output values */
    [0x02] = 0x00, /* Polarity */
    [0x03] = 0x00, /* Dir Config */
};
OcGpio_Port fe_ch1_gain_io = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg =
            &(PCA9557_Cfg){
                    .i2c_dev = { OC_CONNECT1_I2C2,
                                 RFFE_CHANNEL1_IO_TX_ATTEN_ADDR },
            },
    .object_data = &(PCA9557_Obj){},
};

Fe_Gain_Cfg fe_ch1_gain = {
    /* CH1_TX_ATTN_16DB */
    .pin_tx_attn_16db = { &fe_ch1_gain_io, 1 },
    /* CH1_TX_ATTN_P5DB */
    .pin_tx_attn_p5db = { &fe_ch1_gain_io, 2 },
    /* CH1_TX_ATTN_1DB */
    .pin_tx_attn_1db = { &fe_ch1_gain_io, 3 },
    /* CH1_TX_ATTN_2DB */
    .pin_tx_attn_2db = { &fe_ch1_gain_io, 4 },
    /* CH1_TX_ATTN_4DB */
    .pin_tx_attn_4db = { &fe_ch1_gain_io, 5 },
    /* CH1_TX_ATTN_8DB */
    .pin_tx_attn_8db = { &fe_ch1_gain_io, 6 },
    /* CH1_TX_ATTN_ENB */
    .pin_tx_attn_enb = { &fe_ch1_gain_io, 7 },
};

const DATR5APP_Cfg *cfg_1 = (DATR5APP_Cfg *)&fe_ch1_gain;

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
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}

void test_PinGroup_configure(void)
{
    PinGroup pin_group = { .num_pin = 6, /* DATR5APP_PIN_COUNT */
                           .pins = cfg_1->pin_group };

    PCA9557_regs[0] = 0xFF; /* Input values */
    PCA9557_regs[1] = 0xFF; /* Output values */
    PCA9557_regs[2] = 0xFF; /* Polarity */
    PCA9557_regs[3] = 0xFF; /* Dir Config */

    TEST_ASSERT_EQUAL(
            RETURN_OK,
            PinGroup_configure(&pin_group,
                               OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH));

    TEST_ASSERT_EQUAL_HEX8(0x7E, PCA9557_regs[0x01]);
    TEST_ASSERT_EQUAL_HEX8(0x00, PCA9557_regs[0x02]);
    TEST_ASSERT_EQUAL_HEX8(0x00, PCA9557_regs[0x03]);
}

void test_PinGroup_read(void)
{
    PinGroup pin_group = { .num_pin = 6, /* DATR5APP_PIN_COUNT */
                           .pins = cfg_1->pin_group };
    uint8_t value = 0x00;

    TEST_ASSERT_EQUAL(RETURN_OK, PinGroup_read(&pin_group, &value));
    TEST_ASSERT_EQUAL_HEX8(0x3F, value);
}

void test_PinGroup_write(void)
{
    PinGroup pin_group = { .num_pin = 6, /* DATR5APP_PIN_COUNT */
                           .pins = cfg_1->pin_group };

    PCA9557_regs[0] = 0xFF; /* Input values */
    PCA9557_regs[1] = 0xFF; /* Output values */
    PCA9557_regs[2] = 0xFF; /* Polarity */
    PCA9557_regs[3] = 0xFF; /* Dir Config */

    TEST_ASSERT_EQUAL(RETURN_OK, PinGroup_write(&pin_group, 1));
    TEST_ASSERT_EQUAL_HEX8(0xFF, PCA9557_regs[0x00]);
    TEST_ASSERT_EQUAL_HEX8(0x04, PCA9557_regs[0x01]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, PCA9557_regs[0x02]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, PCA9557_regs[0x03]);
}
