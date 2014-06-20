/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host-side misc functions for verified boot.
 */

#ifndef VBOOT_REFERENCE_UTIL_MISC_H_
#define VBOOT_REFERENCE_UTIL_MISC_H_

#include "vboot_struct.h"

/* Prints the sha1sum of the given VbPublicKey to stdout. */
void PrintPubKeySha1Sum(VbPublicKey* key);

#endif  /* VBOOT_REFERENCE_UTIL_MISC_H_ */
