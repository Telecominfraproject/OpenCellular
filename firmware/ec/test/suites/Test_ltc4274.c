/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "include/test_ltc4274.h"

/* ======================== Constants & variables =========================== */
extern bool LTC4274_GpioPins[OC_EC_GBC_PSE_ALERT];
extern LTC4274_Dev gbc_pwr_pse;
extern LTC4274_Dev l_invalid_bus;
extern LTC4274_Dev l_invalid_dev;
extern OcGpio_Port ec_io;
extern uint8_t LTC4274_regs[LTC4274_REG_IHP_STATUS];
extern uint32_t LTC7274_GpioConfig[OC_EC_GBC_PSE_ALERT];
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
    fake_I2C_registerDevSimple(
        gbc_pwr_pse.cfg.i2c_dev.bus, gbc_pwr_pse.cfg.i2c_dev.slave_addr,
        &LTC4274_regs, sizeof(LTC4274_regs), sizeof(LTC4274_regs[0]),
        sizeof(uint8_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
    FakeGpio_registerDevSimple(LTC4274_GpioPins, LTC7274_GpioConfig);
}

void setUp(void)
{
    memset(LTC4274_regs, 0, sizeof(LTC4274_regs));
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}
#if 0
void LTC4274_init(LTC4274_Dev *gbc_pwr_pse) 
{
    int8_t ret = 0;
}
#endif
/* ================================ Tests =================================== */
void test_ltc4274_init(void)
{
    /* Make sure that if we're in a weird state, we reset the best we can */
    /* TODO: Do the reset here */
    LTC7274_GpioConfig[OC_EC_GBC_PSE_ALERT] = OCGPIO_CFG_OUTPUT;
    ltc4274_init(&gbc_pwr_pse);
    TEST_ASSERT_EQUAL(OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_FALLING,
                      LTC7274_GpioConfig[OC_EC_GBC_PSE_ALERT]);

    LTC7274_GpioConfig[OC_EC_GBC_PSE_ALERT] = OCGPIO_CFG_OUTPUT;
}

void test_ltc4274_read(void)
{
    uint8_t read = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_INTERRUPT_MASK] = LTC4274_READ_WRITE_VAL;

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_read(&gbc_pwr_pse.cfg.i2c_dev,
                                   LTC4274_REG_INTERRUPT_MASK, &read));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_READ_WRITE_VAL, read);

    read = LTC4274_DEFAULT_VALUE;
    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_read(&l_invalid_dev.cfg.i2c_dev,
                                   LTC4274_REG_INTERRUPT_MASK, &read));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_read(&l_invalid_bus.cfg.i2c_dev,
                                   LTC4274_REG_INTERRUPT_MASK, &read));
}

void test_ltc4274_write(void)
{
    uint8_t write = LTC4274_READ_WRITE_VAL;
    LTC4274_regs[LTC4274_REG_POWER_EVENT] = LTC4274_DEFAULT_VALUE;

    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_write(&gbc_pwr_pse.cfg.i2c_dev,
                                               LTC4274_REG_POWER_EVENT, write));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_READ_WRITE_VAL,
                           LTC4274_regs[LTC4274_REG_POWER_EVENT]);

    write = LTC4274_READ_WRITE_VAL;
    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_write(&l_invalid_dev.cfg.i2c_dev,
                                    LTC4274_REG_POWER_EVENT, write));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_write(&l_invalid_bus.cfg.i2c_dev,
                                    LTC4274_REG_POWER_EVENT, write));
}

