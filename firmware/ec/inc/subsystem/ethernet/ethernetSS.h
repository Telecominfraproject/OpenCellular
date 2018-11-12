/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef ETHERNETSS_H_
#define ETHERNETSS_H_

#include <stdbool.h>

void ethernet_switch_setup();
bool eth_sw_pre_init(void **driver, void *returnValue);

#endif /* ETHERNETSS_H_ */
