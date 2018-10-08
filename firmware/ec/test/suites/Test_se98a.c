/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "unity.h"
#include "inc/devices/se98a.h"

#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"

#include <stdbool.h>
#include <string.h>

/* ======================== Constants & variables =========================== */
static OcGpio_Port s_fake_io_port = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

static SE98A_Dev s_dev = {
    .cfg =
            {
                    .dev =
                            {
                                    .bus = 3,
                                    .slave_addr = 0x1A,
                            },
            },
};
static SE98A_Dev s_invalid = {
    .cfg =
            {
                    .dev =
                            {
                                    .bus = 3,
                                    .slave_addr = 0xFF,
                            },
            },
};

static uint16_t SE98A_regs[] = {
    [0x00] = 0x00, /* Capabilities */
    [0x01] = 0x00, /* Config */
    [0x02] = 0x00, /* High limit */
    [0x03] = 0x00, /* Low limit */
    [0x04] = 0x00, /* Critical limit */
    [0x05] = 0x00, /* Measured Temperature */
    [0x06] = 0x00, /* MFG ID */
    [0x07] = 0x00, /* Device ID */
};
static bool SE98A_GpioPins[] = {
    [0x05] = 0x1,
};

static uint32_t SE98A_GpioConfig[] = {
    [0x05] = OCGPIO_CFG_INPUT,
};
/* ============================= Fake Functions ============================= */
#include <ti/sysbios/knl/Task.h>
unsigned int s_task_sleep_ticks;
xdc_Void ti_sysbios_knl_Task_sleep__E(xdc_UInt32 nticks)
{
    s_task_sleep_ticks += nticks;
}

/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    FakeGpio_registerDevSimple(SE98A_GpioPins, SE98A_GpioConfig);
    fake_I2C_init();
    fake_I2C_registerDevSimple(s_dev.cfg.dev.bus, s_dev.cfg.dev.slave_addr,
                               SE98A_regs, sizeof(SE98A_regs),
                               sizeof(SE98A_regs[0]), sizeof(uint8_t),
                               FAKE_I2C_DEV_BIG_ENDIAN);
}

void setUp(void)
{
    memset(SE98A_regs, 0, sizeof(SE98A_regs));
    SE98A_regs[0x00] = 0x0037;
    SE98A_regs[0x06] = 0x1131;
    SE98A_regs[0x07] = 0xA102;

    s_task_sleep_ticks = 0;

    OcGpio_init(&s_fake_io_port);

    se98a_init(&s_dev);
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}

/* ================================ Tests =================================== */
void test_se98a_init(void)
{
    /* Make sure that if we're in a weird state, we reset the best we can */
    SE98A_regs[0x01] = 0xFFFF;
    TEST_ASSERT_EQUAL(RETURN_OK, se98a_init(&s_dev));

    /* This might be a bit of a fragile test if we want to change the default
     * the critical part is that we clear any existing events and disable
     * the alert pin
     * NOTE: if the default config ever removes hysteresis, be very careful,
     * the sensors are more noisy than you might think */
    TEST_ASSERT_EQUAL_HEX16(0x0221, SE98A_regs[0x01]);

    /* Now try to init with a pin associated */
    SE98A_Dev alerted_dev = {
        .cfg =
                {
                        .dev = s_dev.cfg.dev,
                        .pin_evt = &(OcGpio_Pin){ &s_fake_io_port, 5 },
                },
    };
    TEST_ASSERT_EQUAL(RETURN_OK, se98a_init(&alerted_dev));
}

static struct Test_AlertData {
    bool triggered;
    SE98A_Event evt;
    int8_t temp;
    void *ctx;
} s_alert_data;

static void alert_handler(SE98A_Event evt, int8_t temperature, void *context)
{
    s_alert_data = (struct Test_AlertData){
        .triggered = true,
        .evt = evt,
        .temp = temperature,
        .ctx = context,
    };
}

/* Helper for testing the various alerts this device can create */
static void _test_alert(SE98A_Dev *dev, uint16_t temp_reg, SE98A_Event exp_evt,
                        int8_t exp_temp)
{
    s_alert_data.triggered = false;
    SE98A_regs[0x01] |= 0x10;
    SE98A_regs[0x01] &= ~(0x20); /* Reset the 'clear' bit */
    SE98A_regs[0x05] = temp_reg;

    FakeGpio_triggerInterrupt(dev->cfg.pin_evt);
    TEST_ASSERT_TRUE(s_alert_data.triggered);
    TEST_ASSERT_EQUAL(exp_evt, s_alert_data.evt);
    TEST_ASSERT_EQUAL(exp_temp, s_alert_data.temp);

    /* Make sure the event was cleared */
    TEST_ASSERT_BITS_HIGH(0x20, SE98A_regs[0x01]);
}

