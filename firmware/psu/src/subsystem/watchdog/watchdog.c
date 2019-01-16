#include "common/inc/global/ocmp_frame.h"
#include "inc/common/global_header.h"
#include "inc/global/OC_CONNECT1.h"
#include "inc/utils/util.h"
#include "inc/devices/debug_ocgpio.h"

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/hal/Timer.h>
#include <xdc/runtime/Error.h>

#define TI_TASKSTACKSIZE        2048
#define TI_TASKPRIORITY         6
static Clock_Handle s_clockHandle;
extern OcGpio_Port ec_io;

static bool watchdog_toggle_wdi()
{
    OcGpio_Pin wdi_pin   = { &ec_io, OC_EC_WD_INPUT};

    OcGpio_configure(&wdi_pin,
                 OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);
    OcGpio_write(&wdi_pin, false);
    LOGGER_DEBUG("Watchdog:INFO:: Toggling the line.\n");

    return true;
}
static void watchdog_handler(UArg arg)
{
    watchdog_toggle_wdi();
//    Util_startClock(s_clockHandle);
}
static void watchdog_Task(UArg arg0, UArg arg1)
{
    uint8_t count = 0;
    uint32_t result = 0;

    Clock_Struct clockStruct;

    watchdog_toggle_wdi();
 //   s_clockHandle = Util_constructClock(&clockStruct, watchdog_handler, 500, 0, true, NULL);
 //   Util_startClock(s_clockHandle);
    Timer_Handle timerHandle;
    Timer_Params timerParams;
    Task_Handle taskHandle;
    Error_Block eb;

    Error_init(&eb);
    Timer_Params_init(&timerParams);
    timerParams.period = 2000*1000; /* 2 ms */
    timerHandle = Timer_create(Timer_ANY, watchdog_handler, &timerParams, &eb);
    if (timerHandle == NULL) {
        LOGGER_DEBUG("Timer create failed");
    }

    return;
}

void watchdog_create_task()
{
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stackSize = TI_TASKSTACKSIZE;
    taskParams.priority = TI_TASKPRIORITY;
    taskParams.arg0 = NULL;//(uintptr_t)&s_intConfigs[s_numDevices];
    Task_Handle task = Task_create(watchdog_Task, &taskParams, NULL);

    return;
}
