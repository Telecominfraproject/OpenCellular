/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_HOST_MISC2_H_
#define VBOOT_REFERENCE_HOST_MISC2_H_

#include <stdint.h>
#include <stdio.h>

#include "2guid.h"

/* Length of string representation, including trailing '\0' */
#define VB2_GUID_MIN_STRLEN 37

/**
 * Convert string to struct vb2_guid.
 *
 * @param str		Example: "C12A7328-F81F-11D2-BA4B-00A0C93EC93B"
 * @param guid          Destination for binary representation
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_str_to_guid(const char *str, struct vb2_guid *guid);

/**
 * Convert struct vb2_guid to string.
 *
 * @param guid          Binary representation
 * @param str		Buffer for result "C12A7328-F81F-11D2-BA4B-00A0C93EC93B"
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_guid_to_str(const struct vb2_guid *guid,
		    char *buf, unsigned int buflen);

#endif  /* VBOOT_REFERENCE_HOST_MISC2_H_ */
