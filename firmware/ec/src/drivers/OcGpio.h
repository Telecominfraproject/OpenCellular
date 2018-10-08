/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _OCGPIO_H_
#define _OCGPIO_H_

#include <stdbool.h>
#include <stdint.h>

/* OC-GPIO functions will return a negative on failure */
#define OCGPIO_SUCCESS 0
#define OCGPIO_FAILURE -1

typedef struct OcGpio_Pin OcGpio_Pin;
typedef struct OcGpio_Port OcGpio_Port;

typedef void (*OcGpio_CallbackFn)(const OcGpio_Pin *pin, void *context);

/* Interface virtual function definitions */
typedef int (*OcGpio_initFn)(const OcGpio_Port *port);
typedef int (*OcGpio_writeFn)(const OcGpio_Pin *pin, bool value);
typedef int (*OcGpio_readFn)(const OcGpio_Pin *pin);
typedef int (*OcGpio_configFn)(const OcGpio_Pin *pin, uint32_t cfg);
typedef int (*OcGpio_setCallbackFn)(const OcGpio_Pin *pin,
                                    OcGpio_CallbackFn callback, void *context);
typedef int (*OcGpio_disableIntFn)(const OcGpio_Pin *pin);
typedef int (*OcGpio_enableIntFn)(const OcGpio_Pin *pin);

typedef struct OcGpio_FnTable {
    OcGpio_initFn probe;
    OcGpio_initFn init; /*!< Port initialization - called once */
    OcGpio_writeFn write;
    OcGpio_readFn read;
    OcGpio_configFn configure;
    OcGpio_setCallbackFn setCallback;
    OcGpio_disableIntFn disableInt;
    OcGpio_enableIntFn enableInt;
} OcGpio_FnTable;

/*! A port defines a specific driver instance to route through */
struct OcGpio_Port {
    const OcGpio_FnTable *fn_table; /*!< virtual table for driver */
    const void *cfg; /*!< driver-specific config settings */
    void *object_data; /*!< driver-specific data (in RAM) */
};

/*! A pin provides us with everything we need to route data to the appropriate
 * driver - a port instance, a pin index, and board-sprcific configuration
 * settings
 */
struct OcGpio_Pin {
    const OcGpio_Port *port; /*!< Pointer to IO driver instance */
    uint16_t idx; /*!< Driver-specific index */
    uint16_t hw_cfg; /*!< Any special attributes for the pin (eg. invert) */
};

/*
 * OC GPIO hardware configuration settings - these are to be used in the board
 * config file since they're largely depicted by the layout. One may provide
 * separate input and output configurations for bi-directional pins
 *
 * Note: these definitions are compatible with the TI GPIO driver configs with
 * a small amount of shifting
 * ============================================================================
 */
typedef union OcGpio_HwCfg {
    struct {
        bool invert : 1;
        uint16_t in_cfg : 3;
        uint16_t out_cfg : 3;
        uint16_t out_str : 2;
    };
    uint16_t uint16;
} OcGpio_HwCfg;

#define OCGPIO_CFG_POL_MASK (((uint32_t)1) << OCGPIO_CFG_POL_LSB)
#define OCGPIO_CFG_IN_PULL_MASK (((uint32_t)6) << OCGPIO_CFG_IN_LSB)
#define OCGPIO_CFG_OUT_OD_MASK (((uint32_t)7) << OCGPIO_CFG_OUT_LSB)
#define OCGPIO_CFG_OUT_STR_MASK (((uint32_t)3) << OCGPIO_CFG_OUT_STR_LSB)

#define OCGPIO_CFG_POL_LSB 0
#define OCGPIO_CFG_IN_LSB 1
#define OCGPIO_CFG_OUT_LSB 4
#define OCGPIO_CFG_OUT_STR_LSB 7

#define OCGPIO_CFG_POL_STD \
    (((uint32_t)0) << OCGPIO_CFG_POL_LSB) /*!< Standard polarity */
#define OCGPIO_CFG_INVERT \
    (((uint32_t)1) << OCGPIO_CFG_POL_LSB) /*!< Polarity inverted */

#define OCGPIO_CFG_IN_NOPULL \
    (((uint32_t)0) << OCGPIO_CFG_IN_LSB) /*!< Input pin has no PU/PD */