void test_ltc4274_operation_mode(void)
{
    uint8_t operation_mode = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_OPERATION_MODE] = LTC4274_AUTO_MODE;

    TEST_ASSERT_EQUAL(
        RETURN_OK,
        ltc4274_get_operation_mode(&gbc_pwr_pse.cfg.i2c_dev, &operation_mode));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_AUTO_MODE, operation_mode);

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_set_cfg_operation_mode(&gbc_pwr_pse.cfg.i2c_dev,
                                                     LTC4274_SHUTDOWN_MODE));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_SHUTDOWN_MODE,
                           LTC4274_regs[LTC4274_REG_OPERATION_MODE]);

    TEST_ASSERT_EQUAL(
        RETURN_OK,
        ltc4274_get_operation_mode(&gbc_pwr_pse.cfg.i2c_dev, &operation_mode));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_SHUTDOWN_MODE, operation_mode);

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_set_cfg_operation_mode(&gbc_pwr_pse.cfg.i2c_dev,
                                                     LTC4274_MANUAL_MODE));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_MANUAL_MODE,
                           LTC4274_regs[LTC4274_REG_OPERATION_MODE]);

    TEST_ASSERT_EQUAL(
        RETURN_OK,
        ltc4274_get_operation_mode(&gbc_pwr_pse.cfg.i2c_dev, &operation_mode));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_MANUAL_MODE, operation_mode);

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_set_cfg_operation_mode(&gbc_pwr_pse.cfg.i2c_dev,
                                                     LTC4274_SEMIAUTO_MODE));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_SEMIAUTO_MODE,
                           LTC4274_regs[LTC4274_REG_OPERATION_MODE]);

    TEST_ASSERT_EQUAL(
        RETURN_OK,
        ltc4274_get_operation_mode(&gbc_pwr_pse.cfg.i2c_dev, &operation_mode));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_SEMIAUTO_MODE, operation_mode);

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_set_cfg_operation_mode(&gbc_pwr_pse.cfg.i2c_dev,
                                                     LTC4274_AUTO_MODE));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_AUTO_MODE,
                           LTC4274_regs[LTC4274_REG_OPERATION_MODE]);

    /* Nagtive case */
    /* To do: code need to modify to accept only 2 last bits value */
    operation_mode = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_OPERATION_MODE] = LTC4274_INVALID_MODE;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_set_cfg_operation_mode(&gbc_pwr_pse.cfg.i2c_dev,
                                                     LTC4274_INVALID_MODE));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_INVALID_MODE,
                           LTC4274_regs[LTC4274_REG_OPERATION_MODE]);

    TEST_ASSERT_EQUAL(
        RETURN_OK,
        ltc4274_get_operation_mode(&gbc_pwr_pse.cfg.i2c_dev, &operation_mode));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_INVALID_MODE, operation_mode);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_set_cfg_operation_mode(&l_invalid_dev.cfg.i2c_dev,
                                                     LTC4274_SEMIAUTO_MODE));

    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_get_operation_mode(&l_invalid_dev.cfg.i2c_dev,
                                                 &operation_mode));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_set_cfg_operation_mode(&l_invalid_bus.cfg.i2c_dev,
                                                     LTC4274_SEMIAUTO_MODE));

    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_get_operation_mode(&l_invalid_bus.cfg.i2c_dev,
                                                 &operation_mode));
}

void test_ltc4274_detect_enable(void)
{
    uint8_t detectenable = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_DETECT_CLASS_ENABLE] = LTC4274_DETCET_CLASS_ENABLE;

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_set_cfg_detect_enable(&gbc_pwr_pse.cfg.i2c_dev,
                                                    LTC4274_DETECT_ENABLE));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_DETECT_ENABLE,
                           LTC4274_regs[LTC4274_REG_DETECT_CLASS_ENABLE]);

    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_detect_enable(
                                     &gbc_pwr_pse.cfg.i2c_dev, &detectenable));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_DETECT_ENABLE & 0x07, detectenable);

    TEST_ASSERT_EQUAL(
        RETURN_OK, ltc4274_set_cfg_detect_enable(&gbc_pwr_pse.cfg.i2c_dev,
                                                 LTC4274_DETCET_CLASS_ENABLE));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_DETCET_CLASS_ENABLE,
                           LTC4274_regs[LTC4274_REG_DETECT_CLASS_ENABLE]);

    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_detect_enable(
                                     &gbc_pwr_pse.cfg.i2c_dev, &detectenable));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_DETCET_CLASS_ENABLE & 0x07, detectenable);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4274_set_cfg_detect_enable(
                                        &l_invalid_dev.cfg.i2c_dev,
                                        LTC4274_DETCET_CLASS_ENABLE));
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_detect_enable(&l_invalid_dev.cfg.i2c_dev, &detectenable));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4274_set_cfg_detect_enable(
                                        &l_invalid_bus.cfg.i2c_dev,
                                        LTC4274_DETCET_CLASS_ENABLE));
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_detect_enable(&l_invalid_bus.cfg.i2c_dev, &detectenable));
}

