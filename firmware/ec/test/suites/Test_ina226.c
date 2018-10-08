/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "unity.h"
#include "inc/devices/ina226.h"

#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"

#include <string.h>

/* ======================== Constants & variables =========================== */
static OcGpio_Port s_fake_io_port = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

static INA226_Dev s_dev = {
    .cfg =
            {
                    .dev =
                            {
                                    .bus = 4,
                                    .slave_addr = 0x01,
                            },
            },
};

static INA226_Dev s_invalid_dev = {
    .cfg =
            {
                    .dev =
                            {
                                    .bus = 4,
                                    .slave_addr = 0x02,
                            },
            },
};

static uint16_t INA226_regs[] = {
    [0x00] = 0x0000, /* Configuration */
    [0x01] = 0x0000, /* Shunt Volatge */
    [0x02] = 0x0000, /* Bus Voltage */
    [0x03] = 0x0000, /* Power */
    [0x04] = 0x0000, /* Current */
    [0x05] = 0x0000, /* Calibration */
    [0x06] = 0x0000, /* Mask Enable */
    [0x07] = 0x0000, /* Alert Limit */
    [0xFE] = 0x0000, /* Manf Id */
    [0xFF] = 0x0000, /* Die Id */
};

static bool INA226_GpioPins[] = {
    [0x05] = 0x1,
};

static uint32_t INA226_GpioConfig[] = {
    [0x05] = OCGPIO_CFG_INPUT,
};
/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    FakeGpio_registerDevSimple(INA226_GpioPins, INA226_GpioConfig);
    fake_I2C_init();

    fake_I2C_registerDevSimple(s_dev.cfg.dev.bus, s_dev.cfg.dev.slave_addr,
                               INA226_regs, sizeof(INA226_regs),
                               sizeof(INA226_regs[0]), sizeof(uint8_t),
                               FAKE_I2C_DEV_BIG_ENDIAN);
}

void setUp(void)
{
    memset(INA226_regs, 0, sizeof(INA226_regs));

    OcGpio_init(&s_fake_io_port);

    ina226_init(&s_dev);
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}

/* ================================ Tests =================================== */
void test_ina226_init(void)
{
    /* Make sure that if we're in a weird state, we reset best we can */
    INA226_regs[0x00] = 0x1234;
    INA226_regs[0x05] = 0x0000; /* Calibration reg */
    TEST_ASSERT_EQUAL(RETURN_OK, ina226_init(&s_dev));

    /* Make sure we've reset the device */
    TEST_ASSERT_BITS_HIGH(0x0001, INA226_regs[0x00]);

    /* Make sure calibration register gets initialized */
    TEST_ASSERT_NOT_EQUAL(0, INA226_regs[0x05]);

    /* TODO: check rest of configuration values */

    /* Now try to init with a pin associated */
    INA226_Dev alerted_dev = {
        .cfg =
                {
                        .dev = s_dev.cfg.dev,
                        .pin_alert = &(OcGpio_Pin){ &s_fake_io_port, 5 },
                },
    };
    TEST_ASSERT_EQUAL(RETURN_OK, ina226_init(&alerted_dev));
}

static struct Test_AlertData {
    unsigned int triggered;
    INA226_Event evt;
    uint16_t val;
    void *ctx;
} s_alert_data;

static void _ina226_alert_handler(INA226_Event evt, uint16_t value,
                                  void *context)
{
    s_alert_data = (struct Test_AlertData){
        .triggered = s_alert_data.triggered + 1,
        .evt = evt,
        .val = value,
        .ctx = context,
    };
}

