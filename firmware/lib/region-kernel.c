/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware API for loading and verifying rewritable firmware.
 * (Firmware portion)
 */

#include "sysincludes.h"

#include "bmpblk_header.h"
#include "region.h"
#include "gbb_access.h"
#include "gbb_header.h"
#include "load_kernel_fw.h"
#include "utility.h"
#include "vboot_api.h"
#include "vboot_struct.h"

static VbError_t VbRegionReadGbb(VbCommonParams *cparams, uint32_t offset,
				  uint32_t size, void *buf)
{
	return VbRegionReadData(cparams, VB_REGION_GBB, offset, size, buf);
}

VbError_t VbGbbReadBmpHeader(VbCommonParams *cparams, BmpBlockHeader *hdr_ret)
{
	BmpBlockHeader *hdr;
	VbError_t ret;

	if (!cparams)
		return VBERROR_INVALID_GBB;
	if (!cparams->bmp) {
		GoogleBinaryBlockHeader *gbb = cparams->gbb;

		if (0 == gbb->bmpfv_size)
			return VBERROR_INVALID_GBB;

		hdr = VbExMalloc(sizeof(*hdr));
		ret = VbRegionReadGbb(cparams, gbb->bmpfv_offset,
				      sizeof(BmpBlockHeader), hdr);
		if (ret) {
			VbExFree(hdr);
			return ret;
		}

		/* Sanity-check the bitmap block header */
		if ((0 != Memcmp(hdr->signature, BMPBLOCK_SIGNATURE,
				BMPBLOCK_SIGNATURE_SIZE)) ||
		(hdr->major_version > BMPBLOCK_MAJOR_VERSION) ||
		((hdr->major_version == BMPBLOCK_MAJOR_VERSION) &&
		(hdr->minor_version > BMPBLOCK_MINOR_VERSION))) {
			VBDEBUG(("VbDisplayScreenFromGBB(): "
				"invalid/too new bitmap header\n"));
			VbExFree(hdr);
			return VBERROR_INVALID_BMPFV;
		}
		cparams->bmp = hdr;
	}

	*hdr_ret = *cparams->bmp;
	return VBERROR_SUCCESS;
}

VbError_t VbRegionReadHWID(VbCommonParams *cparams, char *hwid,
			   uint32_t max_size)
{
	GoogleBinaryBlockHeader *gbb;
	VbError_t ret;

	if (!max_size)
		return VBERROR_INVALID_PARAMETER;
	*hwid = '\0';
	StrnAppend(hwid, "{INVALID}", max_size);
	if (!cparams)
		return VBERROR_INVALID_GBB;

	gbb = cparams->gbb;

	if (0 == gbb->hwid_size) {
		VBDEBUG(("VbHWID(): invalid hwid size\n"));
		return VBERROR_SUCCESS; /* oddly enough! */
	}

	if (gbb->hwid_size > max_size) {
		VBDEBUG(("VbDisplayDebugInfo(): invalid hwid offset/size\n"));
		return VBERROR_INVALID_PARAMETER;
	}
	ret = VbRegionReadGbb(cparams, gbb->hwid_offset, gbb->hwid_size, hwid);
	if (ret)
		return ret;

	return VBERROR_SUCCESS;
}

VbError_t VbGbbReadImage(VbCommonParams *cparams,
			       uint32_t localization, uint32_t screen_index,
			       uint32_t image_num, ScreenLayout *layout,
			       ImageInfo *image_info, char **image_datap,
			       uint32_t *image_data_sizep)
{
	uint32_t layout_offset, image_offset, data_offset, data_size;
	GoogleBinaryBlockHeader *gbb;
	BmpBlockHeader hdr;
	void *data = NULL;
	VbError_t ret;

	if (!cparams)
		return VBERROR_INVALID_GBB;

	ret = VbGbbReadBmpHeader(cparams, &hdr);
	if (ret)
		return ret;

	gbb = cparams->gbb;
	layout_offset = gbb->bmpfv_offset + sizeof(BmpBlockHeader) +
		localization * hdr.number_of_screenlayouts *
			sizeof(ScreenLayout) +
		screen_index * sizeof(ScreenLayout);
	ret = VbRegionReadGbb(cparams, layout_offset, sizeof(*layout), layout);
	if (ret)
		return ret;

	if (!layout->images[image_num].image_info_offset)
		return VBERROR_NO_IMAGE_PRESENT;

	image_offset = gbb->bmpfv_offset +
			layout->images[image_num].image_info_offset;
	ret = VbRegionReadGbb(cparams, image_offset, sizeof(*image_info),
			      image_info);
	if (ret)
		return ret;

	data_offset = image_offset + sizeof(*image_info);
	data_size = image_info->compressed_size;
	if (data_size) {
		void *orig_data;

		data = VbExMalloc(image_info->compressed_size);
		ret = VbRegionReadGbb(cparams, data_offset,
				      image_info->compressed_size, data);
		if (ret) {
			VbExFree(data);
			return ret;
		}
		if (image_info->compression != COMPRESS_NONE) {
			uint32_t inoutsize = image_info->original_size;

			orig_data = VbExMalloc(image_info->original_size);
			ret = VbExDecompress(data,
					     image_info->compressed_size,
					     image_info->compression,
					     orig_data, &inoutsize);
			data_size = inoutsize;
			VbExFree(data);
			data = orig_data;
			if (ret) {
				VbExFree(data);
				return ret;
			}
		}
	}

	*image_datap = data;
	*image_data_sizep = data_size;

	return VBERROR_SUCCESS;
}

#define OUTBUF_LEN 128

void VbRegionCheckVersion(VbCommonParams *cparams)
{
	GoogleBinaryBlockHeader *gbb;

	if (!cparams)
		return;

	gbb = cparams->gbb;

	/*
	 * If GBB flags is nonzero, complain because that's something that the
	 * factory MUST fix before shipping. We only have to do this here,
	 * because it's obvious that something is wrong if we're not displaying
	 * screens from the GBB.
	 */
	if (gbb->major_version == GBB_MAJOR_VER && gbb->minor_version >= 1 &&
	    (gbb->flags != 0)) {
		uint32_t used = 0;
		char outbuf[OUTBUF_LEN];

		*outbuf = '\0';
		used += StrnAppend(outbuf + used, "gbb.flags is nonzero: 0x",
				OUTBUF_LEN - used);
		used += Uint64ToString(outbuf + used, OUTBUF_LEN - used,
				       gbb->flags, 16, 8);
		used += StrnAppend(outbuf + used, "\n", OUTBUF_LEN - used);
		(void)VbExDisplayDebugInfo(outbuf);
	}
}
