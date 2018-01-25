/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef _OCMP_INA226_H
#define _OCMP_INA226_H

typedef struct INA226_Config {
    uint16_t current_lim;
} INA226_Config;

extern const Driver INA226;

#endif /* _OCMP_INA226_H */
