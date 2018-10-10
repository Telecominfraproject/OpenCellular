#include "inc/subsystem/gpp/gpp.h"
#include "inc/subsystem/gpp/ebmp.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include <string.h>
#include "unity.h"
/* ======================== Constants & variables =========================== */
uint8_t apUp = 1;
extern uint8_t taskCreated;

#define OC_PMIC_ENABLE          (1)
#define OC_PMIC_DISABLE         (0)

extern Gpp_gpioCfg gbc_gpp_gpioCfg;
OcGpio_Port ec_io = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

static bool gpp_GpioPins[] = {
    [1] = 0x1, /* Pin = 1 */
    [115] = 0x1,
};

static uint32_t gpp_GpioConfig[] = {
    [1] = OCGPIO_CFG_INPUT,
    [115] = OCGPIO_CFG_INPUT,
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
    FakeGpio_registerDevSimple(gpp_GpioPins, gpp_GpioConfig);
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
void test_gpp_check_processor_reset(void)
{
    gpp_GpioPins[57] = 1;
    TEST_ASSERT_TRUE(gpp_check_processor_reset(&gbc_gpp_gpioCfg));
    gpp_GpioPins[57] = 0;
    TEST_ASSERT_FALSE(gpp_check_processor_reset(&gbc_gpp_gpioCfg));
}

void test_gpp_check_core_power(void)
{
    gpp_GpioPins[56] = 1;
    TEST_ASSERT_TRUE(gpp_check_core_power(&gbc_gpp_gpioCfg));
    gpp_GpioPins[56] = 0;
    TEST_ASSERT_FALSE(gpp_check_core_power(&gbc_gpp_gpioCfg));
}

void test_gpp_pmic_control(void)
{
    /* ENABLE */
    gpp_GpioPins[57] = 1;
    gpp_GpioPins[56] = 1;
    gpp_GpioPins[58] = 0;
    TEST_ASSERT_TRUE(gpp_pmic_control(&gbc_gpp_gpioCfg, OC_PMIC_ENABLE));
    TEST_ASSERT_EQUAL(OC_PMIC_ENABLE, gpp_GpioPins[58]);
    TEST_ASSERT_EQUAL_HEX8(gpp_GpioConfig[35], OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_BOTH_EDGES);
    TEST_ASSERT_EQUAL_HEX8(gpp_GpioConfig[83], OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_BOTH_EDGES);
    TEST_ASSERT_EQUAL_HEX8(gpp_GpioConfig[57], OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_BOTH_EDGES);

    /* DISABLE */
    gpp_GpioPins[58] = 1;
    TEST_ASSERT_TRUE(gpp_pmic_control(&gbc_gpp_gpioCfg, OC_PMIC_DISABLE));
    TEST_ASSERT_EQUAL(OC_PMIC_DISABLE, gpp_GpioPins[58]);

    /* Invalid pin cases */
    gpp_GpioPins[57] = 0;
    gpp_GpioPins[56] = 1;
    TEST_ASSERT_FALSE(gpp_pmic_control(&gbc_gpp_gpioCfg, OC_PMIC_ENABLE));

    gpp_GpioPins[57] = 1;
    gpp_GpioPins[56] = 0;
    TEST_ASSERT_FALSE(gpp_pmic_control(&gbc_gpp_gpioCfg, OC_PMIC_ENABLE));

    gpp_GpioPins[57] = 0;
    gpp_GpioPins[56] = 0;
    TEST_ASSERT_FALSE(gpp_pmic_control(&gbc_gpp_gpioCfg, OC_PMIC_ENABLE));

    /* Invalid status cases */
    gpp_GpioPins[57] = 1;
    gpp_GpioPins[56] = 1;
    TEST_ASSERT_FALSE(gpp_pmic_control(&gbc_gpp_gpioCfg, -1));
    TEST_ASSERT_FALSE(gpp_pmic_control(&gbc_gpp_gpioCfg, 2));
    TEST_ASSERT_FALSE(gpp_pmic_control(NULL, OC_PMIC_ENABLE));
}

void test_gpp_msata_das(void)
{
    gpp_GpioPins[113] = 0;
    TEST_ASSERT_TRUE(gpp_msata_das(&gbc_gpp_gpioCfg));
    gpp_GpioPins[113] = 1;
    TEST_ASSERT_FALSE(gpp_msata_das(&gbc_gpp_gpioCfg));
    TEST_ASSERT_FALSE(gpp_msata_das(NULL));
}

void test_gpp_pwrgd_protection(void)
{
    gpp_GpioPins[107] = 1;
    TEST_ASSERT_TRUE(gpp_pwrgd_protection(&gbc_gpp_gpioCfg));
    gpp_GpioPins[107] = 0;
    TEST_ASSERT_FALSE(gpp_pwrgd_protection(&gbc_gpp_gpioCfg));
    TEST_ASSERT_FALSE(gpp_pwrgd_protection(NULL));
}

void test_gpp_pre_init(void)
{
    gpp_GpioConfig[57] = 0;
    gpp_GpioConfig[56] = 0;
    gpp_GpioConfig[113] = 0;
    gpp_GpioConfig[107] = 0;
    gpp_GpioConfig[58] = 0;
    gpp_GpioConfig[115] = 0;
    
    TEST_ASSERT_TRUE(gpp_pre_init(&gbc_gpp_gpioCfg, NULL));
    TEST_ASSERT_EQUAL_HEX8(gpp_GpioConfig[57], OCGPIO_CFG_INPUT);
    TEST_ASSERT_EQUAL_HEX8(gpp_GpioConfig[56], OCGPIO_CFG_INPUT);
    TEST_ASSERT_EQUAL_HEX8(gpp_GpioConfig[113], OCGPIO_CFG_INPUT);
    TEST_ASSERT_EQUAL_HEX8(gpp_GpioConfig[107], OCGPIO_CFG_INPUT);
    TEST_ASSERT_EQUAL_HEX8(gpp_GpioConfig[58], OCGPIO_CFG_OUTPUT |
                                             OCGPIO_CFG_OUT_LOW);
    TEST_ASSERT_EQUAL_HEX8(gpp_GpioConfig[115], OCGPIO_CFG_OUTPUT |
                                                 OCGPIO_CFG_OUT_HIGH);
}

void test_gpp_post_init(void)
{
    eSubSystemStates state;
    gpp_GpioPins[57] = 1;
    gpp_GpioPins[56] = 1;
    gpp_GpioPins[58] = 0;
    gpp_GpioPins[107] = 0;
    TEST_ASSERT_TRUE(gpp_post_init(&gbc_gpp_gpioCfg, &state));
    TEST_ASSERT_EQUAL_HEX8(state, SS_STATE_CFG);

    gpp_GpioPins[107] = 1;
    TEST_ASSERT_FALSE(gpp_post_init(&gbc_gpp_gpioCfg, &state));
    TEST_ASSERT_EQUAL_HEX8(state, SS_STATE_FAULTY);

    gpp_GpioPins[107] = 0;
    gpp_GpioPins[57] = 0;
    gpp_GpioPins[56] = 1;
    TEST_ASSERT_FALSE(gpp_post_init(&gbc_gpp_gpioCfg, &state));
    TEST_ASSERT_EQUAL_HEX8(state, SS_STATE_FAULTY);
    TEST_ASSERT_FALSE(gpp_post_init(NULL, &state));
    TEST_ASSERT_FALSE(gpp_post_init(&gbc_gpp_gpioCfg, NULL));
}

void test_GPP_ap_Reset()
{
    gpp_GpioPins[115] = 0;

    TEST_ASSERT_TRUE(GPP_ap_Reset(&gbc_gpp_gpioCfg, NULL));
    TEST_ASSERT_EQUAL_HEX8(1, gpp_GpioPins[115]);
    TEST_ASSERT_FALSE(GPP_ap_Reset(NULL, NULL));

    //TODO: #489 GpioNative_write is successful for Input pins
}

void test_ebmp_create_task(void)
{
    /*
     * Assertions checks are added in the create task RTOS system call 
     */
    taskCreated = false;
    ebmp_create_task();
    TEST_ASSERT_TRUE(taskCreated);
}

void test_ebmp_init(void)
{
    gpp_GpioConfig[35] = 0;
    gpp_GpioConfig[83] = 0;
    gpp_GpioConfig[57] = 0;

    ebmp_init(&gbc_gpp_gpioCfg);
    TEST_ASSERT_EQUAL_HEX8(gpp_GpioConfig[35], OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_BOTH_EDGES);
    TEST_ASSERT_EQUAL_HEX8(gpp_GpioConfig[83], OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_BOTH_EDGES);
    TEST_ASSERT_EQUAL_HEX8(gpp_GpioConfig[57], OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_BOTH_EDGES);
    ebmp_init(NULL);
}

void test_ebmp_check_stages(void)
{
    ebmp_init(&gbc_gpp_gpioCfg);
    gpp_GpioPins[35] = 0;
    gpp_GpioPins[83] = 0;
    ebmp_check_boot_pin_status();
    TEST_ASSERT_EQUAL_HEX8(STATE_T1, apState);

    gpp_GpioPins[35] = 1;
    gpp_GpioPins[83] = 0;
    ebmp_check_boot_pin_status();
    TEST_ASSERT_EQUAL_HEX8(STATE_T2, apState);
    
    gpp_GpioPins[35] = 1;
    gpp_GpioPins[83] = 1;
    ebmp_check_boot_pin_status();
    TEST_ASSERT_EQUAL_HEX8(STATE_T3, apState);

    gpp_GpioPins[35] = 0;
    gpp_GpioPins[83] = 1;
    ebmp_check_boot_pin_status();
    TEST_ASSERT_EQUAL_HEX8(STATE_T4, apState);

    gpp_GpioPins[35] = 0;
    gpp_GpioPins[83] = 0;
    ebmp_check_boot_pin_status();
    TEST_ASSERT_EQUAL_HEX8(STATE_T5, apState);

    gpp_GpioPins[35] = 1;
    gpp_GpioPins[83] = 0;
    ebmp_check_boot_pin_status();
    TEST_ASSERT_EQUAL_HEX8(STATE_T6, apState);
    
    gpp_GpioPins[35] = 1;
    gpp_GpioPins[83] = 1;
    ebmp_check_boot_pin_status();
    TEST_ASSERT_EQUAL_HEX8(STATE_T7, apState);

    gpp_GpioPins[57] = 1;
    ebmp_check_soc_plt_reset();
    TEST_ASSERT_EQUAL_HEX8(STATE_T0, apState);

    gpp_GpioPins[56] = 0;
    gpp_GpioPins[57] = 0;
    ebmp_check_soc_plt_reset();
    TEST_ASSERT_EQUAL_HEX8(STATE_INVALID, apState);
}