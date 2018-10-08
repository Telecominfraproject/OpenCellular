/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "inc/ocmp_wrappers/ocmp_debugocgpio.h"

#include "common/inc/global/Framework.h"
#include "common/inc/global/ocmp_frame.h"
#include "inc/common/global_header.h"
#include "inc/devices/debug_ocgpio.h"

#include <ti/drivers/GPIO.h>

#define NO_GPIO_PINS_IN_GROUP 8
extern GPIO_PinConfig gpioPinConfigs[];

bool ocgpio_set(void *gpio_cfg, void *oc_gpio)
{
    S_OCGPIO_Cfg *oc_gpio_cfg = (S_OCGPIO_Cfg *)gpio_cfg;
    S_OCGPIO *s_oc_gpio = (S_OCGPIO *)oc_gpio;
    int ret = 0;
    uint8_t idx = ((oc_gpio_cfg->group != 0) ?
                           (((oc_gpio_cfg->group - 1) * NO_GPIO_PINS_IN_GROUP) +
                            s_oc_gpio->pin) :
                           s_oc_gpio->pin);
    OcGpio_Pin ocgpio = { (oc_gpio_cfg->port), idx,
                          ((oc_gpio_cfg->group != 0) ?
                                   (gpioPinConfigs[idx] >> 16) :
                                   OCGPIO_CFG_OUT_STD) };
    ret = OcGpio_configure(&ocgpio, OCGPIO_CFG_OUTPUT);
    ret = OcGpio_write(&ocgpio, s_oc_gpio->value);
    return (ret == 0);
}

bool ocgpio_get(void *gpio_cfg, void *oc_gpio)
{
    S_OCGPIO_Cfg *oc_gpio_cfg = (S_OCGPIO_Cfg *)gpio_cfg;
    S_OCGPIO *s_oc_gpio = (S_OCGPIO *)oc_gpio;
    int ret = 0;
    uint8_t idx = ((oc_gpio_cfg->group != 0) ?
                           (((oc_gpio_cfg->group - 1) * NO_GPIO_PINS_IN_GROUP) +
                            s_oc_gpio->pin) :
                           s_oc_gpio->pin);
    OcGpio_Pin ocgpio = { (oc_gpio_cfg->port), idx,
                          ((oc_gpio_cfg->group != 0) ?
                                   (gpioPinConfigs[idx] >> 16) :
                                   OCGPIO_CFG_IN_PU) };
    ret = OcGpio_configure(&ocgpio, OCGPIO_CFG_INPUT);
    s_oc_gpio->value = OcGpio_read(&ocgpio);
    if (s_oc_gpio->value < 0) {
        ret = -1;
    }
    return (ret == 0);
}

static ePostCode _probe(S_OCGPIO_Cfg *oc_gpio_cfg)
{
    if (OcGpio_probe(oc_gpio_cfg->port) != 0) {
        return POST_DEV_MISSING;
    } else {
        return POST_DEV_FOUND;
    }
}

static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    //Dummy functions.
    return POST_DEV_CFG_DONE;
}

const Driver_fxnTable DEBUG_OCGPIO_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,
    .cb_init = _init,
};
