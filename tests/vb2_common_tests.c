/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware 2common.c
 */

#include "2sysincludes.h"
#include "2common.h"
#include "vboot_struct.h"  /* For old struct sizes */

#include "test_common.h"

/**
 * Test memory compare functions
 */
static void test_memcmp(void)
{
	TEST_EQ(vb2_safe_memcmp("foo", "foo", 3), 0, "memcmp equal");
	TEST_NEQ(vb2_safe_memcmp("foo1", "foo2", 4), 0, "memcmp different");
	TEST_EQ(vb2_safe_memcmp("foo1", "foo2", 0), 0, "memcmp 0-size");
}

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

	/* Test new struct sizes */
	TEST_EQ(EXPECTED_GUID_SIZE,
		sizeof(struct vb2_guid),
		"sizeof(vb2_guid)");
	TEST_EQ(EXPECTED_VB2_STRUCT_COMMON_SIZE,
		sizeof(struct vb2_struct_common),
		"sizeof(vb2_struct_common)");
	TEST_EQ(EXPECTED_VB2_PACKED_KEY2_SIZE,
		sizeof(struct vb2_packed_key2),
		"sizeof(vb2_packed_key2)");
	TEST_EQ(EXPECTED_VB2_SIGNATURE2_SIZE,
		sizeof(struct vb2_signature2),
		"sizeof(vb2_signature2)");
	TEST_EQ(EXPECTED_VB2_KEYBLOCK2_SIZE,
		sizeof(struct vb2_keyblock2),
		"sizeof(vb2_keyblock2)");
	TEST_EQ(EXPECTED_VB2_FW_PREAMBLE2_SIZE,
		sizeof(struct vb2_fw_preamble2),
		"sizeof(vb2_fw_preamble2)");
	TEST_EQ(EXPECTED_VB2_FW_PREAMBLE2_HASH_SIZE,
		sizeof(struct vb2_fw_preamble2_hash),
		"sizeof(vb2_fw_preamble2_hash)");
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

/**
 * Common header functions
 */
