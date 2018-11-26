#include "include/test_powerSource.h"

extern bool PWR_GpioPins[OC_EC_PWR_PRSNT_POE];
extern const OcGpio_FnTable GpioSX1509_fnTable;
extern OcGpio_Port ec_io;
extern OcGpio_Port gbc_io_0;
extern PWRSRC_Dev gbc_pwr_powerSource;
extern PWRSRC_Dev gbc_pwr_powerSource_invalid;
extern uint32_t PWR_GpioConfig[OC_EC_PWR_PRSNT_POE];
extern uint8_t SX1509_regs[SX1509_REG_TEST_2];
/* ============================= Boilerplate ================================ */
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
    OcGpio_init(&gbc_io_0);
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
    fake_I2C_deinit();
}
/* ================================ Tests =================================== */
void test_init(void)
{
    PWR_GpioPins[OC_EC_PWR_PRSNT_SOLAR_AUX] = 0x1;
    AlertData alert_data = {
        .subsystem = 7,
        .componentId = 1,
        .deviceId = 0,
    };
    AlertData *alert_data_cp = malloc(sizeof(AlertData));
    *alert_data_cp = alert_data;
    TEST_ASSERT_EQUAL(
        POST_DEV_NO_CFG_REQ,
        PWRSRC_fxnTable.cb_init(&gbc_pwr_powerSource, NULL, alert_data_cp));
}

void test_probe(void)
{
    POSTData postData;
    PWR_GpioConfig[OC_EC_PWR_PRSNT_SOLAR_AUX] = OCGPIO_CFG_OUTPUT;
    PWR_GpioConfig[OC_EC_PWR_PRSNT_POE] = OCGPIO_CFG_OUTPUT;

    TEST_ASSERT_EQUAL(POST_DEV_NOSTATUS, PWRSRC_fxnTable.cb_probe(
                                             &gbc_pwr_powerSource, &postData));
    TEST_ASSERT_EQUAL(OCGPIO_CFG_INPUT,
                      PWR_GpioConfig[OC_EC_PWR_PRSNT_SOLAR_AUX]);
    TEST_ASSERT_EQUAL(OCGPIO_CFG_INPUT, PWR_GpioConfig[OC_EC_PWR_PRSNT_POE]);
}

void test_get_status_poeAvailable(void)
{
    /* POE State available */
    uint8_t powerStatus = PWR_SRC_NOT_AVAILABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_POE] = PWR_STATE_ENABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_SOLAR_AUX] = PWR_STATE_DISABLE;
    SX1509_regs[SX1509_REG_DATA_B] = PWR_INT_EXT_BAT_DISABLE_FIRST_BYTE;
    SX1509_regs[SX1509_REG_DATA_A] = PWR_INT_EXT_BAT_DISABLE_SECOND_BYTE;
    pwr_source_init();
    pwr_get_source_info(&gbc_pwr_powerSource);
    TEST_ASSERT_EQUAL(true, PWRSRC_fxnTable.cb_get_status(
                                &gbc_pwr_powerSource, PWR_STAT_POE_AVAILABILITY,
                                &powerStatus));
    TEST_ASSERT_EQUAL(PWR_SRC_ACTIVE_AVAILABLE, powerStatus);

    /* POE State Not Available */
    powerStatus = PWR_SRC_NOT_AVAILABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_POE] = PWR_STATE_DISABLE;
    pwr_source_init();
    pwr_get_source_info(&gbc_pwr_powerSource);
    TEST_ASSERT_EQUAL(true, PWRSRC_fxnTable.cb_get_status(
                                &gbc_pwr_powerSource, PWR_STAT_POE_AVAILABILITY,
                                &powerStatus));
    TEST_ASSERT_EQUAL(PWR_SRC_NOT_AVAILABLE, powerStatus);
}

