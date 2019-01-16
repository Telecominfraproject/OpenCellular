/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _OCMW_MSGPROC_H_
#define _OCMW_MSGPROC_H_

#include "ocmp_frame.h"

int ocmw_msgproc_send_msg(char * argv[], uint8_t action, int8_t msgtype,
                        const int8_t* paramstr, void* paramvalue);

#endif /* _OCMW_MSGPROC_H_ */
