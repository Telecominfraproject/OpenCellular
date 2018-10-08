/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "unity.h"
#include "inc/devices/ltc4015.h"

#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"

#include <string.h>

/* ======================== Constants & variables =========================== */
static OcGpio_Port s_fake_io_port = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

static const I2C_Dev I2C_DEV = {
    .bus = 0,
    .slave_addr = 0x68,
};

static LTC4015_Dev s_dev = {
    .cfg =
            {
                    .i2c_dev =
                            {
                                    .bus = 0,
                                    .slave_addr = 0x68,
                            },
            },
};

static LTC4015_Dev s_invalid_dev = {
    .cfg =
            {
                    .i2c_dev =
                            {
                                    .bus = 2,
                                    .slave_addr = 0x52,
                            },
            },
};

static uint16_t LTC4015_regs[] = {
    [0x01] = 0x00, /* Battery voltage low alert limit */
    [0x02] = 0x00, /* Battery voltage high alert limit */
    [0x03] = 0x00, /* Input voltage low alert limit */
    [0x04] = 0x00, /* Input voltage high alert limit */
    [0x05] = 0x00, /* Output voltage low alert limit */
    [0x06] = 0x00, /* Output voltage high alert limit */
    [0x07] = 0x00, /* Input current high alert limit */
    [0x08] = 0x00, /* Charge current low alert limit */
    [0x09] = 0x00, /* Die temperature high alert limit */
    [0x0A] = 0x00, /* Battery series resistance high alert limit */
    [0x0B] = 0x00, /* Thermistor ratio high(cold battery) alert limit */
    [0x0C] = 0x00, /* Thermistor ratio low(hot battery) alert limit */
    [0x0D] = 0x00, /* Enable limit monitoring and alert notification */
    [0x0E] = 0x00, /* Enable charger state alert notification */
    [0x0F] = 0x00, /* Enable charger status alert notification */
    [0x10] = 0x00, /* Columb counter QCOUNT low alert limit */
    [0x11] = 0x00, /* Columb counter QCOUNT high alert limit */
    [0x12] = 0x00, /* Columb counter prescale factor */
    [0x13] = 0x00, /* Columb counter value */
    [0x14] = 0x00, /* Configuration Settings */
    [0x15] = 0x00, /* Input current limit setting */
    [0x16] = 0x00, /* UVCLFB input undervoltage limit */
    [0x17] = 0x00, /* Reserved */
    [0x18] = 0x00, /* Reserved */
    [0x19] = 0x00, /* Arm ship mode */
    [0x1A] = 0x00, /* Charge current target */
    [0x1B] = 0x00, /* Charge voltage target */
    [0x1C] = 0x00, /* Low IBAT Threshold for C/x termination */
    [0x1D] =
            0x00, /* Time in seconds with battery charger in the CV state before timer termination */
    [0x1E] =
            0x00, /* Time in seconds before a max_charge_time fault is declared */
    [0x1F] = 0x00, /* JEITA_T1 */
    [0x20] = 0x00, /* JEITA_T2 */
    [0x21] = 0x00, /* JEITA_T3 */
    [0x22] = 0x00, /* JEITA_T4 */
    [0x23] = 0x00, /* JEITA_T5 */
    [0x24] = 0x00, /* JEITA_T6 */
    [0x25] = 0x00, /* VCHARGE values JEITA_6_5 */
    [0x26] = 0x00, /* VCHARGE values JEITA_4_3_2 */
    [0x27] = 0x00, /* ICHARGE_TARGET values JEITA_6_5 */
    [0x28] = 0x00, /* ICHARGE_TARGET values JEITA_4_3_2 */
    [0x29] = 0x00, /* Battery charger cfguration settings */
    [0x2A] = 0x00, /* LiFePO4/lead-acid absorb voltage adder */
    [0x2B] = 0x00, /* Maximum time for LiFePO4/lead-acid absorb charge */
    [0x2C] = 0x00, /* Lead-acid equalize charge voltage adder */
    [0x2D] = 0x00, /* Lead-acid equalization time */
    [0x2E] = 0x00, /* LiFeP04 recharge threshold */
    [0x2F] = 0x00, /* Reserved */
    [0x30] = 0x00, /* Max Charge Timer for lithium chemistries */
    [0x31] = 0x00, /* Constant-voltage regulation for lithium chemistries */
    [0x32] = 0x00, /* Absorb Timer for LiFePO4 and lead-acid batteries */
    [0x33] = 0x00, /* Eqaulize Timer for lead-acid batteries */
    [0x34] = 0x00, /* Real time battery charger state indicator */
    [0x35] = 0x00, /* Charge status indicator */
    [0x36] = 0x00, /* Limit alert register */
    [0x37] = 0x00, /* Charger state alert register */
    [0x38] = 0x00, /* Charger status alert indicator */
    [0x39] = 0x00, /* Real time system status indicator */
    [0x3A] = 0x00, /* VBAT value(BATSENS) */
    [0x3B] = 0x00, /* VIN */
    [0x3C] = 0x00, /* VSYS */
    [0x3D] = 0x00, /* Battery current(IBAT) */
    [0x3E] = 0x00, /* Input Current(IIN) */
    [0x3F] = 0x00, /* Die temperature */
    [0x40] = 0x00, /* NTC thermistor ratio */
    [0x41] = 0x00, /* Battery series resistance */
    [0x42] =
            0x00, /* JEITA temperature region of the NTC thermistor (Li Only) */
    [0x43] = 0x00, /* CHEM and CELLS pin settings */
    [0x44] = 0x00, /* Charge current control DAC control bits */
    [0x45] = 0x00, /* Charge voltage control DAC control bits */
    [0x46] = 0x00, /* Input current limit control DAC control word */
    [0x47] = 0x00, /* Digitally filtered battery voltage */
    [0x48] = 0x00, /* Value of IBAT (0x3D) used in calculating BSR */
    [0x49] = 0x00, /* Reserved */
    [0x4A] = 0x00, /* Measurement valid bit */
};

