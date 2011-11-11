/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Delay/beep functions used in dev-mode kernel selection.
 */

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

#define DEV_LOOP_TIME 10                /* Minimum note granularity in msecs */

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


/* Arg is 16-bit, but use 32-bit to avoid rollover */
static uint32_t VbMsecToLoops(uint32_t msec) {
  return (DEV_LOOP_TIME / 2 + msec) / DEV_LOOP_TIME;
}

/* Find and return a valid set of note events. We'll use the user's struct
 * if possible, but we will still enforce the 30-second timeout and require at
 * least a second of audible noise within that period. We allocate storage for
 * two reasons: the user's struct will be in flash, which is slow to read, and
 * we may need one extra note at the end to pad out the user's notes to a full
 * 30 seconds. The caller should free it when finished.
 */
static void VbGetDevMusicNotes(VbAudioContext *audio, int use_short) {
  VbDevMusicNote *notebuf = 0;
  VbDevMusicNote *builtin = 0;
  VbDevMusic *hdr = CUSTOM_MUSIC_NOTES;
  uint32_t maxsize = CUSTOM_MUSIC_MAXSIZE; /* always <= flash size (8M) */
  uint32_t maxnotes, mysum, mylen, i;
  uint64_t on_loops, total_loops, min_loops;
  uint32_t this_loops;
  uint32_t count;

  VBDEBUG(("VbGetDevMusicNotes: use_short is %d, hdr is %lx, maxsize is %d\n",
           use_short, hdr, maxsize));

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

  /* How many notes will fit in the flash region? One more than you'd think,
   * because there's one note in the header itself.
   */
  maxnotes = 1 + (maxsize - sizeof(VbDevMusic)) / sizeof(VbDevMusicNote);
  if (hdr->count == 0 || hdr->count > maxnotes) {
    VBDEBUG(("VbGetDevMusicNotes: count=%d maxnotes=%d\n",
             hdr->count, maxnotes));
    goto nope;
  }

  /* CUSTOM_MUSIC_MAXSIZE can't be larger than the size of the flash (around 8M
   * or so) so this isn't really necessary, but let's be safe anyway.
   */
  if ((sizeof(VbDevMusicNote) > UINT_MAX / hdr->count) ||
      (sizeof(hdr->count) > UINT_MAX - hdr->count * sizeof(VbDevMusicNote))) {
    VBDEBUG(("VbGetDevMusicNotes: count=%d, just isn't right\n"));
    goto nope;
  }

  /* Now we know this won't overflow */
  mylen = (uint32_t)(sizeof(hdr->count) + hdr->count * sizeof(VbDevMusicNote));
  mysum = Crc32(&(hdr->count), mylen);

  if (mysum != hdr->checksum) {
    VBDEBUG(("VbGetDevMusicNotes: mysum=%08x, want=%08x\n",
             mysum, hdr->checksum));
    goto nope;
  }

  VBDEBUG(("VbGetDevMusicNotes: custom notes struct found at %lx\n", hdr));

  /* Measure the audible sound up to the first 22 seconds, being careful to
   * avoid rollover. The note time is 16 bits, and the note count is 32 bits.
   * The product should fit in 64 bits.
   */
  total_loops = 0;
  on_loops = 0;
  min_loops = VbMsecToLoops(22000);
  for (i=0; i < hdr->count; i++) {
    this_loops = VbMsecToLoops(hdr->notes[i].msec);
    if (this_loops) {
      total_loops += this_loops;
      if (total_loops <= min_loops &&
          hdr->notes[i].frequency >= 100 && hdr->notes[i].frequency <= 2000)
        on_loops += this_loops;
    }
  }

  /* We require at least one second of noise in the first 22 seconds */
  VBDEBUG(("VbGetDevMusicNotes:   with %ld msecs of sound to begin\n",
           on_loops * DEV_LOOP_TIME));
  if (on_loops < VbMsecToLoops(1000)) {
    goto nope;
  }

  /* We'll also require that the total time be less than a minute. No real
   * reason, it just gives us less to worry about.
   */
  VBDEBUG(("VbGetDevMusicNotes:   lasting %ld msecs\n",
           total_loops * DEV_LOOP_TIME));
  if (total_loops > VbMsecToLoops(60000)) {
    goto nope;
  }

  /* One more check, just to be paranoid. */
  if (hdr->count > (UINT_MAX / sizeof(VbDevMusicNote) - 1)) {
    VBDEBUG(("VbGetDevMusicNotes:   they're all out to get me!\n"));
    goto nope;
  }

  /* Okay, it looks good. Allocate the space (plus one) and copy it over. */
  notebuf = VbExMalloc((hdr->count + 1) * sizeof(VbDevMusicNote));
  Memcpy(notebuf, hdr->notes, hdr->count * sizeof(VbDevMusicNote));
  count = hdr->count;

  /* We also require at least 30 seconds of delay. */
  min_loops = VbMsecToLoops(30000);
  if (total_loops < min_loops) {
    /* If the total time is less than 30 seconds, the needed difference will
     * fit in 16 bits.
     */
    this_loops = (min_loops - total_loops) & 0xffff;
    notebuf[hdr->count].msec = (uint16_t)(this_loops * DEV_LOOP_TIME);
    notebuf[hdr->count].frequency = 0;
    count++;
    VBDEBUG(("VbGetDevMusicNotes:   adding %ld msecs of silence\n",
             this_loops * DEV_LOOP_TIME));
  }

  /* done */
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


/* Initialization function. Returns context for processing dev-mode delay */
VbAudioContext* VbAudioOpen(VbCommonParams* cparams) {
  GoogleBinaryBlockHeader* gbb = (GoogleBinaryBlockHeader*)cparams->gbb_data;
  VbAudioContext* audio = &au;
  int use_short = 0;

  /* Note: may need to allocate things here in future */

  /* defaults */
  audio->note_count = 0;
  audio->music_notes = 0;
  audio->current_note = 0;
  audio->current_note_loops = 0;
  audio->background_beep = 1;

  /* See if we have full background sound capability or not. */
  if (VBERROR_SUCCESS != VbExBeep(0,0)) {
    VBDEBUG(("VbAudioOpen() - VbExBeep() is limited\n"));
    audio->background_beep = 0;
  }

  /* Prepare to generate audio/delay event. Use a short developer screen delay
   * if indicated by GBB flags.
   */
  if (gbb->major_version == GBB_MAJOR_VER && gbb->minor_version >= 1
      && (gbb->flags & GBB_FLAG_DEV_SCREEN_SHORT_DELAY)) {
    VBDEBUG(("VbAudioOpen() - using short developer screen delay\n"));
    use_short = 1;
  }

  VbGetDevMusicNotes(audio, use_short);
  VBDEBUG(("VbAudioOpen() - note count %d\n", audio->note_count));

  return audio;
}

/* Caller should loop without extra delay until this returns false */
int VbAudioLooping(VbAudioContext* audio) {

    /* Time to play a note? */
    if (!audio->current_note_loops) {
      VBDEBUG(("VbAudioLooping() - current_note is %d\n", audio->current_note));

      /* Hooray, out of notes! */
      if (audio->current_note >= audio->note_count)
        return 0;

      /* For how many loops do we hold this note? */
      audio->current_note_loops =
        VbMsecToLoops(audio->music_notes[audio->current_note].msec);
      VBDEBUG(("VbAudioLooping() - new current_note_loops == %d\n",
               audio->current_note_loops));

      if (audio->background_beep) {

        /* start (or stop) the sound */
        VbExBeep(0, audio->music_notes[audio->current_note].frequency);

      } else if (audio->music_notes[audio->current_note].frequency) {

        /* the sound will block, so don't loop repeatedly */
        audio->current_note_loops = 1;
        VbExBeep(audio->music_notes[audio->current_note].msec,
                 audio->music_notes[audio->current_note].frequency);
      }

      audio->current_note++;
    }

    /* Wait a bit. Yes, one extra loop sometimes, but it's only 10msec */
    VbExSleepMs(DEV_LOOP_TIME);

    /* That's one... */
    if (audio->current_note_loops)
      audio->current_note_loops--;

  return 1;
}

/* Caller should call this prior to booting */
void VbAudioClose(VbAudioContext* audio) {
  VbExBeep(0,0);
  if (audio->free_notes_when_done)
    VbExFree(audio->music_notes);
}
