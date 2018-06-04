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
	uint64_t buf[8] __attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
	uint8_t *p0 = (uint8_t *)buf, *ptr;
	struct vb2_workbuf wb;

	/* NOTE: There are several magic numbers below which assume that
	 * VB2_WORKBUF_ALIGN == 16 */

	/* Init */
	vb2_workbuf_init(&wb, p0, 64);
	TEST_EQ(vb2_offset_of(p0, wb.buf), 0, "Workbuf init aligned");
	TEST_EQ(wb.size, 64, "  size");

	vb2_workbuf_init(&wb, p0 + 4, 64);
	TEST_EQ(vb2_offset_of(p0, wb.buf), VB2_WORKBUF_ALIGN,
		"Workbuf init unaligned");
	TEST_EQ(wb.size, 64 - VB2_WORKBUF_ALIGN + 4, "  size");

	vb2_workbuf_init(&wb, p0 + 2, 5);
	TEST_EQ(wb.size, 0, "Workbuf init tiny unaligned size");

	/* Alloc rounds up */
	vb2_workbuf_init(&wb, p0, 64);
	ptr = vb2_workbuf_alloc(&wb, 22);
	TEST_EQ(vb2_offset_of(p0, ptr), 0, "Workbuf alloc");
	TEST_EQ(vb2_offset_of(p0, wb.buf), 32, "  buf");
	TEST_EQ(wb.size, 32, "  size");

	vb2_workbuf_init(&wb, p0, 32);
	TEST_PTR_EQ(vb2_workbuf_alloc(&wb, 33), NULL, "Workbuf alloc too big");

	/* Free reverses alloc */
	vb2_workbuf_init(&wb, p0, 32);
	vb2_workbuf_alloc(&wb, 22);
	vb2_workbuf_free(&wb, 22);
	TEST_EQ(vb2_offset_of(p0, wb.buf), 0, "Workbuf free buf");
	TEST_EQ(wb.size, 32, "  size");

	/* Realloc keeps same pointer as alloc */
	vb2_workbuf_init(&wb, p0, 64);
	vb2_workbuf_alloc(&wb, 6);
	ptr = vb2_workbuf_realloc(&wb, 6, 21);
	TEST_EQ(vb2_offset_of(p0, ptr), 0, "Workbuf realloc");
	TEST_EQ(vb2_offset_of(p0, wb.buf), 32, "  buf");
	TEST_EQ(wb.size, 32, "  size");
}

int main(int argc, char* argv[])
{
	test_memcmp();
	test_align();
	test_workbuf();

	return gTestSuccess ? 0 : 255;
}