void test_se98a_alerts(void)
{
    s_alert_data = (struct Test_AlertData){};

    /* Now try to init with a pin associated */
    SE98A_Dev alerted_dev = {
        .cfg =
                {
                        .dev = s_dev.cfg.dev,
                        .pin_evt = &(OcGpio_Pin){ &s_fake_io_port, 5 },
                },
    };
    TEST_ASSERT_EQUAL(RETURN_OK, se98a_init(&alerted_dev));

    TEST_ASSERT_EQUAL(
            RETURN_OK,
            se98a_set_limit(&alerted_dev, CONF_TEMP_SE98A_LOW_LIMIT_REG, -10));
    TEST_ASSERT_EQUAL(
            RETURN_OK,
            se98a_set_limit(&alerted_dev, CONF_TEMP_SE98A_HIGH_LIMIT_REG, 75));
    TEST_ASSERT_EQUAL(RETURN_OK,
                      se98a_set_limit(&alerted_dev,
                                      CONF_TEMP_SE98A_CRITICAL_LIMIT_REG, 80));
    se98a_set_alert_handler(&alerted_dev, alert_handler, NULL);
    s_task_sleep_ticks = 0;
    TEST_ASSERT_EQUAL(RETURN_OK, se98a_enable_alerts(&alerted_dev));
    TEST_ASSERT_MESSAGE(s_task_sleep_ticks >= 125,
                        "Didn't wait long enough after setting alert window");

    /* Test that the enable alert flag is set */
    TEST_ASSERT_BITS_HIGH(0x08, SE98A_regs[0x01]);

    /* Test that things run properly on a shared line (interrupt when we didn't
     * post an event */
    SE98A_regs[0x01] &= ~(0x10);
    FakeGpio_triggerInterrupt(alerted_dev.cfg.pin_evt);
    TEST_ASSERT_FALSE(s_alert_data.triggered);

    /* Test alert below window */
    _test_alert(&alerted_dev, 0x2000 | 0x1E64 /* LOW | -25.75 */, SE98A_EVT_BAW,
                -26);

    /* Test alert above window */
    _test_alert(&alerted_dev, 0x4000 | (76 << 4) /* HIGH | 75 */, SE98A_EVT_AAW,
                76);

    /* Test alert critical */
    _test_alert(&alerted_dev, 0x8000 | (90 << 4) /* CRIT | 90 */, SE98A_EVT_ACT,
                90);

    /* Make sure the critical alert takes precedence */
    _test_alert(&alerted_dev, 0xE000 | (90 << 4) /* CRIT | 90 */, SE98A_EVT_ACT,
                90);
}

void test_se98a_probe(void)
{
    /* Test with the actual values
     * (dev id is hi-byte)
     * (1131h = NXP Semiconductors PCI-SIG)*/
    POSTData postData;
    SE98A_regs[0x07] = 0xA102;
    SE98A_regs[0x06] = 0x1131;
    TEST_ASSERT_EQUAL(POST_DEV_FOUND, se98a_probe(&s_dev, &postData));

    /* Test with an incorrect device ID */
    SE98A_regs[0x07] = 0xFACE;
    TEST_ASSERT_EQUAL(POST_DEV_ID_MISMATCH, se98a_probe(&s_dev, &postData));

    /* Test with an incorrect mfg ID */
    SE98A_regs[0x07] = 0xA102;
    SE98A_regs[0x06] = 0xABCD;
    TEST_ASSERT_EQUAL(POST_DEV_ID_MISMATCH, se98a_probe(&s_dev, &postData));

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING, se98a_probe(&s_invalid, &postData));
}

/* Helper to let us run through the various limits we can set */
static void test_set_x_limit(eTempSensor_ConfigParamsId limitToConfig,
                             uint8_t reg_addr)
{
    /* Register map:
     * [15..13] RFU
     * [12]     SIGN (2's complement)
     * [11..4]  Integer part (8 bits)
     * [3..2]   Fractional part (0.5, 0.25)
     * [1..0]   RFU
     */

    TEST_ASSERT_EQUAL(RETURN_OK, se98a_set_limit(&s_dev, limitToConfig, 0));
    TEST_ASSERT_EQUAL_HEX16(0, SE98A_regs[reg_addr]);

    TEST_ASSERT_EQUAL(RETURN_OK, se98a_set_limit(&s_dev, limitToConfig, 1));
    TEST_ASSERT_EQUAL_HEX16(1 << 4, SE98A_regs[reg_addr]);

    TEST_ASSERT_EQUAL(RETURN_OK, se98a_set_limit(&s_dev, limitToConfig, 75));
    TEST_ASSERT_EQUAL_HEX16(75 << 4, SE98A_regs[reg_addr]);

    TEST_ASSERT_EQUAL(RETURN_OK, se98a_set_limit(&s_dev, limitToConfig, -75));
    TEST_ASSERT_EQUAL_HEX16(0x1B50, SE98A_regs[reg_addr]);

    TEST_ASSERT_EQUAL(RETURN_OK, se98a_set_limit(&s_dev, limitToConfig, 127));
    TEST_ASSERT_EQUAL_HEX16(0x07F0, SE98A_regs[reg_addr]);

    TEST_ASSERT_EQUAL(RETURN_OK, se98a_set_limit(&s_dev, limitToConfig, -128));
    TEST_ASSERT_EQUAL_HEX16(0x1800, SE98A_regs[reg_addr]);
}

