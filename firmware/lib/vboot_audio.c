/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Delay/beep functions used in dev-mode kernel selection.
 */

#include "2sysincludes.h"
#include "2common.h"
#include "2misc.h"

#include "sysincludes.h"
#include "utility.h"
#include "vboot_api.h"
#include "vboot_audio.h"
#include "vboot_common.h"

/* 1000 microsecond ticks per msec */
#define TICKS_PER_MSEC 1000

int audio_open_count = 0;	/* Times audio has been opened */
static int audio_use_short;	/* Use short delay? */
static uint64_t open_time;	/* Time of last open */
static int beep_count;		/* Number of beeps so far */

/**
 * Initialization function.
 */
void vb2_audio_start(struct vb2_context *ctx)
{
	struct vb2_shared_data *sd = vb2_get_sd(ctx);

	open_time = VbExGetTimer(); /* "zero" starts now */
	beep_count = 0;

	/*
	 * Use a short developer screen delay on the first audio if indicated
	 * by GBB flags.
	 */
	if ((sd->gbb_flags & VB2_GBB_FLAG_DEV_SCREEN_SHORT_DELAY) &&
	    (audio_open_count++ == 0)) {
		VB2_DEBUG("vb2_audio_start() - using short dev screen delay\n");
		audio_use_short = 1;
	} else {
		audio_use_short = 0;
	}
}

/**
 * Caller should loop without extra delay until this returns false.
 */
int vb2_audio_looping(void)
{
	uint64_t now = VbExGetTimer() - open_time;

	/* If we're using short delay, wait 2 seconds and don't beep */
	if (audio_use_short)
		return (now < 2000 * TICKS_PER_MSEC);

	/* Otherwise, beep at 20 and 20.5 seconds */
	if ((beep_count == 0 && now > 20000 * TICKS_PER_MSEC) ||
	    (beep_count == 1 && now > 20500 * TICKS_PER_MSEC)) {
		VbExBeep(250, 400);
		beep_count++;
	}

	/* Stop after 30 seconds */
	return (now < 30000 * TICKS_PER_MSEC);
}
