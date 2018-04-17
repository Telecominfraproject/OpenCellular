#include "src/registry/Framework.h"

#include "inc/devices/led.h"
#include "inc/devices/ocmp_wrappers/ocmp_led.h"

/* TODO: Implement enable and disable led commands in future if nedded */
bool led_testpattern_control(void *driver, void *params)
{
    ReturnStatus status = RETURN_OK;

    switch ((ledTestParam) driver) {
        case HCI_LED_OFF:
        {
            status = hci_led_turnoff_all();
            break;
        }
        case HCI_LED_RED:
        {
            status = hci_led_turnon_red();
            break;
        }
        case HCI_LED_GREEN:
        {
            status = hci_led_turnon_green();
        }
        default:
        {
            LOGGER_ERROR("HCILED::Unknown param %d\n", params);
        }
    }
    return status;
}

static ePostCode _probe(void *driver)
{
    return led_probe();
}

static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    if (led_init() != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }

    if (hci_led_system_boot() != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }

    return POST_DEV_CFG_DONE;
}

const Driver HCI_LED = {
    .name = "HCI_LED",
    .status = NULL,
    .config = NULL,
    .alerts = NULL,

    /* Message handlers */
    .cb_probe = _probe,
    .cb_init = _init,
    .cb_get_status = NULL,
    .cb_get_config = NULL,
    .cb_set_config = NULL,
};
