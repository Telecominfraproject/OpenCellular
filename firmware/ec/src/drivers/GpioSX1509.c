/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "GpioSX1509.h"

#include "devices/i2c/threaded_int.h"
#include "inc/common/global_header.h"
#include "inc/devices/sx1509.h"
#include "helpers/memory.h"

#include <stdlib.h>
#include <string.h>

/* Helper functions to perform bank switching */
static sx1509RegType GetBank(uint8_t pin_idx) {
    return (pin_idx > 7) ? SX1509_REG_B : SX1509_REG_A;
}

static uint8_t RelativePinIdx(uint8_t pin_idx) {
    return (pin_idx > 7) ? (pin_idx - 8) : pin_idx;
}

/* IRQ handler - reads in triggered interrupts and dispatches */
static void HandleIRQ(void *context) {
    const OcGpio_Port *port = context;
    const SX1509_Cfg *sx_cfg = port->cfg;
    SX1509_Obj *obj = port->object_data;

    /* Figure out which pin this interrupt came from */
    /* TODO: this seems risky - what if an interrupt occurs between reading
     * src and clearing it? It's dumb that the IC doesn't clear the irq on
     * reading the source */
    uint16_t irq_src;
    ioexp_led_get_interrupt_source(&sx_cfg->i2c_dev, &irq_src);
    ioexp_led_clear_interrupt_source(&sx_cfg->i2c_dev);

    // Dispatch the interrupt handlers
    int pin_idx = 0;
    while (irq_src) {
        if (irq_src & 0x01) {
            SX1509CallbackData *cbData = obj->cb_data[pin_idx];
            while (cbData) {
                cbData->callback(cbData->pin, cbData->context);
                cbData = cbData->next;
            }
        }
        ++pin_idx;
        irq_src >>= 1;
    }
}

static int GpioSX1509_probe(const OcGpio_Port *port) {
    /* if we are able to read configuration register this means PCA device is accessible*/
    const SX1509_Cfg *sx_cfg = port->cfg;
    uint8_t input_reg;
    if (ioexp_led_get_data(&sx_cfg->i2c_dev, 0, &input_reg) != RETURN_OK) {
        return OCGPIO_FAILURE;
    }
    return OCGPIO_SUCCESS;
}

static int GpioSX1509_init(const OcGpio_Port *port) {
    const SX1509_Cfg *sx_cfg = port->cfg;
    SX1509_Obj *obj = port->object_data;

    obj->mutex = GateMutex_create(NULL, NULL);
    for (int i = 0; i < SX1509_NUM_BANKS; ++i) {
        obj->regs[i] = (SX1509_Registers){
            /* We only need to set the non-zero registers */
            .direction = 0xff,
            .data = 0xff,
            .int_mask = 0xff,
        };
    }

    memset(obj->cb_data, 0, sizeof(obj->cb_data));

    /* Make sure the IC is set to default config */
    if (ioexp_led_software_reset(&sx_cfg->i2c_dev)
            != RETURN_OK) {
        return OCGPIO_FAILURE;
    }

    /* Register the SX1509's own IRQ pin (optional) */
    if (sx_cfg->pin_irq) {
        const uint32_t pin_irq_cfg = OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_FALLING;
        if (OcGpio_configure(sx_cfg->pin_irq, pin_irq_cfg) < OCGPIO_SUCCESS) {
            return OCGPIO_FAILURE;
        }

        /* Use a threaded interrupt to handle IRQ */
        ThreadedInt_Init(sx_cfg->pin_irq, HandleIRQ, (void *)port);
    }
    return OCGPIO_SUCCESS;
}