static void test_common_header_functions(void)
{
	uint8_t cbuf[sizeof(struct vb2_struct_common) + 128];
	uint8_t cbufgood[sizeof(cbuf)];
	struct vb2_struct_common *c = (struct vb2_struct_common *)cbuf;
	struct vb2_struct_common *c2;
	uint32_t desc_end, m;

	c->total_size = sizeof(cbuf);
	c->fixed_size = sizeof(*c);
	c->desc_size = 32;
	desc_end = c->fixed_size + c->desc_size;
	cbuf[desc_end - 1] = 0;

	c2 = (struct vb2_struct_common *)(cbuf + desc_end);
	c2->total_size = c->total_size - desc_end;
	c2->fixed_size = sizeof(*c2);
	c2->desc_size = 0;

	TEST_SUCC(vb2_verify_common_header(cbuf, sizeof(cbuf)),
		  "vb2_verify_common_header() good");
	memcpy(cbufgood, cbuf, sizeof(cbufgood));

	memcpy(cbuf, cbufgood, sizeof(cbuf));
	c->total_size += 4;
	TEST_EQ(vb2_verify_common_header(cbuf, sizeof(cbuf)),
		VB2_ERROR_COMMON_TOTAL_SIZE,
		"vb2_verify_common_header() total size");

	memcpy(cbuf, cbufgood, sizeof(cbuf));
	c->fixed_size = c->total_size + 4;
	TEST_EQ(vb2_verify_common_header(cbuf, sizeof(cbuf)),
		VB2_ERROR_COMMON_FIXED_SIZE,
		"vb2_verify_common_header() fixed size");

	memcpy(cbuf, cbufgood, sizeof(cbuf));
	c->desc_size = c->total_size - c->fixed_size + 4;
	TEST_EQ(vb2_verify_common_header(cbuf, sizeof(cbuf)),
		VB2_ERROR_COMMON_DESC_SIZE,
		"vb2_verify_common_header() desc size");

	memcpy(cbuf, cbufgood, sizeof(cbuf));
	c->total_size--;
	TEST_EQ(vb2_verify_common_header(cbuf, sizeof(cbuf)),
		VB2_ERROR_COMMON_TOTAL_UNALIGNED,
		"vb2_verify_common_header() total unaligned");

	memcpy(cbuf, cbufgood, sizeof(cbuf));
	c->fixed_size++;
	TEST_EQ(vb2_verify_common_header(cbuf, sizeof(cbuf)),
		VB2_ERROR_COMMON_FIXED_UNALIGNED,
		"vb2_verify_common_header() fixed unaligned");

	memcpy(cbuf, cbufgood, sizeof(cbuf));
	c->desc_size--;
	TEST_EQ(vb2_verify_common_header(cbuf, sizeof(cbuf)),
		VB2_ERROR_COMMON_DESC_UNALIGNED,
		"vb2_verify_common_header() desc unaligned");

	memcpy(cbuf, cbufgood, sizeof(cbuf));
	c->desc_size = -4;
	TEST_EQ(vb2_verify_common_header(cbuf, sizeof(cbuf)),
		VB2_ERROR_COMMON_DESC_WRAPS,
		"vb2_verify_common_header() desc wraps");

	memcpy(cbuf, cbufgood, sizeof(cbuf));
	cbuf[desc_end - 1] = 1;
	TEST_EQ(vb2_verify_common_header(cbuf, sizeof(cbuf)),
		VB2_ERROR_COMMON_DESC_TERMINATOR,
		"vb2_verify_common_header() desc not terminated");

	/* Member checking function */
	memcpy(cbuf, cbufgood, sizeof(cbuf));
	m = 0;
	TEST_SUCC(vb2_verify_common_member(cbuf, &m, c->total_size - 8, 4),
		  "vb2_verify_common_member()");
	TEST_EQ(m, c->total_size - 4, "  new minimum");

	m = desc_end;
	TEST_SUCC(vb2_verify_common_member(cbuf, &m, desc_end, 4),
		  "vb2_verify_common_member() good offset");
	TEST_EQ(m, desc_end + 4, "  new minimum");

	m = 0;
	TEST_EQ(vb2_verify_common_member(cbuf, &m, c->total_size - 8, -4),
		VB2_ERROR_COMMON_MEMBER_WRAPS,
		"vb2_verify_common_member() wraps");

	m = 0;
	TEST_EQ(vb2_verify_common_member(cbuf, &m, c->total_size - 7, 4),
		VB2_ERROR_COMMON_MEMBER_UNALIGNED,
		"vb2_verify_common_member() offset unaligned");

	m = 0;
	TEST_EQ(vb2_verify_common_member(cbuf, &m, c->total_size - 8, 5),
		VB2_ERROR_COMMON_MEMBER_UNALIGNED,
		"vb2_verify_common_member() size unaligned");

	m = 0;
	TEST_EQ(vb2_verify_common_member(cbuf, &m, desc_end - 4, 4),
		VB2_ERROR_COMMON_MEMBER_OVERLAP,
		"vb2_verify_common_member() overlap");

	m = desc_end + 4;
	TEST_EQ(vb2_verify_common_member(cbuf, &m, desc_end, 4),
		VB2_ERROR_COMMON_MEMBER_OVERLAP,
		"vb2_verify_common_member() overlap 2");

	m = 0;
	TEST_EQ(vb2_verify_common_member(cbuf, &m, c->total_size - 4, 8),
		VB2_ERROR_COMMON_MEMBER_SIZE,
		"vb2_verify_common_member() size");

	/* Subobject checking */
	m = 0;
	TEST_SUCC(vb2_verify_common_subobject(cbuf, &m, desc_end),
		  "vb2_verify_common_subobject() good offset");
	TEST_EQ(m, sizeof(cbuf), "  new minimum");

	m = desc_end + 4;
	TEST_EQ(vb2_verify_common_subobject(cbuf, &m, desc_end),
		VB2_ERROR_COMMON_MEMBER_OVERLAP,
		"vb2_verify_common_subobject() overlap");

	m = 0;
	c2->total_size += 4;
	TEST_EQ(vb2_verify_common_subobject(cbuf, &m, desc_end),
		VB2_ERROR_COMMON_TOTAL_SIZE,
		"vb2_verify_common_subobject() size");
}

int main(int argc, char* argv[])
{
	test_memcmp();
	test_align();
	test_workbuf();
	test_struct_packing();
	test_helper_functions();
	test_common_header_functions();

	return gTestSuccess ? 0 : 255;
}
