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
#include "gbb_header.h"
#include "host_common.h"
#include "region.h"
#include "test_common.h"
#include "vboot_common.h"
#include "vboot_display.h"
#include "vboot_kernel.h"

/* Mock data */
static VbCommonParams cparams;
static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader *shared = (VbSharedDataHeader *)shared_data;
static char gbb_data[4096 + sizeof(GoogleBinaryBlockHeader)];
static GoogleBinaryBlockHeader *gbb = (GoogleBinaryBlockHeader *)gbb_data;
static BmpBlockHeader *bhdr;
static char debug_info[4096];
static struct vb2_context ctx;
static uint8_t workbuf[VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE];

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

	gbb->bmpfv_offset = gbb_used;
	bhdr = (BmpBlockHeader *)(gbb_data + gbb->bmpfv_offset);
	gbb->bmpfv_size = sizeof(BmpBlockHeader);
	gbb_used = (gbb_used + gbb->bmpfv_size + 7) & ~7;
	memcpy(bhdr->signature, BMPBLOCK_SIGNATURE, BMPBLOCK_SIGNATURE_SIZE);
	bhdr->major_version = BMPBLOCK_MAJOR_VERSION;
	bhdr->minor_version = BMPBLOCK_MINOR_VERSION;
	bhdr->number_of_localizations = 3;

	memset(&cparams, 0, sizeof(cparams));
	cparams.shared_data_size = sizeof(shared_data);
	cparams.shared_data_blob = shared_data;
	cparams.gbb_data = gbb;
	cparams.gbb_size = sizeof(gbb_data);

	/*
	 * Note, VbApiKernelFree() expects this to be allocated by
	 * malloc(), so we cannot just assign it staticly.
	 */
	cparams.gbb = malloc(sizeof(*gbb));
	gbb->header_size = sizeof(*gbb);
	gbb->rootkey_offset = gbb_used;
	gbb->rootkey_size = 64;
	gbb_used += 64;
	gbb->recovery_key_offset = gbb_used;
	gbb->recovery_key_size = 64;
	gbb_used += 64;
	memcpy(cparams.gbb, gbb, sizeof(*gbb));

	memset(&ctx, 0, sizeof(ctx));
	ctx.workbuf = workbuf;
	ctx.workbuf_size = sizeof(workbuf);
	vb2_init_context(&ctx);
	vb2_nv_init(&ctx);

	memset(&shared_data, 0, sizeof(shared_data));
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
	char hwid[VB_REGION_HWID_LEN];
	int i;

	/* Recovery string should be non-null for any code */
	for (i = 0; i < 0x100; i++)
		TEST_PTR_NEQ(RecoveryReasonString(i), NULL, "Non-null reason");

	/* HWID should come from the gbb */
	ResetMocks();
	VbRegionReadHWID(&cparams, hwid, sizeof(hwid));
	TEST_EQ(strcmp(hwid, "Test HWID"), 0, "HWID");
	VbApiKernelFree(&cparams);

	ResetMocks();
	cparams.gbb_size = 0;
	VbRegionReadHWID(&cparams, hwid, sizeof(hwid));
	TEST_EQ(strcmp(hwid, "{INVALID}"), 0, "HWID bad gbb");
	VbApiKernelFree(&cparams);

	ResetMocks();
	cparams.gbb->hwid_size = 0;
	VbRegionReadHWID(&cparams, hwid, sizeof(hwid));
	TEST_EQ(strcmp(hwid, "{INVALID}"), 0, "HWID missing");
	VbApiKernelFree(&cparams);

	ResetMocks();
	cparams.gbb->hwid_offset = cparams.gbb_size + 1;
	VbRegionReadHWID(&cparams, hwid, sizeof(hwid));
	TEST_EQ(strcmp(hwid, "{INVALID}"), 0, "HWID past end");
	VbApiKernelFree(&cparams);

	ResetMocks();
	cparams.gbb->hwid_size = cparams.gbb_size;
	VbRegionReadHWID(&cparams, hwid, sizeof(hwid));
	TEST_EQ(strcmp(hwid, "{INVALID}"), 0, "HWID overflow");
	VbApiKernelFree(&cparams);

	/* Display debug info */
	ResetMocks();
	VbDisplayDebugInfo(&ctx, &cparams);
	TEST_NEQ(*debug_info, '\0', "Some debug info was displayed");
	VbApiKernelFree(&cparams);
}