static int GpioSX1509_write(const OcGpio_Pin *pin, bool value) {
    const SX1509_Cfg *sx_cfg = pin->port->cfg;
    SX1509_Obj *obj = pin->port->object_data;
    int res = OCGPIO_FAILURE;

    const IArg mutexKey = GateMutex_enter(obj->mutex);
    {
        /* Reading the value of output pins seems unreliable on the SX1509,
         * so we cache the output value instead (it's faster anyway)
         */
        const sx1509RegType bank = GetBank(pin->idx);
        const uint8_t pin_idx = RelativePinIdx(pin->idx);
        const uint8_t new_reg_value = set_bit8(obj->regs[bank].data,
                                               pin_idx, value);
        if (ioexp_led_set_data(&sx_cfg->i2c_dev, bank, new_reg_value, 0x00)
                != RETURN_OK) {
            goto cleanup;
        }
        obj->regs[bank].data = new_reg_value;
        res = OCGPIO_SUCCESS;
    }
cleanup:
    GateMutex_leave(obj->mutex, mutexKey);
    return res;
}

static int GpioSX1509_read(const OcGpio_Pin *pin) {
    const SX1509_Cfg *sx_cfg = pin->port->cfg;

    /* We don't need a mutex here since i2c driver protects against
     * simultaneous access and we're just reading a value */
    const sx1509RegType bank = GetBank(pin->idx);
    const uint8_t pin_idx = RelativePinIdx(pin->idx);
    uint8_t input_reg;
    if (ioexp_led_get_data(&sx_cfg->i2c_dev, bank, &input_reg) != RETURN_OK) {
        return OCGPIO_FAILURE;
    }

    return (input_reg >> pin_idx) & 0x01;
}

/* TODO: this mapping is pretty gross with the shifts */
static const uint8_t EDGE_SENSE_MAP[] = {
    [OCGPIO_CFG_INT_NONE >> OCGPIO_CFG_INT_LSB]       = 0x00, /* 00 */
    [OCGPIO_CFG_INT_RISING >> OCGPIO_CFG_INT_LSB]     = 0x01, /* 01 */
    [OCGPIO_CFG_INT_FALLING >> OCGPIO_CFG_INT_LSB]    = 0x02, /* 10 */
    [OCGPIO_CFG_INT_BOTH_EDGES >> OCGPIO_CFG_INT_LSB] = 0x03, /* 11 */
};

/* TODO: handle things nicely if we get a failure part way through config -
 * right now we break the rule of the other functions to only store the new
 * value if the IC accepted the changed value */
