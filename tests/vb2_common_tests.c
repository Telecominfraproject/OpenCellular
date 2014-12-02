/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware 2common.c
 */

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "host_fw_preamble2.h"
#include "host_key2.h"
#include "host_keyblock2.h"
#include "host_signature2.h"
#include "vboot_struct.h"  /* For old struct sizes */

#include "test_common.h"

static const uint8_t test_data[] = "This is some test data to sign.";
static const uint8_t test_data2[] = "Some more test data";
static const uint8_t test_data3[] = "Even more test data";

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
	const char test_desc[32] = "test desc";
	uint32_t desc_end, m;

	c->total_size = sizeof(cbuf);
	c->fixed_size = sizeof(*c);
	c->desc_size = sizeof(test_desc);
	memcpy(cbuf + c->fixed_size, test_desc, sizeof(test_desc));
	desc_end = c->fixed_size + c->desc_size;

	c2 = (struct vb2_struct_common *)(cbuf + desc_end);
	c2->total_size = c->total_size - desc_end;
	c2->fixed_size = sizeof(*c2);
	c2->desc_size = 0;

	/* Description helper */
	TEST_EQ(0, strcmp(vb2_common_desc(c), test_desc), "vb2_common_desc()");
	TEST_EQ(0, strcmp(vb2_common_desc(c2), ""), "vb2_common_desc() empty");

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

/**
 * Signature size
 */
static void test_sig_size(void)
{
	TEST_EQ(vb2_sig_size(VB2_SIG_INVALID, VB2_HASH_SHA256), 0,
		"vb2_sig_size() sig invalid");

	TEST_EQ(vb2_sig_size(VB2_SIG_RSA2048, VB2_HASH_INVALID), 0,
		"vb2_sig_size() hash invalid");

	TEST_EQ(vb2_sig_size(VB2_SIG_RSA2048, VB2_HASH_SHA256), 2048 / 8,
		"vb2_sig_size() RSA2048");
	TEST_EQ(vb2_sig_size(VB2_SIG_RSA4096, VB2_HASH_SHA256), 4096 / 8,
		"vb2_sig_size() RSA4096");
	TEST_EQ(vb2_sig_size(VB2_SIG_RSA8192, VB2_HASH_SHA512), 8192 / 8,
		"vb2_sig_size() RSA8192");

	TEST_EQ(vb2_sig_size(VB2_SIG_NONE, VB2_HASH_SHA1),
		VB2_SHA1_DIGEST_SIZE, "vb2_sig_size() SHA1");
	TEST_EQ(vb2_sig_size(VB2_SIG_NONE, VB2_HASH_SHA256),
		VB2_SHA256_DIGEST_SIZE, "vb2_sig_size() SHA256");
	TEST_EQ(vb2_sig_size(VB2_SIG_NONE, VB2_HASH_SHA512),
		VB2_SHA512_DIGEST_SIZE, "vb2_sig_size() SHA512");
}

/**
 * Verify data on bare hash
 */
static void test_verify_hash(void)
{
	struct vb2_signature2 *sig;
	const struct vb2_private_key *prik;
	struct vb2_public_key pubk;
	uint8_t workbuf[VB2_VERIFY_DATA_WORKBUF_BYTES];
	struct vb2_workbuf wb;

	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	TEST_SUCC(vb2_private_key_hash(&prik, VB2_HASH_SHA256),
		  "create private hash key");
	TEST_SUCC(vb2_public_key_hash(&pubk, VB2_HASH_SHA256),
		  "create hash key");

	/* Create the signature */
	TEST_SUCC(vb2_sign_data(&sig, test_data, sizeof(test_data),
				prik, NULL),
		  "create hash sig");

	TEST_SUCC(vb2_verify_data2(test_data, sizeof(test_data),
				   sig, &pubk, &wb),
		  "vb2_verify_data2() hash ok");

	*((uint8_t *)sig + sig->sig_offset) ^= 0xab;
	TEST_EQ(vb2_verify_data2(test_data, sizeof(test_data), sig, &pubk, &wb),
		VB2_ERROR_VDATA_VERIFY_DIGEST, "vb2_verify_data2() hash bad");

	free(sig);
}

/**
 * Verify keyblock
 */