static bool LTC4015_GpioPins[] = {
    [0x05] = 0x1,
};

static uint32_t LTC4015_GpioConfig[] = {
    [0x05] = OCGPIO_CFG_INPUT,
};
/* ============================= Boilerplate ================================ */
void suite_setUp(void)
{
    FakeGpio_registerDevSimple(LTC4015_GpioPins, LTC4015_GpioConfig);
    fake_I2C_init();

    fake_I2C_registerDevSimple(I2C_DEV.bus, I2C_DEV.slave_addr, LTC4015_regs,
                               sizeof(LTC4015_regs), sizeof(LTC4015_regs[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
}

void setUp(void)
{
    memset(LTC4015_regs, 0, sizeof(LTC4015_regs));

    OcGpio_init(&s_fake_io_port);

    LTC4015_init(&s_dev);
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}

/* ================================ Tests =================================== */
void test_ltc4015_init(void)
{
    /* Make sure that if we're in a weird state, we reset the best we can */
    /* TODO: Do the reset here */

    /* Now try to init with a pin associated */
    LTC4015_Dev alerted_dev = {
        .cfg =
                {
                        .i2c_dev = s_dev.cfg.i2c_dev,
                        .pin_alert = &(OcGpio_Pin){ &s_fake_io_port, 5 },
                },
    };
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_init(&alerted_dev));
}

static struct Test_AlertData {
    bool triggered;
    LTC4015_Event evt;
    int16_t val;
    void *ctx;
} s_alert_data;

static void _alert_handler(LTC4015_Event evt, int16_t value, void *context)
{
    s_alert_data = (struct Test_AlertData){
        .triggered = true,
        .evt = evt,
        .val = value,
        .ctx = context,
    };
}

static void _test_alert(LTC4015_Dev *dev, LTC4015_Event evt,
                        uint16_t alert_mask, int16_t val)
{
    /* Clear all interrupts to see how we do */
    LTC4015_regs[0x0D] = 0;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_enableLimitAlerts(dev, evt));

    /* Test that the required enable alert flag is set */
    TEST_ASSERT_BITS_HIGH(alert_mask, LTC4015_regs[0x0D]);

    /* Make sure previous bits were cleared */
    TEST_ASSERT_BITS_LOW(~alert_mask, LTC4015_regs[0x0D]);

    /* Test that things run properly on a shared line (interrupt when we didn't
     * post an event) */
    s_alert_data.triggered = 0;
    LTC4015_regs[0x36] = 0;
    FakeGpio_triggerInterrupt(dev->cfg.pin_alert);
    TEST_ASSERT_EQUAL(0, s_alert_data.triggered);

    LTC4015_regs[0x36] |= alert_mask; /* Fault caused alert */

    FakeGpio_triggerInterrupt(dev->cfg.pin_alert);
    TEST_ASSERT_EQUAL(1, s_alert_data.triggered);
    TEST_ASSERT_EQUAL_HEX16(evt, s_alert_data.evt);
    TEST_ASSERT_EQUAL(val, s_alert_data.val);
}

void test_LTC4015_alerts(void)
{
    s_alert_data = (struct Test_AlertData){};

    /* Now try to init with a pin associated */
    LTC4015_Dev alerted_dev = {
        .cfg =
                {
                        .i2c_dev = s_dev.cfg.i2c_dev,
                        .chem = LTC4015_CHEM_LEAD_ACID,
                        .r_snsb = 3,
                        .r_snsi = 7,
                        .cellcount = 6,
                        .pin_alert = &(OcGpio_Pin){ &s_fake_io_port, 5 },
                },
    };

    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_init(&alerted_dev));

    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_battery_voltage_low(&alerted_dev, 9500));
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_battery_voltage_high(&alerted_dev, 9500));
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_input_voltage_low(&alerted_dev, 16200));
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_input_current_high(&alerted_dev, 5000));
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_battery_current_low(&alerted_dev, 2000));
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_die_temperature_high(&alerted_dev, 80));

    LTC4015_setAlertHandler(&alerted_dev, _alert_handler, NULL);

    LTC4015_regs[0x3A] = 0x3040; /* VBAT 9500mV */
    LTC4015_regs[0x3B] = 0x2666; /* VIN  16200mV */
    LTC4015_regs[0x3D] = 0x0FFF; /* IBAT 2000mA */
    LTC4015_regs[0x3E] = 0x5D54; /* IIN  5000mA */
    LTC4015_regs[0x3F] = 0x3D2A; /* T    80C */

    _test_alert(&alerted_dev, LTC4015_EVT_BVL, 0x0800, 9499);
    _test_alert(&alerted_dev, LTC4015_EVT_BVH, 0x0400, 9499);
    _test_alert(&alerted_dev, LTC4015_EVT_IVL, 0x0200, 16199);
    _test_alert(&alerted_dev, LTC4015_EVT_ICH, 0x0020, 4999);
    _test_alert(&alerted_dev, LTC4015_EVT_BCL, 0x0010, 1999);
    _test_alert(&alerted_dev, LTC4015_EVT_DTH, 0x0008, 80);
}

