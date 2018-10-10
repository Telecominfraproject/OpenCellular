#include "common/inc/global/OC_CONNECT1.h"
#include "drivers/GpioPCA9557.h"
#include "fake/fake_I2C.h"
#include "fake/fake_GPIO.h"
#include "inc/subsystem/sdr/sdr.h"
#include <string.h>
#include "unity.h"
/* ======================== Constants & variables =========================== */
#define SDR_FX3_IOEXP_ADDRESS                       0x1E
#define OC_CONNECT1_I2C3                            0x05

static OcGpio_Port s_fake_io_port = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

OcGpio_Port sdr_fx3_io = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg = &(PCA9557_Cfg) {
        .i2c_dev = { OC_CONNECT1_I2C3, SDR_FX3_IOEXP_ADDRESS },
    },
    .object_data = &(PCA9557_Obj){},
};

static uint8_t PCA9557_regs[] = {
    [0x00] = 0x00, /* Input values */
    [0x01] = 0x00, /* Output values */
    [0x02] = 0x00, /* Polarity */
    [0x03] = 0x00, /* Dir Config */
};

Sdr_gpioCfg sdr_gpioCfg = (Sdr_gpioCfg) {
    /* EC_TRXFECONN_GPIO2/SDR_REG_LDO_PGOOD */
    .pin_sdr_reg_ldo_pgood  = { &s_fake_io_port, OC_EC_SDR_PWR_GD },
    /* TRXFE_12V_ONOFF */
    .pin_trxfe_12v_onoff    = { &s_fake_io_port, OC_EC_SDR_PWR_CNTRL },
    /* EC_FE_RESET_OUT/RF_FE_IO_RESET */
    .pin_rf_fe_io_reset     = { &s_fake_io_port, OC_EC_SDR_FE_IO_RESET_CTRL },
    /* EC_TRXFECONN_GPIO1/SDR_RESET_IN */
    .pin_sdr_reset_in       = { &s_fake_io_port, OC_EC_SDR_DEVICE_CONTROL },
    /* EC_TRXFE_RESET */
    .pin_ec_trxfe_reset     = { &s_fake_io_port, OC_EC_RFFE_RESET },
    /* FX3_RESET */
    .pin_fx3_reset       = { &sdr_fx3_io, 0 },
};

static bool sdr_GpioPins[] = {
    [1] = 0x1, /* Pin = 1 */
    [OC_EC_GPIOCOUNT] = 0x1,
};

static uint32_t sdr_GpioConfig[] = {
    [1] = OCGPIO_CFG_INPUT,
    [OC_EC_GPIOCOUNT] = OCGPIO_CFG_INPUT,
};

extern int apState;

/* ============================= Boilerplate ================================ */
#include <ti/sysbios/knl/Task.h>
unsigned int s_task_sleep_ticks;
xdc_Void ti_sysbios_knl_Task_sleep__E( xdc_UInt32 nticks )
{
    s_task_sleep_ticks += nticks;
}
void SysCtlDelay(uint32_t ui32Count)
{

}