static void test_verify_keyblock(void)
{
	const char desc[16] = "test keyblock";
	const struct vb2_private_key *prik[2];
	struct vb2_public_key pubk, pubk2, pubk3;
	struct vb2_signature2 *sig;
	struct vb2_keyblock2 *kbuf;
	uint32_t buf_size;
	uint8_t *buf, *buf2;

	uint8_t workbuf[VB2_KEY_BLOCK_VERIFY_WORKBUF_BYTES];
	struct vb2_workbuf wb;

	TEST_SUCC(vb2_public_key_hash(&pubk, VB2_HASH_SHA256),
		  "create hash key 1");
	TEST_SUCC(vb2_public_key_hash(&pubk2, VB2_HASH_SHA512),
		  "create hash key 2");
	TEST_SUCC(vb2_public_key_hash(&pubk3, VB2_HASH_SHA1),
		  "create hash key 3");

	TEST_SUCC(vb2_private_key_hash(prik + 0, VB2_HASH_SHA256),
		  "create private key 1");
	TEST_SUCC(vb2_private_key_hash(prik + 1, VB2_HASH_SHA512),
		  "create private key 2");

	/* Create the test keyblock */
	TEST_SUCC(vb2_keyblock_create(&kbuf, &pubk3, prik, 2, 0x4321, desc),
		  "create keyblock");

	buf = (uint8_t *)kbuf;
	buf_size = kbuf->c.total_size;

	/* Make a copy of the buffer, so we can mangle it for tests */
	buf2 = malloc(buf_size);
	memcpy(buf2, buf, buf_size);

	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));
	kbuf = (struct vb2_keyblock2 *)buf;

	TEST_SUCC(vb2_verify_keyblock2(kbuf, buf_size, &pubk, &wb),
		  "vb2_verify_keyblock2()");

	memcpy(buf, buf2, buf_size);
	TEST_SUCC(vb2_verify_keyblock2(kbuf, buf_size, &pubk2, &wb),
		  "vb2_verify_keyblock2() key 2");

	memcpy(buf, buf2, buf_size);
	TEST_EQ(vb2_verify_keyblock2(kbuf, buf_size, &pubk3, &wb),
		VB2_ERROR_KEYBLOCK_SIG_GUID,
		"vb2_verify_keyblock2() key not present");

	memcpy(buf, buf2, buf_size);
	kbuf->c.magic = VB2_MAGIC_PACKED_KEY2;
	TEST_EQ(vb2_verify_keyblock2(kbuf, buf_size, &pubk, &wb),
		VB2_ERROR_KEYBLOCK_MAGIC,
		"vb2_verify_keyblock2() magic");

	memcpy(buf, buf2, buf_size);
	kbuf->c.fixed_size++;
	TEST_EQ(vb2_verify_keyblock2(kbuf, buf_size, &pubk, &wb),
		VB2_ERROR_COMMON_FIXED_UNALIGNED,
		"vb2_verify_keyblock2() header");

	memcpy(buf, buf2, buf_size);
	kbuf->c.struct_version_major++;
	TEST_EQ(vb2_verify_keyblock2(kbuf, buf_size, &pubk, &wb),
		VB2_ERROR_KEYBLOCK_HEADER_VERSION,
		"vb2_verify_keyblock2() major version");

	memcpy(buf, buf2, buf_size);
	kbuf->c.struct_version_minor++;
	/* That changes the signature, so resign the keyblock */
	vb2_sign_data(&sig, buf, kbuf->sig_offset, prik[0], NULL);
	memcpy(buf + kbuf->sig_offset, sig, sig->c.total_size);
	free(sig);
	TEST_SUCC(vb2_verify_keyblock2(kbuf, buf_size, &pubk, &wb),
		  "vb2_verify_keyblock2() minor version");

	memcpy(buf, buf2, buf_size);
	kbuf->c.fixed_size -= 4;
	kbuf->c.desc_size += 4;
	TEST_EQ(vb2_verify_keyblock2(kbuf, buf_size, &pubk, &wb),
		VB2_ERROR_KEYBLOCK_SIZE,
		"vb2_verify_keyblock2() header size");

	memcpy(buf, buf2, buf_size);
	kbuf->key_offset = kbuf->c.total_size - 4;
	TEST_EQ(vb2_verify_keyblock2(kbuf, buf_size, &pubk, &wb),
		VB2_ERROR_COMMON_MEMBER_SIZE,
		"vb2_verify_keyblock2() data key outside");

	memcpy(buf, buf2, buf_size);
	sig = (struct vb2_signature2 *)(buf + kbuf->sig_offset);
	sig->data_size--;
	TEST_EQ(vb2_verify_keyblock2(kbuf, buf_size, &pubk, &wb),
		VB2_ERROR_KEYBLOCK_SIGNED_SIZE,
		"vb2_verify_keyblock2() signed wrong size");

	memcpy(buf, buf2, buf_size);
	sig = (struct vb2_signature2 *)(buf + kbuf->sig_offset);
	sig->c.total_size = kbuf->c.total_size - 4;
	TEST_EQ(vb2_verify_keyblock2(kbuf, buf_size, &pubk, &wb),
		VB2_ERROR_COMMON_TOTAL_SIZE,
		"vb2_verify_keyblock2() key outside keyblock");

	memcpy(buf, buf2, buf_size);
	sig = (struct vb2_signature2 *)(buf + kbuf->sig_offset);
	sig->c.struct_version_major++;
	TEST_EQ(vb2_verify_keyblock2(kbuf, buf_size, &pubk, &wb),
		VB2_ERROR_SIG_VERSION,
		"vb2_verify_keyblock2() corrupt key");

	memcpy(buf, buf2, buf_size);
	kbuf->c.struct_version_minor++;
	TEST_EQ(vb2_verify_keyblock2(kbuf, buf_size, &pubk, &wb),
		VB2_ERROR_VDATA_VERIFY_DIGEST,
		"vb2_verify_keyblock2() corrupt");

	free(buf);
	free(buf2);
}