void test_LTC4015_enableLimitAlerts(void)
{
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_enableLimitAlerts(
                              &s_dev, LTC4015_EVT_BVL | LTC4015_EVT_ICH));
    TEST_ASSERT_EQUAL_HEX16(0x0820, LTC4015_regs[0x0D]);
}

void test_LTC4015_probe(void)
{
    POSTData postData;
    /* Test with the actual value */
    LTC4015_regs[0x39] = LTC4015_CHARGER_ENABLED;
    TEST_ASSERT_EQUAL(POST_DEV_FOUND, LTC4015_probe(&s_dev, &postData));

    /* Test with an incorrect value */
    LTC4015_regs[0x39] = ~LTC4015_CHARGER_ENABLED;
    TEST_ASSERT_EQUAL(POST_DEV_MISSING, LTC4015_probe(&s_dev, &postData));

    /* Test with a missing device */
    TEST_ASSERT_EQUAL(POST_DEV_MISSING,
                      LTC4015_probe(&s_invalid_dev, &postData));
}

void test_LTC4015_cfg_icharge(void)
{
    /* Maximum charge current target = (ICHARGE_TARGET + 1) • 1mV/RSNSB */
    /* The only thing that matters for this calc is Rsnsb */
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                        .r_snsb = 3,
                },
    };

    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_icharge(&charger, 3000));
    TEST_ASSERT_EQUAL(8, LTC4015_regs[0x1A]);

    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_icharge(&charger, 5000));
    TEST_ASSERT_EQUAL(14, LTC4015_regs[0x1A]);

    /* Test a case that results in rounding */
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_icharge(&charger, 4321));
    TEST_ASSERT_EQUAL(12, LTC4015_regs[0x1A]);

    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_icharge(&charger, 4150));
    TEST_ASSERT_EQUAL(11, LTC4015_regs[0x1A]);

    /* Make sure Rsnsb is taken into account properly */
    charger.cfg.r_snsb = 4;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_icharge(&charger, 3000));
    TEST_ASSERT_EQUAL(11, LTC4015_regs[0x1A]);

    /* Make sure low values are handled properly (iCharge * Rsnsb < 1000) */
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_icharge(&charger, 100));
    TEST_ASSERT_EQUAL(0, LTC4015_regs[0x1A]);
}