void suite_setUp(void)
{
    FakeGpio_registerDevSimple(sdr_GpioPins, sdr_GpioConfig);
    fake_I2C_init();
    fake_I2C_registerDevSimple(OC_CONNECT1_I2C3, SDR_FX3_IOEXP_ADDRESS,
                               PCA9557_regs, sizeof(PCA9557_regs),
                               sizeof(PCA9557_regs[0]), sizeof(uint8_t),
                               FAKE_I2C_DEV_LITTLE_ENDIAN);
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
void test_sdr_pwr_control(void)
{
    /* Enable */
    sdr_GpioConfig[OC_EC_SDR_PWR_CNTRL] = 0;
    sdr_pwr_control(&sdr_gpioCfg, OC_SDR_ENABLE);
    TEST_ASSERT_EQUAL(sdr_GpioConfig[OC_EC_SDR_PWR_CNTRL], OCGPIO_CFG_OUTPUT |
                                    OCGPIO_CFG_OUT_HIGH);
    /* Disable */
    sdr_pwr_control(&sdr_gpioCfg, OC_SDR_DISABLE);
    TEST_ASSERT_EQUAL(sdr_GpioConfig[OC_EC_SDR_PWR_CNTRL], OCGPIO_CFG_OUTPUT |
                                    OCGPIO_CFG_OUT_LOW);
    
    /* Invalid */
    sdr_GpioConfig[OC_EC_SDR_PWR_CNTRL] = 0;
    sdr_pwr_control(&sdr_gpioCfg, 2);
    TEST_ASSERT_EQUAL(sdr_GpioConfig[OC_EC_SDR_PWR_CNTRL], 0);

    sdr_pwr_control(NULL, OC_SDR_ENABLE);
}

void test_SDR_Init(void)
{
    sdr_GpioConfig[OC_EC_SDR_PWR_GD] = 0;
    sdr_GpioConfig[OC_EC_SDR_FE_IO_RESET_CTRL] = 0;
    sdr_GpioConfig[OC_EC_SDR_DEVICE_CONTROL] = 0;
    sdr_GpioConfig[OC_EC_RFFE_RESET] = 0;
    sdr_GpioConfig[OC_EC_SDR_PWR_CNTRL] = 0;
    sdr_GpioPins[OC_EC_SDR_FE_IO_RESET_CTRL] = 0;
    sdr_GpioPins[OC_EC_SDR_DEVICE_CONTROL] = 0;
    sdr_GpioPins[OC_EC_RFFE_RESET] = 0;
    sdr_GpioConfig[0] = 0;

    TEST_ASSERT_TRUE(SDR_Init(&sdr_gpioCfg, NULL));
    TEST_ASSERT_EQUAL(sdr_GpioConfig[OC_EC_SDR_PWR_GD], OCGPIO_CFG_INPUT);
    TEST_ASSERT_EQUAL(sdr_GpioConfig[OC_EC_SDR_FE_IO_RESET_CTRL], OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
    TEST_ASSERT_EQUAL(sdr_GpioConfig[OC_EC_SDR_DEVICE_CONTROL], OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
    TEST_ASSERT_EQUAL(sdr_GpioConfig[OC_EC_RFFE_RESET], OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
    TEST_ASSERT_EQUAL(sdr_GpioConfig[OC_EC_SDR_PWR_CNTRL], OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);
    TEST_ASSERT_TRUE(sdr_GpioPins[OC_EC_SDR_FE_IO_RESET_CTRL]);
    TEST_ASSERT_TRUE(sdr_GpioPins[OC_EC_SDR_DEVICE_CONTROL]);
    TEST_ASSERT_TRUE(sdr_GpioPins[OC_EC_RFFE_RESET]);
    TEST_ASSERT_EQUAL(PCA9557_regs[0x01], 1);

    /* Invalid */
    TEST_ASSERT_FALSE(SDR_Init(NULL, NULL));
}

void test_SDR_reset(void)
{
    /*Reset*/
    sdr_GpioPins[OC_EC_SDR_DEVICE_CONTROL] = 0;
    TEST_ASSERT_TRUE(SDR_reset(&sdr_gpioCfg, NULL));
    TEST_ASSERT_TRUE(sdr_GpioPins[OC_EC_SDR_DEVICE_CONTROL]);
    
    /* Invalid */
    TEST_ASSERT_FALSE(SDR_reset(NULL, NULL));
}

void test_SDR_fx3Reset(void)
{
    /* Reset */
    TEST_ASSERT_TRUE(SDR_fx3Reset(&sdr_gpioCfg, NULL));
    TEST_ASSERT_EQUAL(PCA9557_regs[0x01], 1);

    /* Invalid */
    TEST_ASSERT_FALSE(SDR_fx3Reset(NULL, NULL));
}