/**
 * Verify firmware preamble
 */
static void test_verify_fw_preamble(void)
{
	const char desc[16] = "test preamble";
	const struct vb2_private_key *prikhash;
	struct vb2_signature2 *hashes[3];
	struct vb2_public_key pubk;
	struct vb2_signature2 *sig;
	struct vb2_fw_preamble2 *pre;
	uint32_t buf_size;
	uint8_t *buf, *buf2;

	uint8_t workbuf[VB2_VERIFY_FIRMWARE_PREAMBLE_WORKBUF_BYTES];
	struct vb2_workbuf wb;

	/*
	 * Preambles will usually be signed with a real key not a bare hash,
	 * but the call to vb2_verify_data2() inside the preamble check is the
	 * same (and its functionality is verified separately), and using a
	 * bare hash here saves us from needing to have a private key to do
	 * this test.
	 */
	TEST_SUCC(vb2_public_key_hash(&pubk, VB2_HASH_SHA256),
		  "create hash key");
	TEST_SUCC(vb2_private_key_hash(&prikhash, VB2_HASH_SHA256),
			  "Create private hash key");

	/* Create some signatures */
	TEST_SUCC(vb2_sign_data(hashes + 0, test_data, sizeof(test_data),
				prikhash, "Hash 1"),
		  "Hash 1");
	TEST_SUCC(vb2_sign_data(hashes + 1, test_data2, sizeof(test_data2),
				prikhash, "Hash 2"),
		  "Hash 2");
	TEST_SUCC(vb2_sign_data(hashes + 2, test_data3, sizeof(test_data3),
				prikhash, "Hash 3"),
			  "Hash 3");

	/* Test good preamble */
	TEST_SUCC(vb2_fw_preamble_create(&pre, prikhash,
					 (const struct vb2_signature2 **)hashes,
					 3, 0x1234, 0x5678, desc),
		  "Create preamble good");

	buf = (uint8_t *)pre;
	buf_size = pre->c.total_size;

	/* Make a copy of the buffer, so we can mangle it for tests */
	buf2 = malloc(buf_size);
	memcpy(buf2, buf, buf_size);

	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));
	pre = (struct vb2_fw_preamble2 *)buf;

	TEST_SUCC(vb2_verify_fw_preamble2(pre, buf_size, &pubk, &wb),
		  "vb2_verify_fw_preamble2()");

	memcpy(buf, buf2, buf_size);
	pre->c.magic = VB2_MAGIC_PACKED_KEY2;
	TEST_EQ(vb2_verify_fw_preamble2(pre, buf_size, &pubk, &wb),
		VB2_ERROR_PREAMBLE_MAGIC,
		"vb2_verify_fw_preamble2() magic");

	memcpy(buf, buf2, buf_size);
	pre->c.fixed_size++;
	TEST_EQ(vb2_verify_fw_preamble2(pre, buf_size, &pubk, &wb),
		VB2_ERROR_COMMON_FIXED_UNALIGNED,
		"vb2_verify_fw_preamble2() header");

	memcpy(buf, buf2, buf_size);
	pre->c.struct_version_major++;
	TEST_EQ(vb2_verify_fw_preamble2(pre, buf_size, &pubk, &wb),
		VB2_ERROR_PREAMBLE_HEADER_VERSION,
		"vb2_verify_fw_preamble2() major version");

	memcpy(buf, buf2, buf_size);
	pre->c.struct_version_minor++;
	/* That changes the signature, so resign the fw_preamble */
	vb2_sign_data(&sig, buf, pre->sig_offset, prikhash, NULL);
	memcpy(buf + pre->sig_offset, sig, sig->c.total_size);
	free(sig);
	TEST_SUCC(vb2_verify_fw_preamble2(pre, buf_size, &pubk, &wb),
		  "vb2_verify_fw_preamble2() minor version");

	memcpy(buf, buf2, buf_size);
	pre->c.fixed_size -= 4;
	pre->c.desc_size += 4;
	TEST_EQ(vb2_verify_fw_preamble2(pre, buf_size, &pubk, &wb),
		VB2_ERROR_PREAMBLE_SIZE,
		"vb2_verify_fw_preamble2() header size");

	memcpy(buf, buf2, buf_size);
	sig = (struct vb2_signature2 *)(buf + pre->hash_offset);
	sig->c.total_size += pre->c.total_size;
	TEST_EQ(vb2_verify_fw_preamble2(pre, buf_size, &pubk, &wb),
		VB2_ERROR_COMMON_TOTAL_SIZE,
		"vb2_verify_fw_preamble2() hash size");

	memcpy(buf, buf2, buf_size);
	sig = (struct vb2_signature2 *)(buf + pre->hash_offset);
	sig->sig_size /= 2;
	TEST_EQ(vb2_verify_fw_preamble2(pre, buf_size, &pubk, &wb),
		VB2_ERROR_SIG_SIZE,
		"vb2_verify_fw_preamble2() hash integrity");

	memcpy(buf, buf2, buf_size);
	pre->hash_count++;
	TEST_EQ(vb2_verify_fw_preamble2(pre, buf_size, &pubk, &wb),
		VB2_ERROR_COMMON_MEMBER_OVERLAP,
		"vb2_verify_fw_preamble2() hash count");

	memcpy(buf, buf2, buf_size);
	sig = (struct vb2_signature2 *)(buf + pre->sig_offset);
	sig->c.total_size += 4;
	TEST_EQ(vb2_verify_fw_preamble2(pre, buf_size, &pubk, &wb),
		VB2_ERROR_COMMON_TOTAL_SIZE,
		"vb2_verify_fw_preamble2() sig inside");

	memcpy(buf, buf2, buf_size);
	sig = (struct vb2_signature2 *)(buf + pre->sig_offset);
	buf[pre->sig_offset + sig->sig_offset]++;
	TEST_EQ(vb2_verify_fw_preamble2(pre, buf_size, &pubk, &wb),
		VB2_ERROR_VDATA_VERIFY_DIGEST,
		"vb2_verify_fw_preamble2() sig corrupt");

	memcpy(buf, buf2, buf_size);
	pre->flags++;
	TEST_EQ(vb2_verify_fw_preamble2(pre, buf_size, &pubk, &wb),
		VB2_ERROR_VDATA_VERIFY_DIGEST,
		"vb2_verify_fw_preamble2() preamble corrupt");

	free(buf);
	free(buf2);
}

int main(int argc, char* argv[])
{
	test_memcmp();
	test_align();
	test_workbuf();
	test_struct_packing();
	test_helper_functions();
	test_common_header_functions();
	test_sig_size();
	test_verify_hash();
	test_verify_keyblock();
	test_verify_fw_preamble();

	return gTestSuccess ? 0 : 255;
}
