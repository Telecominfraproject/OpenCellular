/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_HOST_MISC2_H_
#define VBOOT_REFERENCE_HOST_MISC2_H_

#include <stdint.h>
#include <stdio.h>

#include "2id.h"

/* Length of string representation, including trailing '\0' */
#define VB2_ID_MIN_STRLEN (2 * VB2_ID_NUM_BYTES + 1)

/**
 * Convert hex string to struct vb2_id.
 *
 * @param str		Example: "01ABef000042"
 * @param id            Destination for binary representation
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_str_to_id(const char *str, struct vb2_id *id);

#endif  /* VBOOT_REFERENCE_HOST_MISC2_H_ */
