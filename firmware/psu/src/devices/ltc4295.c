/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

//*****************************************************************************
//                                HEADER FILES
//*****************************************************************************
#include "devices/i2c/threaded_int.h"
#include "helpers/math.h"
#include "helpers/memory.h"
#include "inc/common/byteorder.h"
#include "inc/devices/ltc4295.h"
#include "inc/subsystem/power/power.h"

#include <math.h>
#include <ti/sysbios/knl/Task.h>

tPower_PDStatus_Info PDStatus_Info;


/******************************************************************************
 * @fn          ltc4295_handle_irq
 *
 * @brief       Read the change in the PD state and callbacks the registerd function.
 *
 * @args        Alert Context
 *
 * @return      None
 */
static void ltc4295_handle_irq(void *context) {
    LTC4295_Dev *dev = context;

    const IArg mutexKey = GateMutex_enter(dev->obj.mutex); {
        ltc4295_update_status(dev);
    } GateMutex_leave(dev->obj.mutex, mutexKey);

    /* See if we have a callback assigned to handle alerts */
    if (!dev->obj.alert_cb) {
        return;
    }

    /* Since > CRIT implies above window, we only handle the highest priority
     * event to avoid duplicate events being sent */
    if (PDStatus_Info.pdalert == LTC4295_CONNECT_ALERT) {
        dev->obj.alert_cb(LTC4295_CONNECT_EVT, dev->obj.cb_context);
    } else if (PDStatus_Info.pdalert == LTC4295_DISCONNECT_ALERT) {
        dev->obj.alert_cb(LTC4295_DISCONNECT_EVT, dev->obj.cb_context);
    } else if (PDStatus_Info.pdalert == LTC4295_INCOMPATIBLE_ALERT) {
        dev->obj.alert_cb(LTC4295_INCOMPATIBLE_EVT, dev->obj.cb_context);
    }
}

/******************************************************************************
 * @fn          ltc4295_configure
 *
 * @brief       configure GPIO's.
 *
 * @args        None
 *
 * @return      None
 */
void ltc4295_config(const LTC4295_Dev *dev)
{
    OcGpio_configure(dev->cfg.pin_evt, OCGPIO_CFG_INPUT);
    OcGpio_configure(dev->cfg.pin_detect, OCGPIO_CFG_INPUT);
}

/******************************************************************************
 * @fn          ltc4295_probe
 *
 * @brief       Intializes PD update struct.
 *
 * @args        None
 *
 * @return      None
 */
ePostCode ltc4295_probe(const LTC4295_Dev *dev, POSTData *postData)
{
    ePostCode postCode = POST_DEV_MISSING;
    ePDPowerState pdStatus = LTC4295_POWERGOOD_NOTOK;
    ReturnStatus ret = ltc4295_get_power_good(dev, &pdStatus);
    if (ret != RETURN_OK) {
        LOGGER("LTC4295::ERROR: Power good signal read failed.\n");
        return postCode;
    }
    if (pdStatus == LTC4295_POWERGOOD ) {
        PDStatus_Info.pdStatus.classStatus = LTC4295_CLASSTYPE_UNKOWN;
        PDStatus_Info.pdStatus.powerGoodStatus = LTC4295_POWERGOOD;
        PDStatus_Info.state = LTC4295_STATE_NOTOK;
        PDStatus_Info.pdalert = LTC4295_DISCONNECT_ALERT;
        postCode = POST_DEV_FOUND;
    } else {
        PDStatus_Info.pdStatus.classStatus = LTC4295_CLASSTYPE_UNKOWN;
        PDStatus_Info.pdStatus.powerGoodStatus = LTC4295_POWERGOOD_NOTOK;
        PDStatus_Info.state = LTC4295_STATE_NOTOK;
        PDStatus_Info.pdalert = LTC4295_DISCONNECT_ALERT;
    }
    post_update_POSTData(postData, 0xFF, 0xFF, 0xFF, 0xFF);
    return postCode;
}

/******************************************************************************
 * @fn          ltc4295_init
 *
 * @brief       Intializes PD update struct.
 *
 * @args        None
 *
 * @return      None
 */