static int GpioSX1509_configure(const OcGpio_Pin *pin, uint32_t cfg) {
    const SX1509_Cfg *sx_cfg = pin->port->cfg;
    SX1509_Obj *obj = pin->port->object_data;
    int res = OCGPIO_FAILURE;

    const OcGpio_ioCfg io_cfg = { .uint32 = cfg };

    const sx1509RegType bank = GetBank(pin->idx);
    const uint8_t pin_idx = RelativePinIdx(pin->idx);
    SX1509_Registers *reg = &obj->regs[bank];

    const IArg mutexKey = GateMutex_enter(obj->mutex);
    {
        /* Invert the polarity (1) if necessary */
        reg->polarity = set_bit8(reg->polarity, pin_idx,
                                 pin->hw_cfg & OCGPIO_CFG_INVERT);
        if (ioexp_led_config_polarity(&sx_cfg->i2c_dev, bank, reg->polarity,
                                      0x00) != RETURN_OK) {
            goto cleanup;
        }

        bool pu_en;
        bool pd_en;

        /* Set pull-up/down registers */
        /* Set output-specific registers if applicable */
        if (io_cfg.dir == OCGPIO_CFG_OUTPUT) {
            /* Enable (1) open drain */
            const bool od_en = (pin->hw_cfg & OCGPIO_CFG_OUT_OD_MASK);
            reg->open_drain = set_bit8(reg->open_drain, pin_idx, od_en);
            if (ioexp_led_config_opendrain(&sx_cfg->i2c_dev,
                                           bank,
                                           reg->open_drain,
                                           0x00) != RETURN_OK) {
                goto cleanup;
            }

            /* Disable (1) the input buffer */
            reg->input_buf_disable = set_bit8(reg->input_buf_disable, pin_idx,
                                              1);
            if (ioexp_led_config_inputbuffer(&sx_cfg->i2c_dev,
                                             bank,
                                             reg->input_buf_disable,
                                             0x00) != RETURN_OK) {
                goto cleanup;
            }

            /* Set default value */
            GpioSX1509_write(pin, io_cfg.default_val);

            /* TODO: this is kind of gross, not sure if it's worth keeping
             * compatibility with TI-GPIO cfg */
            pu_en = ((pin->hw_cfg & OCGPIO_CFG_OUT_OD_MASK) ==
                        OCGPIO_CFG_OUT_OD_PU);
            pd_en = ((pin->hw_cfg & OCGPIO_CFG_OUT_OD_MASK) ==
                        OCGPIO_CFG_OUT_OD_PD);
        } else {
            /* Enable (0) the input buffer */
            reg->input_buf_disable = set_bit8(reg->input_buf_disable, pin_idx,
                                              0);
            if (ioexp_led_config_inputbuffer(&sx_cfg->i2c_dev,
                                             bank,
                                             reg->input_buf_disable,
                                             0x00) != RETURN_OK) {
                goto cleanup;
            }

            /* Set interrupt edge detection */
            /* This is a bit tricky since we need to set a pair of bits */
            const uint16_t EDGE_SENSE_MASK = 0x03 << (pin_idx * 2);
            reg->edge_sense &= ~EDGE_SENSE_MASK;
            reg->edge_sense |= EDGE_SENSE_MAP[io_cfg.int_cfg] << (pin_idx * 2);

            switch (bank) {
                case SX1509_REG_A:
                    if (ioexp_led_config_edge_sense_A(
                            &sx_cfg->i2c_dev,
                            SX1509_EDGE_SENSE_REG_LOW_HIGH,
                            LOBYTE(reg->edge_sense),
                            HIBYTE(reg->edge_sense)) != RETURN_OK) {
                        goto cleanup;
                    }
                    break;
                case SX1509_REG_B:
                    if (ioexp_led_config_edge_sense_B(
                            &sx_cfg->i2c_dev,
                            SX1509_EDGE_SENSE_REG_LOW_HIGH,
                            LOBYTE(reg->edge_sense),
                            HIBYTE(reg->edge_sense)) != RETURN_OK) {
                        goto cleanup;
                    }
                    break;
                default:
                    LOGGER_ERROR("SX1509: Unknown bank number: %d\n", bank);
                    goto cleanup;
            }

            pu_en = ((pin->hw_cfg & OCGPIO_CFG_IN_PULL_MASK) ==
                        OCGPIO_CFG_IN_PU);
            pd_en = ((pin->hw_cfg & OCGPIO_CFG_IN_PULL_MASK) ==
                        OCGPIO_CFG_IN_PD);
        }

        /* Set pull-up/down registers */
        reg->pull_up = set_bit8(reg->pull_up, pin_idx, pu_en);
        if (ioexp_led_config_pullup(&sx_cfg->i2c_dev,
                                    bank,
                                    reg->pull_up,
                                    0x00) != RETURN_OK) {
            goto cleanup;
        }

        reg->pull_down = set_bit8(reg->pull_down, pin_idx, pd_en);
        if (ioexp_led_config_pulldown(&sx_cfg->i2c_dev,
                                      bank,
                                      reg->pull_down,
                                      0x00) != RETURN_OK) {
            goto cleanup;
        }

        /* Set pin direction (0 output, 1 input) */
        reg->direction = set_bit8(reg->direction, pin_idx, io_cfg.dir);
        if (ioexp_led_config_data_direction(&sx_cfg->i2c_dev,
                                            bank,
                                            reg->direction,
                                            0x00) != RETURN_OK) {
            goto cleanup;
        }

        /* The SX1509 doesn't support drive strength */
        res = OCGPIO_SUCCESS;
    }
cleanup:
    GateMutex_leave(obj->mutex, mutexKey);
    return res;
}

