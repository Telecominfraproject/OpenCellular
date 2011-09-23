/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Display functions used in kernel selection.
 */

#ifndef VBOOT_REFERENCE_VBOOT_DISPLAY_H_
#define VBOOT_REFERENCE_VBOOT_DISPLAY_H_

#include "vboot_api.h"
#include "vboot_nvstorage.h"

VbError_t VbDisplayScreenFromGBB(VbCommonParams* cparams, uint32_t screen,
                                 VbNvContext *vncptr);
VbError_t VbDisplayScreen(VbCommonParams* cparams, uint32_t screen, int force,
                          VbNvContext *vncptr);
VbError_t VbDisplayDebugInfo(VbCommonParams* cparams, VbNvContext *vncptr);
VbError_t VbCheckDisplayKey(VbCommonParams* cparams, uint32_t key,
                            VbNvContext *vncptr);

void VbExEasterEgg(VbCommonParams* cparams, VbNvContext *vncptr);

typedef struct VbDevMusicNote {
  uint16_t msec;
  uint16_t frequency;
} __attribute__((packed)) VbDevMusicNote;

typedef struct VbDevMusic {
  uint8_t sig[4];                       /* "$SND" */
  uint32_t checksum;                    /* crc32 over count & all notes */
  uint32_t count;                       /* number of notes */
  VbDevMusicNote notes[1];              /* gcc allows [0], MSVC doesn't */
  /* more VbDevMusicNotes follow immediately */
} __attribute__((packed)) VbDevMusic;

#endif /* VBOOT_REFERENCE_VBOOT_DISPLAY_H_ */

