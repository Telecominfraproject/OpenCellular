/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "GpioPCA9557.h"

#include "helpers/attribute.h"
#include "helpers/memory.h"
#include "inc/common/global_header.h"
#include "inc/devices/pca9557.h"

static int GpioPCA9557_probe(const OcGpio_Port *port)
{
    /* if we are able to read configuration register this means PCA device is accessible*/
    const PCA9557_Cfg *pca_cfg = port->cfg;
    PCA9557_Obj *obj = port->object_data;
    if (PCA9557_getConfig(&pca_cfg->i2c_dev, &obj->reg_config) != RETURN_OK) {
        return OCGPIO_FAILURE;
    }
    return OCGPIO_SUCCESS;
}

static int GpioPCA9557_init(const OcGpio_Port *port)
{
    const PCA9557_Cfg *pca_cfg = port->cfg;
    PCA9557_Obj *obj = port->object_data;

    obj->mutex = GateMutex_create(NULL, NULL);

    if (!obj->mutex) {
        return OCGPIO_FAILURE;
    }

    /* We'll set what the datasheet says are the default values as a failsafe */
    obj->reg_output = 0x00;
    obj->reg_polarity = 0xF0;
    obj->reg_config = 0xFF;

    /* Just in case, we'll read the true values */
    if (PCA9557_getOutput(&pca_cfg->i2c_dev, &obj->reg_output) != RETURN_OK ||
        PCA9557_getPolarity(&pca_cfg->i2c_dev, &obj->reg_polarity) !=
                RETURN_OK ||
        PCA9557_getConfig(&pca_cfg->i2c_dev, &obj->reg_config) != RETURN_OK) {
        return OCGPIO_FAILURE;
    }

    return OCGPIO_SUCCESS;
}

static int GpioPCA9557_write(const OcGpio_Pin *pin, bool value)
{
    const PCA9557_Cfg *pca_cfg = pin->port->cfg;
    PCA9557_Obj *obj = pin->port->object_data;
    int res = OCGPIO_FAILURE;

    /* Polarity reg. only affects input - invert output value if necessary */
    if (pin->hw_cfg & OCGPIO_CFG_INVERT) {
        value = !value;
    }

    const IArg mutexKey = GateMutex_enter(obj->mutex);
    {
        /* Update a shadow copy of the output register in case we get a
         * failure */
        uint8_t output_reg = set_bit8(obj->reg_output, pin->idx, value);
        if (PCA9557_setOutput(&pca_cfg->i2c_dev, output_reg) != RETURN_OK) {
            goto cleanup;
        }
        obj->reg_output = output_reg;
        res = OCGPIO_SUCCESS;
    }
cleanup:
    GateMutex_leave(obj->mutex, mutexKey);
    return res;
}

static int GpioPCA9557_read(const OcGpio_Pin *pin)
{
    const PCA9557_Cfg *pca_cfg = pin->port->cfg;
    PCA9557_Obj *obj = pin->port->object_data;

    /* We don't need a mutex here since i2c driver protects against
     * simultaneous access and we're just reading a value */

    /* If this is an output, read the output register instead */
    if (!(obj->reg_config & (0x01 << pin->idx))) {
        bool value = (obj->reg_output >> pin->idx) & 0x01;
        if (pin->hw_cfg & OCGPIO_CFG_INVERT) {
            value = !value;
        }
        return value;
    }

    uint8_t input_reg;
    if (PCA9557_getInput(&pca_cfg->i2c_dev, &input_reg) != RETURN_OK) {
        return OCGPIO_FAILURE;
    }
    return (input_reg >> pin->idx) & 0x01;
}

static int GpioPCA9557_configure(const OcGpio_Pin *pin, uint32_t cfg)
{
    const PCA9557_Cfg *pca_cfg = pin->port->cfg;
    PCA9557_Obj *obj = pin->port->object_data;
    int res = OCGPIO_FAILURE;

    const OcGpio_ioCfg io_cfg = { .uint32 = cfg };

    const IArg mutexKey = GateMutex_enter(obj->mutex);
    {
        /* Invert the polarity (1) if necessary (only affects inputs) */
        uint8_t polarity_reg = set_bit8(obj->reg_polarity, pin->idx,
                                        pin->hw_cfg & OCGPIO_CFG_INVERT);
        if (PCA9557_setPolarity(&pca_cfg->i2c_dev, polarity_reg) != RETURN_OK) {
            goto cleanup;
        }
        obj->reg_polarity = polarity_reg;

        /* If this is an output, configure pin initial value */
        if (io_cfg.dir == OCGPIO_CFG_OUTPUT) {
            if (GpioPCA9557_write(pin, io_cfg.default_val) < OCGPIO_SUCCESS) {
                goto cleanup;
            }
        }

        /* Set pin direction (0 output, 1 input) */
        uint8_t config_reg = set_bit8(obj->reg_config, pin->idx, io_cfg.dir);
        if (PCA9557_setConfig(&pca_cfg->i2c_dev, config_reg) != RETURN_OK) {
            goto cleanup;
        }
        obj->reg_config = config_reg;

        /* The PCA9557 doesn't support interrupts or the more advanced config
         * settings such as pull ups and drive strength, so we ignore them
         */
        res = OCGPIO_SUCCESS;
    }
cleanup:
    GateMutex_leave(obj->mutex, mutexKey);
    return res;
}

static int GpioPCA9557_setCallback(const OcGpio_Pin *pin,
                                   OcGpio_CallbackFn callback, void *context)
{
    UNUSED(pin);
    UNUSED(callback);
    UNUSED(context);
    LOGGER_ERROR("setCallback not supported by PCA9557 driver\n");
    return OCGPIO_FAILURE;
}

static int GpioPCA9557_disableInt(const OcGpio_Pin *pin)
{
    UNUSED(pin);
    LOGGER_ERROR("disableInt not supported by PCA9557 driver\n");
    return OCGPIO_FAILURE;
}

static int GpioPCA9557_enableInt(const OcGpio_Pin *pin)
{
    UNUSED(pin);
    LOGGER_ERROR("enableInt not supported by PCA9557 driver\n");
    return OCGPIO_FAILURE;
}

const OcGpio_FnTable GpioPCA9557_fnTable = {
    .probe = GpioPCA9557_probe,
    .init = GpioPCA9557_init,
    .write = GpioPCA9557_write,
    .read = GpioPCA9557_read,
    .configure = GpioPCA9557_configure,
    .setCallback = GpioPCA9557_setCallback,
    .disableInt = GpioPCA9557_disableInt,
    .enableInt = GpioPCA9557_enableInt,
};
