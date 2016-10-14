/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_RSA_H_
#define VBOOT_REFERENCE_RSA_H_

#ifndef VBOOT_REFERENCE_CRYPTOLIB_H_
#error "Do not include this file directly. Use cryptolib.h instead."
#endif

#include "sysincludes.h"

#define RSA1024NUMBYTES 128  /* 1024 bit key length */
#define RSA2048NUMBYTES 256  /* 2048 bit key length */
#define RSA4096NUMBYTES 512  /* 4096 bit key length */
#define RSA8192NUMBYTES 1024  /* 8192 bit key length */

#define RSA1024NUMWORDS (RSA1024NUMBYTES / sizeof(uint32_t))
#define RSA2048NUMWORDS (RSA2048NUMBYTES / sizeof(uint32_t))
#define RSA4096NUMWORDS (RSA4096NUMBYTES / sizeof(uint32_t))
#define RSA8192NUMWORDS (RSA8192NUMBYTES / sizeof(uint32_t))

#endif  /* VBOOT_REFERENCE_RSA_H_ */
