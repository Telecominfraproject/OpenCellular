/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Display functions used in kernel selection.
 */

#ifndef VBOOT_REFERENCE_VBOOT_DISPLAY_H_
#define VBOOT_REFERENCE_VBOOT_DISPLAY_H_

#include "bmpblk_font.h"

struct vb2_context;

VbError_t VbDisplayScreenFromGBB(struct vb2_context *ctx,
				 VbCommonParams *cparams, uint32_t screen,
                                 uint32_t locale);
VbError_t VbDisplayScreen(struct vb2_context *ctx, VbCommonParams *cparams,
			  uint32_t screen, int force);
VbError_t VbDisplayMenu(struct vb2_context *ctx, VbCommonParams *cparams,
			uint32_t screen, int force, uint32_t selected_index,
			uint32_t disabled_idx_mask);
VbError_t VbDisplayDebugInfo(struct vb2_context *ctx, VbCommonParams *cparams);
VbError_t VbCheckDisplayKey(struct vb2_context *ctx, VbCommonParams *cparams,
			    uint32_t key);

/* Internal functions, for unit testing */

typedef FontArrayHeader VbFont_t;

VbFont_t *VbInternalizeFontData(FontArrayHeader *fonthdr);

void VbDoneWithFontForNow(VbFont_t *ptr);

ImageInfo *VbFindFontGlyph(VbFont_t *font, uint32_t ascii,
			   void **bufferptr, uint32_t *buffersize);

/**
 * Try to display the specified text at a particular position.
 */
void VbRenderTextAtPos(const char *text, int right_to_left,
		       uint32_t x, uint32_t y, VbFont_t *font);

/**
 * Return a description of the recovery reason code.
 */
const char *RecoveryReasonString(uint8_t code);

/**
 * Get the number of localizations in the GBB bitmap data.
 */
VbError_t VbGetLocalizationCount(VbCommonParams *cparams, uint32_t *count);

#endif /* VBOOT_REFERENCE_VBOOT_DISPLAY_H_ */

