/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for misc library
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_common.h"
#include "vboot_common.h"

#include "2api.h"
#include "2common.h"
#include "2misc.h"

static void misc_test(void)
{
	uint8_t workbuf[VB2_WORKBUF_RECOMMENDED_SIZE];

	struct vb2_context c = {
		.workbuf = workbuf,
		.workbuf_size = sizeof(workbuf),
	};

	TEST_SUCC(vb2_init_context(&c), "Init context good");
	TEST_EQ(c.workbuf_used, sizeof(struct vb2_shared_data),
		"Init vbsd");

	/* Don't re-init if used is non-zero */
	c.workbuf_used = 200;
	TEST_SUCC(vb2_init_context(&c), "Re-init context good");
	TEST_EQ(c.workbuf_used, 200, "Didn't re-init");

	/* Handle workbuf errors */
	c.workbuf_used = 0;
	c.workbuf_size = sizeof(struct vb2_shared_data) - 1;
	TEST_EQ(vb2_init_context(&c),
		VB2_ERROR_INITCTX_WORKBUF_SMALL, "Init too small");
	c.workbuf_size = sizeof(workbuf);

	/* Handle workbuf unaligned */
	c.workbuf++;
	TEST_EQ(vb2_init_context(&c),
		VB2_ERROR_INITCTX_WORKBUF_ALIGN, "Init unaligned");
}

int main(int argc, char* argv[])
{
	misc_test();

	return gTestSuccess ? 0 : 255;
}