void test_ltc4274_interrupt_mask(void)
{
    uint8_t interrupt_mask = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_INTERRUPT_MASK] = LTC4274_SET_INTERRUPT_MASK;

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_set_interrupt_mask(&gbc_pwr_pse.cfg.i2c_dev,
                                                 LTC4274_INTERRUPT_MASK));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_INTERRUPT_MASK,
                           LTC4274_regs[LTC4274_REG_INTERRUPT_MASK]);

    TEST_ASSERT_EQUAL(
        RETURN_OK,
        ltc4274_get_interrupt_mask(&gbc_pwr_pse.cfg.i2c_dev, &interrupt_mask));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_INTERRUPT_MASK, interrupt_mask);

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_set_interrupt_mask(&gbc_pwr_pse.cfg.i2c_dev,
                                                 LTC4274_SET_INTERRUPT_MASK));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_SET_INTERRUPT_MASK,
                           LTC4274_regs[LTC4274_REG_INTERRUPT_MASK]);

    TEST_ASSERT_EQUAL(
        RETURN_OK,
        ltc4274_get_interrupt_mask(&gbc_pwr_pse.cfg.i2c_dev, &interrupt_mask));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_SET_INTERRUPT_MASK, interrupt_mask);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_set_interrupt_mask(&l_invalid_dev.cfg.i2c_dev,
                                                 LTC4274_SET_INTERRUPT_MASK));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_get_interrupt_mask(&l_invalid_dev.cfg.i2c_dev,
                                                 &interrupt_mask));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_set_interrupt_mask(&l_invalid_bus.cfg.i2c_dev,
                                                 LTC4274_SET_INTERRUPT_MASK));
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_get_interrupt_mask(&l_invalid_bus.cfg.i2c_dev,
                                                 &interrupt_mask));
}

void test_ltc4274_interrupt_enable(void)
{
    bool interruptEnable = true;

    LTC4274_regs[LTC4274_REG_MCONF] = LTC4274_DEFAULT_VALUE;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_cfg_interrupt_enable(&gbc_pwr_pse.cfg.i2c_dev,
                                                   interruptEnable));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_INTERRUPT_ENABLE,
                           LTC4274_regs[LTC4274_REG_MCONF]);

    interruptEnable = false;
    LTC4274_regs[LTC4274_REG_MCONF] = LTC4274_INTERRUPT_ENABLE;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_cfg_interrupt_enable(&gbc_pwr_pse.cfg.i2c_dev,
                                                   interruptEnable));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_INTERRUPT_ENABLE_FALSE,
                           LTC4274_regs[LTC4274_REG_MCONF]);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_cfg_interrupt_enable(&l_invalid_dev.cfg.i2c_dev,
                                                   interruptEnable));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_cfg_interrupt_enable(&l_invalid_bus.cfg.i2c_dev,
                                                   interruptEnable));
}

void test_ltc4274_get_interrupt_enable(void)
{
    uint8_t interrupt_enable = LTC4274_DEFAULT_VALUE;

    LTC4274_regs[LTC4274_REG_MCONF] = LTC4274_INTERRUPT_ENABLE;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_get_interrupt_enable(&gbc_pwr_pse.cfg.i2c_dev,
                                                   &interrupt_enable));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_INTERRUPT_ENABLE, interrupt_enable);

    LTC4274_regs[LTC4274_REG_MCONF] = LTC4274_INTERRUPT_ENABLE_0x5D;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_get_interrupt_enable(&gbc_pwr_pse.cfg.i2c_dev,
                                                   &interrupt_enable));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_INTERRUPT_ENABLE_0x5D, interrupt_enable);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_get_interrupt_enable(&l_invalid_dev.cfg.i2c_dev,
                                                   &interrupt_enable));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_get_interrupt_enable(&l_invalid_bus.cfg.i2c_dev,
                                                   &interrupt_enable));
}

