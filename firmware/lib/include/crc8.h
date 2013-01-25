/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Very simple 8-bit CRC function.
 */
#ifndef VBOOT_REFERENCE_CRC8_H_
#define VBOOT_REFERENCE_CRC8_H_
#include "sysincludes.h"

uint8_t Crc8(const void *data, int len);

#endif /* VBOOT_REFERENCE_CRC8_H_ */