void test_LTC4015_get_cfg_icharge(void)
{
    /* Maximum charge current target = (ICHARGE_TARGET + 1) • 1mV/RSNSB */
    /* The only thing that matters for this calc is Rsnsb */
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                        .r_snsb = 3,
                },
    };
    uint16_t max_chargeCurrent;

    LTC4015_regs[0x1A] = 8;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_get_cfg_icharge(&charger, &max_chargeCurrent));
    TEST_ASSERT_EQUAL(3000, max_chargeCurrent);

    LTC4015_regs[0x1A] = 14;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_get_cfg_icharge(&charger, &max_chargeCurrent));
    TEST_ASSERT_EQUAL(5000, max_chargeCurrent);

    LTC4015_regs[0x1A] = 12;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_get_cfg_icharge(&charger, &max_chargeCurrent));
    TEST_ASSERT_EQUAL(4333, max_chargeCurrent);

    /* Make sure Rsnsb is taken into account properly */
    charger.cfg.r_snsb = 4;
    LTC4015_regs[0x1A] = 11;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_get_cfg_icharge(&charger, &max_chargeCurrent));
    TEST_ASSERT_EQUAL(3000, max_chargeCurrent);
}

void test_LTC4015_cfg_vcharge(void)
{
    /* Chemistry and #cells affect this calc */
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                },
    };

    /* TODO: check if we're using temperature comp. / if the driver should
     * deal with it */

    /* Test lithium ion chemistry */
    /* vcharge = (VCHARGE_SETTING/80.0 + 3.8125)V/cell */
    charger.cfg.chem = LTC4015_CHEM_LI_ION;
    charger.cfg.cellcount = 6;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_vcharge(&charger, 25200));
    TEST_ASSERT_EQUAL(31, LTC4015_regs[0x1B]);

    charger.cfg.cellcount = 3;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_vcharge(&charger, 12300));
    TEST_ASSERT_EQUAL(23, LTC4015_regs[0x1B]);

    /* Make sure too-low values don't do anything bad */
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_vcharge(&charger, 10000));
    TEST_ASSERT_EQUAL(0, LTC4015_regs[0x1B]);

    /* Test LiFePO4 chemistry */
    /* vcharge = (VCHARGE_SETTING/80.0 + 3.4125)V/cell */
    charger.cfg.chem = LTC4015_CHEM_LI_FE_PO4;
    charger.cfg.cellcount = 6;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_vcharge(&charger, 25000));
    TEST_ASSERT_EQUAL(60, LTC4015_regs[0x1B]);

    charger.cfg.cellcount = 2;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_vcharge(&charger, 7600));
    TEST_ASSERT_EQUAL(31, LTC4015_regs[0x1B]);

    /* Make sure too-low values don't do anything bad */
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_vcharge(&charger, 5000));
    TEST_ASSERT_EQUAL(0, LTC4015_regs[0x1B]);

    /* Test lead acid chemistry */
    /* vcharge = (VCHARGE_SETTING/105.0 + 2.0)V/cell w/o temp comp.*/
    charger.cfg.chem = LTC4015_CHEM_LEAD_ACID;
    charger.cfg.cellcount = 6;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_vcharge(&charger, 12600));
    TEST_ASSERT_EQUAL(11, LTC4015_regs[0x1B]);

    charger.cfg.cellcount = 4;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_vcharge(&charger, 12300));
    TEST_ASSERT_EQUAL(113, LTC4015_regs[0x1B]);

    /* Make sure too-low values don't do anything bad */
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_vcharge(&charger, 6000));
    TEST_ASSERT_EQUAL(0, LTC4015_regs[0x1B]);
}

void test_LTC4015_get_cfg_vcharge(void)
{
    /* Chemistry and #cells affect this calc */
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                },
    };
    uint16_t vcharge;

    /* Test lithium ion chemistry */
    /* vcharge = (VCHARGE_SETTING/80.0 + 3.8125)V/cell */
    charger.cfg.chem = LTC4015_CHEM_LI_ION;
    charger.cfg.cellcount = 6;
    LTC4015_regs[0x1B] = 31;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_vcharge(&charger, &vcharge));
    TEST_ASSERT_EQUAL(25200, vcharge);

    charger.cfg.cellcount = 3;
    LTC4015_regs[0x1B] = 23;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_vcharge(&charger, &vcharge));
    TEST_ASSERT_EQUAL(12300, vcharge);

    /* Test LiFePO4 chemistry */
    /* vcharge = (VCHARGE_SETTING/80.0 + 3.4125)V/cell */
    charger.cfg.chem = LTC4015_CHEM_LI_FE_PO4;
    charger.cfg.cellcount = 6;
    LTC4015_regs[0x1B] = 60;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_vcharge(&charger, &vcharge));
    TEST_ASSERT_EQUAL(24975, vcharge);

    charger.cfg.cellcount = 2;
    LTC4015_regs[0x1B] = 31;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_vcharge(&charger, &vcharge));
    TEST_ASSERT_EQUAL(7600, vcharge);

    /* Test lead acid chemistry */
    /* vcharge = (VCHARGE_SETTING/105.0 + 2.0)V/cell w/o temp comp.*/
    charger.cfg.chem = LTC4015_CHEM_LEAD_ACID;
    charger.cfg.cellcount = 6;
    LTC4015_regs[0x1B] = 11;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_vcharge(&charger, &vcharge));
    TEST_ASSERT_EQUAL(12629, vcharge);

    charger.cfg.cellcount = 4;
    LTC4015_regs[0x1B] = 113;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_vcharge(&charger, &vcharge));
    TEST_ASSERT_EQUAL(12305, vcharge);
}

