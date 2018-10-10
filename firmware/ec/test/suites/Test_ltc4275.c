/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
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

// Parameters are not used as this is just used to test assigning the 
//   alert_handler right now.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static void alert_handler(LTC4275_Event evt, void *context)
{

}
#pragma GCC diagnostic pop

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
void test_ltc4275_init(void)
{
    LTC4275_GpioPins[0x60] = 0;
    LTC4275_GpioPins[0x40] = 1;
    LTC4275_GpioConfig[0x60] = OCGPIO_CFG_INPUT;

    TEST_ASSERT_EQUAL(RETURN_OK, ltc4275_init(&l_dev));
    TEST_ASSERT_EQUAL(LTC4275_POWERGOOD, PDStatus_Info.pdStatus.powerGoodStatus);
    TEST_ASSERT_EQUAL(OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_BOTH_EDGES, LTC4275_GpioConfig[0x60]);
}

void test_ltc4275_get_power_good(void)
{
    ePDPowerState val;

    LTC4275_GpioPins[0x60] = 0;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4275_get_power_good(&l_dev,&val));
    TEST_ASSERT_EQUAL(LTC4275_POWERGOOD, val);

    LTC4275_GpioPins[0x60] = 1;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4275_get_power_good(&l_dev,&val));
    TEST_ASSERT_EQUAL(LTC4275_POWERGOOD_NOTOK, val);

}

void test_ltc4275_probe(void)
{
    LTC4275_GpioPins[0x60] = 1;
    POSTData postData;
    TEST_ASSERT_EQUAL(POST_DEV_MISSING, ltc4275_probe(&l_dev, &postData));
    TEST_ASSERT_EQUAL(LTC4275_CLASSTYPE_UNKOWN, PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(LTC4275_STATE_NOTOK, PDStatus_Info.state);
    TEST_ASSERT_EQUAL(LTC4275_DISCONNECT_ALERT, PDStatus_Info.pdalert);

    LTC4275_GpioPins[0x60] = 0;
    TEST_ASSERT_EQUAL(POST_DEV_FOUND, ltc4275_probe(&l_dev, &postData));
    TEST_ASSERT_EQUAL(LTC4275_CLASSTYPE_UNKOWN, PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(LTC4275_STATE_NOTOK, PDStatus_Info.state);
    TEST_ASSERT_EQUAL(LTC4275_DISCONNECT_ALERT, PDStatus_Info.pdalert);
}

void test_ltc4275_get_class(void)
{
    LTC4275_GpioPins[0x40] = 1;
    ePDClassType val;

    TEST_ASSERT_EQUAL(RETURN_OK, ltc4275_get_class(&l_dev, &val));
    TEST_ASSERT_EQUAL(LTC4275_CLASSTYPE_2, val);

    LTC4275_GpioPins[0x40] = 0;
    TEST_ASSERT_EQUAL(RETURN_OK, ltc4275_get_class(&l_dev, &val));
    TEST_ASSERT_EQUAL(LTC4275_CLASSTYPE_1, val);
}

void test_ltc4275_set_alert_handler(void)
{
    int context;
    ltc4275_set_alert_handler(&l_dev, alert_handler, &context);
    TEST_ASSERT_EQUAL(&context, (int *)l_dev.obj.cb_context);
    TEST_ASSERT_EQUAL(alert_handler, (int *)l_dev.obj.alert_cb);
}

void test_ltc4275_update_status(void)
{
    LTC4275_GpioPins[0x60] = 1;
    LTC4275_GpioPins[0x40] = 1;
    ltc4275_update_status(&l_dev);
    TEST_ASSERT_EQUAL(LTC4275_CLASSTYPE_UNKOWN, PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(LTC4275_STATE_NOTOK, PDStatus_Info.state);
    TEST_ASSERT_EQUAL(LTC4275_DISCONNECT_ALERT, PDStatus_Info.pdalert);

    LTC4275_GpioPins[0x60] = 0;
    LTC4275_GpioPins[0x40] = 0;
    // Store values of state and alert before call, since when class is TYPE_1,
    // state and alert should not be modified. 
    ePDState oldState = PDStatus_Info.state;
    ePDAlert oldAlert = PDStatus_Info.pdalert;
    ltc4275_update_status(&l_dev);
    TEST_ASSERT_EQUAL(LTC4275_CLASSTYPE_1, PDStatus_Info.pdStatus.classStatus);
    TEST_ASSERT_EQUAL(oldState, PDStatus_Info.state);
    TEST_ASSERT_EQUAL(oldAlert, PDStatus_Info.pdalert);
}
