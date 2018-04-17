/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef _OCMP_ADT7481_H
#define _OCMP_ADT7481_H

typedef union ADT7481_Config {
    struct {
        int8_t lowlimit;
        int8_t highlimit;
        int8_t critlimit;
    };
    int8_t limits[3];
} ADT7481_Config;

extern const Driver ADT7481;

#endif /* _OCMP_ADT7481_H */
