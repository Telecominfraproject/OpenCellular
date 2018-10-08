/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "GpioNative.h"

#include "inc/common/global_header.h"

#include <ti/drivers/GPIO.h>
#include <ti/sysbios/gates/GateMutex.h>

#include <stdlib.h>

static GateMutex_Handle s_cb_data_mutex;

static int GpioNative_probe(void)
{
    //This probe function is just a dummy as we are all ready accessing EC.
    return OCGPIO_SUCCESS;
}
void GpioNative_init(void)
{
    s_cb_data_mutex = GateMutex_create(NULL, NULL);
}

static int GpioNative_write(const OcGpio_Pin *pin, bool value)
{
    if (pin->hw_cfg & OCGPIO_CFG_INVERT) {
        value = !value;
    }
    GPIO_write(pin->idx, value);
    return OCGPIO_SUCCESS;
}

static int GpioNative_read(const OcGpio_Pin *pin)
{
    bool value = GPIO_read(pin->idx);
    if (pin->hw_cfg & OCGPIO_CFG_INVERT) {
        value = !value;
    }
    return value;
}

static int GpioNative_configure(const OcGpio_Pin *pin, uint32_t cfg)
{
    /* TODO: translate config values to account for inversion
     * eg. If inverted, change rising edge trigger to falling edge
     */
    /* TODO: need to be careful in multi-subscriber case - if we reconfigure,
     * the TI driver clears any interrupts on this pin, so we could potentially
     * miss edges for already subscribed pins
     */
    const OcGpio_HwCfg hw_cfg = { .uint16 = pin->hw_cfg };
    const OcGpio_ioCfg io_cfg = { .uint32 = cfg };
    uint32_t ti_cfg = 0x00;

    if (cfg & OCGPIO_CFG_INPUT) {
        ti_cfg |= GPIO_CFG_INPUT;
        ti_cfg |= (hw_cfg.in_cfg << GPIO_CFG_IO_LSB); /* Include PU/PD cfg */

        /* Process interrupt config (respect inversion settings) */
        if (hw_cfg.invert) {
            switch ((uint32_t)io_cfg.int_cfg << GPIO_CFG_INT_LSB) {
                case GPIO_CFG_IN_INT_FALLING:
                    ti_cfg |= GPIO_CFG_IN_INT_RISING;
                    break;
                case GPIO_CFG_IN_INT_RISING:
                    ti_cfg |= GPIO_CFG_IN_INT_FALLING;
                    break;
                case GPIO_CFG_IN_INT_LOW:
                    ti_cfg |= GPIO_CFG_IN_INT_HIGH;
                    break;
                case GPIO_CFG_IN_INT_HIGH:
                    ti_cfg |= GPIO_CFG_IN_INT_LOW;
                    break;

                case OCGPIO_CFG_INT_NONE:
                case GPIO_CFG_IN_INT_BOTH_EDGES:
                default:
                    /* No change */
                    ti_cfg |= (io_cfg.int_cfg << GPIO_CFG_INT_LSB);
                    break;
            }
        } else {
            ti_cfg |= (io_cfg.int_cfg << GPIO_CFG_INT_LSB);
        }
    } else {
        ti_cfg |= GPIO_CFG_OUTPUT;
        ti_cfg |=
                (hw_cfg.out_cfg << GPIO_CFG_IO_LSB); /* Include od/pu/pd cfg */
        ti_cfg |= (hw_cfg.out_str << GPIO_CFG_OUT_STRENGTH_LSB);
        ti_cfg |= (io_cfg.default_val << GPIO_CFG_OUT_BIT);
    }
    GPIO_setConfig(pin->idx, ti_cfg);
    return OCGPIO_SUCCESS;
}

/* TODO: since every GPIO driver will probably need this, might be worth
 * moving out to GPIO I/F module
 */
typedef struct GpioCallbackData {
    const OcGpio_Pin *pin;
    OcGpio_CallbackFn callback;
    void *context;
    struct GpioCallbackData *next; /*!< Pointer to next pin subscriber */
} GpioCallbackData;

/* This allows us to work around the native GPIO driver's poor design and allow
 * a context pointer to be passed to the calling function
 */
/* TODO: magic number - number of native gpio pins */
static GpioCallbackData *cb_data[120];

/* Wrapper to allow us to map TI-GPIO callback to all our subscribers (with
 * context passing)
 */
static void _nativeCallback(unsigned int idx)
{
    GpioCallbackData *cbData = cb_data[idx];
    while (cbData) {
        cbData->callback(cbData->pin, cbData->context);
        cbData = cbData->next;
    }
}

static int GpioNative_setCallback(const OcGpio_Pin *pin,
                                  OcGpio_CallbackFn callback, void *context)
{
    /* TODO: we may want to support callback removal at some point */
    if (!callback) {
        return OCGPIO_FAILURE;
    }

    GpioCallbackData *cb_entry = malloc(sizeof(GpioCallbackData));
    if (!cb_entry) {
        LOGGER_ERROR("Unable to malloc GPIO callback");
        return OCGPIO_FAILURE;
    }

    *cb_entry = (GpioCallbackData){
        .pin = pin,
        .callback = callback,
        .context = context,
    };

    /* find next blank entry & assign new callback data (protect against
     * multiple tasks accessing simultaneously)
     * Note: we don't need to worry about the actual callback function using
     * a mutex, since the 'next' pointer assignment is atomic */
    const IArg mutexKey = GateMutex_enter(s_cb_data_mutex);
    {
        GpioCallbackData **next = &cb_data[pin->idx];
        while (*next) {
            next = &(*next)->next;
        }
        *next = cb_entry;
    }
    GateMutex_leave(s_cb_data_mutex, mutexKey);
    GPIO_setCallback(pin->idx, _nativeCallback);
    return OCGPIO_SUCCESS;
}

/* TODO: what if multiple tasks are sharing a pin and one of them
 * disables the interrupt? */
static int GpioNative_disableInt(const OcGpio_Pin *pin)
{
    GPIO_disableInt(pin->idx);
    return OCGPIO_SUCCESS;
}

static int GpioNative_enableInt(const OcGpio_Pin *pin)
{
    GPIO_enableInt(pin->idx);
    return OCGPIO_SUCCESS;
}

const OcGpio_FnTable GpioNative_fnTable = {
    .probe = GpioNative_probe,
    .write = GpioNative_write,
    .read = GpioNative_read,
    .configure = GpioNative_configure,
    .setCallback = GpioNative_setCallback,
    .disableInt = GpioNative_disableInt,
    .enableInt = GpioNative_enableInt,
};