void test_LTC4015_cfg_battery_voltage_low(void)
{
    /* Chemistry and #cells affect this calc */
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                },
    };

    /* Test lithium ion chemistry */
    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 192.264(uV) */
    charger.cfg.chem = LTC4015_CHEM_LI_ION;
    charger.cfg.cellcount = 3;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_battery_voltage_low(&charger, 9000));
    TEST_ASSERT_EQUAL(15603, LTC4015_regs[0x01]);

    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_battery_voltage_low(&charger, 18000));
    TEST_ASSERT_EQUAL(31207, LTC4015_regs[0x01]);

    /* Make sure too-low values don't do anything bad */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_battery_voltage_low(&charger, 100));
    TEST_ASSERT_EQUAL(173, LTC4015_regs[0x01]);

    /* Test lead acid chemistry */
    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 128.176(uV) */
    charger.cfg.chem = LTC4015_CHEM_LEAD_ACID;
    charger.cfg.cellcount = 6;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_battery_voltage_low(&charger, 10350));
    TEST_ASSERT_EQUAL(13458, LTC4015_regs[0x01]);

    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_battery_voltage_low(&charger, 9500));
    TEST_ASSERT_EQUAL(12352, LTC4015_regs[0x01]);

    /* Make sure too-low values don't do anything bad */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_battery_voltage_low(&charger, 100));
    TEST_ASSERT_EQUAL(130, LTC4015_regs[0x01]);
}

void test_LTC4015_get_cfg_battery_voltage_low(void)
{
    /* Chemistry and #cells affect this calc */
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                },
    };
    int16_t underVoltage;

    /* Test lithium ion chemistry */
    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 192.264(uV) */
    charger.cfg.chem = LTC4015_CHEM_LI_ION;
    charger.cfg.cellcount = 3;
    LTC4015_regs[0x01] = 15603;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_battery_voltage_low(
                                         &charger, &underVoltage));
    TEST_ASSERT_EQUAL(8999, underVoltage);

    LTC4015_regs[0x01] = 31208;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_battery_voltage_low(
                                         &charger, &underVoltage));
    TEST_ASSERT_EQUAL(18000, underVoltage);

    /* Test lead acid chemistry */
    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 128.176(uV) */
    charger.cfg.chem = LTC4015_CHEM_LEAD_ACID;
    charger.cfg.cellcount = 6;
    LTC4015_regs[0x01] = 13458;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_battery_voltage_low(
                                         &charger, &underVoltage));
    TEST_ASSERT_EQUAL(10349, underVoltage);

    LTC4015_regs[0x01] = 12353;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_battery_voltage_low(
                                         &charger, &underVoltage));
    TEST_ASSERT_EQUAL(9500, underVoltage);
}

void test_LTC4015_cfg_battery_voltage_high(void)
{
    /* Chemistry and #cells affect this calc */
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                },
    };

    /* Test lithium ion chemistry */
    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 192.264(uV) */
    charger.cfg.chem = LTC4015_CHEM_LI_ION;
    charger.cfg.cellcount = 3;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_battery_voltage_high(&charger, 12600));
    TEST_ASSERT_EQUAL(21844, LTC4015_regs[0x02]);

    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_battery_voltage_high(&charger, 19000));
    TEST_ASSERT_EQUAL(32940, LTC4015_regs[0x02]);

    /* Make sure too-low values don't do anything bad */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_battery_voltage_high(&charger, 50));
    TEST_ASSERT_EQUAL(86, LTC4015_regs[0x02]);

    /* Test lead acid chemistry */
    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 128.176(uV) */
    charger.cfg.chem = LTC4015_CHEM_LEAD_ACID;
    charger.cfg.cellcount = 6;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_battery_voltage_high(&charger, 13800));
    TEST_ASSERT_EQUAL(17944, LTC4015_regs[0x02]);

    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_battery_voltage_high(&charger, 8000));
    TEST_ASSERT_EQUAL(10402, LTC4015_regs[0x02]);

    /* Make sure too-low values don't do anything bad */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_battery_voltage_high(&charger, 300));
    TEST_ASSERT_EQUAL(390, LTC4015_regs[0x02]);
}

