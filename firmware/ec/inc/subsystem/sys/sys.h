/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef _SYS_H
#define _SYS_H
#include "common/inc/global/Framework.h"

#include <stdbool.h>
#include <ti/sysbios/knl/Task.h>

bool SYS_cmdReset(void *driver, void *params);
bool SYS_cmdEcho(void *driver, void *params);
bool SYS_post_enable(void **postActivate);
bool SYS_post_get_results(void **getpostResultMsg);
extern const Driver Driver_EepromSID;
extern const Driver Driver_EepromInv;
extern const Driver Driver_MAC;

#endif /* _SYS_H */
