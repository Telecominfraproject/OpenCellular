/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_IMAGE_TYPES_H_
#define VBOOT_REFERENCE_IMAGE_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <stdint.h>
#include "bmpblk_header.h"

/* Identify the data. Fill in known values if info is not NULL */
ImageFormat identify_image_type(const void *buf, uint32_t bufsize,
                                ImageInfo *info);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* VBOOT_REFERENCE_IMAGE_TYPES_H_ */

