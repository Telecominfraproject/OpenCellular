/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * GUID structure.  Defined in appendix A of EFI standard.
 */

#ifndef VBOOT_REFERENCE_VBOOT_2GUID_H_
#define VBOOT_REFERENCE_VBOOT_2GUID_H_
#include <stdint.h>

#define UUID_NODE_LEN 6
#define GUID_SIZE 16

struct vb2_guid {
	union {
		struct {
			uint32_t time_low;
			uint16_t time_mid;
			uint16_t time_high_and_version;
			uint8_t clock_seq_high_and_reserved;
			uint8_t clock_seq_low;
			uint8_t node[UUID_NODE_LEN];
		} uuid;
		uint8_t raw[GUID_SIZE];
	};
} __attribute__((packed));

#define EXPECTED_GUID_SIZE GUID_SIZE

/* Key GUIDs to use for VB2_SIG_NONE and hash algorithms */

#define VB2_GUID_NONE_SHA1 \
	{{{0xcfb5687a,0x6092,0x11e4,0x96,0xe1,{0x8f,0x3b,0x1a,0x60,0xa2,0x1d}}}}

#define VB2_GUID_NONE_SHA256 \
	{{{0x0e4114e0,0x6093,0x11e4,0x9d,0xcb,{0x8f,0x8a,0xf4,0xca,0x2e,0x32}}}}

#define VB2_GUID_NONE_SHA512 \
	{{{0x1c695960,0x6093,0x11e4,0x82,0x63,{0xdb,0xee,0xe9,0x3c,0xcd,0x7e}}}}

#endif  /* VBOOT_REFERENCE_VBOOT_2GUID_H_ */