void test_LTC4015_get_cfg_battery_voltage_high(void)
{
    /* Chemistry and #cells affect this calc */
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                },
    };
    int16_t overVoltage;

    /* Test lithium ion chemistry */
    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 192.264(uV) */
    charger.cfg.chem = LTC4015_CHEM_LI_ION;
    charger.cfg.cellcount = 3;
    LTC4015_regs[0x02] = 21845;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_battery_voltage_high(
                                         &charger, &overVoltage));
    TEST_ASSERT_EQUAL(12600, overVoltage);

    LTC4015_regs[0x02] = 15603;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_battery_voltage_high(
                                         &charger, &overVoltage));
    TEST_ASSERT_EQUAL(8999, overVoltage);

    /* Test lead acid chemistry */
    /* under voltage limit = [VBAT_LO_ALERT_LIMIT] • 128.176(uV) */
    charger.cfg.chem = LTC4015_CHEM_LEAD_ACID;
    charger.cfg.cellcount = 6;
    LTC4015_regs[0x02] = 17945;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_battery_voltage_high(
                                         &charger, &overVoltage));
    TEST_ASSERT_EQUAL(13800, overVoltage);

    LTC4015_regs[0x02] = 390;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_battery_voltage_high(
                                         &charger, &overVoltage));
    TEST_ASSERT_EQUAL(299, overVoltage);
}

void test_LTC4015_cfg_input_voltage_low(void)
{
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                },
    };

    /* VIN_LO_ALERT_LIMIT = limit/1.648mV */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_input_voltage_low(&charger, 16200));
    TEST_ASSERT_EQUAL(9830, LTC4015_regs[0x03]);

    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_input_voltage_low(&charger, 5000));
    TEST_ASSERT_EQUAL(3033, LTC4015_regs[0x03]);

    /* Make sure too-low values don't do anything bad */
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_input_voltage_low(&charger, 100));
    TEST_ASSERT_EQUAL(60, LTC4015_regs[0x03]);
}

void test_LTC4015_get_cfg_input_voltage_low(void)
{
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                },
    };
    int16_t inputVoltage;

    /* VIN_LO_ALERT_LIMIT = limit/1.648mV */
    LTC4015_regs[0x03] = 3034;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_input_voltage_low(
                                         &charger, &inputVoltage));
    TEST_ASSERT_EQUAL(5000, inputVoltage);

    LTC4015_regs[0x03] = 60;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_input_voltage_low(
                                         &charger, &inputVoltage));
    TEST_ASSERT_EQUAL(98, inputVoltage);
}

void test_LTC4015_cfg_input_current_high(void)
{
    /* Rsnsi value affects this calculation */
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                        .r_snsi = 7,
                },
    };

    /* IIN_HI_ALERT_LIMIT = (limit*RSNSI)/1.46487uV */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_input_current_high(&charger, 5000));
    TEST_ASSERT_EQUAL(23892, LTC4015_regs[0x07]);

    charger.cfg.r_snsi = 2;

    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_input_current_high(&charger, 17000));
    TEST_ASSERT_EQUAL(23210, LTC4015_regs[0x07]);

    /* Make sure too-low values don't do anything bad */
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_input_current_high(&charger, 100));
    TEST_ASSERT_EQUAL(136, LTC4015_regs[0x07]);
}

void test_LTC4015_get_cfg_input_current_high(void)
{
    /* Rsnsi value affects this calculation */
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                        .r_snsi = 7,
                },
    };
    int16_t inputCurrent;

    /* IIN_HI_ALERT_LIMIT = (limit*RSNSI)/1.46487uV */
    LTC4015_regs[0x07] = 23892;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_input_current_high(
                                         &charger, &inputCurrent));
    TEST_ASSERT_EQUAL(4999, inputCurrent);

    charger.cfg.r_snsi = 2;

    LTC4015_regs[0x07] = 23211;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_input_current_high(
                                         &charger, &inputCurrent));
    TEST_ASSERT_EQUAL(17000, inputCurrent);
}

void test_LTC4015_cfg_battery_current_low(void)
{
    /* Rsnsb value affects this calculation */
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                        .r_snsb = 30,
                },
    };

    /* IBAT_LO_ALERT_LIMIT = (limit*RSNSB)/1.46487uV */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_battery_current_low(&charger, 100));
    TEST_ASSERT_EQUAL(2047, LTC4015_regs[0x08]);

    charger.cfg.r_snsb = 3;

    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_battery_current_low(&charger, 2000));
    TEST_ASSERT_EQUAL(4095, LTC4015_regs[0x08]);

    /* Make sure too-low values don't do anything bad */
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_battery_current_low(&charger, 10));
    TEST_ASSERT_EQUAL(20, LTC4015_regs[0x08]);
}

