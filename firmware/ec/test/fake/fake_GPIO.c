/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "fake_GPIO.h"

#include "helpers/attribute.h"

#include <stdlib.h>

static bool *FakeGpio_reg;
static uint32_t *FakeGpio_cfg_reg;

void FakeGpio_registerDevSimple(void *GpioPins, void *GpioConfig)
{
    if (GpioPins) {
        FakeGpio_reg = GpioPins;
    }
    if (GpioConfig) {
        FakeGpio_cfg_reg = GpioConfig;
    }
    return;
}

static int FakeGpio_init(const OcGpio_Port *port)
{
    FakeGpio_Obj *obj = port->object_data;
    *obj = (FakeGpio_Obj){};
    return OCGPIO_SUCCESS;
}

static int FakeGpio_write(const OcGpio_Pin *pin, bool value)
{
    uint16_t final_pin = pin->idx;
    if (pin->hw_cfg & OCGPIO_CFG_INVERT) {
        value = !value;
    }
    FakeGpio_reg[final_pin] = value;
    return OCGPIO_SUCCESS;
}

static int FakeGpio_read(const OcGpio_Pin *pin)
{
    bool chk = 0;
    uint16_t final_pin = pin->idx;
    chk = FakeGpio_reg[final_pin];
    if (pin->hw_cfg & OCGPIO_CFG_INVERT) {
        chk = !chk;
    }
    return chk;
}

static int FakeGpio_configure(const OcGpio_Pin *pin, uint32_t cfg)
{
    uint16_t final_pin = pin->idx;
    FakeGpio_cfg_reg[final_pin] = cfg;
    return OCGPIO_SUCCESS;
}

static int FakeGpio_setCallback(const OcGpio_Pin *pin,
                                OcGpio_CallbackFn callback, void *context)
{
    FakeGpio_Obj *obj = pin->port->object_data;

    if (!obj) {
        return OCGPIO_FAILURE;
    }

    obj->callback[pin->idx].fn = callback;
    obj->callback[pin->idx].context = context;
    return OCGPIO_SUCCESS;
}

static int FakeGpio_disableInt(const OcGpio_Pin *pin)
{
    UNUSED(pin);
    return OCGPIO_SUCCESS;
}

static int FakeGpio_enableInt(const OcGpio_Pin *pin)
{
    UNUSED(pin);
    return OCGPIO_SUCCESS;
}

const OcGpio_FnTable FakeGpio_fnTable = {
    .init = FakeGpio_init,
    .write = FakeGpio_write,
    .read = FakeGpio_read,
    .configure = FakeGpio_configure,
    .setCallback = FakeGpio_setCallback,
    .disableInt = FakeGpio_disableInt,
    .enableInt = FakeGpio_enableInt,
};

/******************************************************************************
 * Hooks into the driver for faking things such as interrupts
 ******************************************************************************/
void FakeGpio_triggerInterrupt(const OcGpio_Pin *pin)
{
    FakeGpio_Obj *obj = pin->port->object_data;
    if (obj->callback[pin->idx].fn) {
        obj->callback[pin->idx].fn(pin, obj->callback[pin->idx].context);
    }
}
