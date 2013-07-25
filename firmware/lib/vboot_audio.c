/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Delay/beep functions used in dev-mode kernel selection.
 */

#include "sysincludes.h"

#include "crc32.h"
#include "gbb_header.h"
#include "utility.h"
#include "vboot_api.h"
#include "vboot_audio.h"
#include "vboot_audio_private.h"
#include "vboot_common.h"

/* BIOS doesn't have /usr/include */
#ifndef UINT_MAX
#define UINT_MAX 4294967295U            /* 0xffffffff */
#endif

/*
 * Need one second of noise in the first 22 seconds.
 * Total delay >= 30 seconds, <= 60 seconds.
 */
#define REQUIRED_NOISE_TIME    1000
#define REQUIRED_NOISE_WITHIN 22000
#define REQUIRED_TOTAL_DELAY  30000
#define MAX_CUSTOM_DELAY      60000

/* These are visible externally only to make testing easier */
VbDevMusicNote default_notes_[] = { {20000, 0}, /* 20 seconds */
                                    {250, 400}, /* two beeps */
                                    {250, 0},
                                    {250, 400},
                                    {9250, 0} }; /* total 30 seconds */
uint32_t default_count_ = sizeof(default_notes_) / sizeof(VbDevMusicNote);

VbDevMusicNote short_notes_[] = { {2000, 0} }; /* two seconds */
uint32_t short_count_ = sizeof(short_notes_) / sizeof(VbDevMusicNote);

/* No need to dynamically allocate this, is there? */
static VbAudioContext au;

/* Convert from msecs to VbExGetTimer() units. */
static uint64_t ticks_per_msec = 0;     /* Initialized by VbAudioOpen() */
static uint64_t VbMsecToTicks(uint16_t msec) {
  return ticks_per_msec * msec;
}

/**
 * Find and return a valid set of note events.
 *
 * We'll use the user's struct if possible, but we will still enforce the
 * 30-second timeout and require at least a second of audible noise within that
 * period. We allocate storage for two reasons: the user's struct will be in
 * flash, which is slow to read, and we may need one extra note at the end to
 * pad out the user's notes to a full 30 seconds. The caller should free it
 * when finished.
 */
static void VbGetDevMusicNotes(VbAudioContext *audio, int use_short)
{
	VbDevMusicNote *notebuf = 0;
	VbDevMusicNote *builtin = 0;
	VbDevMusic *hdr = CUSTOM_MUSIC_NOTES;
	uint32_t maxsize = CUSTOM_MUSIC_MAXSIZE; /* always <= flash size (8M) */
	uint32_t maxnotes, mysum, mylen, i;
	uint32_t this_msecs, on_msecs, total_msecs;
	uint32_t count;

	VBDEBUG(("VbGetDevMusicNotes: use_short is %d, hdr is %p, "
		 "maxsize is %d\n", use_short, hdr, maxsize));

	if (use_short) {
		builtin = short_notes_;
		count = short_count_;
		goto nope;
	}

	builtin = default_notes_;
	count = default_count_;

	/* If we can't beep in the background, don't allow customization. */
	if (!audio->background_beep)
		goto nope;

	if (!hdr || maxsize < sizeof(VbDevMusic))
		goto nope;

	if (0 != Memcmp(hdr->sig, "$SND", sizeof(hdr->sig))) {
		VBDEBUG(("VbGetDevMusicNotes: bad sig\n"));
		goto nope;
	}

	/*
	 * How many notes will fit in the flash region? One more than you'd
	 * think, because there's one note in the header itself.
	 */
	maxnotes = 1 + (maxsize - sizeof(VbDevMusic)) / sizeof(VbDevMusicNote);
	if (hdr->count == 0 || hdr->count > maxnotes) {
		VBDEBUG(("VbGetDevMusicNotes: count=%d maxnotes=%d\n",
			 hdr->count, maxnotes));
		goto nope;
	}

	/*
	 * CUSTOM_MUSIC_MAXSIZE can't be larger than the size of the flash
	 * (around 8M or so) so this isn't really necessary, but let's be safe
	 * anyway.
	 */
	if ((sizeof(VbDevMusicNote) > UINT_MAX / hdr->count) ||
	    (sizeof(hdr->count) >
	     UINT_MAX - hdr->count * sizeof(VbDevMusicNote))) {
		VBDEBUG(("VbGetDevMusicNotes: count=%d, just isn't right\n",
			 hdr->count));
		goto nope;
	}

	/* Now we know this won't overflow */
	mylen = (uint32_t)(sizeof(hdr->count) +
			   hdr->count * sizeof(VbDevMusicNote));
	mysum = Crc32(&(hdr->count), mylen);

	if (mysum != hdr->checksum) {
		VBDEBUG(("VbGetDevMusicNotes: mysum=%08x, want=%08x\n",
			 mysum, hdr->checksum));
		goto nope;
	}

	VBDEBUG(("VbGetDevMusicNotes: custom notes struct at %p\n", hdr));

	/*
	 * Measure the audible sound up to the first 22 seconds, being careful
	 * to avoid rollover. The note time is 16 bits, and the note count is
	 * 32 bits.  The product should fit in 64 bits.
	 */
	total_msecs = 0;
	on_msecs = 0;
	for (i=0; i < hdr->count; i++) {
		this_msecs = hdr->notes[i].msec ;
		if (this_msecs) {
			total_msecs += this_msecs;
			if (total_msecs <= REQUIRED_NOISE_WITHIN &&
			    hdr->notes[i].frequency >= 100 &&
			    hdr->notes[i].frequency <= 2000)
				on_msecs += this_msecs;
		}
	}

	/* We require at least one second of noise in the first 22 seconds */
	VBDEBUG(("VbGetDevMusicNotes:   with %d msecs of sound to begin\n",
		 on_msecs));
	if (on_msecs < REQUIRED_NOISE_TIME)
		goto nope;

	/*
	 * We'll also require that the total time be less than a minute. No
	 * real reason, it just gives us less to worry about.
	 */
	VBDEBUG(("VbGetDevMusicNotes:   lasting %d msecs\n", total_msecs));
	if (total_msecs > MAX_CUSTOM_DELAY) {
		goto nope;
	}

	/* One more check, just to be paranoid. */
	if (hdr->count > (UINT_MAX / sizeof(VbDevMusicNote) - 1)) {
		VBDEBUG(("VbGetDevMusicNotes:   they're all out to get me!\n"));
		goto nope;
	}

	/* Looks good. Allocate the space (plus one) and copy it over. */
	notebuf = VbExMalloc((hdr->count + 1) * sizeof(VbDevMusicNote));
	Memcpy(notebuf, hdr->notes, hdr->count * sizeof(VbDevMusicNote));
	count = hdr->count;

	/* We also require at least 30 seconds of delay. */
	if (total_msecs < REQUIRED_TOTAL_DELAY) {
		/*
		 * If the total time is less than 30 seconds, the needed
		 * difference will fit in 16 bits.
		 */
		this_msecs = (REQUIRED_TOTAL_DELAY - total_msecs) & 0xffff;
		notebuf[hdr->count].msec = this_msecs;
		notebuf[hdr->count].frequency = 0;
		count++;
		VBDEBUG(("VbGetDevMusicNotes:   adding %d msecs of silence\n",
			 this_msecs));
	}

	/* Done */
	audio->music_notes = notebuf;
	audio->note_count = count;
	audio->free_notes_when_done = 1;
	return;

 nope:
	/* No custom notes, use the default. The count is already set. */
	VBDEBUG(("VbGetDevMusicNotes: using %d default notes\n", count));
	audio->music_notes = builtin;
	audio->note_count = count;
	audio->free_notes_when_done = 0;
}


