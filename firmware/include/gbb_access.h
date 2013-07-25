/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Access to portions of the GBB using the region API.
 */

#ifndef VBOOT_REFERENCE_GBB_ACCESS_H_
#define VBOOT_REFERENCE_GBB_ACCESS_H_

#include "vboot_api.h"

struct BmpBlockHeader;
struct ImageInfo;
struct GoogleBinaryBlockHeader;
struct ScreenLayout;
struct VbPublicKey;

/**
 * Read the GBB header
 *
 * This accesses the GBB and reads its header.
 *
 * @param cparams	Vboot common parameters
 * @param gbb		Place to put GBB header
 */
VbError_t VbGbbReadHeader_static(VbCommonParams *cparams,
				 struct GoogleBinaryBlockHeader *gbb);

/**
 * Read the root key from the GBB
 *
 * @param cparams	Vboot common parameters
 * @param keyp		Returns a pointer to the key. The caller must call
 *			VbExFree() on the key when finished with it.
 * @return VBERROR_... error, VBERROR_SUCCESS on success,
 */
VbError_t VbGbbReadRootKey(VbCommonParams *cparams,
			   struct VbPublicKey **keyp);

/**
 * Read the recovery key from the GBB
 *
 * @param cparams	Vboot common parameters
 * @param keyp		Returns a pointer to the key. The caller must call
 *			VbExFree() on the key when finished with it.
 * @return VBERROR_... error, VBERROR_SUCCESS on success,
 */
VbError_t VbGbbReadRecoveryKey(VbCommonParams *cparams,
			       struct VbPublicKey **keyp);

/**
 * Read the bitmap block header from the GBB
 *
 * @param cparams	Vboot common parameters
 * @param hdr		The header is placed in this block
 * @return VBERROR_... error, VBERROR_SUCCESS on success,
 */
VbError_t VbGbbReadBmpHeader(VbCommonParams *cparams,
			     struct BmpBlockHeader *hdr);

/**
 * Read a image from the GBB
 *
 * The caller must call VbExFree() on *image_datap when finished with it.
 *
 * @param cparams	Vboot common parameters
 * @param localization	Localization/language number
 * @param screen_index	Index of screen to display (VB_SCREEN_...)
 * @param image_num	Image number within the screen
 * @param layout	Returns layout information (x, y position)
 * @param image_info	Returns information about the image (format)
 * @param image_datap	Returns a pointer to the image data
 * @param iamge_data_sizep	Return size of image data
 * @return VBERROR_... error, VBERROR_SUCCESS on success,
 */
VbError_t VbGbbReadImage(VbCommonParams *cparams,
			 uint32_t localization, uint32_t screen_index,
			 uint32_t image_num, struct ScreenLayout *layout,
			 struct ImageInfo *image_info, char **image_datap,
			 uint32_t *image_data_sizep);

#endif
