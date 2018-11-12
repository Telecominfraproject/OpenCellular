/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
/*****************************************************************************
 *                                HEADER FILES
 *****************************************************************************/
#include "inc/subsystem/hci/hci.h"

#include "registry/SSRegistry.h"

/*****************************************************************************
 *                             HANDLES DEFINITION
 *****************************************************************************/
/* Global Task Configuration Variables */

bool HCI_Init(void *driver, void *return_buf)
{
    HciBuzzer_init();
    return true;
}
