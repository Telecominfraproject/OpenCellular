/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware display library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gbb_header.h"
#include "host_common.h"
#include "test_common.h"
#include "vboot_common.h"
#include "vboot_display.h"
#include "vboot_nvstorage.h"

/* Mock data */
static VbCommonParams cparams;
static VbNvContext vnc;
static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader *shared = (VbSharedDataHeader *)shared_data;
static GoogleBinaryBlockHeader gbb;
static char debug_info[4096];

/* Reset mock data (for use before each test) */
static void ResetMocks(void)
{
	Memset(&cparams, 0, sizeof(cparams));
	cparams.shared_data_size = sizeof(shared_data);
	cparams.shared_data_blob = shared_data;
	cparams.gbb_data = &gbb;

	Memset(&gbb, 0, sizeof(gbb));
	gbb.major_version = GBB_MAJOR_VER;
	gbb.minor_version = GBB_MINOR_VER;
	gbb.flags = 0;

	Memset(&vnc, 0, sizeof(vnc));
	VbNvSetup(&vnc);
	VbNvTeardown(&vnc);                   /* So CRC gets generated */

	Memset(&shared_data, 0, sizeof(shared_data));
	VbSharedDataInit(shared, sizeof(shared_data));

	*debug_info = 0;
}

/* Mocks */

VbError_t VbExDisplayDebugInfo(const char *info_str)
{
	strncpy(debug_info, info_str, sizeof(debug_info));
	debug_info[sizeof(debug_info) - 1] = '\0';
	return VBERROR_SUCCESS;
}

/* Test displaying debug info */
static void DebugInfoTest(void)
{
	int i;

	/* Recovery string should be non-null for any code */
	for (i = 0; i < 0x100; i++)
		TEST_PTR_NEQ(RecoveryReasonString(i), NULL, "Non-null reason");

	/* Display debug info */
	VbDisplayDebugInfo(&cparams, &vnc);
	TEST_NEQ(*debug_info, '\0', "Some debug info was displayed");
}

/* disable MSVC warnings on unused arguments */
__pragma(warning (disable: 4100))

int main(int argc, char* argv[])
{
	ResetMocks(); // KLUDGE

	DebugInfoTest();

	return gTestSuccess ? 0 : 255;
}
