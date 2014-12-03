/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware 2common.c
 */

#include "2sysincludes.h"
#include "vb2_common.h"
#include "vboot_struct.h"  /* For old struct sizes */
#include "test_common.h"

/*
 * Test struct packing for vboot_struct.h structs which are passed between
 * firmware and OS, or passed between different phases of firmware.
 */
static void test_struct_packing(void)
{
	/* Test vboot2 versions of vboot1 structs */
	TEST_EQ(EXPECTED_VB2_PACKED_KEY_SIZE,
		sizeof(struct vb2_packed_key),
		"sizeof(vb2_packed_key)");
	TEST_EQ(EXPECTED_VB2_SIGNATURE_SIZE,
		sizeof(struct vb2_signature),
		"sizeof(vb2_signature)");
	TEST_EQ(EXPECTED_VB2_KEYBLOCK_SIZE,
		sizeof(struct vb2_keyblock),
		"sizeof(vb2_keyblock)");
	TEST_EQ(EXPECTED_VB2_FW_PREAMBLE_SIZE,
		sizeof(struct vb2_fw_preamble),
		"sizeof(vb2_fw_preamble)");
	TEST_EQ(EXPECTED_VB2_GBB_HEADER_SIZE,
		sizeof(struct vb2_gbb_header),
		"sizeof(vb2_gbb_header)");

	/* And make sure they're the same as their vboot1 equivalents */
	TEST_EQ(EXPECTED_VB2_PACKED_KEY_SIZE,
		EXPECTED_VBPUBLICKEY_SIZE,
		"vboot1->2 packed key sizes same");
	TEST_EQ(EXPECTED_VB2_SIGNATURE_SIZE,
		EXPECTED_VBSIGNATURE_SIZE,
		"vboot1->2 signature sizes same");
	TEST_EQ(EXPECTED_VB2_KEYBLOCK_SIZE,
		EXPECTED_VBKEYBLOCKHEADER_SIZE,
		"vboot1->2 keyblock sizes same");
	TEST_EQ(EXPECTED_VB2_FW_PREAMBLE_SIZE,
		EXPECTED_VBFIRMWAREPREAMBLEHEADER2_1_SIZE,
		"vboot1->2 firmware preamble sizes same");
}

/**
 * Helper functions not dependent on specific key sizes
 */
