/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * GUID structure.  Defined in appendix A of EFI standard.
 */

#ifndef VBOOT_REFERENCE_VBOOT_2GUID_H_
#define VBOOT_REFERENCE_VBOOT_2GUID_H_
#include <stdint.h>

#define NUM_GUID_BYTES 20

struct vb2_guid {
	uint8_t raw[NUM_GUID_BYTES];
} __attribute__((packed));

#define EXPECTED_GUID_SIZE NUM_GUID_BYTES

/* GUIDs to use for "keys" with sig_alg==VB2_SIG_NONE */
#define VB2_GUID_NONE_SHA1   {{0x00, 0x01,} }
#define VB2_GUID_NONE_SHA256 {{0x02, 0x56,} }
#define VB2_GUID_NONE_SHA512 {{0x05, 0x12,} }

#endif  /* VBOOT_REFERENCE_VBOOT_2GUID_H_ */
