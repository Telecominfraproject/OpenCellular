/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/ocmp_wrappers/ocmp_eeprom_cat24c04.h"
#include "common/inc/global/Framework.h"
#include "include/test_eeprom.h"

extern bool Eeprom_GpioPins[0x02];
extern const OcGpio_FnTable GpioPCA9557_fnTable;
extern const OcGpio_FnTable GpioSX1509_fnTable;
extern Eeprom_Cfg e_invalid_bus;
extern Eeprom_Cfg *e_invalid_cfg;
extern Eeprom_Cfg e_invalid_dev;
extern Eeprom_Cfg e_invalid_ss_cfg;
extern Eeprom_Cfg enable_dev;
extern Eeprom_Cfg eeprom_fe_inv;
extern Eeprom_Cfg eeprom_gbc_inv;
extern Eeprom_Cfg eeprom_gbc_sid;
extern Eeprom_Cfg eeprom_sdr_inv;
extern OcGpio_Port fe_ch1_lna_io;
extern OcGpio_Port gbc_io_0;
extern uint8_t PCA9557_regs[PCA9557_REGS_DIR_CONFIG];
extern uint8_t SX1509_regs[SX1509_REG_TEST_2];
extern uint16_t EEPROM_regs[EEPROM_REG_END];
extern uint32_t Eeprom_GpioConfig[0x02];

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

    FakeGpio_registerDevSimple(Eeprom_GpioPins, Eeprom_GpioConfig);

    fake_I2C_registerDevSimple(eeprom_gbc_sid.i2c_dev.bus,
                               eeprom_gbc_sid.i2c_dev.slave_addr, EEPROM_regs,
                               sizeof(EEPROM_regs), sizeof(EEPROM_regs[0]),
                               sizeof(uint16_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
    fake_I2C_registerDevSimple(eeprom_gbc_inv.i2c_dev.bus,
                               eeprom_gbc_inv.i2c_dev.slave_addr, EEPROM_regs,
                               sizeof(EEPROM_regs), sizeof(EEPROM_regs[0]),
                               sizeof(uint16_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
    fake_I2C_registerDevSimple(eeprom_sdr_inv.i2c_dev.bus,
                               eeprom_sdr_inv.i2c_dev.slave_addr, EEPROM_regs,
                               sizeof(EEPROM_regs), sizeof(EEPROM_regs[0]),
                               sizeof(uint16_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
    fake_I2C_registerDevSimple(eeprom_fe_inv.i2c_dev.bus,
                               eeprom_fe_inv.i2c_dev.slave_addr, EEPROM_regs,
                               sizeof(EEPROM_regs), sizeof(EEPROM_regs[0]),
                               sizeof(uint16_t), FAKE_I2C_DEV_LITTLE_ENDIAN);
    fake_I2C_registerDevSimple(6, 0x45, SX1509_regs, sizeof(SX1509_regs),
                               sizeof(SX1509_regs[0]), sizeof(uint8_t),
                               FAKE_I2C_DEV_LITTLE_ENDIAN);
    fake_I2C_registerDevSimple(2, 0x18, PCA9557_regs, sizeof(PCA9557_regs),
                               sizeof(PCA9557_regs[0]), sizeof(uint8_t),
                               FAKE_I2C_DEV_LITTLE_ENDIAN);
}

void setUp(void)
{
    memset(EEPROM_regs, 0, sizeof(EEPROM_regs));
    OcGpio_init(&gbc_io_0);
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit(); /* This will automatically unregister devices */
}

/* ================================ Tests =================================== */
void test_init_eeprom(void)
{
    Eeprom_GpioConfig[0x02] = OCGPIO_CFG_OUT_HIGH;
    EEPROM_regs[EEPROM_REG_DEF_INIT] = EEPROM_READ_WRITE_VALUE;
    EEPROM_regs[EEPROM_REG_FFFF] = EEPROM_FFFF_REG_VALUE;
    PCA9557_regs[PCA9557_REGS_OUTPUT_VALUE] = EEPROM_DEFAULT_VALUE_NULL;
    SX1509_regs[SX1509_REG_DATA_B] = EEPROM_DEFAULT_VALUE_NULL;
    SX1509_regs[SX1509_REG_DATA_A] = EEPROM_DEFAULT_VALUE_NULL;

    AlertData alert_data = {
        .subsystem = 0,
        .componentId = 1,
        .deviceId = 0,
    };

    TEST_ASSERT_EQUAL(
        POST_DEV_CFG_DONE,
        CAT24C04_gbc_sid_fxnTable.cb_init(&eeprom_gbc_sid, NULL, &alert_data));
    TEST_ASSERT_EQUAL(EEPROM_DISABLE_WRITE, SX1509_regs[SX1509_REG_DATA_A]);
    TEST_ASSERT_EQUAL(OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH,
                      Eeprom_GpioConfig[0x02]);

    TEST_ASSERT_EQUAL(
        POST_DEV_CFG_DONE,
        CAT24C04_gbc_sid_fxnTable.cb_init(&eeprom_gbc_inv, NULL, &alert_data));
    TEST_ASSERT_EQUAL(EEPROM_DISABLE_WRITE, SX1509_regs[SX1509_REG_DATA_A]);
    TEST_ASSERT_EQUAL(OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH,
                      Eeprom_GpioConfig[0x02]);

    TEST_ASSERT_EQUAL(
        POST_DEV_CFG_DONE,
        CAT24C04_gbc_sid_fxnTable.cb_init(&eeprom_sdr_inv, NULL, &alert_data));
    TEST_ASSERT_EQUAL(EEPROM_DISABLE_WRITE, SX1509_regs[SX1509_REG_DATA_A]);
    TEST_ASSERT_EQUAL(OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH,
                      Eeprom_GpioConfig[0x02]);

    TEST_ASSERT_EQUAL(
        POST_DEV_CFG_DONE,
        CAT24C04_gbc_sid_fxnTable.cb_init(&eeprom_fe_inv, NULL, &alert_data));
    TEST_ASSERT_EQUAL(EEPROM_DISABLE_FE_WRITE,
                      PCA9557_regs[PCA9557_REGS_OUTPUT_VALUE]);
    TEST_ASSERT_EQUAL(OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH,
                      Eeprom_GpioConfig[0x02]);

    /* Invalid device Test */
    TEST_ASSERT_EQUAL(
        POST_DEV_CFG_FAIL,
        CAT24C04_gbc_sid_fxnTable.cb_init(&e_invalid_dev, NULL, &alert_data));
    /* Invalid bus Test */
    TEST_ASSERT_EQUAL(
        POST_DEV_CFG_FAIL,
        CAT24C04_gbc_sid_fxnTable.cb_init(&e_invalid_bus, NULL, &alert_data));
}

void test_RFFE_InventoryGetStatus(void)
{
    uint8_t *buffer = (uint8_t *)malloc(EEPROM_BOARD_SIZE);

    EEPROM_regs[EEPROM_REG_BOARD_INFO_1] = EEPROM_ASCII_VAL_SA;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_2] = EEPROM_ASCII_VAL_17;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_3] = EEPROM_ASCII_VAL_18;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_4] = EEPROM_ASCII_VAL_LI;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_5] = EEPROM_ASCII_VAL_FE;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_6] = EEPROM_ASCII_VAL_3F;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_7] = EEPROM_ASCII_VAL_E0;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_8] = EEPROM_ASCII_VAL_00;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_9] = EEPROM_ASCII_VAL_05;
    memset(buffer, EEPROM_DEFAULT_VALUE_NULL, EEPROM_BOARD_SIZE);

    TEST_ASSERT_EQUAL(
        true, CAT24C04_fe_inv_fxnTable.cb_get_status(
                  &eeprom_fe_inv, EEPROM_RFFE_INVENTORY_PARAM_ID, buffer));
    TEST_ASSERT_EQUAL_STRING(EEPROM_FE_BOARD_INFO, buffer);

    /* Invalid Param ID */
    memset(buffer, EEPROM_DEFAULT_VALUE_NULL, EEPROM_BOARD_SIZE);
    TEST_ASSERT_EQUAL(false,
                      CAT24C04_fe_inv_fxnTable.cb_get_status(
                          &eeprom_fe_inv, EEPROM_INVALID_PARAM_ID, buffer));
    TEST_ASSERT_EQUAL_STRING("\0", buffer);

    /* Invalid device Test */
    TEST_ASSERT_EQUAL(
        false, CAT24C04_fe_inv_fxnTable.cb_get_status(
                   &e_invalid_dev, EEPROM_RFFE_INVENTORY_PARAM_ID, buffer));
    /* Invalid bus Test */
    TEST_ASSERT_EQUAL(
        false, CAT24C04_fe_inv_fxnTable.cb_get_status(
                   &e_invalid_bus, EEPROM_RFFE_INVENTORY_PARAM_ID, buffer));
}