/**
 * Initialization function. Returns context for processing dev-mode delay.
 */
VbAudioContext *VbAudioOpen(VbCommonParams *cparams)
{
	GoogleBinaryBlockHeader *gbb = cparams->gbb;
	VbAudioContext *audio = &au;
	int use_short = 0;
	uint64_t a, b;

	/* Note: may need to allocate things here in future */

	/* Calibrate audio delay */
	a = VbExGetTimer();
	VbExSleepMs(10);
	b = VbExGetTimer();
	ticks_per_msec = (b - a) / 10ULL ;
	VBDEBUG(("VbAudioOpen() - ticks_per_msec is %" PRIu64 "\n",
		ticks_per_msec));

	/* Initialize */
	Memset(audio, 0, sizeof(*audio));
	audio->background_beep = 1;
	audio->play_until = b;                /* "zero" starts now */

	/* See if we have full background sound capability or not. */
	if (VBERROR_SUCCESS != VbExBeep(0,0)) {
		VBDEBUG(("VbAudioOpen() - VbExBeep() is limited\n"));
		audio->background_beep = 0;
	}

	/*
	 * Prepare to generate audio/delay event. Use a short developer screen
	 * delay if indicated by GBB flags.
	 */
	if (gbb->major_version == GBB_MAJOR_VER && gbb->minor_version >= 1
	    && (gbb->flags & GBB_FLAG_DEV_SCREEN_SHORT_DELAY)) {
		VBDEBUG(("VbAudioOpen() - using short dev screen delay\n"));
		use_short = 1;
	}

	VbGetDevMusicNotes(audio, use_short);
	VBDEBUG(("VbAudioOpen() - note count %d\n", audio->note_count));

	return audio;
}

/**
 * Caller should loop without extra delay until this returns false.
 */
int VbAudioLooping(VbAudioContext *audio)
{
	uint64_t now;
	uint16_t freq = audio->current_frequency;
	uint16_t msec = 0;
	int looping = 1;

	now = VbExGetTimer();
	while (audio->next_note < audio->note_count &&
	       now >= audio->play_until) {
		freq = audio->music_notes[audio->next_note].frequency;
		msec = audio->music_notes[audio->next_note].msec;
		audio->play_until += VbMsecToTicks(msec);
		audio->next_note++;
	}

	if (now >= audio->play_until) {
		looping = 0;
		freq = 0;
	}

	/* Do action here. */
	if (audio->background_beep) {
		if (audio->current_frequency != freq) {
			VbExBeep(0, freq);
			audio->current_frequency = freq;
		}
	} else if (freq && msec) {
		VbExBeep(msec, freq);
		now = VbExGetTimer();
	}

	audio->last_time = now;
	return looping;
}

/**
 * Caller should call this prior to booting.
 */
void VbAudioClose(VbAudioContext *audio)
{
	VbExBeep(0,0);
	if (audio->free_notes_when_done)
		VbExFree(audio->music_notes);
}