static void test_helper_functions(void)
{
	{
		uint8_t *p = (uint8_t *)test_helper_functions;
		TEST_EQ((int)vb2_offset_of(p, p), 0, "vb2_offset_of() equal");
		TEST_EQ((int)vb2_offset_of(p, p+10), 10,
			"vb2_offset_of() positive");
	}

	{
		struct vb2_packed_key k = {.key_offset = sizeof(k)};
		TEST_EQ((int)vb2_offset_of(&k, vb2_packed_key_data(&k)),
			sizeof(k), "vb2_packed_key_data() adjacent");
	}

	{
		struct vb2_packed_key k = {.key_offset = 123};
		TEST_EQ((int)vb2_offset_of(&k, vb2_packed_key_data(&k)), 123,
			"vb2_packed_key_data() spaced");
	}

	{
		struct vb2_signature s = {.sig_offset = sizeof(s)};
		TEST_EQ((int)vb2_offset_of(&s, vb2_signature_data(&s)),
			sizeof(s), "vb2_signature_data() adjacent");
	}

	{
		struct vb2_signature s = {.sig_offset = 123};
		TEST_EQ((int)vb2_offset_of(&s, vb2_signature_data(&s)), 123,
			"vb2_signature_data() spaced");
	}

	{
		uint8_t *p = (uint8_t *)test_helper_functions;
		TEST_SUCC(vb2_verify_member_inside(p, 20, p, 6, 11, 3),
			  "MemberInside ok 1");
		TEST_SUCC(vb2_verify_member_inside(p, 20, p+4, 4, 8, 4),
			  "MemberInside ok 2");
		TEST_EQ(vb2_verify_member_inside(p, 20, p-4, 4, 8, 4),
			VB2_ERROR_INSIDE_MEMBER_OUTSIDE,
			"MemberInside member before parent");
		TEST_EQ(vb2_verify_member_inside(p, 20, p+20, 4, 8, 4),
			VB2_ERROR_INSIDE_MEMBER_OUTSIDE,
			"MemberInside member after parent");
		TEST_EQ(vb2_verify_member_inside(p, 20, p, 21, 0, 0),
			VB2_ERROR_INSIDE_MEMBER_OUTSIDE,
			"MemberInside member too big");
		TEST_EQ(vb2_verify_member_inside(p, 20, p, 4, 21, 0),
			VB2_ERROR_INSIDE_DATA_OUTSIDE,
			"MemberInside data after parent");
		TEST_EQ(vb2_verify_member_inside(p, 20, p, 4, SIZE_MAX, 0),
			VB2_ERROR_INSIDE_DATA_OUTSIDE,
			"MemberInside data before parent");
		TEST_EQ(vb2_verify_member_inside(p, 20, p, 4, 4, 17),
			VB2_ERROR_INSIDE_DATA_OUTSIDE,
			"MemberInside data too big");
		TEST_EQ(vb2_verify_member_inside(p, 20, p, 8, 4, 8),
			VB2_ERROR_INSIDE_DATA_OVERLAP,
			"MemberInside data overlaps member");
		TEST_EQ(vb2_verify_member_inside(p, -8, p, 12, 0, 0),
			VB2_ERROR_INSIDE_PARENT_WRAPS,
			"MemberInside wraparound 1");
		TEST_EQ(vb2_verify_member_inside(p, 20, p, -8, 0, 0),
			VB2_ERROR_INSIDE_MEMBER_WRAPS,
			"MemberInside wraparound 2");
		TEST_EQ(vb2_verify_member_inside(p, 20, p, 4, 4, -12),
			VB2_ERROR_INSIDE_DATA_WRAPS,
			"MemberInside wraparound 3");
	}

	{
		struct vb2_packed_key k = {.key_offset = sizeof(k),
					   .key_size = 128};
		TEST_SUCC(vb2_verify_packed_key_inside(&k, sizeof(k)+128, &k),
			  "PublicKeyInside ok 1");
		TEST_SUCC(vb2_verify_packed_key_inside(&k - 1,
						       2*sizeof(k)+128, &k),
			  "PublicKeyInside ok 2");
		TEST_EQ(vb2_verify_packed_key_inside(&k, 128, &k),
			VB2_ERROR_INSIDE_DATA_OUTSIDE,
			"PublicKeyInside key too big");
	}

	{
		struct vb2_packed_key k = {.key_offset = 100,
					   .key_size = 4};
		TEST_EQ(vb2_verify_packed_key_inside(&k, 99, &k),
			VB2_ERROR_INSIDE_DATA_OUTSIDE,
			"PublicKeyInside offset too big");
	}

	{
		struct vb2_signature s = {.sig_offset = sizeof(s),
					  .sig_size = 128};
		TEST_SUCC(vb2_verify_signature_inside(&s, sizeof(s)+128, &s),
			"SignatureInside ok 1");
		TEST_SUCC(vb2_verify_signature_inside(&s - 1,
						      2*sizeof(s)+128, &s),
			  "SignatureInside ok 2");
		TEST_EQ(vb2_verify_signature_inside(&s, 128, &s),
			VB2_ERROR_INSIDE_DATA_OUTSIDE,
			"SignatureInside sig too big");
	}

	{
		struct vb2_signature s = {.sig_offset = 100,
					  .sig_size = 4};
		TEST_EQ(vb2_verify_signature_inside(&s, 99, &s),
			VB2_ERROR_INSIDE_DATA_OUTSIDE,
			"SignatureInside offset too big");
	}
}

int main(int argc, char* argv[])
{
	test_struct_packing();
	test_helper_functions();

	return gTestSuccess ? 0 : 255;
}