void test_get_status_poeAccessible(void)
{
    /* POE State Accessible */
    uint8_t powerStatus = PWR_SRC_NOT_AVAILABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_POE] = PWR_STATE_ENABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_SOLAR_AUX] = PWR_STATE_DISABLE;
    SX1509_regs[SX1509_REG_DATA_B] = PWR_INT_EXT_BAT_DISABLE_FIRST_BYTE;
    SX1509_regs[SX1509_REG_DATA_A] = PWR_INT_EXT_BAT_DISABLE_SECOND_BYTE;
    pwr_source_init();
    pwr_get_source_info(&gbc_pwr_powerSource);
    TEST_ASSERT_EQUAL(true, PWRSRC_fxnTable.cb_get_status(
                                &gbc_pwr_powerSource,
                                PWR_STAT_POE_ACCESSIBILITY, &powerStatus));
    TEST_ASSERT_EQUAL(PWR_SRC_ACTIVE_AVAILABLE, powerStatus);

    /* POE State Not Accessible */
    powerStatus = PWR_SRC_NOT_AVAILABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_POE] = PWR_STATE_DISABLE;
    pwr_source_init();
    pwr_get_source_info(&gbc_pwr_powerSource);
    TEST_ASSERT_EQUAL(true, PWRSRC_fxnTable.cb_get_status(
                                &gbc_pwr_powerSource,
                                PWR_STAT_POE_ACCESSIBILITY, &powerStatus));
    TEST_ASSERT_EQUAL(PWR_SRC_NOT_AVAILABLE, powerStatus);
}

void test_get_status_solaravailable(void)
{
    /* Solar State Available */
    uint8_t powerStatus = PWR_SRC_NOT_AVAILABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_POE] = PWR_STATE_DISABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_SOLAR_AUX] = PWR_STATE_ENABLE;
    SX1509_regs[SX1509_REG_DATA_B] = PWR_INT_EXT_BAT_DISABLE_FIRST_BYTE;
    SX1509_regs[SX1509_REG_DATA_A] = PWR_INT_EXT_BAT_DISABLE_SECOND_BYTE;
    pwr_source_init();
    pwr_get_source_info(&gbc_pwr_powerSource);
    TEST_ASSERT_EQUAL(true, PWRSRC_fxnTable.cb_get_status(
                                &gbc_pwr_powerSource,
                                PWR_STAT_SOLAR_AVAILABILITY, &powerStatus));
    TEST_ASSERT_EQUAL(PWR_SRC_ACTIVE_AVAILABLE, powerStatus);

    /* Solar State Not Available */
    powerStatus = PWR_SRC_NOT_AVAILABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_SOLAR_AUX] = PWR_STATE_DISABLE;
    pwr_source_init();
    pwr_get_source_info(&gbc_pwr_powerSource);
    TEST_ASSERT_EQUAL(true, PWRSRC_fxnTable.cb_get_status(
                                &gbc_pwr_powerSource,
                                PWR_STAT_SOLAR_AVAILABILITY, &powerStatus));
    TEST_ASSERT_EQUAL(PWR_SRC_NOT_AVAILABLE, powerStatus);
}

void test_get_status_solaraccessible(void)
{
    /* Solar State Accessible */
    uint8_t powerStatus = PWR_SRC_NOT_AVAILABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_POE] = PWR_STATE_DISABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_SOLAR_AUX] = PWR_STATE_ENABLE;
    SX1509_regs[SX1509_REG_DATA_B] = PWR_INT_EXT_BAT_DISABLE_FIRST_BYTE;
    SX1509_regs[SX1509_REG_DATA_A] = PWR_INT_EXT_BAT_DISABLE_SECOND_BYTE;
    pwr_source_init();
    pwr_get_source_info(&gbc_pwr_powerSource);
    TEST_ASSERT_EQUAL(true, PWRSRC_fxnTable.cb_get_status(
                                &gbc_pwr_powerSource,
                                PWR_STAT_SOLAR_ACCESSIBILITY, &powerStatus));
    TEST_ASSERT_EQUAL(PWR_SRC_ACTIVE_AVAILABLE, powerStatus);

    /* Solar State Not Accessible */
    powerStatus = PWR_SRC_NOT_AVAILABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_SOLAR_AUX] = PWR_STATE_DISABLE;
    pwr_source_init();
    pwr_get_source_info(&gbc_pwr_powerSource);
    TEST_ASSERT_EQUAL(true, PWRSRC_fxnTable.cb_get_status(
                                &gbc_pwr_powerSource,
                                PWR_STAT_SOLAR_ACCESSIBILITY, &powerStatus));
    TEST_ASSERT_EQUAL(PWR_SRC_NOT_AVAILABLE, powerStatus);
}

