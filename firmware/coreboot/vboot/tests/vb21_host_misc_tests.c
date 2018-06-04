/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for host misc library vboot2 functions
 */

#include <stdio.h>
#include <unistd.h>

#include "2sysincludes.h"
#include "2common.h"
#include "vb21_common.h"
#include "host_common.h"
#include "host_misc.h"

#include "test_common.h"

static void misc_tests(void)
{
	TEST_EQ(roundup32(0), 0, "roundup32(0)");
	TEST_EQ(roundup32(15), 16, "roundup32(15)");
	TEST_EQ(roundup32(16), 16, "roundup32(16)");

	TEST_EQ(vb2_desc_size(NULL), 0, "desc size null");
	TEST_EQ(vb2_desc_size(""), 0, "desc size empty");
	TEST_EQ(vb2_desc_size("foo"), 4, "desc size 'foo'");
	TEST_EQ(vb2_desc_size("foob"), 8, "desc size 'foob'");
}

static void file_tests(const char *temp_dir)
{
	char *testfile;
	const uint8_t test_data[] = "Some test data";
	uint8_t *read_data;
	uint32_t read_size;

	uint8_t cbuf[sizeof(struct vb21_struct_common) + 12];
	struct vb21_struct_common *c = (struct vb21_struct_common *)cbuf;

	xasprintf(&testfile, "%s/file_tests.dat", temp_dir);

	unlink(testfile);

	TEST_EQ(vb2_read_file(testfile, &read_data, &read_size),
		VB2_ERROR_READ_FILE_OPEN, "vb2_read_file() missing");
	TEST_EQ(vb2_write_file("no/such/dir", test_data, sizeof(test_data)),
		VB2_ERROR_WRITE_FILE_OPEN, "vb2_write_file() open");

	TEST_SUCC(vb2_write_file(testfile, test_data, sizeof(test_data)),
		  "vb2_write_file() good");
	TEST_SUCC(vb2_read_file(testfile, &read_data, &read_size),
		  "vb2_read_file() good");
	TEST_EQ(read_size, sizeof(test_data), "  data size");
	TEST_EQ(memcmp(read_data, test_data, read_size), 0, "  data");
	free(read_data);
	unlink(testfile);

	memset(cbuf, 0, sizeof(cbuf));
	c->fixed_size = sizeof(*c);
	c->total_size = sizeof(cbuf);
	c->magic = 0x1234;
	cbuf[sizeof(cbuf) - 1] = 0xed;  /* Some non-zero data at the end */
	TEST_SUCC(vb21_write_object(testfile, c), "vb2_write_object() good");
	TEST_SUCC(vb2_read_file(testfile, &read_data, &read_size),
		  "vb2_read_file() object");
	TEST_EQ(read_size, c->total_size, "  data size");
	/* Compare the entire buffer, including the non-zero data at the end */
	TEST_EQ(memcmp(read_data, c, read_size), 0, "  data");
	free(read_data);
	unlink(testfile);
}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <temp_dir>\n", argv[0]);
		return -1;
	}
	const char *temp_dir = argv[1];

	misc_tests();
	file_tests(temp_dir);

	return gTestSuccess ? 0 : 255;
}
