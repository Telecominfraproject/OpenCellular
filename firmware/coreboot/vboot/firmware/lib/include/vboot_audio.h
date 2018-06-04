/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Delay/beep functions used in dev-mode kernel selection.
 */

#ifndef VBOOT_REFERENCE_VBOOT_AUDIO_H_
#define VBOOT_REFERENCE_VBOOT_AUDIO_H_

#include "vboot_api.h"

/**
 * Initialization function.
 */
void vb2_audio_start(struct vb2_context *ctx);

/**
 * Caller should loop without extra delay until this returns false.
 */
int vb2_audio_looping(void);

#endif /* VBOOT_REFERENCE_VBOOT_AUDIO_H_ */

