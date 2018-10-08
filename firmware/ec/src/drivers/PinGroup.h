/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _PIN_GROUP_H_
#define _PIN_GROUP_H_

/* Helper to allow a collection of GPIO pins to be written to with
 * a single integer
 *
 * @note: Output is not guaranteed to be simultaneously updated right now
 */

#include "inc/common/global_header.h"
#include "OcGpio.h"

typedef struct PinGroup {
    int num_pin;
    const OcGpio_Pin *pins;
} PinGroup;

/*! Configure a group of OC-GPIO Pins
 * @param group Pointer to the PinGroup to configure
 * @param cfg OcGpio configuration value to set
 * @return Standard ReturnStatus codes
 *
 * @note All pins in the group receive the same cfg setting
 */
ReturnStatus PinGroup_configure(const PinGroup *group, uint32_t cfg);

/*! Write to a group of OC-GPIO Pins
 * @param group Pointer to the PinGroup to configure
 * @param value Binary value to write out to the pins
 * @return Standard ReturnStatus codes
 */
ReturnStatus PinGroup_write(const PinGroup *group, uint8_t value);

/*! Read from a group of OC-GPIO Pins
 * @param group Pointer to the PinGroup to configure
 * @param value Pointer to read value
 * @return Standard ReturnStatus codes
 */
ReturnStatus PinGroup_read(const PinGroup *group, uint8_t *value);

#endif /* _PIN_GROUP_H_ */
