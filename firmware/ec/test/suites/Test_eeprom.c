/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_eeprom.h"

extern bool Eeprom_GpioPins[0x02];
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
extern uint8_t SX1509_regs[SX1509_REG_TEST_2];
extern uint16_t EEPROM_regs[EEPROM_REG_END];
extern uint32_t Eeprom_GpioConfig[0x02];

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
void test_eeprom_init(void)
{
    EEPROM_regs[EEPROM_REG_DEF_INIT] = EEPROM_READ_WRITE_VALUE;
    Eeprom_GpioConfig[0x02] = OCGPIO_CFG_OUT_HIGH;

    TEST_ASSERT_EQUAL(true, eeprom_init(&eeprom_gbc_sid));
    TEST_ASSERT_EQUAL(OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH,
                      Eeprom_GpioConfig[0x02]);

    /* Checking for NULL cfg */
    TEST_ASSERT_EQUAL(false, eeprom_init(e_invalid_cfg));

    /* Checking with invalid slave address */
    TEST_ASSERT_EQUAL(false, eeprom_init(&e_invalid_dev));

    /* Checking with invalid bus address */
    TEST_ASSERT_EQUAL(false, eeprom_init(&e_invalid_bus));
}

void test_eeprom_read(void)
{
    uint16_t buffer;
    EEPROM_regs[EEPROM_REG_SERIAL_INFO_1] = EEPROM_READ_WRITE_VALUE;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      eeprom_read(&eeprom_gbc_sid, OC_CONNECT1_SERIAL_INFO,
                                  &buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_HEX(EEPROM_READ_WRITE_VALUE, buffer);

    /* Checking for NULL cfg */
    buffer = EEPROM_DEFAULT_VALUE_NULL;
    EEPROM_regs[EEPROM_REG_SERIAL_INFO_1] = EEPROM_READ_WRITE_VALUE;
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      eeprom_read(e_invalid_cfg, OC_CONNECT1_SERIAL_INFO,
                                  &buffer, sizeof(buffer)));

    /* Checking with invalid slave address */
    buffer = EEPROM_DEFAULT_VALUE_NULL;
    EEPROM_regs[EEPROM_REG_SERIAL_INFO_1] = EEPROM_READ_WRITE_VALUE;
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      eeprom_read(&e_invalid_dev, OC_CONNECT1_SERIAL_INFO,
                                  &buffer, sizeof(buffer)));

    /* Checking with invalid bus address */
    buffer = EEPROM_DEFAULT_VALUE_NULL;
    EEPROM_regs[EEPROM_REG_SERIAL_INFO_1] = EEPROM_READ_WRITE_VALUE;
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      eeprom_read(&e_invalid_bus, OC_CONNECT1_SERIAL_INFO,
                                  &buffer, sizeof(buffer)));

    /* Checking with 0xFFFF register */
    buffer = EEPROM_DEFAULT_VALUE_NULL;
    EEPROM_regs[EEPROM_REG_FFFF] = EEPROM_READ_WRITE_VALUE;
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read(&eeprom_gbc_sid, EEPROM_REG_FFFF,
                                             &buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_HEX16(EEPROM_READ_WRITE_VALUE, buffer);
}

void test_eeprom_write(void)
{
    uint16_t buffer = EEPROM_READ_WRITE_VALUE;
    EEPROM_regs[EEPROM_REG_SERIAL_INFO_1] = EEPROM_DEFAULT_VALUE_NULL;

    TEST_ASSERT_EQUAL(RETURN_OK,
                      eeprom_write(&eeprom_gbc_inv, OC_CONNECT1_SERIAL_INFO,
                                   &buffer, EEPROM_WRITE_SIZE));
    TEST_ASSERT_EQUAL_HEX16(EEPROM_READ_WRITE_VALUE,
                            EEPROM_regs[EEPROM_REG_SERIAL_INFO_1]);

    /* Test with size > page_size */
    EEPROM_regs[EEPROM_REG_SERIAL_INFO_1] = EEPROM_DEFAULT_VALUE_NULL;
    TEST_ASSERT_EQUAL(RETURN_OK,
                      eeprom_write(&eeprom_gbc_inv, OC_CONNECT1_SERIAL_INFO,
                                   &buffer, EEPROM_BIG_WRITE_SIZE));
    TEST_ASSERT_EQUAL_HEX16(EEPROM_READ_WRITE_VALUE,
                            EEPROM_regs[EEPROM_REG_SERIAL_INFO_1]);

    /* Checking for NULL cfg */
    buffer = EEPROM_READ_WRITE_VALUE;
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      eeprom_write(e_invalid_cfg, OC_CONNECT1_SERIAL_INFO,
                                   &buffer, EEPROM_WRITE_SIZE));

    /* Checking with invalid slave address */
    buffer = EEPROM_READ_WRITE_VALUE;
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      eeprom_write(&e_invalid_dev, OC_CONNECT1_SERIAL_INFO,
                                   &buffer, EEPROM_WRITE_SIZE));

    /* Checking with invalid bus address */
    buffer = EEPROM_READ_WRITE_VALUE;
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      eeprom_write(&e_invalid_bus, OC_CONNECT1_SERIAL_INFO,
                                   &buffer, EEPROM_WRITE_SIZE));
}

