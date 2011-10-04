/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Private declarations for vboot_audio.c. Defined here for easier testing.
 */

#ifndef VBOOT_REFERENCE_VBOOT_AUDIO_PRIVATE_H_
#define VBOOT_REFERENCE_VBOOT_AUDIO_PRIVATE_H_

#include "vboot_api.h"
#include "vboot_audio.h"

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

struct VbAudioContext {
  uint32_t note_count;
  VbDevMusicNote* music_notes;
  int free_notes_when_done;
  uint32_t current_note;
  uint32_t current_note_loops;
  int background_beep;
};

#ifdef CUSTOM_MUSIC
void *VbExGetMusicPtr(void);
uint32_t VbExMaxMusicSize(void);
#define CUSTOM_MUSIC_NOTES VbExGetMusicPtr()
#define CUSTOM_MUSIC_MAXSIZE VbExMaxMusicSize()
#else
#define CUSTOM_MUSIC_NOTES 0
#define CUSTOM_MUSIC_MAXSIZE 0
#endif

#endif /* VBOOT_REFERENCE_VBOOT_AUDIO_PRIVATE_H_ */