void test_ltc4274_pshp_feature(void)
{
    uint8_t pshp_feature = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_HP_ENABLE] = LTC4274_DEFAULT_VALUE;

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_set_cfg_pshp_feature(&gbc_pwr_pse.cfg.i2c_dev,
                                                   LTC4274_HP_ENABLE));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_HP_ENABLE,
                           LTC4274_regs[LTC4274_REG_HP_ENABLE]);

    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_pshp_feature(
                                     &gbc_pwr_pse.cfg.i2c_dev, &pshp_feature));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_HP_ENABLE, pshp_feature);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_set_cfg_pshp_feature(&l_invalid_dev.cfg.i2c_dev,
                                                   LTC4274_HP_ENABLE));
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_pshp_feature(&l_invalid_dev.cfg.i2c_dev, &pshp_feature));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_set_cfg_pshp_feature(&l_invalid_bus.cfg.i2c_dev,
                                                   LTC4274_HP_ENABLE));
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_pshp_feature(&l_invalid_bus.cfg.i2c_dev, &pshp_feature));
}

void test_ltc4274_class_status(void)
{
    uint8_t pseclass = LTC4274_DEFAULT_VALUE;

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_CLASSTYPE_UNKOWN_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_class_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &pseclass));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_CLASSTYPE_UNKOWN, pseclass);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_CLASSTYPE_1_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_class_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &pseclass));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_CLASSTYPE_1, pseclass);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_CLASSTYPE_2_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_class_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &pseclass));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_CLASSTYPE_2, pseclass);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_CLASSTYPE_3_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_class_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &pseclass));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_CLASSTYPE_3, pseclass);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_CLASSTYPE_4_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_class_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &pseclass));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_CLASSTYPE_4, pseclass);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_CLASSTYPE_RESERVED_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_class_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &pseclass));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_CLASSTYPE_RESERVED, pseclass);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_CLASSTYPE_0_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_class_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &pseclass));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_CLASSTYPE_0, pseclass);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_OVERCURRENT_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_class_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &pseclass));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_OVERCURRENT, pseclass);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_LTEPOE_TYPE_52_7W_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_class_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &pseclass));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_LTEPOE_TYPE_52_7W, pseclass);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_LTEPOE_TYPE_70W_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_class_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &pseclass));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_LTEPOE_TYPE_70W, pseclass);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_LTEPOE_TYPE_90W_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_class_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &pseclass));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_LTEPOE_TYPE_90W, pseclass);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_LTEPOE_TYPE_38_7W_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_class_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &pseclass));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_LTEPOE_TYPE_38_7W, pseclass);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_LTEPOE_RESERVED_VAL;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_class_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &pseclass));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_LTEPOE_RESERVED, pseclass);

    /* Invalid dev */
    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_CLASSTYPE_1_VAL;
    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4274_get_class_status(
                                        &l_invalid_dev.cfg.i2c_dev, &pseclass));

    /* Invalid bus */
    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_CLASSTYPE_1_VAL;
    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4274_get_class_status(
                                        &l_invalid_bus.cfg.i2c_dev, &pseclass));
}