void test_get_status_extavailable(void)
{
    /* EXT Battery Available */
    uint8_t powerStatus = PWR_SRC_NOT_AVAILABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_POE] = PWR_STATE_DISABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_SOLAR_AUX] = PWR_STATE_DISABLE;
    SX1509_regs[SX1509_REG_DATA_B] = PWR_EXT_BAT_ENABLE_FIRST_BYTE;
    SX1509_regs[SX1509_REG_DATA_A] = PWR_EXT_BAT_ENABLE_SECOND_BYTE;
    pwr_source_init();
    pwr_get_source_info(&gbc_pwr_powerSource);
    TEST_ASSERT_EQUAL(true, PWRSRC_fxnTable.cb_get_status(
                                &gbc_pwr_powerSource,
                                PWR_STAT_EXTBATT_AVAILABILITY, &powerStatus));
    TEST_ASSERT_EQUAL(PWR_SRC_ACTIVE_AVAILABLE, powerStatus);

    /* EXT Battery Not Available */
    powerStatus = PWR_SRC_NOT_AVAILABLE;
    SX1509_regs[SX1509_REG_DATA_B] = PWR_INT_EXT_BAT_DISABLE_FIRST_BYTE;
    SX1509_regs[SX1509_REG_DATA_A] = PWR_INT_EXT_BAT_DISABLE_SECOND_BYTE;
    pwr_source_init();
    pwr_get_source_info(&gbc_pwr_powerSource);
    TEST_ASSERT_EQUAL(true, PWRSRC_fxnTable.cb_get_status(
                                &gbc_pwr_powerSource,
                                PWR_STAT_EXTBATT_AVAILABILITY, &powerStatus));
    TEST_ASSERT_EQUAL(PWR_SRC_NOT_AVAILABLE, powerStatus);
}

void test_get_status_extaccessible(void)
{
    /* EXT Battery Accessible */
    uint8_t powerStatus = PWR_SRC_NOT_AVAILABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_POE] = PWR_STATE_DISABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_SOLAR_AUX] = PWR_STATE_DISABLE;
    SX1509_regs[SX1509_REG_DATA_B] = PWR_EXT_BAT_ENABLE_FIRST_BYTE;
    SX1509_regs[SX1509_REG_DATA_A] = PWR_EXT_BAT_ENABLE_SECOND_BYTE;
    pwr_source_init();
    pwr_get_source_info(&gbc_pwr_powerSource);
    TEST_ASSERT_EQUAL(true, PWRSRC_fxnTable.cb_get_status(
                                &gbc_pwr_powerSource,
                                PWR_STAT_EXTBATT_ACCESSIBILITY, &powerStatus));
    TEST_ASSERT_EQUAL(PWR_SRC_ACTIVE_AVAILABLE, powerStatus);

    /* EXT Battery Not Accessible */
    powerStatus = PWR_SRC_NOT_AVAILABLE;
    SX1509_regs[SX1509_REG_DATA_B] = PWR_INT_EXT_BAT_DISABLE_FIRST_BYTE;
    SX1509_regs[SX1509_REG_DATA_A] = PWR_INT_EXT_BAT_DISABLE_SECOND_BYTE;
    pwr_source_init();
    pwr_get_source_info(&gbc_pwr_powerSource);
    TEST_ASSERT_EQUAL(true, PWRSRC_fxnTable.cb_get_status(
                                &gbc_pwr_powerSource,
                                PWR_STAT_EXTBATT_ACCESSIBILITY, &powerStatus));
    TEST_ASSERT_EQUAL(PWR_SRC_NOT_AVAILABLE, powerStatus);
}

