/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Unit tests NVM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bdb_api.h"
#include "test_common.h"

static void test_nvmrw(void)
{
	struct vba_context ctx;
	uint32_t val;

	memset(&ctx.nvmrw, 0, sizeof(ctx.nvmrw));

	TEST_SUCC(nvmrw_set(&ctx, NVMRW_VAR_UPDATE_COUNT, 1), NULL);
	TEST_SUCC(nvmrw_get(&ctx, NVMRW_VAR_UPDATE_COUNT, &val), NULL);
	TEST_EQ(val, 1, NULL);

	TEST_SUCC(nvmrw_set(&ctx, NVMRW_VAR_MIN_KERNEL_DATA_KEY_VERSION, 1),
		  NULL);
	TEST_SUCC(nvmrw_get(&ctx, NVMRW_VAR_MIN_KERNEL_DATA_KEY_VERSION, &val),
		  NULL);
	TEST_EQ(val, 1, NULL);

	TEST_SUCC(nvmrw_set(&ctx, NVMRW_VAR_MIN_KERNEL_VERSION, 1), NULL);
	TEST_SUCC(nvmrw_get(&ctx, NVMRW_VAR_MIN_KERNEL_VERSION, &val), NULL);
	TEST_EQ(val, 1, NULL);

	TEST_SUCC(nvmrw_set(&ctx, NVMRW_VAR_BUC_TYPE, 1), NULL);
	TEST_SUCC(nvmrw_get(&ctx, NVMRW_VAR_BUC_TYPE, &val), NULL);
	TEST_EQ(val, 1, NULL);

	TEST_SUCC(nvmrw_set(&ctx, NVMRW_VAR_FLAG_BUC_PRESENT, 1), NULL);
	TEST_SUCC(nvmrw_get(&ctx, NVMRW_VAR_FLAG_BUC_PRESENT, &val), NULL);
	TEST_TRUE(val, NULL);

	TEST_SUCC(nvmrw_set(&ctx, NVMRW_VAR_FLAG_DFM_DISABLE, 1), NULL);
	TEST_SUCC(nvmrw_get(&ctx, NVMRW_VAR_FLAG_DFM_DISABLE, &val), NULL);
	TEST_TRUE(val, NULL);

	TEST_SUCC(nvmrw_set(&ctx, NVMRW_VAR_FLAG_DOSM, 1), NULL);
	TEST_SUCC(nvmrw_get(&ctx, NVMRW_VAR_FLAG_DOSM, &val), NULL);
	TEST_TRUE(val, NULL);
}

int main(int argc, char *argv[])
{
	printf("Running BDB NVM tests...\n");

	test_nvmrw();

	return gTestSuccess ? 0 : 255;
}