static void _test_alert(INA226_Dev *dev, INA226_Event evt, uint16_t alert_mask,
                        uint16_t val, uint16_t new_mask)
{
    INA226_regs[0x06] |= 0xF800; /* Enable all interrupts to see how we do */
    TEST_ASSERT_EQUAL(RETURN_OK, ina226_enableAlert(dev, evt));

    /* Test that the enable alert flag is set */
    TEST_ASSERT_BITS_HIGH(alert_mask, INA226_regs[0x06]);

    /* Make sure previous bits were cleared */
    TEST_ASSERT_BITS_LOW((~alert_mask) & 0xF800, INA226_regs[0x06]);

    /* Artificially enable the lesser priority alerts to make sure we only
     * process the highest order one */
    while (alert_mask > 0x0800) {
        INA226_regs[0x06] |= (alert_mask >>= 1);
    }

    /* Test that things run properly on a shared line (interrupt when we didn't
     * post an event) */
    s_alert_data.triggered = 0;
    INA226_regs[0x06] &= ~((1 << 3) | (1 << 4));
    FakeGpio_triggerInterrupt(dev->cfg.pin_alert);
    TEST_ASSERT_EQUAL(0, s_alert_data.triggered);

    INA226_regs[0x06] |= (1 << 4); /* Fault caused alert */
    alert_mask = INA226_regs[0x06]; /* Store reg value for later comparison */

    FakeGpio_triggerInterrupt(dev->cfg.pin_alert);
    TEST_ASSERT_EQUAL(1, s_alert_data.triggered);
    TEST_ASSERT_EQUAL_HEX16(evt, s_alert_data.evt);
    TEST_ASSERT_EQUAL(val, s_alert_data.val);

    /* Make sure we changed the mask to avoid multiple alerts */
    TEST_ASSERT_EQUAL_HEX16(new_mask | (alert_mask & 0x07FF),
                            INA226_regs[0x06]);
}
void test_ina226_alerts(void)
{
    s_alert_data = (struct Test_AlertData){};

    /* Create a device with an interrupt pin */
    INA226_Dev alerted_dev = {
        .cfg =
                {
                        .dev = s_dev.cfg.dev,
                        .pin_alert = &(OcGpio_Pin){ &s_fake_io_port, 5 },
                },
    };
    TEST_ASSERT_EQUAL(RETURN_OK, ina226_init(&alerted_dev));

    ina226_setAlertHandler(&alerted_dev, _ina226_alert_handler, NULL);

    INA226_regs[0x01] = 0x0064; /* vShunt 250 uV */
    INA226_regs[0x02] = 0x0A50; /* vBus   3300mV */
    INA226_regs[0x03] = 0x02A8; /* P      1700mW */
    INA226_regs[0x04] = 0x2EE0; /* i      1200mA */

    _test_alert(&alerted_dev, INA226_EVT_SOL, 0x8000, 250, 0x4000);
    _test_alert(&alerted_dev, INA226_EVT_SUL, 0x4000, 250, 0x8000);
    _test_alert(&alerted_dev, INA226_EVT_BOL, 0x2000, 3300, 0x1000);
    _test_alert(&alerted_dev, INA226_EVT_BUL, 0x1000, 3300, 0x2000);
    _test_alert(&alerted_dev, INA226_EVT_POL, 0x0800, 1700, 0x0800);

    /* Test meta alerts which are based on other alerts */
    /* Current alerts based on shunt - 250uV ~ 1200mA with this config */
    _test_alert(&alerted_dev, INA226_EVT_COL, 0x8000, 1200, 0x4000);
    _test_alert(&alerted_dev, INA226_EVT_CUL, 0x4000, 1200, 0x8000);
}

void test_ina226_probe(void)
{
    POSTData postData;
    /* Test with the actual values */
    INA226_regs[0xFF] = 0x2260;
    INA226_regs[0xFE] = 0x5449;
    TEST_ASSERT_EQUAL(POST_DEV_FOUND, ina226_probe(&s_dev, &postData));

    /* Test with an incorrect device ID */
    INA226_regs[0xFF] = 0xC802;
    TEST_ASSERT_EQUAL(POST_DEV_ID_MISMATCH, ina226_probe(&s_dev, &postData));

    /* Test with an incorrect mfg ID */
    INA226_regs[0xFF] = 0x2260;
    INA226_regs[0xFE] = 0x5DC7;
    TEST_ASSERT_EQUAL(POST_DEV_ID_MISMATCH, ina226_probe(&s_dev, &postData));

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING,
                      ina226_probe(&s_invalid_dev, &postData));
}