void test_Sdr_InventoryGetStatus(void)
{
    uint8_t *buffer = (uint8_t *)malloc(EEPROM_BOARD_SIZE);
    EEPROM_regs[EEPROM_REG_BOARD_INFO_1] = EEPROM_ASCII_VAL_SA;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_2] = EEPROM_ASCII_VAL_17;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_3] = EEPROM_ASCII_VAL_18;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_4] = EEPROM_ASCII_VAL_LI;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_5] = EEPROM_ASCII_VAL_FE;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_6] = EEPROM_ASCII_VAL_3S;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_7] = EEPROM_ASCII_VAL_DR;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_8] = EEPROM_ASCII_VAL_00;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_9] = EEPROM_ASCII_VAL_32;
    memset(buffer, EEPROM_DEFAULT_VALUE_NULL, EEPROM_BOARD_SIZE);
    TEST_ASSERT_EQUAL(
        true, CAT24C04_sdr_inv_fxnTable.cb_get_status(
                  &eeprom_sdr_inv, EEPROM_SDR_INVENTORY_PARAM_ID, buffer));
    TEST_ASSERT_EQUAL_STRING(EEPROM_SDR_BOARD_INFO, buffer);

    /* Invalid Param ID*/
    memset(buffer, EEPROM_DEFAULT_VALUE_NULL, EEPROM_BOARD_SIZE);
    TEST_ASSERT_EQUAL(false,
                      CAT24C04_sdr_inv_fxnTable.cb_get_status(
                          &eeprom_sdr_inv, EEPROM_INVALID_PARAM_ID, buffer));
    TEST_ASSERT_EQUAL_STRING("\0", buffer);

    /* Invalid device Test */
    TEST_ASSERT_EQUAL(
        false, CAT24C04_sdr_inv_fxnTable.cb_get_status(
                   &e_invalid_dev, EEPROM_SDR_INVENTORY_PARAM_ID, buffer));
    /* Invalid bus Test */
    TEST_ASSERT_EQUAL(
        false, CAT24C04_sdr_inv_fxnTable.cb_get_status(
                   &e_invalid_bus, EEPROM_SDR_INVENTORY_PARAM_ID, buffer));
}