/* Test localization */
static void LocalizationTest(void)
{
	uint32_t count = 6;

	ResetMocks();
	cparams.gbb->bmpfv_size = 0;
	TEST_EQ(VbGetLocalizationCount(&cparams, &count),
		VBERROR_UNKNOWN, "VbGetLocalizationCount bad gbb");
	TEST_EQ(count, 0, "  count");
	VbApiKernelFree(&cparams);

	ResetMocks();
	bhdr->signature[0] ^= 0x5a;
	TEST_EQ(VbGetLocalizationCount(&cparams, &count),
		VBERROR_UNKNOWN, "VbGetLocalizationCount bad bmpfv");
	VbApiKernelFree(&cparams);

	ResetMocks();
	TEST_EQ(VbGetLocalizationCount(&cparams, &count), 0,
		"VbGetLocalizationCount()");
	TEST_EQ(count, 3, "  count");
	VbApiKernelFree(&cparams);
}

/* Test display key checking */
static void DisplayKeyTest(void)
{
	ResetMocks();
	VbCheckDisplayKey(&ctx, &cparams, 'q');
	TEST_EQ(*debug_info, '\0', "DisplayKey q = does nothing");
	VbApiKernelFree(&cparams);

	ResetMocks();
	VbCheckDisplayKey(&ctx, &cparams, '\t');
	TEST_NEQ(*debug_info, '\0', "DisplayKey tab = display");
	VbApiKernelFree(&cparams);

	/* Toggle localization */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_LOCALIZATION_INDEX, 0);
	VbCheckDisplayKey(&ctx, &cparams, VB_KEY_DOWN);
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_LOCALIZATION_INDEX), 2,
		"DisplayKey up");
	VbCheckDisplayKey(&ctx, &cparams, VB_KEY_LEFT);
	vb2_nv_get(&ctx, VB2_NV_LOCALIZATION_INDEX);
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_LOCALIZATION_INDEX), 1,
		"DisplayKey left");
	VbCheckDisplayKey(&ctx, &cparams, VB_KEY_RIGHT);
	vb2_nv_get(&ctx, VB2_NV_LOCALIZATION_INDEX);
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_LOCALIZATION_INDEX), 2,
		"DisplayKey right");
	VbCheckDisplayKey(&ctx, &cparams, VB_KEY_UP);
	vb2_nv_get(&ctx, VB2_NV_LOCALIZATION_INDEX);
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_LOCALIZATION_INDEX), 0,
		"DisplayKey up");
	VbApiKernelFree(&cparams);

	/* Reset localization if localization count is invalid */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_LOCALIZATION_INDEX, 1);
	bhdr->signature[0] ^= 0x5a;
	VbCheckDisplayKey(&ctx, &cparams, VB_KEY_UP);
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_LOCALIZATION_INDEX), 0,
		"DisplayKey invalid");
	VbApiKernelFree(&cparams);
}

static void FontTest(void)
{
	FontArrayHeader h;
	FontArrayEntryHeader eh[3] = {
		{
			.ascii = 'A',
			.info.original_size = 10,
		},
		{
			.ascii = 'B',
			.info.original_size = 20,
		},
		{
			.ascii = 'C',
			.info.original_size = 30,
		},
	};
	FontArrayEntryHeader *eptr;
	uint8_t buf[sizeof(h) + sizeof(eh)];
	VbFont_t *fptr;
	void *bufferptr;
	uint32_t buffersize;

	/* Create font data */
	h.num_entries = ARRAY_SIZE(eh);
	memcpy(buf, &h, sizeof(h));
	eptr = (FontArrayEntryHeader *)(buf + sizeof(h));
	memcpy(eptr, eh, sizeof(eh));

	fptr = VbInternalizeFontData((FontArrayHeader *)buf);
	TEST_PTR_EQ(fptr, buf, "Internalize");

	TEST_PTR_EQ(VbFindFontGlyph(fptr, 'B', &bufferptr, &buffersize),
		    &eptr[1].info, "Glyph found");
	TEST_EQ(buffersize, eptr[1].info.original_size, "  size");
	TEST_PTR_EQ(VbFindFontGlyph(fptr, 'X', &bufferptr, &buffersize),
		    &eptr[0].info, "Glyph not found");
	TEST_EQ(buffersize, eptr[0].info.original_size, "  size");

	/* Test invalid rendering params */
	VbRenderTextAtPos(NULL, 0, 0, 0, fptr);
	VbRenderTextAtPos("ABC", 0, 0, 0, NULL);

	VbDoneWithFontForNow(fptr);

}

int main(void)
{
	DebugInfoTest();
	LocalizationTest();
	DisplayKeyTest();
	FontTest();

	return gTestSuccess ? 0 : 255;
}
