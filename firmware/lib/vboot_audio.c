/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Audio functions used in dev-mode kernel selection.
 */

#include "gbb_header.h"
#include "utility.h"
#include "vboot_api.h"
#include "vboot_audio.h"
#include "vboot_common.h"


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

static struct VbAudioContext {
  uint32_t note_count;
  VbDevMusicNote* music_notes;
  uint32_t current_note;
  uint32_t current_note_loops;
  int background_beep;
} au;


#define DEV_LOOP_TIME 10  /* Minimum note granularity in msecs */


static uint16_t VbMsecToLoops(uint16_t msec) {
  return (DEV_LOOP_TIME / 2 + msec) / DEV_LOOP_TIME;
}

static VbDevMusicNote default_notes[] = { {20000, 0}, /* 20 seconds */
                                          {250, 400}, /* two beeps */
                                          {250, 0},
                                          {250, 400},
                                          {9250, 0} }; /* total 30 seconds */

static VbDevMusicNote short_notes[] = { {2000, 0} };   /* two seconds */

/* Return a valid set of note events. */
static VbDevMusicNote* VbGetDevMusicNotes(uint32_t* count, int use_short) {

  if (use_short) {
    *count = sizeof(short_notes) / sizeof(short_notes[0]);
    return short_notes;
  }

  *count = sizeof(default_notes) / sizeof(default_notes[0]);
  return default_notes;
}


/* Initialization function. Returns context for processing dev-mode delay */
VbAudioContext* VbAudioOpen(VbCommonParams* cparams) {
  GoogleBinaryBlockHeader* gbb = (GoogleBinaryBlockHeader*)cparams->gbb_data;
  VbAudioContext* audio = &au;

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
    audio->music_notes = VbGetDevMusicNotes(&(audio->note_count), 1);
  } else {
    audio->music_notes = VbGetDevMusicNotes(&(audio->note_count), 0);
  }

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

  /* Note: Free any allocated structs here */
}