void test_ltc4274_detection_status(void)
{
    uint8_t psedetect = LTC4274_DEFAULT_VALUE;

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_DETECT_UNKOWN;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_detection_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &psedetect));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_DETECT_UNKOWN, psedetect);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_SHORT_CIRCUIT;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_detection_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &psedetect));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_SHORT_CIRCUIT, psedetect);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_CPD_HIGH;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_detection_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &psedetect));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_CPD_HIGH, psedetect);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_RSIG_LOW;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_detection_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &psedetect));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_RSIG_LOW, psedetect);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_SIGNATURE_GOOD;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_detection_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &psedetect));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_SIGNATURE_GOOD, psedetect);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_RSIG_TOO_HIGH;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_detection_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &psedetect));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_RSIG_TOO_HIGH, psedetect);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_OPEN_CIRCUIT;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_detection_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &psedetect));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_OPEN_CIRCUIT, psedetect);

    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_DETECT_ERROR;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_detection_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &psedetect));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_DETECT_ERROR, psedetect);

    /* Invalid dev */
    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_DETECT_ERROR;
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_detection_status(&l_invalid_dev.cfg.i2c_dev, &psedetect));

    /* Invalid bus */
    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_DETECT_ERROR;
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_detection_status(&l_invalid_bus.cfg.i2c_dev, &psedetect));
}

void test_ltc4274_power_good(void)
{
    uint8_t psepowergood = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_POWER_STATUS] = LTC4274_POWERGOOD_VALUE;

    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_powergood_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &psepowergood));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_POWERGOOD, psepowergood);

    LTC4274_regs[LTC4274_REG_POWER_STATUS] = LTC4274_POWERGOOD_NOTOK_VALUE;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_powergood_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &psepowergood));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_POWERGOOD_NOTOK, psepowergood);
}

void test_ltc4246_clear_interrupt(void)
{
    uint8_t pwrEvent = LTC4274_DEFAULT_VALUE;
    uint8_t overCurrent = LTC4274_DEFAULT_VALUE;
    uint8_t supply = LTC4274_DEFAULT_VALUE;

    LTC4274_regs[LTC4274_REG_POWER_EVENT_COR] = LTC4274_POWER_CLEAR_EVENT;
    LTC4274_regs[LTC4274_REG_START_EVENT_COR] = LTC4274_OVERCURRENT_CLEAR_EVENT;
    LTC4274_regs[LTC4274_REG_SUPPLY_EVENT_COR] = LTC4274_SUPPLY_CLEAR_EVENT;

    TEST_ASSERT_EQUAL(
        RETURN_OK, ltc4274_clear_interrupt(&gbc_pwr_pse.cfg.i2c_dev, &pwrEvent,
                                           &overCurrent, &supply));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_POWER_CLEAR_EVENT, pwrEvent);
    TEST_ASSERT_EQUAL_HEX8(LTC4274_OVERCURRENT_CLEAR_EVENT, overCurrent);
    TEST_ASSERT_EQUAL_HEX8(LTC4274_SUPPLY_CLEAR_EVENT, supply);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4274_clear_interrupt(
                                        &l_invalid_dev.cfg.i2c_dev, &pwrEvent,
                                        &overCurrent, &supply));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4274_clear_interrupt(
                                        &l_invalid_bus.cfg.i2c_dev, &pwrEvent,
                                        &overCurrent, &supply));
}

void test_ltc4274_interrupt_status(void)
{
    uint8_t interruptVal = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_INTERRUPT_STATUS] = LTC4274_INTERRUPT_STATUS_VAL;

    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_get_interrupt_status(
                                     &gbc_pwr_pse.cfg.i2c_dev, &interruptVal));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_INTERRUPT_STATUS_VAL, interruptVal);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_get_interrupt_status(&l_invalid_dev.cfg.i2c_dev,
                                                   &interruptVal));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_get_interrupt_status(&l_invalid_bus.cfg.i2c_dev,
                                                   &interruptVal));
}

void test_ltc4274_enable(void)
{
    LTC4274_GpioPins[OC_EC_PWR_PSE_RESET] = true;
    ltc4274_enable(&gbc_pwr_pse, true);
    TEST_ASSERT_EQUAL(false, LTC4274_GpioPins[OC_EC_PWR_PSE_RESET]);

    LTC4274_GpioPins[OC_EC_PWR_PSE_RESET] = false;
    ltc4274_enable(&gbc_pwr_pse, false);
    TEST_ASSERT_EQUAL(true, LTC4274_GpioPins[OC_EC_PWR_PSE_RESET]);
}