#define OCGPIO_CFG_IN_PU \
    (((uint32_t)2) << OCGPIO_CFG_IN_LSB) /*!< Input pin has Pullup */
#define OCGPIO_CFG_IN_PD \
    (((uint32_t)4) << OCGPIO_CFG_IN_LSB) /*!< Input pin has Pulldown */

#define OCGPIO_CFG_OUT_STD \
    (((uint32_t)0) << OCGPIO_CFG_OUT_LSB) /*!< Output pin is not Open Drain */
#define OCGPIO_CFG_OUT_OD_NOPULL \
    (((uint32_t)2) << OCGPIO_CFG_OUT_LSB) /*!< Output pin is Open Drain */
#define OCGPIO_CFG_OUT_OD_PU \
    (((uint32_t)4)           \
     << OCGPIO_CFG_OUT_LSB) /*!< Output pin is Open Drain w/ pull up */
#define OCGPIO_CFG_OUT_OD_PD \
    (((uint32_t)6)           \
     << OCGPIO_CFG_OUT_LSB) /*!< Output pin is Open Drain w/ pull dn */

#define OCGPIO_CFG_OUT_STR_LOW \
    (((uint32_t)0) << OCGPIO_CFG_OUT_STR_LSB) /*!< Low drive strength */
#define OCGPIO_CFG_OUT_STR_MED \
    (((uint32_t)1) << OCGPIO_CFG_OUT_STR_LSB) /*!< Medium drive strength */
#define OCGPIO_CFG_OUT_STR_HIGH \
    (((uint32_t)2) << OCGPIO_CFG_OUT_STR_LSB) /*!< High drive strength */

/*
 * OC GPIO I/O configuration settings - these are to be used by drivers that
 * control GPIO pins. These are settings that are generic enough that every
 * GPIO driver should support. These settings aren't layout specific, but are
 * instead dictated by the higher level driver (see OcGpio_configure)
 * ============================================================================
 */
typedef union OcGpio_ioCfg {
    struct {
        uint32_t dir : 1;
        uint32_t default_val : 1;
        uint32_t int_cfg : 3;
    };
    uint32_t uint32;
} OcGpio_ioCfg;

#define OCGPIO_CFG_DIR_BIT 0
#define OCGPIO_CFG_OUT_BIT 1
#define OCGPIO_CFG_INT_LSB 2

#define OCGPIO_CFG_OUTPUT \
    (((uint32_t)0) << OCGPIO_CFG_DIR_BIT) /*!< Pin is an output. */
#define OCGPIO_CFG_INPUT \
    (((uint32_t)1) << OCGPIO_CFG_DIR_BIT) /*!< Pin is an input. */

#define OCGPIO_CFG_OUT_LOW \
    (((uint32_t)0) << OCGPIO_CFG_OUT_BIT) /*!< Output low by default */
#define OCGPIO_CFG_OUT_HIGH \
    (((uint32_t)1) << OCGPIO_CFG_OUT_BIT) /*!< Output high by default */

#define OCGPIO_CFG_INT_NONE \
    (((uint32_t)0) << OCGPIO_CFG_INT_LSB) /*!< No Interrupt */
#define OCGPIO_CFG_INT_FALLING \
    (((uint32_t)1) << OCGPIO_CFG_INT_LSB) /*!< Interrupt on falling edge */
#define OCGPIO_CFG_INT_RISING \
    (((uint32_t)2) << OCGPIO_CFG_INT_LSB) /*!< Interrupt on rising edge */
#define OCGPIO_CFG_INT_BOTH_EDGES \
    (((uint32_t)3) << OCGPIO_CFG_INT_LSB) /*!< Interrupt on both edges */
#define OCGPIO_CFG_INT_LOW \
    (((uint32_t)4) << OCGPIO_CFG_INT_LSB) /*!< Interrupt on low level */
#define OCGPIO_CFG_INT_HIGH \
    (((uint32_t)5) << OCGPIO_CFG_INT_LSB) /*!< Interrupt on high level */

/* Wrapper functions to dispatch call to appropriate v-table
 * ==================================================================
 */

/* Since this is simply an interface definition, define the functions in the
 * header to allow them to be inlined for better efficiency. Any functions added
 * to this module that are more than a simple wrapper should be added to
 * OcGpio.c instead.
 */

