/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "PinGroup.h"

ReturnStatus PinGroup_configure(const PinGroup *group, uint32_t cfg)
{
    for (int i = 0; i < group->num_pin; ++i) {
        if (group->pins[i].port) {
            if (OcGpio_configure(&group->pins[i], cfg) != OCGPIO_SUCCESS) {
                return RETURN_NOTOK;
            }
        }
    }
    return RETURN_OK;
}

ReturnStatus PinGroup_write(const PinGroup *group, uint8_t value)
{
    for (int i = 0; i < group->num_pin; ++i) {
        if (group->pins[i].port) {
            /* FIXME: There isn't a great way to gracefully handle failure */
            if (OcGpio_write(&group->pins[i], (value >> i) & 0x01) !=
                OCGPIO_SUCCESS) {
                return RETURN_NOTOK;
            }
        }
    }
    return RETURN_OK;
}

ReturnStatus PinGroup_read(const PinGroup *group, uint8_t *value)
{
    *value = 0;
    for (int i = 0; i < group->num_pin; ++i) {
        if (group->pins[i].port) {
            int pin_val = OcGpio_read(&group->pins[i]);
            if (pin_val > OCGPIO_FAILURE) {
                *value |= (pin_val << i);
            } else {
                return RETURN_NOTOK;
            }
        }
    }
    return RETURN_OK;
}
