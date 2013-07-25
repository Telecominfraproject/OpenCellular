/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Display functions used in kernel selection.
 */

#ifndef VBOOT_REFERENCE_VBOOT_DISPLAY_H_
#define VBOOT_REFERENCE_VBOOT_DISPLAY_H_

#include "bmpblk_font.h"
#include "vboot_api.h"
#include "vboot_nvstorage.h"

VbError_t VbDisplayScreenFromGBB(VbCommonParams *cparams, uint32_t screen,
                                 VbNvContext *vncptr);
VbError_t VbDisplayScreen(VbCommonParams *cparams, uint32_t screen, int force,
                          VbNvContext *vncptr);
VbError_t VbDisplayDebugInfo(VbCommonParams *cparams, VbNvContext *vncptr);
VbError_t VbCheckDisplayKey(VbCommonParams *cparams, uint32_t key,
                            VbNvContext *vncptr);

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