void test_LTC4015_get_cfg_battery_current_low(void)
{
    /* Rsnsb value affects this calculation */
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                        .r_snsb = 30,
                },
    };
    int16_t batteryCurrent;

    /* IBAT_LO_ALERT_LIMIT = (limit*RSNSB)/1.46487uV */
    LTC4015_regs[0x08] = 2048;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_battery_current_low(
                                         &charger, &batteryCurrent));
    TEST_ASSERT_EQUAL(100, batteryCurrent);

    charger.cfg.r_snsb = 4;

    LTC4015_regs[0x08] = 8276;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_battery_current_low(
                                         &charger, &batteryCurrent));
    TEST_ASSERT_EQUAL(3030, batteryCurrent);
}

void test_LTC4015_cfg_die_temperature_high(void)
{
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                },
    };

    /* DIE_TEMP_HI_ALERT_LIMIT = (DIE_TEMP – 12010)/45.6°C */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_die_temperature_high(&charger, 35));
    TEST_ASSERT_EQUAL(13606, LTC4015_regs[0x09]);

    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_die_temperature_high(&charger, -10));
    TEST_ASSERT_EQUAL(11554, LTC4015_regs[0x09]);

    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_die_temperature_high(&charger, 80));
    TEST_ASSERT_EQUAL(15658, LTC4015_regs[0x09]);

    /* Make sure too-low values don't do anything bad */
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_cfg_die_temperature_high(&charger, 0));
    TEST_ASSERT_EQUAL(12010, LTC4015_regs[0x09]);
}

void test_LTC4015_get_die_temperature_high(void)
{
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                },
    };
    int16_t dieTemperature;

    /* DIE_TEMP_HI_ALERT_LIMIT = (DIE_TEMP – 12010)/45.6°C */
    LTC4015_regs[0x09] = 15658;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_die_temperature_high(
                                         &charger, &dieTemperature));
    TEST_ASSERT_EQUAL(80, dieTemperature);

    LTC4015_regs[0x09] = 11554;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_die_temperature_high(
                                         &charger, &dieTemperature));
    TEST_ASSERT_EQUAL(-10, dieTemperature);
}

void test_LTC4015_cfg_input_current_limit(void)
{
    /* Rsnsi value affects this calculation */
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                        .r_snsi = 7,
                },
    };

    /* IIN_LIMIT_SETTING = (limit * RSNSI / 500uV) - 1 */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_input_current_limit(&charger, 5570));
    TEST_ASSERT_EQUAL(76, LTC4015_regs[0x15]);

    charger.cfg.r_snsi = 2;

    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_input_current_limit(&charger, 16500));
    TEST_ASSERT_EQUAL(65, LTC4015_regs[0x15]);

    /* Make sure too-low values don't do anything bad */
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_cfg_input_current_limit(&charger, 400));
    TEST_ASSERT_EQUAL(0, LTC4015_regs[0x15]);
}

void test_LTC4015_get_cfg_input_current_limit(void)
{
    /* Rsnsi value affects this calculation */
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                        .r_snsi = 7,
                },
    };
    uint16_t inputCurrent;

    /* IIN_LIMIT_SETTING = (limit * RSNSI / 500uV) - 1 */
    LTC4015_regs[0x15] = 76;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_input_current_limit(
                                         &charger, &inputCurrent));
    TEST_ASSERT_EQUAL(5500, inputCurrent);

    charger.cfg.r_snsi = 2;

    LTC4015_regs[0x15] = 200;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_cfg_input_current_limit(
                                         &charger, &inputCurrent));
    TEST_ASSERT_EQUAL(50250, inputCurrent);
}

void test_LTC4015_get_die_temp(void)
{
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                },
    };
    int16_t dieTemperature;

    /* Temperature = (DIE_TEMP – 12010)/45.6°C */
    LTC4015_regs[0x3F] = 13606;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_get_die_temperature(&charger, &dieTemperature));
    TEST_ASSERT_EQUAL(35, dieTemperature);

    LTC4015_regs[0x3F] = 15666;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_get_die_temperature(&charger, &dieTemperature));
    TEST_ASSERT_EQUAL(80, dieTemperature);
}

