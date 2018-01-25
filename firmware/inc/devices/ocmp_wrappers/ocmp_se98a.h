/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef _OCMP_SE98A_H
#define _OCMP_SE98A_H

typedef union SE98A_Config {
    struct {
        int8_t lowlimit;
        int8_t highlimit;
        int8_t critlimit;
    };
    int8_t limits[3];
} SE98A_Config;

extern const Driver SE98A;

#endif /* _OCMP_SE98A_H */