void test_ltc4274_devid(void)
{
    uint8_t devid = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_ID] = LTC4274_DEV_ID;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_get_devid(&gbc_pwr_pse.cfg.i2c_dev, &devid));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_DEVID(LTC4274_DEV_ID), devid);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_get_devid(&l_invalid_dev.cfg.i2c_dev, &devid));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_get_devid(&l_invalid_bus.cfg.i2c_dev, &devid));
}

void test_ltc4274_detect(void)
{
    uint8_t detect = LTC4274_DEFAULT_VALUE;
    uint8_t val = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_DETECT_EVENT] = LTC4274_DETECT_EVENT_VALUE;
    LTC4274_regs[LTC4274_REG_STATUS] = LTC4274_CLASSIFICATION_VALUE;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_detect(&gbc_pwr_pse.cfg.i2c_dev, &detect, &val));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_DETECT_EVENT_VALUE, detect);
    TEST_ASSERT_EQUAL_HEX8(LTC4274_CLASSIFICATION_VALUE, val);

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4274_detect(&l_invalid_dev.cfg.i2c_dev,
                                                   &detect, &val));

    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4274_detect(&l_invalid_bus.cfg.i2c_dev,
                                                   &detect, &val));
}

void test_ltc4274_probe(void)
{
    POSTData postData;

    /* Correct Dev id */
    LTC4274_regs[LTC4274_REG_ID] = (LTC4274_POST_DEVID << 3);
    TEST_ASSERT_EQUAL(POST_DEV_FOUND, ltc4274_probe(&gbc_pwr_pse, &postData));
    TEST_ASSERT_EQUAL(gbc_pwr_pse.cfg.i2c_dev.bus, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(gbc_pwr_pse.cfg.i2c_dev.slave_addr,
                           postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(LTC4274_POST_MANID, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(LTC4274_POST_DEVID, postData.devId);

    postData.i2cBus = POST_DATA_NULL;
    postData.devAddr = POST_DATA_NULL;
    postData.manId = POST_DATA_NULL;
    postData.devId = POST_DATA_NULL;
    /* Incorrect Dev id */
    LTC4274_regs[LTC4274_REG_ID] = (LTC4274_POST_INCORRECT_DEVID << 3);
    TEST_ASSERT_EQUAL(POST_DEV_ID_MISMATCH,
                      ltc4274_probe(&gbc_pwr_pse, &postData));
    TEST_ASSERT_EQUAL(POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devId);

    /*  Invalid device */
    LTC4274_regs[LTC4274_REG_ID] = (LTC4274_POST_DEVID << 3);
    TEST_ASSERT_EQUAL(POST_DEV_MISSING,
                      ltc4274_probe(&l_invalid_dev, &postData));
    TEST_ASSERT_EQUAL(POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devId);

    /*  Invalid bus */
    LTC4274_regs[LTC4274_REG_ID] = (LTC4274_POST_DEVID << 3);
    TEST_ASSERT_EQUAL(POST_DEV_MISSING,
                      ltc4274_probe(&l_invalid_bus, &postData));
    TEST_ASSERT_EQUAL(POST_DATA_NULL, postData.i2cBus);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devAddr);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.manId);
    TEST_ASSERT_EQUAL_HEX8(POST_DATA_NULL, postData.devId);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static void alert_handler(LTC4274_Event evt, void *context)
{
    /* Do Nothing */
    return;
}
#pragma GCC diagnostic pop

void test_ltc4274_set_alert_handler(void)
{
    ltc4274_set_alert_handler(&gbc_pwr_pse, alert_handler,
                              (void *)LTC4274_ALERT_CB_CONTEXT);
    TEST_ASSERT_EQUAL(LTC4274_ALERT_CB_CONTEXT,
                      (int *)gbc_pwr_pse.obj.cb_context);
    TEST_ASSERT_EQUAL(alert_handler, (int *)gbc_pwr_pse.obj.alert_cb);
}

void test_ltc4274_debug_read(void)
{
    uint8_t read = LTC4274_DEFAULT_VALUE;
    LTC4274_regs[LTC4274_REG_INTERRUPT_MASK] = LTC4274_READ_WRITE_VAL;

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_debug_read(&gbc_pwr_pse.cfg.i2c_dev,
                                         LTC4274_REG_INTERRUPT_MASK, &read));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_READ_WRITE_VAL, read);

    read = LTC4274_DEFAULT_VALUE;
    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_debug_read(&l_invalid_dev.cfg.i2c_dev,
                                         LTC4274_REG_INTERRUPT_MASK, &read));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_debug_read(&l_invalid_bus.cfg.i2c_dev,
                                         LTC4274_REG_INTERRUPT_MASK, &read));
}