void test_temp_sens_set_limit(void)
{
    test_set_x_limit(CONF_TEMP_SE98A_LOW_LIMIT_REG, 0x03);
    test_set_x_limit(CONF_TEMP_SE98A_HIGH_LIMIT_REG, 0x02);
    test_set_x_limit(CONF_TEMP_SE98A_CRITICAL_LIMIT_REG, 0x04);
}

void test_temp_sens_get_temp_val(void)
{
    /*
     * [15..13] Trip Status
     * [12..5] 8-bit integer part
     * [4..1] fractional part
     * [0] RFU
     */
    int8_t temp;

    SE98A_regs[0x05] = 0x019C; /* 25.75 */
    TEST_ASSERT_EQUAL(RETURN_OK, se98a_read(&s_dev, &temp));
    TEST_ASSERT_EQUAL(26, temp);

    SE98A_regs[0x05] = 0x1E64; /* -25.75 */
    TEST_ASSERT_EQUAL(RETURN_OK, se98a_read(&s_dev, &temp));
    TEST_ASSERT_EQUAL(-26, temp);

    SE98A_regs[0x05] = 0x07C0;
    TEST_ASSERT_EQUAL(RETURN_OK, se98a_read(&s_dev, &temp));
    TEST_ASSERT_EQUAL(124, temp);

    SE98A_regs[0x05] = 0x1FF0;
    TEST_ASSERT_EQUAL(RETURN_OK, se98a_read(&s_dev, &temp));
    TEST_ASSERT_EQUAL(-1, temp);

    SE98A_regs[0x05] = 0x1C90;
    TEST_ASSERT_EQUAL(RETURN_OK, se98a_read(&s_dev, &temp));
    TEST_ASSERT_EQUAL(-55, temp);

    /* The device shouldn't return temperatures larger than 125, so we only
     * support int8s - everything else is rounded for now */
    SE98A_regs[0x05] = 0x17E0; /* -130 */
    TEST_ASSERT_EQUAL(RETURN_OK, se98a_read(&s_dev, &temp));
    TEST_ASSERT_EQUAL(-128, temp);

    SE98A_regs[0x05] = 0x0B40; /* 180 */
    TEST_ASSERT_EQUAL(RETURN_OK, se98a_read(&s_dev, &temp));
    TEST_ASSERT_EQUAL(127, temp);

    /* Make sure we mask the status/RFU bits out */
    SE98A_regs[0x05] = 0xFC91;
    TEST_ASSERT_EQUAL(RETURN_OK, se98a_read(&s_dev, &temp));
    TEST_ASSERT_EQUAL(-55, temp);
}

/* Helper to let us run through the various limits we can get */
static void test_get_x_limit(eTempSensor_ConfigParamsId limitToConfig,
                             uint8_t reg_addr)
{
    /* Register map:
     * [15..13] RFU
     * [12] sign
     * [11..4] 9-bit integer part
     * [3..2] fractional part (0.5, 0.25)
     * [1..0] RFU
     */
    int8_t limit;

    SE98A_regs[reg_addr] = 0x0000;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      se98a_get_limit(&s_dev, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(0, limit);

    SE98A_regs[reg_addr] = 1 << 4;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      se98a_get_limit(&s_dev, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(1, limit);

    SE98A_regs[reg_addr] = 75 << 4;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      se98a_get_limit(&s_dev, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(75, limit);

    SE98A_regs[reg_addr] = 0x019C; /* 25.75 */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      se98a_get_limit(&s_dev, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(26, limit);

    SE98A_regs[reg_addr] = 0x1B50;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      se98a_get_limit(&s_dev, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(-75, limit);

    SE98A_regs[reg_addr] = 0x07F0;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      se98a_get_limit(&s_dev, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(127, limit);

    SE98A_regs[reg_addr] = 0x1800;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      se98a_get_limit(&s_dev, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(-128, limit);

    /* Make sure we mask the RFU bits out */
    SE98A_regs[reg_addr] = 0x07FC;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      se98a_get_limit(&s_dev, limitToConfig, &limit));
    TEST_ASSERT_EQUAL(127, limit);
}

void test_temp_sens_get_limit(void)
{
    test_get_x_limit(CONF_TEMP_SE98A_LOW_LIMIT_REG, 0x03);
    test_get_x_limit(CONF_TEMP_SE98A_HIGH_LIMIT_REG, 0x02);
    test_get_x_limit(CONF_TEMP_SE98A_CRITICAL_LIMIT_REG, 0x04);
}
