/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef VBOOT_REFERENCE_GPT_CRC32_H_
#define VBOOT_REFERENCE_GPT_CRC32_H_

#include "sysincludes.h"

uint32_t Crc32(const void *buffer, uint32_t len);

#endif  /* VBOOT_REFERENCE_GPT_CRC32_H_ */
