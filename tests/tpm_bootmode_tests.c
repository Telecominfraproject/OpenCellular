/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for tpm_bootmode functions
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _STUB_IMPLEMENTATION_  /* So we can use memset() ourselves */

#include "test_common.h"
#include "utility.h"
#include "tpm_bootmode.h"

extern const char* kBootStateSHA1Digests[];

/* Last in_digest passed to TlclExtend() for each PCR */
static const uint8_t *last_in[20];

/* Return value to pass for TlclExtend() */
static uint32_t extend_returns;

/* How many calls to TlclExtend() should one SetTPMBootModeState() make? */
static int expected_extend_count;
/* How many did we get? */
static int actual_extend_count;

static GoogleBinaryBlockHeader gbb_v1 = {
	.major_version = GBB_MAJOR_VER,
	.minor_version = 1,
};

static GoogleBinaryBlockHeader gbb_v2 = {
	.major_version = GBB_MAJOR_VER,
	.minor_version = 2,
	.hwid_digest = {1, 2, 3, 4,},
};

/* Mocked TlclExtend() function for testing */
uint32_t TlclExtend(int pcr_num, const uint8_t *in_digest,
		    uint8_t *out_digest)
{
	/* Should be using correct pcr */
	TEST_EQ(pcr_num, actual_extend_count, "TlclExtend pcr_num");

	last_in[actual_extend_count] = in_digest;

	actual_extend_count++;
	return extend_returns;
}


/* Test setting TPM boot mode state */
static void BootStateTest(void)
{
	int recdev;
	int flags;
	int index;
	char what[128];

	/* Test all permutations of developer and recovery mode */
	for (recdev = 0; recdev < 4; recdev++) {
		/* Exhaustively test all permutations of key block flags
		 * currently defined in vboot_struct.h (KEY_BLOCK_FLAG_*) */
		for (flags = 0; flags < 16; flags++) {
			index = recdev * 3;
			if (6 == flags)
				index += 2;
			else if (7 == flags)
				index += 1;

			/* Passing a null pointer for GBB */
			memset(last_in, 0, sizeof(last_in));
			actual_extend_count = 0;
			expected_extend_count = 1;
			TEST_EQ(SetTPMBootModeState(recdev & 2, recdev & 1,
						    flags, 0), 0,
				"SetTPMBootModeState return (gbb0)");
			snprintf(what, sizeof(what),
				 "SetTPMBootModeState %d, 0x%x (gbb0)",
				 recdev, flags);
			TEST_PTR_EQ(last_in[0],
				    kBootStateSHA1Digests[index], what);
			TEST_EQ(expected_extend_count, actual_extend_count,
				"Expected TlclExtend call count (gbb0)");
			snprintf(what, sizeof(what),
				 "SetTPMBootModeState %d, 0x%x (gbb0) PCR1",
				 recdev, flags);
			TEST_PTR_EQ(last_in[1], NULL, what);

			/* GBB v1.1 - should be exactly the same */
			memset(last_in, 0, sizeof(last_in));
			actual_extend_count = 0;
			expected_extend_count = 1;
			TEST_EQ(SetTPMBootModeState(recdev & 2, recdev & 1,
						    flags, &gbb_v1), 0,
				"SetTPMBootModeState return (gbb1)");
			snprintf(what, sizeof(what),
				 "SetTPMBootModeState %d, 0x%x (gbb1)",
				 recdev, flags);
			TEST_PTR_EQ(last_in[0],
				    kBootStateSHA1Digests[index], what);
			TEST_EQ(expected_extend_count, actual_extend_count,
				"Expected TlclExtend call count (gbb1)");
			snprintf(what, sizeof(what),
				 "SetTPMBootModeState %d, 0x%x (gbb1) PCR1",
				 recdev, flags);
			TEST_PTR_EQ(last_in[1], NULL, what);

			/* GBB v1.2 - should extend PCR1 with HWID digest */
			memset(last_in, 0, sizeof(last_in));
			actual_extend_count = 0;
			expected_extend_count = 2;
			TEST_EQ(SetTPMBootModeState(recdev & 2, recdev & 1,
						    flags, &gbb_v2), 0,
				"SetTPMBootModeState return (gbb2)");
			snprintf(what, sizeof(what),
				 "SetTPMBootModeState %d, 0x%x (gbb2)",
				 recdev, flags);
			TEST_PTR_EQ(last_in[0],
				    kBootStateSHA1Digests[index], what);
			TEST_EQ(expected_extend_count, actual_extend_count,
				"Expected TlclExtend call count (gbb2)");
			snprintf(what, sizeof(what),
				 "SetTPMBootModeState %d, 0x%x (gbb2) PCR1",
				 recdev, flags);
			TEST_PTR_EQ(last_in[1], gbb_v2.hwid_digest, what);
		}
	}

	extend_returns = 1;
	actual_extend_count = 0;
	expected_extend_count = 1;
	TEST_EQ(SetTPMBootModeState(0, 0, 0, 0), 1,
		"SetTPMBootModeState error");
}

int main(int argc, char *argv[])
{
	int error_code = 0;

	BootStateTest();

	if (!gTestSuccess)
		error_code = 255;

	return error_code;
}