void test_LTC4015_get_battery_current(void)
{
    /* Rsnsb value affects this calculation */
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                        .r_snsb = 30,
                },
    };
    int16_t batteryCurrent;

    /* Battery current = [IBAT] * 1.46487uV/Rsnsb */
    LTC4015_regs[0x3D] = 2048;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_get_battery_current(&charger, &batteryCurrent));
    TEST_ASSERT_EQUAL(100, batteryCurrent);

    charger.cfg.r_snsb = 4;

    LTC4015_regs[0x3D] = 8276;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_get_battery_current(&charger, &batteryCurrent));
    TEST_ASSERT_EQUAL(3030, batteryCurrent);
}

void test_LTC4015_get_input_current(void)
{
    /* Rsnsi value affects this calculation */
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                        .r_snsi = 7,
                },
    };
    int16_t inputCurrent;

    /* Input current = [IIN] • 1.46487uV/Rsnsi */
    LTC4015_regs[0x3E] = 23892;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_get_input_current(&charger, &inputCurrent));
    TEST_ASSERT_EQUAL(4999, inputCurrent);

    charger.cfg.r_snsi = 2;

    LTC4015_regs[0x3E] = 23211;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_get_input_current(&charger, &inputCurrent));
    TEST_ASSERT_EQUAL(17000, inputCurrent);
}

void test_LTC4015_get_battery_voltage(void)
{
    /* Chemistry and #cells affect this calc */
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                },
    };
    int16_t batteryVoltage;

    /* Test lithium ion chemistry */
    /* Battery Voltage(VBATSENS/cellcount) = [VBAT] • 192.264(uV) */
    charger.cfg.chem = LTC4015_CHEM_LI_ION;
    charger.cfg.cellcount = 3;
    LTC4015_regs[0x3A] = 15603;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_get_battery_voltage(&charger, &batteryVoltage));
    TEST_ASSERT_EQUAL(8999, batteryVoltage);

    /* Test lithium ion chemistry */
    /* Battery Voltage(VBATSENS/cellcount) = [VBAT] • 128.176(uV) */
    charger.cfg.chem = LTC4015_CHEM_LEAD_ACID;
    charger.cfg.cellcount = 6;
    LTC4015_regs[0x3A] = 17945;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_get_battery_voltage(&charger, &batteryVoltage));
    TEST_ASSERT_EQUAL(13800, batteryVoltage);
}

void test_LTC4015_get_input_voltage(void)
{
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                },
    };
    int16_t inputVoltage;

    /* Input voltage = [VIN] • 1.648mV */
    LTC4015_regs[0x3B] = 3034;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_get_input_voltage(&charger, &inputVoltage));
    TEST_ASSERT_EQUAL(5000, inputVoltage);

    LTC4015_regs[0x3B] = 60;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_get_input_voltage(&charger, &inputVoltage));
    TEST_ASSERT_EQUAL(98, inputVoltage);
}

void test_LTC4015_get_system_voltage(void)
{
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                },
    };
    int16_t systemVoltage;

    /* System voltage = [VSYS] • 1.648mV */
    LTC4015_regs[0x3C] = 7000;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_get_system_voltage(&charger, &systemVoltage));
    TEST_ASSERT_EQUAL(11536, systemVoltage);

    LTC4015_regs[0x3C] = 12500;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      LTC4015_get_system_voltage(&charger, &systemVoltage));
    TEST_ASSERT_EQUAL(20600, systemVoltage);
}

void test_LTC4015_get_icharge_dac(void)
{
    /* Rsnsb value affects this calculation */
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                        .r_snsb = 30,
                },
    };
    int16_t iCharge;

    /* Charger Current servo level = (ICHARGE_DAC + 1) • 1mV/RSNSB */
    LTC4015_regs[0x44] = 0x1F;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_icharge_dac(&charger, &iCharge));
    TEST_ASSERT_EQUAL(1, iCharge);

    charger.cfg.r_snsb = 3,

    LTC4015_regs[0x44] = 0x08;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_icharge_dac(&charger, &iCharge));
    TEST_ASSERT_EQUAL(3, iCharge);
}

void test_LTC4015_get_bat_presence(void)
{
    LTC4015_Dev charger = {
        .cfg =
                {
                        .i2c_dev = I2C_DEV,
                },
    };
    bool present;

    /* If battery is not connected */
    LTC4015_regs[0x37] = 0x0002;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_bat_presence(&charger, &present));
    TEST_ASSERT_FALSE(present);

    /* If battery is connected */
    LTC4015_regs[0x37] = 0x0000;
    TEST_ASSERT_EQUAL(RETURN_OK, LTC4015_get_bat_presence(&charger, &present));
    TEST_ASSERT_TRUE(present);
}
