/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_SIGNATURE_DIGEST_H_
#define VBOOT_REFERENCE_SIGNATURE_DIGEST_H_

#include <inttypes.h>

/* Returns a buffer with DigestInfo (which depends on [algorithm])
 * prepended to [digest].
 */
uint8_t* prepend_digestinfo(int algorithm, uint8_t* digest);

#endif  /* VBOOT_REFERENCE_SIGNATURE_DIGEST_H_ */
