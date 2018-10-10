#include "suites/Test_subsys_rffe.h"

bool isAlert = FALSE;
extern uint8_t taskCreated;

//FE Channel 1 ADC
I2C_Dev fe_ch1_ads7830 = {
   .bus = OC_CONNECT1_I2C4,
   .slave_addr = RFFE_CHANNEL1_ADC_ADDR,
};

//FE Channel 2 ADC
I2C_Dev fe_ch2_ads7830 = {
   .bus = OC_CONNECT1_I2C4,
   .slave_addr = RFFE_CHANNEL2_ADC_ADDR,
};

//FE Channel 2 ADC
I2C_Dev fe_invalid_ads7830 = {
   .bus = OC_CONNECT1_I2C4,
   .slave_addr = 0xFF,
};

Fe_Cfg fe_rffecfg = {
    .fe_gpio_cfg = &fe_gpiocfg,
    .fe_ch1_gain_cfg = (Fe_Ch1_Gain_Cfg*)&fe_ch1_tx_gain_cfg,
    .fe_ch2_gain_cfg = (Fe_Ch2_Gain_Cfg*)&fe_ch2_tx_gain_cfg,
    .fe_ch1_lna_cfg = (Fe_Ch1_Lna_Cfg*)&fe_ch1_rx_gain_cfg,
    .fe_ch2_lna_cfg = (Fe_Ch2_Lna_Cfg*)&fe_ch2_rx_gain_cfg,
    .fe_watchdog_cfg = (Fe_Watchdog_Cfg*)&fe_watchdog_cfg,
};

Fe_Ch_Pwr_Cfg fe_ch1_pwrcfg = {
    .channel = RFFE_CHANNEL1,
    .fe_Rffecfg = (Fe_Cfg*)&fe_rffecfg
};

Fe_Ch_Pwr_Cfg fe_ch2_pwrcfg = {
    .channel = RFFE_CHANNEL2,
    .fe_Rffecfg = (Fe_Cfg*)&fe_rffecfg
};