void test_get_status_intavailable(void)
{
    /* Int Battery Available */
    uint8_t powerStatus = PWR_SRC_NOT_AVAILABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_POE] = PWR_STATE_DISABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_SOLAR_AUX] = PWR_STATE_DISABLE;
    SX1509_regs[SX1509_REG_DATA_B] = PWR_INT_BAT_ENABLE_FIRST_BYTE;
    SX1509_regs[SX1509_REG_DATA_A] = PWR_INT_BAT_ENABLE_SECOND_BYTE;
    pwr_source_init();
    pwr_get_source_info(&gbc_pwr_powerSource);
    TEST_ASSERT_EQUAL(true, PWRSRC_fxnTable.cb_get_status(
                                &gbc_pwr_powerSource,
                                PWR_STAT_INTBATT_AVAILABILITY, &powerStatus));
    TEST_ASSERT_EQUAL(PWR_SRC_ACTIVE_AVAILABLE, powerStatus);

    /* Int Battery Not Available */
    powerStatus = PWR_SRC_NOT_AVAILABLE;
    SX1509_regs[SX1509_REG_DATA_B] = PWR_INT_EXT_BAT_DISABLE_FIRST_BYTE;
    SX1509_regs[SX1509_REG_DATA_A] = PWR_INT_EXT_BAT_DISABLE_SECOND_BYTE;
    pwr_source_init();
    pwr_get_source_info(&gbc_pwr_powerSource);
    TEST_ASSERT_EQUAL(true, PWRSRC_fxnTable.cb_get_status(
                                &gbc_pwr_powerSource,
                                PWR_STAT_INTBATT_AVAILABILITY, &powerStatus));
    TEST_ASSERT_EQUAL(PWR_SRC_NOT_AVAILABLE, powerStatus);
}

void test_get_status_intaccessible(void)
{
    uint8_t powerStatus = PWR_SRC_NOT_AVAILABLE;
    /* Int Battery Not Accessible */
    PWR_GpioPins[OC_EC_PWR_PRSNT_POE] = PWR_STATE_DISABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_SOLAR_AUX] = PWR_STATE_DISABLE;
    SX1509_regs[SX1509_REG_DATA_B] = PWR_INT_BAT_ENABLE_FIRST_BYTE;
    SX1509_regs[SX1509_REG_DATA_A] = PWR_INT_BAT_ENABLE_SECOND_BYTE;

    pwr_source_init();
    pwr_get_source_info(&gbc_pwr_powerSource);
    TEST_ASSERT_EQUAL(true, PWRSRC_fxnTable.cb_get_status(
                                &gbc_pwr_powerSource,
                                PWR_STAT_INTBATT_ACCESSIBILITY, &powerStatus));
    TEST_ASSERT_EQUAL(PWR_SRC_ACTIVE_AVAILABLE, powerStatus);

    /* Int Battery Not Accessible */
    powerStatus = PWR_SRC_NOT_AVAILABLE;
    SX1509_regs[SX1509_REG_DATA_B] = PWR_INT_EXT_BAT_DISABLE_FIRST_BYTE;
    SX1509_regs[SX1509_REG_DATA_A] = PWR_INT_EXT_BAT_DISABLE_SECOND_BYTE;
    pwr_source_init();
    pwr_get_source_info(&gbc_pwr_powerSource);
    TEST_ASSERT_EQUAL(true, PWRSRC_fxnTable.cb_get_status(
                                &gbc_pwr_powerSource,
                                PWR_STAT_INTBATT_ACCESSIBILITY, &powerStatus));
    TEST_ASSERT_EQUAL(PWR_SRC_NOT_AVAILABLE, powerStatus);
}

void test_get_status_invalid_param(void)
{
    uint8_t powerStatus = PWR_SRC_NOT_AVAILABLE;
    /* Invalid Param Test */
    PWR_GpioPins[OC_EC_PWR_PRSNT_POE] = PWR_STATE_DISABLE;
    PWR_GpioPins[OC_EC_PWR_PRSNT_SOLAR_AUX] = PWR_STATE_DISABLE;
    SX1509_regs[SX1509_REG_DATA_B] = PWR_INT_BAT_ENABLE_FIRST_BYTE;
    SX1509_regs[SX1509_REG_DATA_A] = PWR_INT_BAT_ENABLE_SECOND_BYTE;

    pwr_source_init();
    pwr_get_source_info(&gbc_pwr_powerSource);
    TEST_ASSERT_EQUAL(
        true, PWRSRC_fxnTable.cb_get_status(
                  &gbc_pwr_powerSource, PWR_STAT_INVALID_PARAM, &powerStatus));
    TEST_ASSERT_EQUAL(PWR_SRC_NOT_AVAILABLE, powerStatus);
}
