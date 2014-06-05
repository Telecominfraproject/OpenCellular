/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware secure storage library.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_common.h"
#include "vboot_common.h"

#include "2common.h"
#include "2api.h"
#include "2secdata.h"

static void test_changed(struct vb2_context *ctx, int changed, const char *why)
{
	if (changed)
		TEST_NEQ(ctx->flags & VB2_CONTEXT_SECDATA_CHANGED, 0, why);
	else
		TEST_EQ(ctx->flags & VB2_CONTEXT_SECDATA_CHANGED, 0, why);

	ctx->flags &= ~VB2_CONTEXT_SECDATA_CHANGED;
};

static void secdata_test(void)
{
	uint8_t workbuf[VB2_WORKBUF_RECOMMENDED_SIZE];
	struct vb2_context c = {
		.flags = 0,
		.workbuf = workbuf,
		.workbuf_size = sizeof(workbuf),
	};
	struct vb2_secdata *s = (struct vb2_secdata *)c.secdata;
	uint32_t v = 1;

	/* Blank data is invalid */
	memset(c.secdata, 0xa6, sizeof(c.secdata));
	TEST_EQ(vb2_secdata_check_crc(&c),
		VB2_ERROR_SECDATA_CRC, "Check blank CRC");
	TEST_EQ(vb2_secdata_init(&c),
		 VB2_ERROR_SECDATA_CRC, "Init blank CRC");

	/* Create good data */
	TEST_SUCC(vb2_secdata_create(&c), "Create");
	TEST_SUCC(vb2_secdata_check_crc(&c), "Check created CRC");
	TEST_SUCC(vb2_secdata_init(&c), "Init created CRC");
	test_changed(&c, 1, "Create changes data");

	/* Now corrupt it */
	c.secdata[2]++;
	TEST_EQ(vb2_secdata_check_crc(&c),
		VB2_ERROR_SECDATA_CRC, "Check invalid CRC");
	TEST_EQ(vb2_secdata_init(&c),
		 VB2_ERROR_SECDATA_CRC, "Init invalid CRC");

	/* Version 1 didn't have a CRC, so init should reject it */
	vb2_secdata_create(&c);
	s->struct_version = 1;
	TEST_EQ(vb2_secdata_init(&c),
		VB2_ERROR_SECDATA_VERSION, "Init old version");

	vb2_secdata_create(&c);
	c.flags = 0;

	/* Read/write flags */
	TEST_SUCC(vb2_secdata_get(&c, VB2_SECDATA_FLAGS, &v), "Get flags");
	TEST_EQ(v, 0, "Flags created 0");
	test_changed(&c, 0, "Get doesn't change data");
	TEST_SUCC(vb2_secdata_set(&c, VB2_SECDATA_FLAGS, 0x12), "Set flags");
	test_changed(&c, 1, "Set changes data");
	TEST_SUCC(vb2_secdata_set(&c, VB2_SECDATA_FLAGS, 0x12), "Set flags 2");
	test_changed(&c, 0, "Set again doesn't change data");
	TEST_SUCC(vb2_secdata_get(&c, VB2_SECDATA_FLAGS, &v), "Get flags 2");
	TEST_EQ(v, 0x12, "Flags changed");
	TEST_EQ(vb2_secdata_set(&c, VB2_SECDATA_FLAGS, 0x100),
		VB2_ERROR_SECDATA_SET_FLAGS, "Bad flags");

	/* Read/write versions */
	TEST_SUCC(vb2_secdata_get(&c, VB2_SECDATA_VERSIONS, &v),
		  "Get versions");
	TEST_EQ(v, 0, "Versions created 0");
	test_changed(&c, 0, "Get doesn't change data");
	TEST_SUCC(vb2_secdata_set(&c, VB2_SECDATA_VERSIONS, 0x123456ff),
		  "Set versions");
	test_changed(&c, 1, "Set changes data");
	TEST_SUCC(vb2_secdata_set(&c, VB2_SECDATA_VERSIONS, 0x123456ff),
		  "Set versions 2");
	test_changed(&c, 0, "Set again doesn't change data");
	TEST_SUCC(vb2_secdata_get(&c, VB2_SECDATA_VERSIONS, &v),
		  "Get versions 2");
	TEST_EQ(v, 0x123456ff, "Versions changed");

	/* Invalid field fails */
	TEST_EQ(vb2_secdata_get(&c, -1, &v),
		VB2_ERROR_SECDATA_GET_PARAM, "Get invalid");
	TEST_EQ(vb2_secdata_set(&c, -1, 456),
		VB2_ERROR_SECDATA_SET_PARAM, "Set invalid");
	test_changed(&c, 0, "Set invalid field doesn't change data");
}

int main(int argc, char* argv[])
{
	secdata_test();

	return gTestSuccess ? 0 : 255;
}