/* ============================= Boilerplate ================================ */
void SysCtlDelay(uint32_t ui32Count)
{

}
OCSubsystem *ss_reg[SUBSYSTEM_COUNT] = {}; 
void OCMP_GenerateAlert(const AlertData *alert_data,
                        unsigned int alert_id,
                        const void *data)
{
    isAlert = TRUE;
}
void suite_setUp(void)
{
    FakeGpio_registerDevSimple(rffe_GpioPins, rffe_GpioConfig);
    fake_I2C_registerDevSimple(((PCA9557_Cfg *)(fe_ch1_gain_io.cfg))->i2c_dev.bus, ((PCA9557_Cfg *)(fe_ch1_gain_io.cfg))->i2c_dev.slave_addr,
                               PCA9557_regs_ch1_TX, sizeof(PCA9557_regs_ch1_TX), sizeof(PCA9557_regs_ch1_TX[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_BIG_ENDIAN);
    fake_I2C_registerDevSimple(((PCA9557_Cfg *)(fe_ch2_gain_io.cfg))->i2c_dev.bus, ((PCA9557_Cfg *)(fe_ch2_gain_io.cfg))->i2c_dev.slave_addr,
                               PCA9557_regs_ch2_TX, sizeof(PCA9557_regs_ch2_TX), sizeof(PCA9557_regs_ch2_TX[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_BIG_ENDIAN);
    fake_I2C_registerDevSimple(((PCA9557_Cfg *)(fe_ch1_lna_io.cfg))->i2c_dev.bus, ((PCA9557_Cfg *)(fe_ch1_lna_io.cfg))->i2c_dev.slave_addr,
                               PCA9557_regs_ch1_RX, sizeof(PCA9557_regs_ch1_RX), sizeof(PCA9557_regs_ch1_RX[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_BIG_ENDIAN);
    fake_I2C_registerDevSimple(((PCA9557_Cfg *)(fe_ch2_lna_io.cfg))->i2c_dev.bus, ((PCA9557_Cfg *)(fe_ch2_lna_io.cfg))->i2c_dev.slave_addr,
                               PCA9557_regs_ch2_RX, sizeof(PCA9557_regs_ch2_RX), sizeof(PCA9557_regs_ch2_RX[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_BIG_ENDIAN);
    fake_I2C_registerDevSimple(((PCA9557_Cfg *)(fe_watchdog_io.cfg))->i2c_dev.bus, ((PCA9557_Cfg *)(fe_watchdog_io.cfg))->i2c_dev.slave_addr,
                               PCA9557_regs_revpower, sizeof(PCA9557_regs_revpower), sizeof(PCA9557_regs_revpower[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_BIG_ENDIAN);
    fake_I2C_registerDevSimple(fe_ch1_ads7830.bus, fe_ch1_ads7830.slave_addr,
                               ch1_ads7830, sizeof(ch1_ads7830), sizeof(ch1_ads7830[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_BIG_ENDIAN);
    fake_I2C_registerDevSimple(fe_ch2_ads7830.bus, fe_ch2_ads7830.slave_addr,
                               ch2_ads7830, sizeof(ch2_ads7830), sizeof(ch2_ads7830[0]),
                               sizeof(uint8_t), FAKE_I2C_DEV_BIG_ENDIAN);
}

void setUp(void)
{
}

void tearDown(void)
{
}

void suite_tearDown(void)
{
}

/* ================================ Tests =================================== */
void test_rffe_pwr_control(void)
{
    rffe_GpioPins[OC_EC_FE_CONTROL] = 0;
    rffe_pwr_control(&fe_gpiocfg, OC_FE_ENABLE);
    TEST_ASSERT_TRUE(rffe_GpioPins[OC_EC_FE_CONTROL]);

    rffe_GpioPins[OC_EC_FE_CONTROL] = 1;
    rffe_pwr_control(&fe_gpiocfg, OC_FE_DISABLE);
    TEST_ASSERT_FALSE(rffe_GpioPins[OC_EC_FE_CONTROL]);
    
    rffe_GpioPins[OC_EC_FE_CONTROL] = 1;
    rffe_pwr_control(&fe_gpiocfg, 2);
    TEST_ASSERT_FALSE(rffe_GpioPins[OC_EC_FE_CONTROL]);
    
    rffe_GpioPins[OC_EC_FE_CONTROL] = 0;
    rffe_pwr_control(NULL, OC_FE_ENABLE);
}

void test_rffe_pre_init(void)
{
    rffe_GpioConfig[OC_EC_FE_PWR_GD] = 0; 
    rffe_GpioConfig[OC_EC_FE_CONTROL] = 0; 
    rffe_GpioPins[OC_EC_FE_PWR_GD] = 0; 
    PCA9557_regs_ch2_TX[02] = 1;
    PCA9557_regs_ch2_TX[03] = 1;
    PCA9557_regs_ch2_RX[2] = 1;
    PCA9557_regs_ch2_RX[3] = 1;
    PCA9557_regs_revpower[2] = 1;
    PCA9557_regs_revpower[3] = 1;
    
    TEST_ASSERT_TRUE(rffe_pre_init(&fe_rffecfg, NULL));
    TEST_ASSERT_EQUAL(rffe_GpioConfig[OC_EC_FE_PWR_GD], OCGPIO_CFG_INPUT);
    TEST_ASSERT_EQUAL(rffe_GpioConfig[OC_EC_FE_CONTROL], OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
    TEST_ASSERT_TRUE(rffe_GpioConfig[OC_EC_FE_PWR_GD]);
    TEST_ASSERT_EQUAL(PCA9557_regs_ch2_TX[2], 0);
    TEST_ASSERT_EQUAL(PCA9557_regs_ch2_TX[3], 0);
    TEST_ASSERT_EQUAL(PCA9557_regs_ch2_RX[2], 0);
    TEST_ASSERT_EQUAL(PCA9557_regs_ch2_RX[3], 0);
    TEST_ASSERT_EQUAL(PCA9557_regs_revpower[2], 0);
    TEST_ASSERT_EQUAL(PCA9557_regs_revpower[3], 0);

    TEST_ASSERT_FALSE(rffe_pre_init(NULL, NULL));
}

void test_rffe_post_init(void)
{
    eSubSystemStates state;
    PCA9557_regs_ch2_TX[1] = 0;
    PCA9557_regs_ch2_RX[1] = 0;
    PCA9557_regs_ch1_TX[1] = 0;
    PCA9557_regs_ch1_RX[1] = 0;
    PCA9557_regs_revpower[1] = 0;
    
    (*(PCA9557_Obj *)fe_watchdog_io.object_data).reg_output = 0;
    TEST_ASSERT_TRUE(rffe_post_init(&fe_rffecfg, &state));
    TEST_ASSERT_EQUAL(SS_STATE_CFG, state);

    TEST_ASSERT_EQUAL(0x1, PCA9557_regs_ch2_TX[1]);
    TEST_ASSERT_EQUAL(0x2, PCA9557_regs_ch2_RX[1]);
    TEST_ASSERT_EQUAL(0x2, PCA9557_regs_revpower[1]);
    TEST_ASSERT_EQUAL(0, PCA9557_regs_ch1_TX[1]);
    TEST_ASSERT_EQUAL(0, PCA9557_regs_ch1_TX[1]);
    
    TEST_ASSERT_FALSE(rffe_post_init(NULL, &state));
}

void test_RFFE_reset(void)
{
    rffe_GpioPins[OC_EC_RFFE_RESET] = 0;
    TEST_ASSERT_TRUE(RFFE_reset(&sdr_gpioCfg, NULL));
    TEST_ASSERT_EQUAL(1, rffe_GpioPins[OC_EC_RFFE_RESET]);

    TEST_ASSERT_FALSE(RFFE_reset(NULL, NULL));
}
void test_rffe_watchdog_handler(void)
{
}
#if 0
void test_rffe_watchdog_handler(void)
{
    /* Ch1 positive watchdog case */
    (*(PCA9557_Obj *)fe_watchdog_io.object_data).reg_output = 0x0C;
    isAlert = FALSE;
    _rffe_watchdog_handler(&fe_ch1_watchdog);
    TEST_ASSERT_TRUE(isAlert);

    /* Ch1 negative watchdog case */
    (*(PCA9557_Obj *)fe_watchdog_io.object_data).reg_output = 0x0;
    isAlert = FALSE;
    _rffe_watchdog_handler(&fe_ch1_watchdog);
    TEST_ASSERT_FALSE(isAlert);

    /* Ch2 negative watchdog case */
    _rffe_watchdog_handler(&fe_ch2_watchdog);
    TEST_ASSERT_FALSE(isAlert);

    /* Ch2 positive watchdog case */
    isAlert = FALSE;
    (*(PCA9557_Obj *)fe_watchdog_io.object_data).reg_output = 0x30;
    _rffe_watchdog_handler(&fe_ch2_watchdog);
    TEST_ASSERT_TRUE(isAlert);

    /* Invalid */
    _rffe_watchdog_handler(NULL);
}
#endif
void test_rffe_ctrl_configure_power_amplifier(void)
{
    PCA9557_regs_ch2_TX[1] = 0;
    PCA9557_regs_ch2_RX[1] = 0;
    PCA9557_regs_ch1_TX[1] = 0;
    PCA9557_regs_ch1_RX[1] = 0;
    
    /* Enable Channel 1 */
    TEST_ASSERT_EQUAL(RETURN_OK,
                     rffe_ctrl_configure_power_amplifier(&fe_ch1_pwrcfg,
                                                  RFFE_ACTIVATE_PA));
    TEST_ASSERT_EQUAL(0x1, PCA9557_regs_ch2_TX[1]);
    TEST_ASSERT_EQUAL(0x2, PCA9557_regs_ch2_RX[1]);

    /* Disable Channel 1 */
    TEST_ASSERT_EQUAL(RETURN_OK,
                     rffe_ctrl_configure_power_amplifier(&fe_ch1_pwrcfg,
                                                  RFFE_DEACTIVATE_PA));
    TEST_ASSERT_EQUAL(0x1, PCA9557_regs_ch2_TX[1]);
    TEST_ASSERT_EQUAL(0x0, PCA9557_regs_ch2_RX[1]);

    /* Enable Channel 2 */
    PCA9557_regs_revpower[1] = 0;
    (*(PCA9557_Obj *)fe_watchdog_io.object_data).reg_output = 0;
    TEST_ASSERT_EQUAL(RETURN_OK,
                     rffe_ctrl_configure_power_amplifier(&fe_ch2_pwrcfg,
                     RFFE_ACTIVATE_PA));
    TEST_ASSERT_EQUAL(0x2, PCA9557_regs_revpower[1]);

    /* Disable Channel 2 */
    TEST_ASSERT_EQUAL(RETURN_OK,
                     rffe_ctrl_configure_power_amplifier(&fe_ch2_pwrcfg,
                     RFFE_DEACTIVATE_PA));
    TEST_ASSERT_EQUAL(0x0, PCA9557_regs_revpower[1]);
    
    /*Invalid cases */
  //  TEST_ASSERT_EQUAL(RETURN_NOTOK,
    //                rffe_ctrl_configure_power_amplifier(RFFE_CHANNEL2, 2));
  //  TEST_ASSERT_EQUAL(RETURN_NOTOK,
    //                rffe_ctrl_configure_power_amplifier(2, 2));
}

uint8_t channel = RFFE_CHANNEL1;
void test_RFFE_enablePA(void)
{
    PCA9557_regs_ch2_TX[1] = 0;
    PCA9557_regs_ch2_RX[1] = 0;
    PCA9557_regs_ch1_TX[1] = 0;
    PCA9557_regs_ch1_RX[1] = 0;
    
    /* Enable Channel 1 */
    TEST_ASSERT_TRUE(RFFE_enablePA(&fe_ch1_pwrcfg, NULL));
    TEST_ASSERT_EQUAL(0x1, PCA9557_regs_ch2_TX[1]);
    TEST_ASSERT_EQUAL(0x2, PCA9557_regs_ch2_RX[1]);
    
    /* Enable Channel 2 */
    channel = RFFE_CHANNEL2;
    PCA9557_regs_revpower[1] = 0;
    (*(PCA9557_Obj *)fe_watchdog_io.object_data).reg_output = 0;
    TEST_ASSERT_TRUE(RFFE_enablePA(&fe_ch2_pwrcfg, NULL));
    TEST_ASSERT_EQUAL(0x2, PCA9557_regs_revpower[1]);
    
    /* Invalid case */
    channel = 2;
 //   TEST_ASSERT_FALSE(RFFE_enablePA(&channel, NULL));
}

void test_RFFE_disablePA(void)
{
    
    /* Enable Channel 1 */
    channel = RFFE_CHANNEL1;
    TEST_ASSERT_TRUE(RFFE_disablePA(&fe_ch1_pwrcfg, NULL));
    TEST_ASSERT_EQUAL(0x1, PCA9557_regs_ch2_TX[1]);
    TEST_ASSERT_EQUAL(0x0, PCA9557_regs_ch2_RX[1]);
    
    /* Enable Channel 2 */
    channel = RFFE_CHANNEL2;
    PCA9557_regs_revpower[1] = 0;
    (*(PCA9557_Obj *)fe_watchdog_io.object_data).reg_output = 0x2;
    TEST_ASSERT_TRUE(RFFE_disablePA(&fe_ch2_pwrcfg, NULL));
    TEST_ASSERT_EQUAL(0x0, PCA9557_regs_revpower[1]);
    
    /* Invalid case */
    channel = 2;
   // TEST_ASSERT_FALSE(RFFE_disablePA(&channel, NULL));
}

void test_rffe_powermonitor_createtask(void)
{
    taskCreated = FALSE;
    rffe_powermonitor_createtask();
    TEST_ASSERT_TRUE(taskCreated);
}

void test_rffe_powermonitor_read_power(void)
{
    uint16_t rfPower;
    
    ch1_ads7830[0xA4] = 34;
    TEST_ASSERT_EQUAL(RETURN_OK,
                        rffe_powermonitor_read_power(&fe_ch1_ads7830,
                        RFFE_STAT_FW_POWER, &rfPower));
    TEST_ASSERT_EQUAL(34, rfPower);

    ch2_ads7830[0xA4] = 44;
    TEST_ASSERT_EQUAL(RETURN_OK,
                        rffe_powermonitor_read_power(&fe_ch2_ads7830,
                        RFFE_STAT_FW_POWER, &rfPower));
    TEST_ASSERT_EQUAL(44, rfPower);

    TEST_ASSERT_EQUAL(RETURN_NOTOK,
                        rffe_powermonitor_read_power(&fe_invalid_ads7830,
                        RFFE_STAT_FW_POWER, &rfPower));
}