ReturnStatus ltc4295_init(LTC4295_Dev *dev)
{
    ReturnStatus ret = RETURN_OK;
    dev->obj = (LTC4295_Obj){};

    dev->obj.mutex = GateMutex_create(NULL, NULL);
    if (!dev->obj.mutex) {
        return RETURN_NOTOK;
    }

    ret = ltc4295_get_power_good(dev, &PDStatus_Info.pdStatus.powerGoodStatus);
    if (ret != RETURN_OK) {
        LOGGER("LTC4295::ERROR: Power good signal read failed.\n");
        return ret;
    }
    if (PDStatus_Info.pdStatus.powerGoodStatus == LTC4295_POWERGOOD) {
        ret = ltc4295_get_class(dev, &PDStatus_Info.pdStatus.classStatus);
        if (ret != RETURN_OK) {
            LOGGER("LTC4295::ERROR: Reading PD classification failed.\n");
            return ret;
        }
        if (PDStatus_Info.pdStatus.classStatus == LTC4295_CLASSTYPE_POEPP) {
            PDStatus_Info.state = LTC4295_STATE_OK;
        }
    }

    if (dev->cfg.pin_evt) {
        const uint32_t pin_evt_cfg = OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_BOTH_EDGES ;
        if (OcGpio_configure(dev->cfg.pin_evt, pin_evt_cfg) < OCGPIO_SUCCESS) {
            return RETURN_NOTOK;
        }

        /* Use a threaded interrupt to handle IRQ */
       // ThreadedInt_Init(dev->cfg.pin_evt, ltc4295_handle_irq, (void *)dev);
    }
    return ret;
}

/******************************************************************************
 * @fn          ltc4295_set_alert_handler
 *
 * @brief       Set the alert callback function and context.
 *
 * @args        Device, callBack function and context
 *
 * @return      None
 */
void ltc4295_set_alert_handler(LTC4295_Dev *dev, LTC4295_CallbackFn alert_cb,
                             void *cb_context)
{
    dev->obj.alert_cb = alert_cb;
    dev->obj.cb_context = cb_context;
}

/******************************************************************************
 * @fn          ltc4295_get_power_good
 *
 * @brief       Read GPIO status based on that decide power good signal.
 *
 * @args        Addrress
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4295_get_power_good(const LTC4295_Dev *dev, ePDPowerState *val)
{
    ReturnStatus ret = RETURN_OK;
    /*set default to 1*/
    *val = LTC4295_POWERGOOD_NOTOK;

    /* Check Power Good */
    *val = (ePDPowerState) OcGpio_read(dev->cfg.pin_evt);
    if(*val == 0)
    {
        *val = LTC4295_POWERGOOD;
    }
    DEBUG("LTC4295:INFO:: PD power good is %d.\n", *val);
    return ret;
}

/******************************************************************************
 * @fn          ltc4295_get_class
 *
 * @brief       ReadGPIO status based on that decide the PD class.
 *
 * @args        Addrress
 *
 * @return      ReturnStatus
 */
ReturnStatus ltc4295_get_class(const LTC4295_Dev *dev, ePDClassType *val)
{
    ReturnStatus ret = RETURN_OK;
    uint8_t i = 0;
    uint8_t value = 1;
    uint8_t prev_value = 1;
    uint8_t toggle = 0;

    for (i = 0; i < 15; i++) {
        value = OcGpio_read(dev->cfg.pin_detect);
        LOGGER_DEBUG("LTC4295:INFO:: PD-nT2P activity status %d.\n", value);
        if (value == 1) {
            *val = LTC4295_CLASSTYPE_2;
        } else if (value == 0) {
            *val = LTC4295_CLASSTYPE_1;
        }
        /*Incremented only in the case of POE++ device*/
        if (prev_value != value) {
            toggle++;
        }
        prev_value = value;
        Task_sleep(3);
    }
    if (toggle > 2) {
        *val = LTC4295_CLASSTYPE_POEPP;
    }
    LOGGER("LTC4295:INFO:: PD detects POE of class 0x%x.\n", *val);
    return ret;
}

/******************************************************************************
 * @fn          ltc4295_update_status
 *
 * @brief       Maintains the PS status.
 *
 * @args        CLass and power good state of PD.
 *
 * @return      None
 */
void ltc4295_update_status(const LTC4295_Dev *dev)
{
    ReturnStatus ret = RETURN_NOTOK;
    ret = ltc4295_get_power_good(dev,&PDStatus_Info.pdStatus.powerGoodStatus);
    if (ret != RETURN_OK) {
        LOGGER("LTC4295::ERROR: Power good signal read failed.\n");
        return;
    }
    if (PDStatus_Info.pdStatus.powerGoodStatus == LTC4295_POWERGOOD) {
        ret = ltc4295_get_class(dev, &PDStatus_Info.pdStatus.classStatus);
        if (ret != RETURN_OK) {
            LOGGER("LTC4295::ERROR: Reading PD classification failed.\n");
            return;
        }
        if (PDStatus_Info.pdStatus.classStatus == LTC4295_CLASSTYPE_POEPP) {
            PDStatus_Info.state = LTC4295_STATE_OK;
            PDStatus_Info.pdalert = LTC4295_CONNECT_ALERT;
        }
    } else {
        PDStatus_Info.state = LTC4295_STATE_NOTOK;
        PDStatus_Info.pdalert = LTC4295_DISCONNECT_ALERT;
        PDStatus_Info.pdStatus.classStatus == LTC4295_CLASSTYPE_UNKOWN;
    }
}