void test_ltc4274_debug_write(void)
{
    uint8_t write = LTC4274_READ_WRITE_VAL;
    LTC4274_regs[LTC4274_REG_POWER_EVENT] = LTC4274_DEFAULT_VALUE;

    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_debug_write(&gbc_pwr_pse.cfg.i2c_dev,
                                          LTC4274_REG_POWER_EVENT, write));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_READ_WRITE_VAL,
                           LTC4274_regs[LTC4274_REG_POWER_EVENT]);

    write = LTC4274_READ_WRITE_VAL;
    /* Invalid dev */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_debug_write(&l_invalid_dev.cfg.i2c_dev,
                                          LTC4274_REG_POWER_EVENT, write));

    /* Invalid bus */
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      ltc4274_debug_write(&l_invalid_bus.cfg.i2c_dev,
                                          LTC4274_REG_POWER_EVENT, write));
}

void test_ltc4274_reset(void)
{
    LTC4274_GpioPins[OC_EC_PWR_PSE_RESET] = true;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_reset(&gbc_pwr_pse));
    TEST_ASSERT_EQUAL(false, LTC4274_GpioPins[OC_EC_PWR_PSE_RESET]);
}

void test_ltc4274_default_cfg(void)
{
    uint8_t operatingMode = LTC4274_AUTO_MODE;
    uint8_t detectEnable = LTC4274_DETECT_ENABLE;
    uint8_t intrMask = LTC4274_INTERRUPT_MASK;
    bool interruptEnable = true;
    uint8_t hpEnable = LTC4274_HP_ENABLE;

    LTC4274_regs[LTC4274_REG_OPERATION_MODE] = LTC4274_DEFAULT_VALUE;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_set_cfg_operation_mode(
                                     &gbc_pwr_pse.cfg.i2c_dev, operatingMode));
    TEST_ASSERT_EQUAL_HEX8(operatingMode,
                           LTC4274_regs[LTC4274_REG_OPERATION_MODE]);

    LTC4274_regs[LTC4274_REG_DETECT_CLASS_ENABLE] = LTC4274_DEFAULT_VALUE;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_set_cfg_detect_enable(
                                     &gbc_pwr_pse.cfg.i2c_dev, detectEnable));
    TEST_ASSERT_EQUAL_HEX8(detectEnable,
                           LTC4274_regs[LTC4274_REG_DETECT_CLASS_ENABLE]);

    LTC4274_regs[LTC4274_REG_INTERRUPT_MASK] = LTC4274_DEFAULT_VALUE;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_set_interrupt_mask(
                                     &gbc_pwr_pse.cfg.i2c_dev, intrMask));
    TEST_ASSERT_EQUAL_HEX8(intrMask, LTC4274_regs[LTC4274_REG_INTERRUPT_MASK]);

    LTC4274_regs[LTC4274_REG_MCONF] = LTC4274_DEFAULT_VALUE;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      ltc4274_cfg_interrupt_enable(&gbc_pwr_pse.cfg.i2c_dev,
                                                   interruptEnable));
    TEST_ASSERT_EQUAL_HEX8(LTC4274_INTERRUPT_ENABLE,
                           LTC4274_regs[LTC4274_REG_MCONF]);

    LTC4274_regs[LTC4274_REG_HP_ENABLE] = LTC4274_DEFAULT_VALUE;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4274_set_cfg_pshp_feature(
                                     &gbc_pwr_pse.cfg.i2c_dev, hpEnable));
    TEST_ASSERT_EQUAL_HEX8(hpEnable, LTC4274_regs[LTC4274_REG_HP_ENABLE]);
}

