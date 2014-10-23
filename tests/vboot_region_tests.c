/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for vboot region API
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bmpblk_font.h"
#include "gbb_header.h"
#include "host_common.h"
#include "rollback_index.h"
#include "test_common.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"
#include "vboot_struct.h"

/* Mock data */
static VbCommonParams cparams;
static VbNvContext vnc;
static VbSelectFirmwareParams fparams;
VbSelectAndLoadKernelParams kparams;
static char gbb_data[4096 + sizeof(GoogleBinaryBlockHeader)];
static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader* shared = (VbSharedDataHeader*)shared_data;
/* Mock TPM versions */
static uint32_t mock_tpm_version;
static uint32_t mock_lf_tpm_version;  /* TPM version set by LoadFirmware() */
static uint32_t mock_seen_region;
/* Mock return values, so we can simulate errors */
static VbError_t mock_lf_retval;

#define COMPRESSED_SIZE		200
#define ORIGINAL_SIZE		400

/* Reset mock data (for use before each test) */
static void ResetMocks(void) {
	GoogleBinaryBlockHeader *gbb;
	BmpBlockHeader *bhdr;
	ImageInfo *image_info;
	ScreenLayout *layout;
	int gbb_used;

	Memset(&vnc, 0, sizeof(vnc));
	VbNvSetup(&vnc);
	VbNvTeardown(&vnc);                   /* So CRC gets generated */

	Memset(&cparams, 0, sizeof(cparams));
	cparams.shared_data_size = sizeof(shared_data);
	cparams.shared_data_blob = shared_data;

	Memset(&fparams, 0, sizeof(fparams));

	Memset(gbb_data, 0, sizeof(gbb_data));
	gbb = (GoogleBinaryBlockHeader *)gbb_data;
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
	bhdr->number_of_screenlayouts = 1;

	layout = (ScreenLayout *)(gbb_data + gbb_used);
	gbb_used += sizeof(*layout);
	layout->images[0].x = 1;
	layout->images[0].image_info_offset = gbb_used - gbb->bmpfv_offset;

	/* First image is uncompressed */
	image_info = (ImageInfo *)(gbb_data + gbb_used);
	image_info->format = FORMAT_BMP;
	image_info->compressed_size = ORIGINAL_SIZE;
	image_info->original_size = ORIGINAL_SIZE;
	image_info->compression = COMPRESS_NONE;
	gbb_used += sizeof(*image_info);
	strcpy(gbb_data + gbb_used, "original");
	gbb_used += ORIGINAL_SIZE;

	/* Second image is compressed */
	layout->images[1].image_info_offset = gbb_used - gbb->bmpfv_offset;
	layout->images[1].x = 2;
	image_info = (ImageInfo *)(gbb_data + gbb_used);
	image_info->format = FORMAT_BMP;
	image_info->compressed_size = COMPRESSED_SIZE;
	image_info->original_size = ORIGINAL_SIZE;
	image_info->compression = COMPRESS_LZMA1;
	gbb_used += sizeof(*image_info) + COMPRESSED_SIZE;

	Memset(&shared_data, 0, sizeof(shared_data));
	VbSharedDataInit(shared, sizeof(shared_data));
	shared->fw_keyblock_flags = 0xABCDE0;

	mock_tpm_version = mock_lf_tpm_version = 0x20004;
	shared->fw_version_tpm_start = mock_tpm_version;
	mock_lf_retval = 0;
	mock_seen_region = 0;
}

/****************************************************************************/
/* Mocked verification functions */

uint32_t SetTPMBootModeState(int developer_mode, int recovery_mode,
			     uint64_t fw_keyblock_flags
			     GoogleBinaryBlockHeader *gbb) {
  return VBERROR_SUCCESS;
}

VbError_t VbExNvStorageRead(uint8_t* buf) {
  Memcpy(buf, vnc.raw, sizeof(vnc.raw));
  return VBERROR_SUCCESS;
}

VbError_t VbExNvStorageWrite(const uint8_t* buf) {
  Memcpy(vnc.raw, buf, sizeof(vnc.raw));
  return VBERROR_SUCCESS;
}

VbError_t VbExRegionRead(VbCommonParams *cparams,
			 enum vb_firmware_region region, uint32_t offset,
			 uint32_t size, void *buf)
{
	if (region != VB_REGION_GBB)
		return VBERROR_UNSUPPORTED_REGION;
	mock_seen_region |= 1 << region;
	if (offset + size > sizeof(gbb_data))
		return VBERROR_REGION_READ_INVALID;
	memcpy(buf, gbb_data + offset, size);
	return VBERROR_SUCCESS;
}

VbError_t VbExDisplayImage(uint32_t x, uint32_t y,
                           void *buffer, uint32_t buffersize)
{
	switch (x) {
	case 1:
		TEST_STR_EQ(buffer, "original", "  uncompressed image");
		break;
	case 2:
		TEST_STR_EQ(buffer, "decompressed", "  compressed image");
		break;
	default:
		TEST_STR_EQ(buffer, "invalid", "  correct image");
		break;
	}
	return VBERROR_SUCCESS;
}

VbError_t VbExDecompress(void *inbuf, uint32_t in_size,
                         uint32_t compression_type,
                         void *outbuf, uint32_t *out_size)
{
	*out_size = ORIGINAL_SIZE;
	strcpy(outbuf, "decompressed");
	return VBERROR_SUCCESS;
}

int LoadFirmware(VbCommonParams *cparams, VbSelectFirmwareParams *fparams,
                 VbNvContext *vnc) {
	shared->fw_version_tpm = mock_lf_tpm_version;
	TEST_PTR_NEQ(cparams->gbb, NULL, "  GBB allocated");
	return mock_lf_retval;
}

/****************************************************************************/

static void VbRegionReadTest(void) {
	/* Should read GBB */
	ResetMocks();
	TEST_TRUE(1, "Normal call");
	TEST_EQ(VbSelectFirmware(&cparams, &fparams), VBERROR_SUCCESS,
		"  Success");
	TEST_EQ(mock_seen_region, 1 << VB_REGION_GBB, "  GBB region");
	TEST_PTR_EQ(cparams.gbb, NULL, "  GBB free");

	ResetMocks();
	TEST_EQ(VbSelectAndLoadKernel(&cparams, &kparams),
		VBERROR_NO_DISK_FOUND, "Kernel");
	TEST_PTR_EQ(cparams.gbb, NULL, "  GBB free");
	TEST_PTR_EQ(cparams.bmp, NULL, "  BMP free");

	ResetMocks();
	shared->flags |= VBSD_BOOT_DEV_SWITCH_ON;
	TEST_EQ(VbSelectAndLoadKernel(&cparams, &kparams),
		VBERROR_NO_DISK_FOUND, "Kernel");
}

int main(int argc, char* argv[]) {
  int error_code = 0;

  VbRegionReadTest();

  if (vboot_api_stub_check_memory())
    error_code = 255;
  if (!gTestSuccess)
    error_code = 255;

  return error_code;
}