/*! Probe the device for POStT
 * probe function
 * @param pin OcGPio_Port pointer to the driver instance to find the device
 * @return 0 on success, negative on failure
 */
static inline int OcGpio_probe(const OcGpio_Port *port)
{
    if (port && port->fn_table && port->fn_table->probe) {
        return port->fn_table->probe(port);
    } else {
        return OCGPIO_FAILURE;
    }
}

/*! Initialize the port - tempted to remove in favor of more generic device
 * init function
 * @param pin OcGPio_Port pointer to the driver instance to initialize
 * @return 0 on success, negative on failure
 */
static inline int OcGpio_init(const OcGpio_Port *port)
{
    if (port && port->fn_table && port->fn_table->init) {
        return port->fn_table->init(port);
    } else {
        return OCGPIO_FAILURE;
    }
}

/*! Write a value to a GPIO pin
 * @param pin OcGPio_Pin pointer for the pin to be controlled
 * @param value Boolean value to write to the pin
 * @return 0 on success, negative on failure
 */
static inline int OcGpio_write(const OcGpio_Pin *pin, bool value)
{
    if (pin && pin->port && pin->port->fn_table && pin->port->fn_table->write) {
        return pin->port->fn_table->write(pin, value);
    } else {
        return OCGPIO_FAILURE;
    }
}

/*! Read a value from a GPIO pin
 * @param pin OcGPio_Pin pointer for the pin to read
 * @return Boolean value of pin, or negative if failure
 */
static inline int OcGpio_read(const OcGpio_Pin *pin)
{
    if (pin && pin->port && pin->port->fn_table && pin->port->fn_table->read) {
        return pin->port->fn_table->read(pin);
    } else {
        return OCGPIO_FAILURE;
    }
}

/*! Configure a GPIO pin's parameters (both io and hw params)
 * @note This must be called before using the pin
 * @param pin OcGPio_Pin pointer to the pin to configure
 * @param cfg Bitfield of OCGPIO_CFG io values (direction, interrupt edge)
 * @return 0 on success, negative on failure
 */
static inline int OcGpio_configure(const OcGpio_Pin *pin, uint32_t cfg)
{
    if (pin && pin->port && pin->port->fn_table &&
        pin->port->fn_table->configure) {
        return pin->port->fn_table->configure(pin, cfg);
    } else {
        return OCGPIO_FAILURE;
    }
}

/*! Add a callback subscriber to an interrupt-enabled pin
 * @note Multiple callbacks can be subscribed to a single pin
 * @param pin OcGPio_Pin pointer to subscribe to changes on
 * @param callback Function to call when interrupt is triggered
 * @param context Context pointer that is passed to callback function
 * @return 0 on success, negative on failure
 */
static inline int OcGpio_setCallback(const OcGpio_Pin *pin,
                                     OcGpio_CallbackFn callback, void *context)
{
    if (pin && pin->port && pin->port->fn_table &&
        pin->port->fn_table->setCallback) {
        return pin->port->fn_table->setCallback(pin, callback, context);
    } else {
        return OCGPIO_FAILURE;
    }
}

/*! Disable pin interrupt
 * @param pin OcGPio_Pin pointer for the pin to disable the interrupt on
 * @return 0 on success, negative on failure
 */
static inline int OcGpio_disableInt(const OcGpio_Pin *pin)
{
    if (pin && pin->port && pin->port->fn_table &&
        pin->port->fn_table->disableInt) {
        return pin->port->fn_table->disableInt(pin);
    } else {
        return OCGPIO_FAILURE;
    }
}

/*! Enable pin interrupt
 * @note This must be called after OcGpio_setCallback in order to activate the
 *       interrupt
 * @param pin OcGPio_Pin pointer for the pin to enable the interrupt on
 * @return 0 on success, negative on failure
 */
static inline int OcGpio_enableInt(const OcGpio_Pin *pin)
{
    if (pin && pin->port && pin->port->fn_table &&
        pin->port->fn_table->enableInt) {
        return pin->port->fn_table->enableInt(pin);
    } else {
        return OCGPIO_FAILURE;
    }
}

#endif /* _OCGPIO_H_ */
