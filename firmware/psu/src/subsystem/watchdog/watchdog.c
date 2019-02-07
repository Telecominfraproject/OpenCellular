//#include "common/inc/global/ocmp_frame.h"
//#include "inc/common/global_header.h"
#include "inc/global/OC_CONNECT1.h"
#include "drivers/OcGpio.h"
#include <stdbool.h>
//#include "inc/utils/util.h"
//#include "inc/devices/debug_ocgpio.h"

extern OcGpio_Port ec_io;

bool watchdog_toggle_wdi()
{
    OcGpio_Pin wdi_pin   = { &ec_io, OC_EC_WD_INPUT};

    OcGpio_configure(&wdi_pin,
                 OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);
    while(1) {
    OcGpio_write(&wdi_pin, false);
    OcGpio_write(&wdi_pin, true);
    }
    return true;
}