void test_ltc4274_invalid_dev(void)
{
    uint8_t dummy_val = LTC4274_DEFAULT_VALUE;
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_operation_mode(&l_invalid_dev.cfg.i2c_dev, &dummy_val));
    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4274_set_cfg_operation_mode(
                                        &l_invalid_dev.cfg.i2c_dev, dummy_val));

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_detect_enable(&l_invalid_dev.cfg.i2c_dev, &dummy_val));

    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4274_set_cfg_detect_enable(
                                        &l_invalid_dev.cfg.i2c_dev, dummy_val));

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_interrupt_mask(&l_invalid_dev.cfg.i2c_dev, &dummy_val));

    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4274_set_interrupt_mask(
                                        &l_invalid_dev.cfg.i2c_dev, dummy_val));

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_interrupt_enable(&l_invalid_dev.cfg.i2c_dev, &dummy_val));

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_pshp_feature(&l_invalid_dev.cfg.i2c_dev, &dummy_val));

    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4274_set_cfg_pshp_feature(
                                        &l_invalid_dev.cfg.i2c_dev, dummy_val));

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_detection_status(&l_invalid_dev.cfg.i2c_dev, &dummy_val));

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_class_status(&l_invalid_dev.cfg.i2c_dev, &dummy_val));

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_powergood_status(&l_invalid_dev.cfg.i2c_dev, &dummy_val));

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_interrupt_status(&l_invalid_dev.cfg.i2c_dev, &dummy_val));

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_devid(&l_invalid_dev.cfg.i2c_dev, &dummy_val));
}

void test_ltc4274_invalid_bus(void)
{
    uint8_t dummy_val = LTC4274_DEFAULT_VALUE;
    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_operation_mode(&l_invalid_bus.cfg.i2c_dev, &dummy_val));
    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4274_set_cfg_operation_mode(
                                        &l_invalid_bus.cfg.i2c_dev, dummy_val));

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_detect_enable(&l_invalid_bus.cfg.i2c_dev, &dummy_val));

    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4274_set_cfg_detect_enable(
                                        &l_invalid_bus.cfg.i2c_dev, dummy_val));

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_interrupt_mask(&l_invalid_bus.cfg.i2c_dev, &dummy_val));

    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4274_set_interrupt_mask(
                                        &l_invalid_bus.cfg.i2c_dev, dummy_val));

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_interrupt_enable(&l_invalid_bus.cfg.i2c_dev, &dummy_val));

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_pshp_feature(&l_invalid_bus.cfg.i2c_dev, &dummy_val));

    TEST_ASSERT_EQUAL(RETURN_NOTOK, ltc4274_set_cfg_pshp_feature(
                                        &l_invalid_bus.cfg.i2c_dev, dummy_val));

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_detection_status(&l_invalid_bus.cfg.i2c_dev, &dummy_val));

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_class_status(&l_invalid_bus.cfg.i2c_dev, &dummy_val));

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_powergood_status(&l_invalid_bus.cfg.i2c_dev, &dummy_val));

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_interrupt_status(&l_invalid_bus.cfg.i2c_dev, &dummy_val));

    TEST_ASSERT_EQUAL(
        RETURN_NOTOK,
        ltc4274_get_devid(&l_invalid_bus.cfg.i2c_dev, &dummy_val));
}

void test_ltc4274_config(void)
{
    LTC4274_GpioPins[OC_EC_PWR_PSE_RESET] = true;
    ltc4274_config(&gbc_pwr_pse);
    TEST_ASSERT_EQUAL(false, LTC4274_GpioPins[OC_EC_PWR_PSE_RESET]);
}