void test_sid_get_status_parameters_data(void)
{
    uint8_t *buffer = (uint8_t *)malloc(EEPROM_BOARD_SIZE);

    /* For OC_STAT_SYS_SERIAL_ID */
    EEPROM_regs[EEPROM_REG_SERIAL_INFO_1] = EEPROM_ASCII_VAL_SA;
    EEPROM_regs[EEPROM_REG_SERIAL_INFO_2] = EEPROM_ASCII_VAL_17;
    EEPROM_regs[EEPROM_REG_SERIAL_INFO_3] = EEPROM_ASCII_VAL_18;
    EEPROM_regs[EEPROM_REG_SERIAL_INFO_4] = EEPROM_ASCII_VAL_C0;
    EEPROM_regs[EEPROM_REG_SERIAL_INFO_5] = EEPROM_ASCII_VAL_45;
    EEPROM_regs[EEPROM_REG_SERIAL_INFO_6] = EEPROM_ASCII_VAL_0A;
    EEPROM_regs[EEPROM_REG_SERIAL_INFO_7] = EEPROM_ASCII_VAL_10;
    EEPROM_regs[EEPROM_REG_SERIAL_INFO_8] = EEPROM_ASCII_VAL_04;
    EEPROM_regs[EEPROM_REG_SERIAL_INFO_9] = EEPROM_ASCII_VAL_11;
    memset(buffer, EEPROM_DEFAULT_VALUE_NULL, EEPROM_BOARD_SIZE);

    TEST_ASSERT_EQUAL(true,
                      CAT24C04_gbc_sid_fxnTable.cb_get_status(
                          &eeprom_gbc_sid, OC_STAT_SYS_SERIAL_ID, buffer));
    TEST_ASSERT_EQUAL_STRING(EEPROM_SERIAL_INFO, buffer);

    /* For OC_STAT_SYS_GBC_BOARD_ID */
    memset(buffer, EEPROM_DEFAULT_VALUE_NULL, EEPROM_BOARD_SIZE);
    EEPROM_regs[EEPROM_REG_BOARD_INFO_1] = EEPROM_ASCII_VAL_SA;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_2] = EEPROM_ASCII_VAL_17;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_3] = EEPROM_ASCII_VAL_18;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_4] = EEPROM_ASCII_VAL_LI;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_5] = EEPROM_ASCII_VAL_FE;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_6] = EEPROM_ASCII_VAL_3G;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_7] = EEPROM_ASCII_VAL_BC;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_8] = EEPROM_ASCII_VAL_00;
    EEPROM_regs[EEPROM_REG_BOARD_INFO_9] = EEPROM_ASCII_VAL_41;
    memset(buffer, EEPROM_DEFAULT_VALUE_NULL, EEPROM_BOARD_SIZE);
    TEST_ASSERT_EQUAL(true,
                      CAT24C04_gbc_sid_fxnTable.cb_get_status(
                          &eeprom_gbc_sid, OC_STAT_SYS_GBC_BOARD_ID, buffer));
    TEST_ASSERT_EQUAL_STRING(EEPROM_GBC_BOARD_INFO, buffer);

    /* Invalid ParamID */
    memset(buffer, EEPROM_DEFAULT_VALUE_NULL, EEPROM_BOARD_SIZE);
    TEST_ASSERT_EQUAL(false,
                      CAT24C04_gbc_sid_fxnTable.cb_get_status(
                          &eeprom_gbc_sid, OC_STAT_SYS_STATE, buffer));
    TEST_ASSERT_EQUAL_STRING("\0", buffer);
}
