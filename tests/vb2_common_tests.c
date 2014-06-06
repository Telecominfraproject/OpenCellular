/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware 2common.c
 */

#include "2sysincludes.h"
#include "2common.h"

#include "test_common.h"

/**
 * Test alignment functions
 */
static void test_align(void)
{
	uint64_t buf[4];
	uint8_t *p0, *ptr;
	uint32_t size;

	/* Already aligned */
	p0 = (uint8_t *)buf;
	ptr = p0;
	size = 16;
	TEST_SUCC(vb2_align(&ptr, &size, 4, 16), "vb2_align() aligned");
	TEST_EQ(vb2_offset_of(p0, ptr), 0, "ptr");
	TEST_EQ(size, 16, "  size");
	TEST_EQ(vb2_align(&ptr, &size, 4, 17),
		VB2_ERROR_ALIGN_SIZE, "vb2_align() small");

	/* Offset */
	ptr = p0 + 1;
	size = 15;
	TEST_SUCC(vb2_align(&ptr, &size, 4, 12), "vb2_align() offset");
	TEST_EQ(vb2_offset_of(p0, ptr), 4, "ptr");
	TEST_EQ(size, 12, "  size");

	/* Offset, now too small */
	ptr = p0 + 1;
	size = 15;
	TEST_EQ(vb2_align(&ptr, &size, 4, 15),
		VB2_ERROR_ALIGN_SIZE, "vb2_align() offset small");

	/* Offset, too small even to align */
	ptr = p0 + 1;
	size = 1;
	TEST_EQ(vb2_align(&ptr, &size, 4, 1),
		VB2_ERROR_ALIGN_BIGGER_THAN_SIZE, "vb2_align() offset tiny");
}

/**
 * Test work buffer functions
 */
static void test_workbuf(void)
{
	uint64_t buf[8];
	uint8_t *p0 = (uint8_t *)buf, *ptr;
	struct vb2_workbuf wb;

	/* Init */
	vb2_workbuf_init(&wb, p0, 32);
	TEST_EQ(vb2_offset_of(p0, wb.buf), 0, "Workbuf init aligned");
	TEST_EQ(wb.size, 32, "  size");

	vb2_workbuf_init(&wb, p0 + 4, 32);
	TEST_EQ(vb2_offset_of(p0, wb.buf), 8, "Workbuf init unaligned");
	TEST_EQ(wb.size, 28, "  size");

	vb2_workbuf_init(&wb, p0 + 2, 5);
	TEST_EQ(wb.size, 0, "Workbuf init tiny unaligned size");

	/* Alloc rounds up */
	vb2_workbuf_init(&wb, p0, 32);
	ptr = vb2_workbuf_alloc(&wb, 22);
	TEST_EQ(vb2_offset_of(p0, ptr), 0, "Workbuf alloc");
	TEST_EQ(vb2_offset_of(p0, wb.buf), 24, "  buf");
	TEST_EQ(wb.size, 8, "  size");

	vb2_workbuf_init(&wb, p0, 32);
	TEST_PTR_EQ(vb2_workbuf_alloc(&wb, 33), NULL, "Workbuf alloc too big");

	/* Free reverses alloc */
	vb2_workbuf_init(&wb, p0, 32);
	vb2_workbuf_alloc(&wb, 22);
	vb2_workbuf_free(&wb, 22);
	TEST_EQ(vb2_offset_of(p0, wb.buf), 0, "Workbuf free buf");
	TEST_EQ(wb.size, 32, "  size");

	/* Realloc keeps same pointer as alloc */
	vb2_workbuf_init(&wb, p0, 32);
	vb2_workbuf_alloc(&wb, 6);
	ptr = vb2_workbuf_realloc(&wb, 6, 21);
	TEST_EQ(vb2_offset_of(p0, ptr), 0, "Workbuf realloc");
	TEST_EQ(vb2_offset_of(p0, wb.buf), 24, "  buf");
	TEST_EQ(wb.size, 8, "  size");
}

/*
 * Test struct packing for vboot_struct.h structs which are passed between
 * firmware and OS, or passed between different phases of firmware.
 */
static void test_struct_packing(void)
{
	TEST_EQ(EXPECTED_VBPUBLICKEY_SIZE, sizeof(struct vb2_packed_key),
		"sizeof(vb2_packed_key)");
	TEST_EQ(EXPECTED_VBSIGNATURE_SIZE, sizeof(struct vb2_signature),
		"sizeof(vb2_signature)");
	TEST_EQ(EXPECTED_VB2KEYBLOCKHEADER_SIZE,
		sizeof(struct vb2_keyblock),
		"sizeof(VbKeyBlockHeader)");
	TEST_EQ(EXPECTED_VB2FIRMWAREPREAMBLEHEADER2_1_SIZE,
		sizeof(struct vb2_fw_preamble),
		"sizeof(vb2_fw_preamble)");
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
		TEST_EQ(vb2_verify_member_inside(p, -8, p, 12, 0, 0),
			VB2_ERROR_INSIDE_PARENT_WRAPS,
			"MemberInside wraparound 1");
		TEST_EQ(vb2_verify_member_inside(p, 20, p, -8, 0, 0),
			VB2_ERROR_INSIDE_MEMBER_WRAPS,
			"MemberInside wraparound 2");
		TEST_EQ(vb2_verify_member_inside(p, 20, p, 4, 0, -8),
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
	test_align();
	test_workbuf();
	test_struct_packing();
	test_helper_functions();

	return gTestSuccess ? 0 : 255;
}
