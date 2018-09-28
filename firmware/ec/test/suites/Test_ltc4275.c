#include "unity.h"
#include "inc/devices/ltc4275.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"
#include <string.h>

extern tPower_PDStatus_Info PDStatus_Info;

/* ======================== Constants & variables =========================== */
static uint8_t LTC4275_GpioPins[] = {
    [0x40] = 0x00, /* OC_EC_PWR_PD_NT2P = 64 */
    [0x60] = 0x00, /* OC_EC_PD_PWRGD_ALERT = 96*/
};

static uint32_t LTC4275_GpioConfig[] = {
    [0x40] = OCGPIO_CFG_INPUT,
    [0x60] = OCGPIO_CFG_INPUT,
};
/* ============================= Fake Functions ============================= */
#include <ti/sysbios/knl/Task.h>

unsigned int s_task_sleep_ticks;

xdc_Void ti_sysbios_knl_Task_sleep__E( xdc_UInt32 nticks )
{
    s_task_sleep_ticks += nticks;
}

void test_alert(void)
{
}

static void alert_handler(LTC4275_Event evt, void *context)
{

}

void post_update_POSTData(POSTData *pData, uint8_t I2CBus, uint8_t devAddress, uint16_t manId, uint16_t devId)
{
    pData->i2cBus = I2CBus;
    pData->devAddr = devAddress;
    pData->manId = manId;
    pData->devId = devId;
}
/* ============================= Boilerplate ================================ */
static OcGpio_Port s_fake_io_port = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

void suite_setUp(void)
{
    FakeGpio_registerDevSimple(LTC4275_GpioPins, LTC4275_GpioConfig);
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

LTC4275_Dev l_dev = {
    .cfg = {
        .pin_evt = &(OcGpio_Pin){ &s_fake_io_port, 0x60 },
        .pin_detect = &(OcGpio_Pin){ &s_fake_io_port, 0x40 },

    },
};


/* ================================ Tests =================================== */
void test_ltc4275_init()
{
    LTC4275_GpioPins[0x60] = 0;
    LTC4275_GpioPins[0x40] = 1;
    LTC4275_GpioConfig[0x60] = OCGPIO_CFG_INPUT;

    TEST_ASSERT_EQUAL(RETURN_OK, ltc4275_init(&l_dev));
    TEST_ASSERT_EQUAL(0, PDStatus_Info.pdStatus.powerGoodStatus);
    TEST_ASSERT_EQUAL(OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_BOTH_EDGES, LTC4275_GpioConfig[0x60]);
}

void test_ltc4275_get_power_good()
{
    ePDPowerState val;

    LTC4275_GpioPins[0x60] = 0;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4275_get_power_good(&l_dev,&val));
    TEST_ASSERT_EQUAL(LTC4275_POWERGOOD, val);

    LTC4275_GpioPins[0x60] = 1;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4275_get_power_good(&l_dev,&val));
    TEST_ASSERT_EQUAL(LTC4275_POWERGOOD_NOTOK, val);

}

void test_ltc4275_probe()
{
    LTC4275_GpioPins[0x60] = 1;
    POSTData postData;
    TEST_ASSERT_EQUAL(1, ltc4275_probe(&l_dev, &postData));
    TEST_ASSERT_EQUAL(0, PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(1, PDStatus_Info.state);
    TEST_ASSERT_EQUAL(2, PDStatus_Info.pdalert);

    LTC4275_GpioPins[0x60] = 0;
    TEST_ASSERT_EQUAL(3, ltc4275_probe(&l_dev, &postData));
    TEST_ASSERT_EQUAL(0, PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(1, PDStatus_Info.state);
    TEST_ASSERT_EQUAL(2, PDStatus_Info.pdalert);
}

void test_ltc4275_get_class()
{
    LTC4275_GpioPins[0x40] = 1;
    ePDClassType val;

    TEST_ASSERT_EQUAL(RETURN_OK, ltc4275_get_class(&l_dev, &val));
    TEST_ASSERT_EQUAL(LTC4275_CLASSTYPE_2, val);

    LTC4275_GpioPins[0x40] = 0;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4275_get_class(&l_dev, &val));
    TEST_ASSERT_EQUAL(LTC4275_CLASSTYPE_1, val);
}

void test_ltc4275_set_alert_handler()
{
    ltc4275_set_alert_handler(&l_dev, alert_handler, 1);
    TEST_ASSERT_EQUAL(1, (int *)l_dev.obj.cb_context);
    TEST_ASSERT_EQUAL(alert_handler, (int *)l_dev.obj.alert_cb);
}

void test_ltc4275_update_status()
{
    LTC4275_GpioPins[0x60] = 1;
    LTC4275_GpioPins[0x40] = 1;
    ltc4275_update_status(&l_dev);
    TEST_ASSERT_EQUAL(0, PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(1, PDStatus_Info.state);
    TEST_ASSERT_EQUAL(2, PDStatus_Info.pdalert);

    LTC4275_GpioPins[0x60] = 0;
    LTC4275_GpioPins[0x40] = 0;
    ltc4275_update_status(&l_dev);
    TEST_ASSERT_EQUAL(1, PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(0, PDStatus_Info.state);
    TEST_ASSERT_EQUAL(2, PDStatus_Info.pdalert);
}
