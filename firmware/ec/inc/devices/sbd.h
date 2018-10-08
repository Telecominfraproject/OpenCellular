/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef INC_DEVICES_SBD_H_
#define INC_DEVICES_SBD_H_

#include <stdint.h>

typedef enum {
    IRIDIUM_IMEI = 0,
    IRIDIUM_MFG = 1,
    IRIDIUM_MODEL = 2,
    IRIDIUM_SIG_QUALITY = 3,
    IRIDIUM_REGSTATUS = 4,
    IRIDIUM_NO_OUT_MSG = 5,
    IRIDIUM_LASTERR = 6,
    IRIDIUM_PARAM_MAX /* Limiter */
} eOBC_StatusParam;

typedef enum {
    ERR_RC_INTERNAL = 0,
    ERR_SRC_CMS = 1,
    ERR_SRC_CME = 2
} eOBC_ErrorSource;

typedef struct OBC_lastError {
    eOBC_ErrorSource src;
    uint16_t code;
} OBC_lastError;

#endif /* INC_DEVICES_SBD_H_ */