void test_current_limit(void)
{
    uint16_t current_val = 0xffff;

    INA226_regs[0x07] = 0x0320; //800
    TEST_ASSERT_EQUAL(RETURN_OK, ina226_readCurrentLim(&s_dev, &current_val));
    TEST_ASSERT_EQUAL(1000, current_val); //1000mA

    TEST_ASSERT_EQUAL(RETURN_OK, ina226_setCurrentLim(&s_dev, 3000)); //3000mA
    TEST_ASSERT_EQUAL_HEX16(0x0960, INA226_regs[0x07]); //2400

    TEST_ASSERT_EQUAL(RETURN_OK, ina226_readCurrentLim(&s_dev, &current_val));
    TEST_ASSERT_EQUAL(3000, current_val); //3000mA
}

void test_ina226_enableAlert(void)
{
    TEST_ASSERT_EQUAL(RETURN_OK, ina226_enableAlert(&s_dev, 0x8001));
    TEST_ASSERT_EQUAL_HEX16(0x8001, INA226_regs[0x06]);
}

void test_curr_sens_bus_volatge(void)
{
    uint16_t busVoltage_val = 0xffff;

    INA226_regs[0x02] = 0x2580; //9600
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ina226_readBusVoltage(&s_dev, &busVoltage_val));
    TEST_ASSERT_EQUAL_HEX16(12000, busVoltage_val); //12000mV

    INA226_regs[0x02] = 0x0A50; //2640
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ina226_readBusVoltage(&s_dev, &busVoltage_val));
    TEST_ASSERT_EQUAL_HEX16(3300, busVoltage_val); //3300mV
}

void test_curr_sens_shunt_volatge(void)
{
    uint16_t shuntVoltage_val = 0xffff;

    INA226_regs[0x01] = 0x0168; //360
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ina226_readShuntVoltage(&s_dev, &shuntVoltage_val));
    TEST_ASSERT_EQUAL_HEX16(900, shuntVoltage_val); //900uV

    INA226_regs[0x01] = 0x0064; //100
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ina226_readShuntVoltage(&s_dev, &shuntVoltage_val));
    TEST_ASSERT_EQUAL_HEX16(250, shuntVoltage_val); //250uV
}

void test_curr_sens_current(void)
{
    uint16_t current_val = 0xffff;

    INA226_regs[0x04] = 0x1388; //5000
    TEST_ASSERT_EQUAL(RETURN_OK, ina226_readCurrent(&s_dev, &current_val));
    TEST_ASSERT_EQUAL_HEX16(500, current_val); //500mA

    INA226_regs[0x04] = 0x2EE0; //12000
    TEST_ASSERT_EQUAL(RETURN_OK, ina226_readCurrent(&s_dev, &current_val));
    TEST_ASSERT_EQUAL_HEX16(1200, current_val); //1200mA
}

void test_curr_sens_power(void)
{
    uint16_t power_val = 0xffff;

    INA226_regs[0x03] = 0x02A8; //680
    TEST_ASSERT_EQUAL(RETURN_OK, ina226_readPower(&s_dev, &power_val));
    TEST_ASSERT_EQUAL_HEX16(1700, power_val); //1700mW

    INA226_regs[0x03] = 0x04B0; //1200
    TEST_ASSERT_EQUAL(RETURN_OK, ina226_readPower(&s_dev, &power_val));
    TEST_ASSERT_EQUAL_HEX16(3000, power_val);
}

void test_curr_sens_not_present(void)
{
    /* Ensure that we fail properly if the device isn't on the bus */
    uint16_t dummy_val;
    POSTData postData;
    TEST_ASSERT_EQUAL(POST_DEV_MISSING,
                      ina226_probe(&s_invalid_dev, &postData));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ina226_readCurrentLim(&s_invalid_dev, &dummy_val));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ina226_setCurrentLim(&s_invalid_dev, dummy_val));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ina226_enableAlert(&s_invalid_dev, INA226_EVT_COL));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ina226_readBusVoltage(&s_invalid_dev, &dummy_val));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ina226_readShuntVoltage(&s_invalid_dev, &dummy_val));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ina226_readCurrent(&s_invalid_dev, &dummy_val));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ina226_readPower(&s_invalid_dev, &dummy_val));
}