static int GpioSX1509_setCallback(const OcGpio_Pin *pin,
                                  OcGpio_CallbackFn callback,
                                  void *context) {
    SX1509_Obj *obj = pin->port->object_data;

    /* TODO: we may want to support callback removal at some point */
    if (!callback) {
        return OCGPIO_FAILURE;
    }

    SX1509CallbackData *cb_entry = malloc(sizeof(SX1509CallbackData));
    if (!cb_entry) {
        LOGGER_ERROR("Unable to malloc GPIO callback");
        return OCGPIO_FAILURE;
    }

    *cb_entry = (SX1509CallbackData){
        .pin = pin,
        .callback = callback,
        .context = context,
    };

    /* find next blank entry & assign new callback data (protect against
     * multiple tasks accessing simultaneously)
     * Note: we don't need to worry about the actual callback function using
     * a mutex, since the 'next' pointer assignment is atomic */
    const IArg mutexKey = GateMutex_enter(obj->mutex);
    {
        SX1509CallbackData **next = &obj->cb_data[pin->idx];
        while (*next) {
            next = &(*next)->next;
        }
        *next = cb_entry;
    }
    GateMutex_leave(obj->mutex, mutexKey);
    return OCGPIO_SUCCESS;
}

static int GpioSX1509_disableInt(const OcGpio_Pin *pin) {
    const SX1509_Cfg *sx_cfg = pin->port->cfg;
    SX1509_Obj *obj = pin->port->object_data;
    int res = OCGPIO_FAILURE;

    const sx1509RegType bank = GetBank(pin->idx);
    const uint8_t pin_idx = RelativePinIdx(pin->idx);
    SX1509_Registers *reg = &obj->regs[bank];

    /* Disable (1) interrupt */
    const IArg mutexKey = GateMutex_enter(obj->mutex);
    {
        const uint8_t new_reg_value = set_bit8(reg->int_mask, pin_idx, 1);
        if (ioexp_led_config_interrupt(&sx_cfg->i2c_dev,
                                       bank,
                                       new_reg_value,
                                       0x00) != RETURN_OK) {
            goto cleanup;
        }
        reg->int_mask = new_reg_value;
        res = OCGPIO_SUCCESS;
    }
cleanup:
    GateMutex_leave(obj->mutex, mutexKey);
    return res;
}

static int GpioSX1509_enableInt(const OcGpio_Pin *pin) {
    const SX1509_Cfg *sx_cfg = pin->port->cfg;
    SX1509_Obj *obj = pin->port->object_data;
    int res = OCGPIO_FAILURE;

    const sx1509RegType bank = GetBank(pin->idx);
    const uint8_t pin_idx = RelativePinIdx(pin->idx);
    SX1509_Registers *reg = &obj->regs[bank];

    /* Enable (0) interrupt */
    const IArg mutexKey = GateMutex_enter(obj->mutex);
    {
        const uint8_t new_reg_value = set_bit8(reg->int_mask, pin_idx, 0);
        if (ioexp_led_config_interrupt(&sx_cfg->i2c_dev,
                                       bank,
                                       new_reg_value,
                                       0x00) != RETURN_OK) {
            goto cleanup;
        }
        reg->int_mask = new_reg_value;
        res = OCGPIO_SUCCESS;
    }
cleanup:
    GateMutex_leave(obj->mutex, mutexKey);
    return res;
}

const OcGpio_FnTable GpioSX1509_fnTable = {
    .probe = GpioSX1509_probe,
    .init = GpioSX1509_init,
    .write = GpioSX1509_write,
    .read = GpioSX1509_read,
    .configure = GpioSX1509_configure,
    .setCallback = GpioSX1509_setCallback,
    .disableInt = GpioSX1509_disableInt,
    .enableInt = GpioSX1509_enableInt,
};