void test_eeprom_disable_write(void)
{
    SX1509_regs[SX1509_REG_DATA_B] = EEPROM_ENABLE;
    SX1509_regs[SX1509_REG_DATA_A] = EEPROM_ENABLE;

    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_disable_write(&enable_dev));
    TEST_ASSERT_EQUAL(EEPROM_DISABLE_WRITE, SX1509_regs[SX1509_REG_DATA_A]);

    /* Checking for NULL cfg */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_disable_write(e_invalid_cfg));
}

void test_eeprom_enable_write(void)
{
    SX1509_regs[SX1509_REG_DATA_B] = EEPROM_DISABLE;
    SX1509_regs[SX1509_REG_DATA_A] = EEPROM_DISABLE;

    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_enable_write(&enable_dev));
    TEST_ASSERT_EQUAL(EEPROM_ENABLE_WRITE, SX1509_regs[SX1509_REG_DATA_A]);

    /* Checking for NULL cfg */
    TEST_ASSERT_EQUAL(RETURN_NOTOK, eeprom_enable_write(e_invalid_cfg));
}
/* Values are taken as per GBCV1 test board */
void test_eeprom_read_board_info(void)
{
    uint8_t *buffer = (uint8_t *)malloc(EEPROM_BOARD_SIZE);
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

    TEST_ASSERT_EQUAL(RETURN_OK,
                      eeprom_read_board_info(&eeprom_gbc_inv, buffer));
    TEST_ASSERT_EQUAL_STRING(EEPROM_GBC_BOARD_INFO, buffer);

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
    TEST_ASSERT_EQUAL(RETURN_OK,
                      eeprom_read_board_info(&eeprom_sdr_inv, buffer));
    TEST_ASSERT_EQUAL_STRING(EEPROM_SDR_BOARD_INFO, buffer);

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
    TEST_ASSERT_EQUAL(RETURN_OK,
                      eeprom_read_board_info(&eeprom_fe_inv, buffer));
    TEST_ASSERT_EQUAL_STRING(EEPROM_FE_BOARD_INFO, buffer);

    /* Checking for invalid subsystem cfg */
    memset(buffer, EEPROM_DEFAULT_VALUE_NULL, EEPROM_BOARD_SIZE);
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      eeprom_read_board_info(&e_invalid_ss_cfg, buffer));
    TEST_ASSERT_EQUAL_STRING("\0", buffer);

    /* Checking for NULL cfg */
    memset(buffer, EEPROM_DEFAULT_VALUE_NULL, EEPROM_BOARD_SIZE);
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      eeprom_read_board_info(e_invalid_cfg, buffer));
    TEST_ASSERT_EQUAL_STRING("\0", buffer);

    /* Checking with invalid slave address */
    memset(buffer, EEPROM_DEFAULT_VALUE_NULL, EEPROM_BOARD_SIZE);
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      eeprom_read_board_info(&e_invalid_dev, buffer));
    TEST_ASSERT_EQUAL_STRING("\0", buffer);

    /* Checking with invalid bus address */
    memset(buffer, EEPROM_DEFAULT_VALUE_NULL, EEPROM_BOARD_SIZE);
    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                      eeprom_read_board_info(&e_invalid_bus, buffer));
    TEST_ASSERT_EQUAL_STRING("\0", buffer);
}

void test_eeprom_read_oc_info(void)
{
    uint8_t *buffer = (uint8_t *)malloc(EEPROM_BOARD_SIZE);
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
    TEST_ASSERT_EQUAL(RETURN_OK, eeprom_read_oc_info(buffer));
    TEST_ASSERT_EQUAL_STRING(EEPROM_SERIAL_INFO, buffer);
}
/* Function eeprom_write_device_info_record and eeprom_read_device_info_record
 * are only stack holder in the code. No Test case are required from them as of
 * now */

/* No test case are created for function i2c_eeprom_write and i2c_eeprom_read as
 * they are static function, Can not access them outside the file. Also they
 * are already covered as part of eeprom_reab and eeprom_write.*/
