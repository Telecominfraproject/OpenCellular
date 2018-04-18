/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#ifndef _OCMP_LTC4274_H_
#define _OCMP_LTC4274_H_

typedef union LTC4274_Config {
    struct {
        int8_t operatingMode;
        int8_t detectEnable;
        int8_t interruptMask;
        bool interruptEnable;
        int8_t pseHpEnable;
    };

} LTC4274_Config;

extern const Driver LTC4274;
#endif /* _OCMP_LTC4274_H_ */
