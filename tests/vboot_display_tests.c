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
static char gbb_data[4096 + sizeof(GoogleBinaryBlockHeader)];
static GoogleBinaryBlockHeader *gbb = (GoogleBinaryBlockHeader *)gbb_data;
static BmpBlockHeader *bhdr;
static char debug_info[4096];

/* Reset mock data (for use before each test) */
static void ResetMocks(void)
{
	int gbb_used;

	Memset(gbb_data, 0, sizeof(gbb_data));
	gbb->major_version = GBB_MAJOR_VER;
	gbb->minor_version = GBB_MINOR_VER;
	gbb->flags = 0;
	gbb_used = sizeof(GoogleBinaryBlockHeader);

	gbb->hwid_offset = gbb_used;
	strcpy(gbb_data + gbb->hwid_offset, "Test HWID");
	gbb->hwid_size = strlen(gbb_data + gbb->hwid_offset) + 1;
	gbb_used = (gbb_used + gbb->hwid_size + 7) & ~7;

	gbb->bmpfv_offset = gbb_used;
	bhdr = (BmpBlockHeader *)(gbb_data + gbb->bmpfv_offset);
	gbb->bmpfv_size = sizeof(BmpBlockHeader);
	gbb_used = (gbb_used + gbb->bmpfv_size + 7) & ~7;
	memcpy(bhdr->signature, BMPBLOCK_SIGNATURE, BMPBLOCK_SIGNATURE_SIZE);
	bhdr->major_version = BMPBLOCK_MAJOR_VERSION;
	bhdr->minor_version = BMPBLOCK_MINOR_VERSION;
	bhdr->number_of_localizations = 3;

	Memset(&cparams, 0, sizeof(cparams));
	cparams.shared_data_size = sizeof(shared_data);
	cparams.shared_data_blob = shared_data;
	cparams.gbb_data = gbb;
	cparams.gbb_size = sizeof(gbb_data);

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

	/* HWID should come from the gbb */
	ResetMocks();
	TEST_EQ(strcmp(VbHWID(&cparams), "Test HWID"), 0, "HWID");

	ResetMocks();
	cparams.gbb_size = 0;
	TEST_EQ(strcmp(VbHWID(&cparams), "{INVALID}"), 0, "HWID bad gbb");

	ResetMocks();
	gbb->hwid_size = 0;
	TEST_EQ(strcmp(VbHWID(&cparams), "{INVALID}"), 0, "HWID missing");

	ResetMocks();
	gbb->hwid_offset = cparams.gbb_size + 1;
	TEST_EQ(strcmp(VbHWID(&cparams), "{INVALID}"), 0, "HWID past end");

	ResetMocks();
	gbb->hwid_size = cparams.gbb_size;
	TEST_EQ(strcmp(VbHWID(&cparams), "{INVALID}"), 0, "HWID overflow");

	/* Display debug info */
	ResetMocks();
	VbDisplayDebugInfo(&cparams, &vnc);
	TEST_NEQ(*debug_info, '\0', "Some debug info was displayed");
}

/* Test localization */
static void LocalizationTest(void)
{
	uint32_t count = 6;

	ResetMocks();
	gbb->bmpfv_size = 0;
	TEST_EQ(VbGetLocalizationCount(&cparams, &count),
		VBERROR_INVALID_GBB, "VbGetLocalizationCount bad gbb");
	TEST_EQ(count, 0, "  count");

	ResetMocks();
	bhdr->signature[0] ^= 0x5a;
	TEST_EQ(VbGetLocalizationCount(&cparams, &count),
		VBERROR_INVALID_BMPFV, "VbGetLocalizationCount bad bmpfv");

	ResetMocks();
	TEST_EQ(VbGetLocalizationCount(&cparams, &count), 0,
		"VbGetLocalizationCount()");
	TEST_EQ(count, 3, "  count");

}

/* Test display key checking */
static void DisplayKeyTest(void)
{
	uint32_t u;

	ResetMocks();
	VbCheckDisplayKey(&cparams, 'q', &vnc);
	TEST_EQ(*debug_info, '\0', "DisplayKey q = does nothing");

	ResetMocks();
	VbCheckDisplayKey(&cparams, '\t', &vnc);
	TEST_NEQ(*debug_info, '\0', "DisplayKey tab = display");

	/* Toggle localization */
	VbNvSet(&vnc, VBNV_LOCALIZATION_INDEX, 0);
	VbNvTeardown(&vnc);
	VbCheckDisplayKey(&cparams, VB_KEY_DOWN, &vnc);
	VbNvGet(&vnc, VBNV_LOCALIZATION_INDEX, &u);
	TEST_EQ(u, 2, "DisplayKey up");
	VbCheckDisplayKey(&cparams, VB_KEY_LEFT, &vnc);
	VbNvGet(&vnc, VBNV_LOCALIZATION_INDEX, &u);
	TEST_EQ(u, 1, "DisplayKey left");
	VbCheckDisplayKey(&cparams, VB_KEY_RIGHT, &vnc);
	VbNvGet(&vnc, VBNV_LOCALIZATION_INDEX, &u);
	TEST_EQ(u, 2, "DisplayKey right");
	VbCheckDisplayKey(&cparams, VB_KEY_UP, &vnc);
	VbNvGet(&vnc, VBNV_LOCALIZATION_INDEX, &u);
	TEST_EQ(u, 0, "DisplayKey up");

	/* Reset localization if localization count is invalid */
	VbNvSet(&vnc, VBNV_LOCALIZATION_INDEX, 1);
	VbNvTeardown(&vnc);
	bhdr->signature[0] ^= 0x5a;
	VbCheckDisplayKey(&cparams, VB_KEY_UP, &vnc);
	VbNvGet(&vnc, VBNV_LOCALIZATION_INDEX, &u);
	TEST_EQ(u, 0, "DisplayKey invalid");

}

/* disable MSVC warnings on unused arguments */
__pragma(warning (disable: 4100))

int main(int argc, char* argv[])
{
	DebugInfoTest();
	LocalizationTest();
	DisplayKeyTest();

	return gTestSuccess ? 0 : 255;
}
