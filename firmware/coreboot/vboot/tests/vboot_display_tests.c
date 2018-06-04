/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware display library.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "bmpblk_font.h"
#include "gbb_access.h"
#include "gbb_header.h"
#include "host_common.h"
#include "test_common.h"
#include "vboot_common.h"
#include "vboot_display.h"
#include "vboot_kernel.h"

/* Mock data */
static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader *shared = (VbSharedDataHeader *)shared_data;
static char gbb_data[4096 + sizeof(GoogleBinaryBlockHeader)];
static GoogleBinaryBlockHeader *gbb = (GoogleBinaryBlockHeader *)gbb_data;
static char debug_info[4096];
static struct vb2_context ctx;
struct vb2_shared_data *sd;
static uint8_t workbuf[VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE];
static uint32_t mock_localization_count;

/* Reset mock data (for use before each test) */
static void ResetMocks(void)
{
	int gbb_used;

	memset(gbb_data, 0, sizeof(gbb_data));
	gbb->major_version = GBB_MAJOR_VER;
	gbb->minor_version = GBB_MINOR_VER;
	gbb->flags = 0;
	gbb_used = sizeof(GoogleBinaryBlockHeader);

	gbb->hwid_offset = gbb_used;
	strcpy(gbb_data + gbb->hwid_offset, "Test HWID");
	gbb->hwid_size = strlen(gbb_data + gbb->hwid_offset) + 1;
	gbb_used = (gbb_used + gbb->hwid_size + 7) & ~7;

	mock_localization_count = 3;

	gbb->header_size = sizeof(*gbb);
	gbb->rootkey_offset = gbb_used;
	gbb->rootkey_size = 64;
	gbb_used += 64;
	gbb->recovery_key_offset = gbb_used;
	gbb->recovery_key_size = 64;
	gbb_used += 64;

	memset(&ctx, 0, sizeof(ctx));
	ctx.workbuf = workbuf;
	ctx.workbuf_size = sizeof(workbuf);
	vb2_init_context(&ctx);
	vb2_nv_init(&ctx);

	sd = vb2_get_sd(&ctx);
	sd->vbsd = shared;
	sd->gbb = (struct vb2_gbb_header *)gbb_data;
	sd->gbb_size = sizeof(gbb_data);

	memset(&shared_data, 0, sizeof(shared_data));
	VbSharedDataInit(shared, sizeof(shared_data));

	*debug_info = 0;
}

/* Mocks */

VbError_t VbExGetLocalizationCount(uint32_t *count) {

	if (mock_localization_count == 0xffffffff)
		return VBERROR_UNKNOWN;

	*count = mock_localization_count;
	return VBERROR_SUCCESS;
}

VbError_t VbExDisplayDebugInfo(const char *info_str)
{
	strncpy(debug_info, info_str, sizeof(debug_info));
	debug_info[sizeof(debug_info) - 1] = '\0';
	return VBERROR_SUCCESS;
}

/* Test displaying debug info */
static void DebugInfoTest(void)
{
	char hwid[256];
	int i;

	/* Recovery string should be non-null for any code */
	for (i = 0; i < 0x100; i++)
		TEST_PTR_NEQ(RecoveryReasonString(i), NULL, "Non-null reason");

	/* HWID should come from the gbb */
	ResetMocks();
	VbGbbReadHWID(&ctx, hwid, sizeof(hwid));
	TEST_EQ(strcmp(hwid, "Test HWID"), 0, "HWID");

	ResetMocks();
	sd->gbb_size = 0;
	VbGbbReadHWID(&ctx, hwid, sizeof(hwid));
	TEST_EQ(strcmp(hwid, "{INVALID}"), 0, "HWID bad gbb");

	ResetMocks();
	sd->gbb->hwid_size = 0;
	VbGbbReadHWID(&ctx, hwid, sizeof(hwid));
	TEST_EQ(strcmp(hwid, "{INVALID}"), 0, "HWID missing");

	ResetMocks();
	sd->gbb->hwid_offset = sd->gbb_size + 1;
	VbGbbReadHWID(&ctx, hwid, sizeof(hwid));
	TEST_EQ(strcmp(hwid, "{INVALID}"), 0, "HWID past end");

	ResetMocks();
	sd->gbb->hwid_size = sd->gbb_size;
	VbGbbReadHWID(&ctx, hwid, sizeof(hwid));
	TEST_EQ(strcmp(hwid, "{INVALID}"), 0, "HWID overflow");

	/* Display debug info */
	ResetMocks();
	VbDisplayDebugInfo(&ctx);
	TEST_NEQ(*debug_info, '\0', "Some debug info was displayed");
}

/* Test display key checking */
static void DisplayKeyTest(void)
{
	ResetMocks();
	VbCheckDisplayKey(&ctx, 'q');
	TEST_EQ(*debug_info, '\0', "DisplayKey q = does nothing");

	ResetMocks();
	VbCheckDisplayKey(&ctx, '\t');
	TEST_NEQ(*debug_info, '\0', "DisplayKey tab = display");

	/* Toggle localization */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_LOCALIZATION_INDEX, 0);
	VbCheckDisplayKey(&ctx, VB_KEY_DOWN);
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_LOCALIZATION_INDEX), 2,
		"DisplayKey up");
	VbCheckDisplayKey(&ctx, VB_KEY_LEFT);
	vb2_nv_get(&ctx, VB2_NV_LOCALIZATION_INDEX);
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_LOCALIZATION_INDEX), 1,
		"DisplayKey left");
	VbCheckDisplayKey(&ctx, VB_KEY_RIGHT);
	vb2_nv_get(&ctx, VB2_NV_LOCALIZATION_INDEX);
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_LOCALIZATION_INDEX), 2,
		"DisplayKey right");
	VbCheckDisplayKey(&ctx, VB_KEY_UP);
	vb2_nv_get(&ctx, VB2_NV_LOCALIZATION_INDEX);
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_LOCALIZATION_INDEX), 0,
		"DisplayKey up");

	/* Reset localization if localization count is invalid */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_LOCALIZATION_INDEX, 1);
	mock_localization_count = 0xffffffff;
	VbCheckDisplayKey(&ctx, VB_KEY_UP);
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_LOCALIZATION_INDEX), 0,
		"DisplayKey invalid");
}

int main(void)
{
	DebugInfoTest();
	DisplayKeyTest();

	return gTestSuccess ? 0 : 255;
}
